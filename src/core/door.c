#include "core/world.h"

MmRect mm_door_rect(int x, int y, int facing)
{
    int vertical = facing == MM_DOOR_FACING_UP || facing == MM_DOOR_FACING_DOWN;
    MmRect r = {x, y, x + (vertical ? MM_DOOR_LENGTH : MM_DOOR_DEPTH),
                y + (vertical ? MM_DOOR_DEPTH : MM_DOOR_LENGTH)};
    return r;
}

/* Samus goes through a door by pushing into the strip at its far edge (the wall side). */
int mm_door_touched(MmRect body, int x, int y, int facing)
{
    MmRect strip = mm_door_rect(x, y, facing);
    switch (facing) {
    case MM_DOOR_FACING_LEFT: strip.left = strip.right - MM_DOOR_EDGE; break;
    case MM_DOOR_FACING_RIGHT: strip.right = strip.left + MM_DOOR_EDGE; break;
    case MM_DOOR_FACING_UP: strip.top = strip.bottom - MM_DOOR_EDGE; break;
    default: strip.bottom = strip.top + MM_DOOR_EDGE; break;
    }
    return body.left < strip.right && body.right > strip.left &&
           body.top < strip.bottom && body.bottom > strip.top;
}
