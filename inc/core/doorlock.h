#ifndef MM_DOORLOCK_H
#define MM_DOORLOCK_H
/* Portable C: what it takes to open a door, and the door's state while it opens. */

/* Same order as RoomDoorColor. */
enum { MM_LOCK_BLUE, MM_LOCK_RED, MM_LOCK_GREEN };
/* What hit the door. A beam shot is any beam. */
enum { MM_SHOT_BEAM, MM_SHOT_MISSILE, MM_SHOT_SUPER };
enum { MM_DOOR_CLOSED, MM_DOOR_OPENING, MM_DOOR_OPEN };

#define MM_DOOR_FRAMES 11 /* Frames of the opening animation (doors_door_r00). */
#define MM_DOOR_FRAME_TICKS 4 /* Provisional: not measured in the original yet. */
#define MM_DOOR_OPEN_TICKS (MM_DOOR_FRAMES * MM_DOOR_FRAME_TICKS)
#define MM_RED_DOOR_HITS 5 /* Missiles a red door takes. */

typedef struct { int state, ticks, hits; } MmDoorLock;

void mm_lock_init(MmDoorLock *lock, int open);
/* A shot hit the cap. Returns 1 when it started the opening. Blue takes anything, red missiles
   (MM_RED_DOOR_HITS of them) and supers, green supers only; other shots are turned away. */
int mm_lock_hit(MmDoorLock *lock, int color, int shot);
void mm_lock_tick(MmDoorLock *lock);
/* Whether the cap still blocks: until the animation is done. */
int mm_lock_solid(const MmDoorLock *lock);
/* Animation frame to show: 0 shut ... MM_DOOR_FRAMES fully open. */
int mm_lock_frame(const MmDoorLock *lock);
#endif
