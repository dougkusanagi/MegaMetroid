#include "map.h"
const room_def *room_get(u16 index)
{
    static const room_def * const rooms[ROOM_COUNT] = {&room_crateria_1, &room_crateria_2};
    return index < ROOM_COUNT ? rooms[index] : NULL;
}
