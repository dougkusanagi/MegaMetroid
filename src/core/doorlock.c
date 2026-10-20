#include "core/doorlock.h"

void mm_lock_init(MmDoorLock *lock, int open)
{
    lock->state = open ? MM_DOOR_OPEN : MM_DOOR_CLOSED;
    lock->ticks = lock->hits = 0;
}

int mm_lock_hit(MmDoorLock *lock, int color, int shot)
{
    int needed;
    if (lock->state != MM_DOOR_CLOSED) return 0;
    switch (color) {
    case MM_LOCK_BLUE: needed = 1; break;
    case MM_LOCK_RED: needed = shot == MM_SHOT_SUPER ? 1 : shot == MM_SHOT_MISSILE ? MM_RED_DOOR_HITS : 0; break;
    default: needed = shot == MM_SHOT_SUPER ? 1 : 0; break;
    }
    if (!needed) return 0;
    /* A super opens a red door at once; missiles are counted. */
    if (needed == 1 || ++lock->hits >= needed) {
        lock->state = MM_DOOR_OPENING;
        lock->ticks = 0;
        return 1;
    }
    return 0;
}

void mm_lock_tick(MmDoorLock *lock)
{
    if (lock->state != MM_DOOR_OPENING) return;
    if (++lock->ticks >= MM_DOOR_OPEN_TICKS) lock->state = MM_DOOR_OPEN;
}

int mm_lock_solid(const MmDoorLock *lock) { return lock->state != MM_DOOR_OPEN; }

int mm_lock_frame(const MmDoorLock *lock)
{
    if (lock->state == MM_DOOR_OPEN) return MM_DOOR_FRAMES;
    return lock->state == MM_DOOR_CLOSED ? 0 : lock->ticks / MM_DOOR_FRAME_TICKS;
}
