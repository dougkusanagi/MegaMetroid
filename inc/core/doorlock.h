#ifndef MM_DOORLOCK_H
#define MM_DOORLOCK_H
/* Portable C: what it takes to open a door, and the door's state while it opens. */

/* Same order as RoomDoorColor. */
enum { MM_LOCK_BLUE, MM_LOCK_RED, MM_LOCK_GREEN };
/* What hit the door. A beam shot is any beam. */
enum { MM_SHOT_BEAM, MM_SHOT_MISSILE, MM_SHOT_SUPER };
enum { MM_DOOR_CLOSED, MM_DOOR_OPENING, MM_DOOR_OPEN };

/* Measured in the original (tests/reference/super_metroid/door_open): after the shot lands the cap
   goes through three shapes of 6 ticks each, then is gone and Samus can walk through. The images are
   doors_door_r00 frames 0 (shut), 1-3 (shapes) and 4 (open). */
#define MM_DOOR_STEPS 3
#define MM_DOOR_STEP_TICKS 6
#define MM_DOOR_OPEN_TICKS (MM_DOOR_STEPS * MM_DOOR_STEP_TICKS)
#define MM_DOOR_FRAMES (MM_DOOR_STEPS + 1) /* Image of the fully open door. */
#define MM_RED_DOOR_HITS 5 /* Missiles a red door takes. */

typedef struct { int state, ticks, hits; } MmDoorLock;

void mm_lock_init(MmDoorLock *lock, int open);
/* A shot hit the cap. Returns 1 when it started the opening. Blue takes anything, red missiles
   (MM_RED_DOOR_HITS of them) and supers, green supers only; other shots are turned away. */
int mm_lock_hit(MmDoorLock *lock, int color, int shot);
void mm_lock_tick(MmDoorLock *lock);
/* Whether the cap still blocks: until the animation is done. */
int mm_lock_solid(const MmDoorLock *lock);
/* Image to show: 0 shut, 1..MM_DOOR_STEPS the shapes, MM_DOOR_FRAMES fully open. */
int mm_lock_frame(const MmDoorLock *lock);
#endif
