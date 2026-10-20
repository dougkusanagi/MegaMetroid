#ifndef MM_WORLD_H
#define MM_WORLD_H
/* Portable C: no SGDK, allocation, rendering or global mutable state. */
#define MM_UNIT 256
#define MM_TILE 8
/* Horizontal values are the Super Metroid NTSC ones, measured in the original (docs/FISICA.md). */
#define MM_SPEED 704 /* 2.75 px/tick, the walking speed. */
#define MM_WALK_ACCEL 48 /* +0.1875 px/tick each tick... */
#define MM_WALK_SNAP 480 /* ...and past 1.875 the walk jumps to MM_SPEED: full speed on the 11th tick. */
#define MM_DECEL 128 /* -0.5 px/tick when letting go on the ground, turning around or reversing in the air. */
#define MM_TURN_TICKS 7 /* Turning around on the ground or in a normal jump or fall, the tick it starts on included. */
#define MM_TURN_POSE_TICKS 6 /* Three original turn frames, two simulation ticks each. */
#define MM_RUN_ACCEL 16 /* Dash: +1/16 px/tick each tick... */
#define MM_RUN_EXTRA_MAX 512 /* ...up to +2 (4.75 px/tick in all) after 32 ticks. */
#define MM_AIR_ACCEL 192 /* +0.75 px/tick in the air and in the ball. */
#define MM_JUMP_AIR_SPEED 320 /* 1.25 px/tick in a normal jump, which stops when the direction is let go. */
#define MM_SPIN_AIR_SPEED 352 /* 1.375 px/tick in a spin or wall jump, which keeps its momentum. */
#define MM_FALL_AIR_SPEED 256 /* 1 px/tick after walking off a ledge (alternating with 1.75). */
#define MM_BALL_SPEED 832 /* 3.25 px/tick rolling. */
/* Units and references in docs/FISICA.md. Speeds are Q24.8 px per tick. */
/* Vertical values are the Super Metroid NTSC ones, measured in the original (docs/FISICA.md). */
#define MM_GRAVITY 28 /* 0.109375 px/tick^2 (SM $1C00 subpixels). */
#define MM_JUMP 1248 /* 4.875 px/tick (SM $04E0); 111 px high, 92 ticks in the air. */
#define MM_MAX_FALL 1280 /* Falling stops accelerating once it reaches 5 px/tick, so it ends at 5.03. */
#define MM_JUMP_STILL 2 /* A standing jump rises from its third tick; a spin or wall jump from its second. */
#define MM_WALL_JUMP 1184 /* 4.625 px/tick (SM $04A0). */
#define MM_WALL_REACH 8 /* A wall up to 8 px behind still counts as touched. */
/* Pose durations in simulation ticks; short transitions use measured timings, not the SGDK default. */
#define MM_TAKEOFF_TICKS 5 /* 4B: one frame. */
#define MM_WALL_KICK_TICKS 10 /* 83/84: two frames, then the spin. */
#define MM_LAND_TICKS 7 /* A4/A5: first frame 5 ticks, second 2; ends when walking starts. */
#define MM_SPIN_LAND_TICKS 1 /* A6/A7: one tick before returning to movement. */
#define MM_CROUCH_DOWN_TICKS 3 /* 35/36, measured in the original. */
#define MM_STAND_UP_TICKS 3 /* 3B/3C, measured in the original. */
#define MM_DOUBLE_TAP_TICKS 20 /* Two presses of down less than this (1/3 s) apart morph into the ball. */
#define MM_MORPH_TICKS 6 /* 37/38: two frames, three ticks each. */
#define MM_UNMORPH_TICKS 6 /* 3D/3E: two frames, three ticks each; ends crouched on the ground. */
#define MM_SHOOT_POSE_TICKS 10 /* The cannon stays out this long after the shot button is let go. */

typedef struct { int left, top, right, bottom; } MmRect;
/* Tile grid plus rectangles that block like walls on top of it (closed doors); both optional by zero. */
typedef struct { const unsigned char *tiles; int columns, rows; const MmRect *solids; int solid_count; } MmWorld;

typedef enum {
    MM_BOX_STAND, MM_BOX_JUMP, MM_BOX_SPIN, MM_BOX_CROUCH, MM_BOX_BALL,
    MM_BOX_AIR_DOWN, MM_BOX_COUNT
} MmHitboxKind;
/* Original steady-state radii, expressed relative to the port's drawing origin. */
extern const MmRect mm_samus_boxes[MM_BOX_COUNT];
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
    int samus_hitboxes; /* Production Samus profile; custom mm_body_init boxes stay generic. */
    int grounded, facing_left, jumped;
    int speed, run_extra; /* Base speed along the facing direction and the dash bonus, Q24.8 px/tick. */
    int turn, turned; /* Ticks left turning around, and whether the body already faces the new way. */
    int turn_pose_ticks, turn_pose_left; /* Visual turn, facing at its start (independent of braking). */
    int slide, walking; /* Ground: sliding after letting go, already walking. */
    int falling; /* In the air without a jump: walked off a ledge. */
    int spin_jumping;
    int shoot_pose_timer; /* Ticks the shooting pose still shows; refilled while the shot is held. */
    int ball, crouching; /* Toggled by down/up presses and the d-pad. */
    int crouch_latch; /* Crouch persists after down; jump, direction or a new up press ends it. */
    int down_window; /* Ticks left in which a second press of down morphs the body. */
    int takeoff; /* Ticks a jump from the ground still waits, walking on the ground, before it rises. */
    MmState state; /* Derived at the end of every mm_tick. */
    MmPose pose; /* Derived at the end of every mm_tick, like state. */
    MmAim aim; /* Derived at the end of every mm_tick, while the d-pad is held, shot or not. */
    int jump_frame, jump_frame_ticks; /* Normal jump: rise 0/1, descend 2/3/4, hold 5. */
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
void mm_samus_init(MmBody *body, int x, int y);
void mm_move(const MmWorld *world, MmBody *body);
void mm_tick(const MmWorld *world, MmBody *body, MmInput input);
MmInput mm_input(MmInputState *state, unsigned short buttons,
                 unsigned short left, unsigned short right, unsigned short jump);
const char *mm_state_name(MmState state);
MmCamera mm_camera(int x, int y, int width, int height, MmRect bounds);
MmRect mm_camera_bounds(int x, int y, MmRect fallback,
                        const MmCameraZone *zones, int count);

/* Doors: the side a door faces is where the room is; the far edge is the wall it sits in. */
#define MM_DOOR_DEPTH 32 /* Across the opening. */
#define MM_DOOR_LENGTH 64 /* Along the wall. */
#define MM_DOOR_EDGE 8 /* Depth of the strip at the far edge that Samus must reach to go through. */
enum { MM_DOOR_FACING_LEFT, MM_DOOR_FACING_RIGHT, MM_DOOR_FACING_UP, MM_DOOR_FACING_DOWN };
MmRect mm_door_rect(int x, int y, int facing);
/* The cap: the half of the door on the room's side, which stays solid until the door is open. */
MmRect mm_door_cap(int x, int y, int facing);
int mm_door_touched(MmRect body, int x, int y, int facing);
#endif
