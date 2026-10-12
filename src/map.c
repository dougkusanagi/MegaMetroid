#include "map.h"
const level_def *level_get(u16 index)
{
    static const level_def * const levels[LEVEL_COUNT] = {&level_crateria_1, &level_crateria_2};
    return index < LEVEL_COUNT ? levels[index] : NULL;
}
