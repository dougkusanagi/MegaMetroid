#ifndef MM_WORLDMAP_H
#define MM_WORLDMAP_H
/* Portable C: the room graph of a region (rooms/world.json), read-only. */

#define MM_NO_NODE (-1)
/* The wall of a room a door sits on; the numbers are the original's travel directions. */
enum { MM_WALL_RIGHT, MM_WALL_LEFT, MM_WALL_BOTTOM, MM_WALL_TOP };
/* Regions a door can lead out to (leaves_area); 0 means the door stays inside the region. */
enum { MM_AREA_HERE, MM_AREA_BRINSTAR, MM_AREA_NORFAIR, MM_AREA_WRECKED_SHIP, MM_AREA_MARIDIA, MM_AREA_TOURIAN };

typedef struct {
    int wall;
    int at_x, at_y; /* Door cap in 16 px blocks, -1 for a door out of the region. */
    int to; /* Index of the room it leads to, MM_NO_NODE out of the region. */
    int peer; /* Index of the matching door in that room. */
    int leaves_area; /* MM_AREA_* of the room outside the region, MM_AREA_HERE otherwise. */
} MmWorldDoor;

typedef struct {
    unsigned id; /* Address of the original room header: a stable name, e.g. 0x91F8 = Landing Site. */
    int screens_w, screens_h; /* Size in 16x16-block screens. */
    int map_x, map_y; /* Position on the region map, in map cells. */
    int room; /* The playable room in the room catalog (room_get), MM_NO_NODE if not built. */
    int door_count;
    const MmWorldDoor *doors;
} MmWorldNode;

/* Fills order with the rooms reachable from start without leaving the region, in discovery
   order (breadth first); returns how many. Stops at capacity. */
int mm_world_reach(const MmWorldNode *nodes, int count, int start, int *order, int capacity);
/* Node of a catalog room, MM_NO_NODE if none. */
int mm_world_node_of_room(const MmWorldNode *nodes, int count, int room);
/* Whether every door that stays in the region is answered by its peer on the opposite wall. */
int mm_world_consistent(const MmWorldNode *nodes, int count);
#endif
