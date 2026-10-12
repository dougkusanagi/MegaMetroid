#include "core/world.h"

int mm_pixel(int fixed)
{
    /* Explicit floor, including negative positions, on every compiler. */
    return fixed >= 0 ? fixed / MM_UNIT : -((-fixed + MM_UNIT - 1) / MM_UNIT);
}

int mm_tile(const MmWorld *w, int x, int y)
{
    if (x < 0 || y < 0 || x >= w->columns || y >= w->rows) return 0;
    return w->tiles[y * w->columns + x];
}

int mm_rect_blocked(const MmWorld *w, MmRect r)
{
    int tx, ty, first, last;
    if (r.left < 0 || r.top < 0 || r.right > w->columns * MM_TILE ||
        r.bottom > w->rows * MM_TILE) return 1;
    if (r.right <= r.left || r.bottom <= r.top) return 0;
    first = r.left / MM_TILE;
    last = (r.right - 1) / MM_TILE;
    /* One lookup per tile instead of repeated ROM lookups for every pixel. */
    for (ty = r.top / MM_TILE; ty <= (r.bottom - 1) / MM_TILE; ++ty) {
        const unsigned char *row = w->tiles + ty * w->columns;
        for (tx = first; tx <= last; ++tx) {
            int tile = row[tx];
            int bottom = r.bottom - 1 - ty * MM_TILE;
            if (tile == 0) return 1;
            if (tile == 2) {
                int right = r.right - 1 - tx * MM_TILE;
                if (right > 7) right = 7;
                if (bottom >= 7 - right) return 1;
            } else if (tile == 3) {
                int left = r.left - tx * MM_TILE;
                if (left < 0) left = 0;
                if (bottom >= left) return 1;
            }
        }
    }
    return 0;
}

int mm_blocked(const MmWorld *w, const MmBody *b, int x, int y)
{
    MmRect r = {x + b->hitbox.left, y + b->hitbox.top,
                x + b->hitbox.right, y + b->hitbox.bottom};
    return mm_rect_blocked(w, r);
}

void mm_body_init(MmBody *b, int x, int y, MmRect hitbox)
{
    b->x = x * MM_UNIT; b->y = y * MM_UNIT;
    b->vx = b->vy = 0; b->hitbox = hitbox;
    b->grounded = b->facing_left = b->jumped = 0;
    b->run_extra = b->move_direction = 0;
    b->spin_jumping = b->shoot_pose_timer = 0;
    b->shoot_frame = 3;
}

void mm_move(const MmWorld *w, MmBody *b)
{
    int target, p, step, lift, x = mm_pixel(b->x), y = mm_pixel(b->y);
    int was_grounded = b->grounded;
    int hit_x = 0;
    target = mm_pixel(b->x + b->vx);
    step = b->vx < 0 ? -1 : 1;
    for (p = x; p != target;) {
        if (mm_blocked(w, b, p + step, y)) {
            /* One pixel steps follow 45 degree slopes in both directions. */
            for (lift = 1; lift <= (was_grounded ? 1 : 0); ++lift)
                if (!mm_blocked(w, b, p + step, y - lift)) break;
            if (!was_grounded || lift > 1) { b->vx = 0; hit_x = 1; break; }
            y -= lift; b->y = y * MM_UNIT;
        }
        p += step;
    }
    b->x = hit_x ? p * MM_UNIT : b->x + b->vx;
    x = p;
    target = mm_pixel(b->y + b->vy);
    step = b->vy < 0 ? -1 : 1;
    b->grounded = 0;
    for (p = y; p != target;) {
        if (mm_blocked(w, b, x, p + step)) {
            if (step > 0) b->grounded = 1;
            b->vy = 0; break;
        }
        p += step;
    }
    b->y = b->vy ? b->y + b->vy : p * MM_UNIT;
    /* Follow a descending slope, but release grounded immediately at ledges. */
    if (was_grounded && b->vy >= 0 && !b->grounded) {
        int distance = (b->vx < 0 ? -b->vx : b->vx) / MM_UNIT + 1;
        for (lift = 0; lift <= distance; ++lift) {
            if (mm_blocked(w, b, x, p + lift + 1)) {
                b->y = (p + lift) * MM_UNIT; b->vy = 0; b->grounded = 1; break;
            }
        }
    }
    if (b->vy >= 0 && mm_blocked(w, b, x, mm_pixel(b->y) + 1)) {
        b->grounded = 1; b->vy = 0; b->y = mm_pixel(b->y) * MM_UNIT;
    }
}

void mm_tick(const MmWorld *w, MmBody *b, MmInput input)
{
    int direction = input.axis < 0 ? -1 : input.axis > 0 ? 1 : 0;
    int moving_before_jump = b->vx != 0;
    /* Dash builds on the ground; takeoff retains the bonus. Stopping,
       reversing, releasing on ground or hitting a wall cancels it. */
    if (!direction || direction != b->move_direction) b->run_extra = 0;
    if (b->grounded) {
        if (input.run_held && direction) {
            b->run_extra += MM_RUN_ACCEL;
            if (b->run_extra > MM_RUN_EXTRA_MAX) b->run_extra = MM_RUN_EXTRA_MAX;
        } else b->run_extra = 0;
    }
    b->move_direction = direction;
    b->jumped = input.jump_pressed && b->grounded;
    if (b->jumped) b->spin_jumping = moving_before_jump;
    if (input.shoot_pressed) {
        b->shoot_pose_timer = 10;
        b->spin_jumping = 0;
        if (input.aim_y < 0) b->shoot_frame = input.axis ? 2 : 1;
        else if (input.aim_y > 0 && !b->grounded) b->shoot_frame = input.axis ? 4 : 4;
        else b->shoot_frame = 3;
    } else if (b->shoot_pose_timer) --b->shoot_pose_timer;
    b->vx = direction * (MM_SPEED + b->run_extra);
    if (input.axis) b->facing_left = input.axis < 0;
    if (b->jumped) { b->vy = -MM_JUMP; b->grounded = 0; }
    b->vy += MM_GRAVITY;
    if (b->vy > MM_MAX_FALL) b->vy = MM_MAX_FALL;
    mm_move(w, b);
    if (!b->vx) b->run_extra = 0;
    if (b->grounded) b->spin_jumping = 0;
}
