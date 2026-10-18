#include "core/world.h"

/* Width = 2*5; heights = 2*{21,19,12,16,7,10}, from saved Mesen RAM traces.
   Standing/jumping/spinning share centre y+27. Ground crouch/ball keep feet y+48. */
const MmRect mm_samus_boxes[MM_BOX_COUNT] = {
    {15, 6, 25, 48}, {15, 8, 25, 46}, {15, 15, 25, 39},
    {15, 16, 25, 48}, {15, 34, 25, 48}, {15, 17, 25, 37}
};

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
    b->speed = b->run_extra = b->turn = b->slide = b->walking = b->falling = 0;
    b->turn_pose_ticks = b->turn_pose_left = 0;
    b->spin_jumping = b->shoot_pose_timer = 0;
    b->aim = MM_AIM_FORWARD;
    b->jump_frame = 0; b->jump_frame_ticks = 3;
    b->ball = b->crouching = 0;
    b->crouch_latch = b->down_window = 0;
    b->takeoff = 0;
    b->state = MM_STATE_STAND;
    b->pose = MM_POSE_GROUND; b->pose_timer = 0;
    b->stand_box = hitbox;
    b->samus_hitboxes = 0;
}

void mm_samus_init(MmBody *b, int x, int y)
{
    mm_body_init(b, x, y, mm_samus_boxes[MM_BOX_STAND]);
    b->samus_hitboxes = 1;
}

/* Crouch keeps the feet and drops the top by 2/5 of the standing height (40 -> 24 px). */
static MmRect crouch_box(const MmBody *b)
{
    MmRect s = b->stand_box;
    if (b->samus_hitboxes) return mm_samus_boxes[MM_BOX_CROUCH];
    s.top += (s.bottom - s.top) * 2 / 5;
    return s;
}

/* Ball is a square of 2/3 the standing width (24 -> 16 px), centred on the feet. */
static MmRect ball_box(const MmBody *b)
{
    MmRect s = b->stand_box;
    if (b->samus_hitboxes) return mm_samus_boxes[MM_BOX_BALL];
    int side = (s.right - s.left) * 2 / 3, cx = (s.left + s.right) / 2;
    MmRect r = {cx - side / 2, s.bottom - side, cx - side / 2 + side, s.bottom};
    return r;
}

static int box_blocked(const MmWorld *w, const MmBody *b, MmRect box, int dy)
{
    int x = mm_pixel(b->x), y = mm_pixel(b->y) + dy;
    MmRect r = {x + box.left, y + box.top, x + box.right, y + box.bottom};
    return mm_rect_blocked(w, r);
}

/* True if the body at its current position fits inside box. */
static int fits(const MmWorld *w, const MmBody *b, MmRect box)
{
    return !box_blocked(w, b, box, 0);
}

/* Expands from a ball. On a slope the larger box's lower corners sit in the surface, so the
   body may lift by up to a tile to fit. Returns 0 if no lift up to that makes room. */
static int expand_box(const MmWorld *w, MmBody *b, MmRect box)
{
    int lift, delta = 0;
    if (b->samus_hitboxes && !b->grounded)
        delta = (b->hitbox.top + b->hitbox.bottom - box.top - box.bottom) / 2;
    for (lift = 0; lift <= MM_TILE; ++lift) {
        if (!box_blocked(w, b, box, delta - lift)) {
            b->y += (delta - lift) * MM_UNIT;
            if (b->samus_hitboxes) b->hitbox = box;
            return 1;
        }
    }
    return 0;
}

/* Resize without clipping. Keep grounded feet; in the air keep the collision centre.
   The original air morph moves the centre down 9 px (crouch radius 16 -> ball 7).
   If an expansion cannot fit, retain the previous box until there is room. */
static int resize_samus(const MmWorld *w, MmBody *b, MmRect box, int morphing)
{
    int delta = b->grounded ? b->hitbox.bottom - box.bottom :
        (b->hitbox.top + b->hitbox.bottom - box.top - box.bottom) / 2;
    if (!morphing && box.left == b->hitbox.left && box.top == b->hitbox.top &&
        box.right == b->hitbox.right && box.bottom == b->hitbox.bottom) return 1;
    if (!b->grounded && morphing) delta += 9;
    if (box_blocked(w, b, box, delta)) return 0;
    b->y += delta * MM_UNIT;
    b->hitbox = box;
    return 1;
}

static MmRect samus_box(const MmBody *b)
{
    if (b->ball) return mm_samus_boxes[MM_BOX_BALL];
    if (b->pose == MM_POSE_UNMORPH && b->pose_timer) return mm_samus_boxes[MM_BOX_CROUCH];
    if (b->grounded) return mm_samus_boxes[b->crouching ? MM_BOX_CROUCH : MM_BOX_STAND];
    if (b->pose == MM_POSE_GROUND && !b->vy && !b->falling) return mm_samus_boxes[MM_BOX_STAND];
    if (b->pose == MM_POSE_WALL_KICK && b->pose_timer) return mm_samus_boxes[MM_BOX_JUMP];
    if (b->spin_jumping) return mm_samus_boxes[MM_BOX_SPIN];
    return mm_samus_boxes[b->aim == MM_AIM_DOWN ? MM_BOX_AIR_DOWN : MM_BOX_JUMP];
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
    int hit_x = 0, hit_y = 0;
    /* Rise before testing horizontal motion. At takeoff the feet still touch the
       slope: testing X there first mistakes its rising surface for a wall. */
    int rising = b->vy < 0;
    if (rising) {
        target = mm_pixel(b->y + b->vy);
        for (p = y; p != target;) {
            if (mm_blocked(w, b, x, p - 1)) { b->vy = 0; hit_y = 1; break; }
            --p;
        }
        b->y = hit_y ? p * MM_UNIT : b->y + b->vy;
        y = p;
    }
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
    target = rising ? y : mm_pixel(b->y + b->vy);
    step = b->vy < 0 ? -1 : 1;
    b->grounded = 0;
    for (p = y; p != target;) {
        if (mm_blocked(w, b, x, p + step)) {
            if (step > 0) b->grounded = 1;
            b->vy = 0; hit_y = 1; break;
        }
        p += step;
    }
    /* Keeps the fraction when nothing was hit, also at speed 0 (the apex of a jump). */
    if (!rising) b->y = hit_y ? p * MM_UNIT : b->y + b->vy;
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
    return b->vy < 0 || b->takeoff ? MM_STATE_JUMP : MM_STATE_FALL;
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
    return b->vy < 0 || b->takeoff ? MM_POSE_JUMP_UP : MM_POSE_FALL;
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

static int min_int(int a, int b) { return a < b ? a : b; }
static int max_int(int a, int b) { return a > b ? a : b; }

/* Original normal-jump pose 4D/4E: the rise holds frame 1 until descending;
   frames 2, 3 and 4 last four ticks each, then frame 5 stays until landing.
   Fall pose 29/2A is reserved for walking off a ledge. */
static void update_jump_animation(MmBody *b)
{
    if (b->grounded || b->takeoff || b->spin_jumping || b->pose == MM_POSE_TAKEOFF) {
        b->jump_frame = 0; b->jump_frame_ticks = 3;
    } else if (b->vy <= 0) {
        if (b->jump_frame_ticks) --b->jump_frame_ticks;
        else b->jump_frame = 1;
    } else if (b->jump_frame < 2) {
        b->jump_frame = 2; b->jump_frame_ticks = 4;
    } else if (b->jump_frame < 5 && !--b->jump_frame_ticks) {
        ++b->jump_frame; b->jump_frame_ticks = 4;
    }
}

/* Horizontal speed, as measured in Super Metroid (docs/FISICA.md). speed is the base speed along the
   facing direction and run_extra the dash bonus on top of it; vx is their sum. */
static void steer(MmBody *b, int direction, int run_held)
{
    int facing = b->facing_left ? -1 : 1, nudge = 0, moved;
    if (b->takeoff && !b->spin_jumping) {
        /* A standing jump holds still through its takeoff; a spin takes off at the walking speed. */
        b->speed = b->run_extra = b->turn = b->slide = b->walking = 0;
        b->vx = 0;
        return;
    }
    if (b->turn && !b->ball) {
        /* After the tick it starts on, a turn slows down by 0.5 a tick to a tick at rest, where the body
           faces the new way. A spin turns there; a turn on the ground or in a normal jump or fall lasts
           MM_TURN_TICKS in all and only wobbles the new way for the rest, 0.75 px every other tick. */
        if (!b->turned) {
            b->speed = max_int(0, b->speed - MM_DECEL); b->run_extra = 0;
            if (!b->speed) {
                b->turned = 1; b->facing_left = !b->facing_left;
                if (b->spin_jumping) b->turn = 1;
            }
        } else b->speed = b->speed ? 0 : MM_AIR_ACCEL;
        --b->turn;
        b->vx = b->facing_left ? -b->speed : b->speed;
        return;
    }
    if (direction == -facing && !b->ball && !(b->grounded && b->crouching) &&
        (b->speed || b->run_extra || !b->spin_jumping)) {
        /* The tick the opposite way is pressed still steers the old way, as the original does; a body at
           rest on the ground stays put. A spin at rest just faces the new way. */
        b->turn = MM_TURN_TICKS - 1; b->turned = b->slide = 0;
        if (!b->spin_jumping) {
            b->turn_pose_ticks = MM_TURN_POSE_TICKS;
            b->turn_pose_left = b->facing_left;
        }
        direction = b->grounded && !b->walking ? 0 : facing;
    }
    if (b->grounded && !b->ball) {
        if (b->crouching || !direction) {
            /* Letting go slides on, slowing down, with one extra pixel a tick, as the original does. */
            if (b->walking && b->speed) b->slide = 1;
            b->walking = b->run_extra = 0;
            if (b->slide) {
                /* The slide moves at the speed it had, then slows down for the next tick. A spin
                   jump from a slide takes off at that speed, without the extra pixel. */
                b->vx = facing * (b->speed + (b->takeoff ? 0 : MM_UNIT));
                if (b->speed) b->speed = max_int(0, b->speed - MM_DECEL);
                else b->slide = 0;
                return;
            }
            b->speed = 0;
        } else if (!b->walking) {
            /* The first step from rest moves one pixel, then the walk accelerates from rest. */
            b->walking = 1; b->slide = b->speed = 0; nudge = MM_UNIT;
        } else {
            b->speed += MM_WALK_ACCEL;
            if (b->speed > MM_WALK_SNAP) b->speed = MM_SPEED;
            b->run_extra = run_held ? min_int(b->run_extra + MM_RUN_ACCEL, MM_RUN_EXTRA_MAX) : 0;
        }
    } else if (b->grounded) {
        /* The ball rolls only while a direction is held and turns at once. */
        b->run_extra = b->turn = b->slide = b->walking = 0;
        if (!direction) b->speed = 0;
        else {
            if (direction != facing) { b->facing_left = direction < 0; b->speed = 0; }
            b->speed = b->speed >= MM_BALL_SPEED ? MM_BALL_SPEED : b->speed + MM_AIR_ACCEL;
        }
    } else {
        int max = b->spin_jumping ? MM_SPIN_AIR_SPEED : b->falling || b->ball ? MM_FALL_AIR_SPEED : MM_JUMP_AIR_SPEED;
        b->slide = b->walking = 0;
        if (!direction && !b->spin_jumping) {
            /* A jump or a fall stops when the direction is let go; a spin carries on as if held. */
            b->speed = b->run_extra = 0;
        } else if (direction && direction != facing && (b->speed || b->run_extra)) {
            /* A ball in the air does not turn around: it slows down, then rolls the new way. */
            b->speed = max_int(0, b->speed - MM_DECEL); b->run_extra = 0;
        } else {
            if (direction) b->facing_left = direction < 0;
            /* A fall alternates 1 and 1.75 px a tick: the original caps it before adding. */
            if (b->falling || b->ball) b->speed = b->speed > max ? max : b->speed + MM_AIR_ACCEL;
            else b->speed = min_int(b->speed + MM_AIR_ACCEL, max);
        }
    }
    moved = b->speed + b->run_extra + nudge;
    b->vx = b->facing_left ? -moved : moved;
}

/* Gravity after the move, as in Super Metroid. The apex keeps one tick at speed 0, since the original
   keeps the speed and its direction apart, and a fall stops accelerating once it reaches MM_MAX_FALL. */
static void fall(MmBody *b)
{
    if (b->vy < 0 && b->vy + MM_GRAVITY > 0) b->vy = 0;
    else if (b->vy < MM_MAX_FALL) b->vy += MM_GRAVITY;
}

void mm_tick(const MmWorld *w, MmBody *b, MmInput input)
{
    int direction = input.axis < 0 ? -1 : input.axis > 0 ? 1 : 0;
    int moving_before_jump = b->vx != 0, wall_jump, was_grounded = b->grounded, was_crouching = b->crouching;
    int was_ball = b->ball;
    if (b->pose_timer) --b->pose_timer;
    if (b->turn_pose_ticks) --b->turn_pose_ticks;
    b->jumped = input.jump_pressed && b->grounded && !b->ball &&
        (!b->crouching || fits(w, b, b->stand_box));
    /* As in Super Metroid, only a jump while walking spins: from rest, even with a direction pressed on
       the same tick, the jump is a standing one. A jump during a slide, with no direction, still spins. */
    if (b->jumped) b->spin_jumping = moving_before_jump && (direction || b->slide);
    /* Wall jump, as in Super Metroid: only in a spin jump, touching a wall and pressing away from it.
       The kick turns the body the pressed way and accelerates from rest, with free steering. */
    wall_jump = input.jump_pressed && !b->jumped && !b->grounded && !b->ball && b->spin_jumping &&
                direction && wall_behind(w, b, direction);
    if (wall_jump) {
        b->jumped = 1; b->spin_jumping = 1;
        b->facing_left = direction < 0; b->speed = b->run_extra = b->turn = 0;
    }
    /* Shooting stops a spin; the cannon stays out while the shot is held and a moment after. */
    if (input.shoot_pressed) b->spin_jumping = 0;
    if (input.shoot_held && !b->ball) b->shoot_pose_timer = MM_SHOOT_POSE_TICKS;
    else if (b->shoot_pose_timer) --b->shoot_pose_timer;
    /* Crouching persists after releasing down, however long it was held. */
    if (b->grounded && !b->ball && input.aim_y > 0 && !direction && !input.shoot_held)
        b->crouch_latch = 1;
    if (b->jumped || !b->grounded || direction || input.up_pressed) b->crouch_latch = 0;
    /* Down pressed while crouched morphs, however long the crouch lasted, as in Super Metroid. Two presses
       less than MM_DOUBLE_TAP_TICKS apart also morph, on the ground or in the air. A shot that aims down
       is not a press. Unmorphing only needs room for the target box. */
    if (b->down_window) --b->down_window;
    if (input.down_pressed && !input.shoot_held) {
        if (b->down_window || (b->crouching && b->grounded)) {
            b->ball = 1; b->spin_jumping = 0; b->down_window = b->crouch_latch = 0;
        }
        else b->down_window = MM_DOUBLE_TAP_TICKS;
    }
    /* Up or jump unmorphs without adding a jump impulse. On the ground it ends
       crouched; another up press stands up.
       Only the target box must fit, so low ceilings can allow crouch but forbid standing. */
    if (b->ball && (input.up_pressed || input.jump_pressed) &&
        expand_box(w, b, b->grounded || b->samus_hitboxes ? crouch_box(b) : b->stand_box)) {
        b->ball = 0;
        b->crouch_latch = b->grounded;
        b->down_window = 0;
        if (b->samus_hitboxes) { b->pose = MM_POSE_UNMORPH; b->pose_timer = MM_UNMORPH_TICKS; }
    }
    /* Crouch needs no direction for a hold, and holds while a standing box would hit a ceiling. */
    b->crouching = b->grounded && !b->ball &&
        ((input.aim_y > 0 && !direction) || b->crouch_latch || (b->crouching && !fits(w, b, b->stand_box)));
    if (!b->samus_hitboxes)
        b->hitbox = b->ball ? ball_box(b) : b->crouching ? crouch_box(b) : b->stand_box;
    if (b->jumped && !wall_jump) b->takeoff = b->spin_jumping ? 1 : MM_JUMP_STILL;
    steer(b, direction, input.run_held);
    if (b->jumped) {
        b->falling = 0;
        /* A wall jump rises at once; a jump from the ground first stands still, as in Super Metroid. */
        if (wall_jump) b->vy = -MM_WALL_JUMP;
        else b->vy = 0;
        b->grounded = 0;
        /* A kick shows its own pose first, a spin goes straight into the spin, a standing jump shows its
           takeoff. A ball that jumps stays a ball. */
        b->pose = wall_jump ? MM_POSE_WALL_KICK : b->spin_jumping ? MM_POSE_SPIN : MM_POSE_TAKEOFF;
        b->pose_timer = wall_jump ? MM_WALL_KICK_TICKS : b->spin_jumping ? 0 : MM_TAKEOFF_TICKS;
        if (b->ball) { b->pose = MM_POSE_BALL; b->pose_timer = 0; }
    }
    /* Releasing jump stops the ascent at once, also before the takeoff ends: a tap is a short hop. */
    if (!input.jump_held) { if (b->vy < 0) b->vy = 0; b->takeoff = 0; }
    /* A takeoff still moves along the ground, slopes included, and rises once it ends. */
    if (b->takeoff) b->grounded = 1;
    if (b->samus_hitboxes) {
        b->aim = body_aim(b, input);
        resize_samus(w, b, samus_box(b), b->ball && !was_ball);
    }
    mm_move(w, b);
    if (b->takeoff) {
        b->grounded = 0;
        if (!--b->takeoff) b->vy = -MM_JUMP;
    } else if (!b->grounded) fall(b);
    if (b->samus_hitboxes && b->grounded && !resize_samus(w, b, samus_box(b), 0) && !b->ball) {
        b->crouching = b->crouch_latch = 1;
        resize_samus(w, b, crouch_box(b), 0);
    }
    /* A wall stops the body; so does a landing, which then accelerates again from rest. Walking off a
       ledge is a fall, slower in the air than a jump. */
    if (!b->vx || (b->grounded && !was_grounded)) { b->speed = b->run_extra = b->slide = b->walking = 0; }
    if (b->grounded && !was_grounded) b->turn = 0;
    if (was_grounded && !b->grounded && !b->jumped) b->falling = 1;
    if (b->grounded) b->falling = 0;
    /* Before the spin flag is cleared: a landing needs to know whether it came from a spin. */
    update_pose(b, was_grounded, was_crouching, was_ball);
    update_jump_animation(b);
    if (b->grounded) b->spin_jumping = 0;
    if (b->ball || b->spin_jumping || b->pose == MM_POSE_WALL_KICK) b->turn_pose_ticks = 0;
    b->state = body_state(b);
    b->aim = body_aim(b, input);
}

const char *mm_state_name(MmState state)
{
    static const char *const names[] = {"STAND", "WALK", "CROUCH", "BALL", "JUMP", "SPIN", "FALL"};
    return names[state];
}
