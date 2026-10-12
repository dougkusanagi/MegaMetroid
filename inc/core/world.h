#ifndef MM_WORLD_H
#define MM_WORLD_H
/* Portable C: no SGDK, allocation, rendering or global mutable state. */
#define MM_UNIT 256
#define MM_TILE 8
#define MM_SPEED 704 /* 2.75 pixels/tick, the SNES ground speed cap. */
#define MM_RUN_ACCEL 8 /* +1/32 pixel/tick; +2 after 64 ticks. */
#define MM_RUN_EXTRA_MAX 512
#define MM_GRAVITY 56
#define MM_JUMP 1690
#define MM_MAX_FALL 1536

typedef struct { int left, top, right, bottom; } MmRect;
typedef struct { const unsigned char *tiles; int columns, rows; } MmWorld;
typedef enum {
    MM_STATE_STAND, MM_STATE_WALK, MM_STATE_CROUCH, MM_STATE_BALL,
    MM_STATE_JUMP, MM_STATE_SPIN, MM_STATE_FALL
} MmState;
typedef struct {
    int x, y; /* Q24.8 world position; never discard fractional movement. */
    int vx, vy;
    MmRect hitbox; /* Half-open local pixel bounds. */
    int grounded, facing_left, jumped;
    int run_extra, move_direction;
    int spin_jumping, shoot_pose_timer, shoot_frame;
    int ball, crouching; /* Toggled by down/up presses and the d-pad. */
    MmState state; /* Derived at the end of every mm_tick. */
} MmBody;
typedef struct {
    int axis, jump_pressed;
    int run_held, shoot_held, shoot_pressed, aim_y;
    int down_pressed, up_pressed;
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
