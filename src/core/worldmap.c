#include "core/worldmap.h"

int mm_world_reach(const MmWorldNode *nodes, int count, int start, int *order, int capacity)
{
    int found = 0, next, i;
    if (start < 0 || start >= count || capacity <= 0) return 0;
    order[found++] = start;
    for (next = 0; next < found; ++next) {
        const MmWorldNode *node = &nodes[order[next]];
        for (i = 0; i < node->door_count; ++i) {
            int to = node->doors[i].to, j, known = 0;
            if (to < 0 || to >= count) continue;
            for (j = 0; j < found; ++j) known |= order[j] == to;
            if (!known && found < capacity) order[found++] = to;
        }
    }
    return found;
}

int mm_world_node_of_room(const MmWorldNode *nodes, int count, int room)
{
    int i;
    if (room < 0) return MM_NO_NODE;
    for (i = 0; i < count; ++i) if (nodes[i].room == room) return i;
    return MM_NO_NODE;
}

int mm_world_consistent(const MmWorldNode *nodes, int count)
{
    int n, i;
    for (n = 0; n < count; ++n) {
        for (i = 0; i < nodes[n].door_count; ++i) {
            const MmWorldDoor *d = &nodes[n].doors[i];
            const MmWorldDoor *back;
            if (d->to == MM_NO_NODE) {
                if (d->leaves_area == MM_AREA_HERE) return 0;
                continue;
            }
            if (d->to < 0 || d->to >= count || d->peer < 0 || d->peer >= nodes[d->to].door_count) return 0;
            back = &nodes[d->to].doors[d->peer];
            if (back->to != n || back->peer != i || back->wall != (d->wall ^ 1)) return 0;
        }
    }
    return 1;
}
