#ifndef MM_WORLD_H
#define MM_WORLD_H
/* Portable C: no SGDK, allocation, rendering or global mutable state. */
#define MM_UNIT 256
#define MM_TILE 8
#define MM_SPEED 704 /* 2.75 pixels/tick, the SNES ground speed cap. */
#define MM_RUN_ACCEL 8 /* +1/32 pixel/tick; +2 after 64 ticks. */
#define MM_RUN_EXTRA_MAX 512
/* Units and references in docs/FISICA.md. Speeds are Q24.8 px per tick. */
#define MM_GRAVITY 56 /* 0.22 px/tick^2; SM NTSC is 0.11 (regional $1C00). */
#define MM_JUMP 1690 /* 6.6 px/tick at takeoff; SM normal jump is 4.875 ($04E0). */
#define MM_MAX_FALL 1536 /* 6 px/tick; SM stops accelerating near 5 px/frame ($0005). */
#define MM_JUMP_CUT 768 /* 3 px/tick: letting go of jump while rising caps the ascent here. */
#define MM_COYOTE_TICKS 6 /* Refilled while grounded; a jump still starts in the 5 ticks after leaving. */
#define MM_WALL_JUMP 1184 /* 4.625 px/tick takeoff; SM wall jump is $04A0 (4.625 px/frame). */
#define MM_WALL_LOCK 12 /* Ticks a wall kick ignores steering and keeps its speed away from the wall. */
#define MM_WALL_REACH 8 /* A wall up to 8 px behind still counts as touched: pressing away moves 2.75 px a tick. */
/* One-shot poses last whole frames of the sheet, which play 5 ticks each (SPRITE ... 5 in res/resources.res). */
#define MM_TAKEOFF_TICKS 5 /* 4B: one frame. */
#define MM_WALL_KICK_TICKS 10 /* 83/84: two frames, then the spin. */
#define MM_LAND_TICKS 10 /* A4/A5, standing landing: two frames; ends at once when the body starts walking. */
#define MM_SPIN_LAND_TICKS 15 /* A6/A7, landing from a spin jump: three frames. */
#define MM_CROUCH_DOWN_TICKS 5 /* 35/36, crouching: one frame. */
#define MM_STAND_UP_TICKS 5 /* 3B/3C, standing up from a crouch: one frame. */
#define MM_TAP_TICKS 12 /* Down held for up to 12 ticks (0.2 s) is a tap: the crouch stays after release. */
#define MM_DOUBLE_TAP_TICKS 20 /* Two presses of down less than this (1/3 s) apart morph into the ball. */
#define MM_MORPH_TICKS 10 /* 37/38, curling into the ball: two frames. */
#define MM_UNMORPH_TICKS 10 /* 3D/3E, uncurling from the ball: two frames. */
#define MM_SHOOT_POSE_TICKS 10 /* The cannon stays out this long after the shot button is let go. */

typedef struct { int left, top, right, bottom; } MmRect;
typedef struct { const unsigned char *tiles; int columns, rows; } MmWorld;
typedef enum {
    MM_STATE_STAND, MM_STATE_WALK, MM_STATE_CROUCH, MM_STATE_BALL,
    MM_STATE_JUMP, MM_STATE_SPIN, MM_STATE_FALL
} MmState;
/* Sprite pose. GROUND covers stand and walk, which the SGDK layer tells apart by vx. */
typedef enum {
    MM_POSE_GROUND, MM_POSE_TAKEOFF, MM_POSE_JUMP_UP, MM_POSE_FALL,
    MM_POSE_SPIN, MM_POSE_WALL_KICK, MM_POSE_LAND, MM_POSE_SPIN_LAND,
    MM_POSE_CROUCH_DOWN, MM_POSE_CROUCH, MM_POSE_STAND_UP,
    MM_POSE_MORPH, MM_POSE_BALL, MM_POSE_UNMORPH
} MmPose;
/* Where the cannon points, from the d-pad: up, up and forward, down (in the air), down and forward. */
typedef enum {
    MM_AIM_FORWARD, MM_AIM_UP, MM_AIM_DIAG_UP, MM_AIM_DOWN, MM_AIM_DIAG_DOWN
} MmAim;
typedef struct {
    int x, y; /* Q24.8 world position; never discard fractional movement. */
    int vx, vy;
    MmRect hitbox; /* Half-open local pixel bounds; crouch and ball shrink it. */
    MmRect stand_box; /* Standing hitbox, the base for the crouch and ball boxes. */
    int grounded, facing_left, jumped;
    int run_extra, move_direction;
    int spin_jumping;
    int shoot_pose_timer; /* Ticks the shooting pose still shows; refilled while the shot is held. */
    int ball, crouching; /* Toggled by down/up presses and the d-pad. */
    int down_ticks; /* Ticks down has been held, capped just past MM_TAP_TICKS. */
    int crouch_latch; /* Crouch kept after a tap on the ground; a jump, a direction or up ends it. */
    int down_window; /* Ticks left in which a second press of down morphs the body. */
    int coyote; /* Ticks left in which a grounded jump is still allowed. */
    int wall_lock, wall_vx; /* Wall kick: ticks left and the fixed speed while locked. */
    MmState state; /* Derived at the end of every mm_tick. */
    MmPose pose; /* Derived at the end of every mm_tick, like state. */
    MmAim aim; /* Derived at the end of every mm_tick, while the d-pad is held, shot or not. */
    int pose_timer; /* Ticks left in a one-shot pose (takeoff, wall kick, landing); 0 for looping poses. */
} MmBody;
typedef struct {
    int axis, jump_pressed;
    int run_held, shoot_held, shoot_pressed, aim_y;
    int jump_held, down_pressed, up_pressed;
} MmInput;
typedef struct { unsigned short previous; } MmInputState;
typedef struct { MmRect trigger, bounds; } MmCameraZone;
typedef struct { int x, y; } MmCamera;

int mm_pixel(int fixed);
int mm_tile(const MmWorld *world, int x, int y);
int mm_rect_blocked(const MmWorld *world, MmRect rect);
int mm_blocked(const MmWorld *world, const MmBody *body, int x, int y);
void mm_body_init(MmBody *body, int x, int y, MmRect hitbox);
void mm_move(const MmWorld *world, MmBody *body);
void mm_tick(const MmWorld *world, MmBody *body, MmInput input);
MmInput mm_input(MmInputState *state, unsigned short buttons,
                 unsigned short left, unsigned short right, unsigned short jump);
const char *mm_state_name(MmState state);
MmCamera mm_camera(int x, int y, int width, int height, MmRect bounds);
MmRect mm_camera_bounds(int x, int y, MmRect fallback,
                        const MmCameraZone *zones, int count);
#endif
