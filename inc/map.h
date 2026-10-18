#pragma once
#include "types.h"
#include "map_crateria_1.h"
#include "map_crateria_2.h"

#define ROOM_CRATERIA_1 0
#define ROOM_CRATERIA_2 1
#define ROOM_COUNT 2

const room_def *room_get(u16 index);
