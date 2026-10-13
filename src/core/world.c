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
    b->aim = MM_AIM_FORWARD;
    b->ball = b->crouching = 0;
    b->down_ticks = b->crouch_latch = b->down_window = 0;
    b->coyote = 0;
    b->wall_lock = b->wall_vx = 0;
    b->state = MM_STATE_STAND;
    b->pose = MM_POSE_GROUND; b->pose_timer = 0;
    b->stand_box = hitbox;
}

/* Crouch keeps the feet and drops the top by 2/5 of the standing height (40 -> 24 px). */
static MmRect crouch_box(MmRect s)
{
    s.top += (s.bottom - s.top) * 2 / 5;
    return s;
}

/* Ball is a square of 2/3 the standing width (24 -> 16 px), centred on the feet. */
static MmRect ball_box(MmRect s)
{
    int side = (s.right - s.left) * 2 / 3, cx = (s.left + s.right) / 2;
    MmRect r = {cx - side / 2, s.bottom - side, cx - side / 2 + side, s.bottom};
    return r;
}

/* True if the body at its current position fits inside box. */
static int fits(const MmWorld *w, const MmBody *b, MmRect box)
{
    MmBody probe = *b;
    probe.hitbox = box;
    return !mm_blocked(w, &probe, mm_pixel(b->x), mm_pixel(b->y));
}

/* Stands up from a ball. On a slope the standing box's lower corners sit in the surface, so the
   body may lift by up to a tile to fit. Returns 0 if no lift up to that makes room. */
static int stand_up(const MmWorld *w, MmBody *b)
{
    MmBody probe = *b;
    int lift;
    probe.hitbox = b->stand_box;
    for (lift = 0; lift <= MM_TILE; ++lift) {
        if (!mm_blocked(w, &probe, mm_pixel(b->x), mm_pixel(b->y) - lift)) {
            b->y -= lift * MM_UNIT;
            return 1;
        }
    }
    return 0;
}

/* True if a wall is within MM_WALL_REACH px on the side opposite to direction. */
static int wall_behind(const MmWorld *w, const MmBody *b, int direction)
{
    int d, x = mm_pixel(b->x), y = mm_pixel(b->y);
    for (d = 1; d <= MM_WALL_REACH; ++d)
        if (mm_blocked(w, b, x - direction * d, y)) return 1;
    return 0;
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

static MmState body_state(const MmBody *b)
{
    if (b->ball) return MM_STATE_BALL;
    if (b->grounded) return b->crouching ? MM_STATE_CROUCH : b->vx ? MM_STATE_WALK : MM_STATE_STAND;
    if (b->spin_jumping) return MM_STATE_SPIN;
    return b->vy < 0 ? MM_STATE_JUMP : MM_STATE_FALL;
}

/* Up aims up, or up and forward with a direction. Down aims down in the air; on the ground it
   crouches instead, unless a direction makes it aim down and forward while walking. */
static MmAim body_aim(const MmBody *b, MmInput input)
{
    if (b->ball) return MM_AIM_FORWARD;
    if (input.aim_y < 0) return input.axis ? MM_AIM_DIAG_UP : MM_AIM_UP;
    if (input.aim_y > 0) {
        if (input.axis) return MM_AIM_DIAG_DOWN;
        if (!b->grounded) return MM_AIM_DOWN;
    }
    return MM_AIM_FORWARD;
}

/* The pose once no one-shot pose is running. */
static MmPose looping_pose(const MmBody *b)
{
    if (b->ball) return MM_POSE_BALL;
    if (b->grounded) return b->crouching ? MM_POSE_CROUCH : MM_POSE_GROUND;
    if (b->spin_jumping) return MM_POSE_SPIN;
    return b->vy < 0 ? MM_POSE_JUMP_UP : MM_POSE_FALL;
}

/* Landings, crouching and standing up hold their pose for a while. The hold also ends when the
   body leaves the ground or, for the standing landing and the stand-up, when it starts walking. */
static void update_pose(MmBody *b, int was_grounded, int was_crouching, int was_ball)
{
    /* A body from mm_body_init has GROUND as its pose, so its first tick is not a landing. */
    if (b->grounded && !was_grounded && !b->ball && b->pose != MM_POSE_GROUND) {
        b->pose = b->spin_jumping ? MM_POSE_SPIN_LAND : MM_POSE_LAND;
        b->pose_timer = b->spin_jumping ? MM_SPIN_LAND_TICKS : MM_LAND_TICKS;
    }
    if (b->grounded && b->crouching && !was_crouching) {
        b->pose = MM_POSE_CROUCH_DOWN; b->pose_timer = MM_CROUCH_DOWN_TICKS;
    }
    if (b->grounded && !b->crouching && was_crouching) {
        b->pose = MM_POSE_STAND_UP; b->pose_timer = MM_STAND_UP_TICKS;
    }
    /* After the crouch checks: morphing from a crouch curls up instead of standing up. */
    if (b->ball && !was_ball) { b->pose = MM_POSE_MORPH; b->pose_timer = MM_MORPH_TICKS; }
    if (!b->ball && was_ball) { b->pose = MM_POSE_UNMORPH; b->pose_timer = MM_UNMORPH_TICKS; }
    if (b->pose == MM_POSE_LAND && b->vx) b->pose_timer = 0;
    if (b->pose == MM_POSE_STAND_UP && b->vx) b->pose_timer = 0;
    if (!b->grounded && (b->pose == MM_POSE_LAND || b->pose == MM_POSE_SPIN_LAND ||
                         b->pose == MM_POSE_CROUCH_DOWN || b->pose == MM_POSE_STAND_UP)) b->pose_timer = 0;
    if (!b->pose_timer) b->pose = looping_pose(b);
}

void mm_tick(const MmWorld *w, MmBody *b, MmInput input)
{
    int direction = input.axis < 0 ? -1 : input.axis > 0 ? 1 : 0;
    int moving_before_jump = b->vx != 0, wall_jump, was_grounded = b->grounded, was_crouching = b->crouching;
    int was_ball = b->ball;
    if (b->pose_timer) --b->pose_timer;
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
    b->jumped = input.jump_pressed && (b->grounded || b->coyote > 0);
    if (b->jumped) { b->spin_jumping = moving_before_jump; b->coyote = 0; }
    /* Wall jump, as in Super Metroid: only in a spin jump, touching a wall and pressing away from it.
       The kick goes the pressed way and locks steering. */
    wall_jump = input.jump_pressed && !b->jumped && !b->grounded && !b->ball && b->spin_jumping &&
                direction && wall_behind(w, b, direction);
    if (wall_jump) {
        b->jumped = 1; b->spin_jumping = 1;
        b->wall_lock = MM_WALL_LOCK; b->wall_vx = direction * MM_SPEED;
    }
    /* Shooting stops a spin; the cannon stays out while the shot is held and a moment after. */
    if (input.shoot_pressed) b->spin_jumping = 0;
    if (input.shoot_held && !b->ball) b->shoot_pose_timer = MM_SHOOT_POSE_TICKS;
    else if (b->shoot_pose_timer) --b->shoot_pose_timer;
    /* A tap of down (held up to MM_TAP_TICKS, and never while aiming with the shot) keeps the crouch
       after release on the ground. A longer hold stands the body up when it is released. */
    if (input.aim_y > 0) {
        if (b->down_ticks <= MM_TAP_TICKS) ++b->down_ticks;
        if (input.shoot_held) b->down_ticks = MM_TAP_TICKS + 1;
    } else if (b->down_ticks) {
        b->crouch_latch = b->grounded && !b->ball && b->down_ticks <= MM_TAP_TICKS;
        b->down_ticks = 0;
    }
    if (b->jumped || !b->grounded || direction || input.aim_y < 0) b->crouch_latch = 0;
    /* Down pressed while crouched morphs, however long the crouch lasted, as in Super Metroid. Two presses
       less than MM_DOUBLE_TAP_TICKS apart also morph, on the ground or in the air. A shot that aims down
       is not a press. Up stands the body up where stand_up finds room. */
    if (b->down_window) --b->down_window;
    if (input.down_pressed && !input.shoot_held) {
        if (b->down_window || (b->crouching && b->grounded)) { b->ball = 1; b->down_window = b->crouch_latch = 0; }
        else b->down_window = MM_DOUBLE_TAP_TICKS;
    }
    if (input.up_pressed && stand_up(w, b)) b->ball = 0;
    /* Crouch needs no direction for a hold, and holds while a standing box would hit a ceiling. */
    b->crouching = b->grounded && !b->ball &&
        ((input.aim_y > 0 && !direction) || b->crouch_latch || (b->crouching && !fits(w, b, b->stand_box)));
    b->hitbox = b->ball ? ball_box(b->stand_box) :
                b->crouching ? crouch_box(b->stand_box) : b->stand_box;
    b->vx = b->wall_lock ? b->wall_vx : direction * (MM_SPEED + b->run_extra);
    if (b->wall_lock) --b->wall_lock;
    if (input.axis) b->facing_left = input.axis < 0;
    if (b->jumped) {
        b->vy = wall_jump ? -MM_WALL_JUMP : -MM_JUMP; b->grounded = 0;
        /* A kick shows its own pose first, a spin goes straight into the spin, a standing jump shows its
           takeoff. A ball that jumps stays a ball. */
        b->pose = wall_jump ? MM_POSE_WALL_KICK : b->spin_jumping ? MM_POSE_SPIN : MM_POSE_TAKEOFF;
        b->pose_timer = wall_jump ? MM_WALL_KICK_TICKS : b->spin_jumping ? 0 : MM_TAKEOFF_TICKS;
        if (b->ball) { b->pose = MM_POSE_BALL; b->pose_timer = 0; }
    }
    /* Releasing jump while rising caps the ascent, so a tap makes a short hop. */
    if (!input.jump_held && b->vy < -MM_JUMP_CUT) b->vy = -MM_JUMP_CUT;
    b->vy += MM_GRAVITY;
    if (b->vy > MM_MAX_FALL) b->vy = MM_MAX_FALL;
    mm_move(w, b);
    if (!b->vx) b->run_extra = 0;
    /* Before the spin flag is cleared: a landing needs to know whether it came from a spin. */
    update_pose(b, was_grounded, was_crouching, was_ball);
    if (b->grounded) { b->spin_jumping = 0; b->wall_lock = 0; }
    if (b->grounded) b->coyote = MM_COYOTE_TICKS;
    else if (b->coyote) --b->coyote;
    b->state = body_state(b);
    b->aim = body_aim(b, input);
}

const char *mm_state_name(MmState state)
{
    static const char *const names[] = {"STAND", "WALK", "CROUCH", "BALL", "JUMP", "SPIN", "FALL"};
    return names[state];
}
