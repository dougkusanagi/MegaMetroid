#pragma once
#include <genesis.h>
#include "core/world.h"

#define ROOM_NO_CONNECTION 0xFFFF

typedef struct {
    u16 kind;
    Vect2D_s16 position;
} RoomEnemySpawn;

typedef enum {
    ROOM_DOOR_BLUE,
    ROOM_DOOR_RED,
    ROOM_DOOR_GREEN
} RoomDoorColor;

typedef enum {
    ROOM_DOOR_FACING_LEFT,
    ROOM_DOOR_FACING_RIGHT,
    ROOM_DOOR_FACING_UP,
    ROOM_DOOR_FACING_DOWN
} RoomDoorFacing;

typedef struct {
    Vect2D_s16 position; /* Upper-left corner in world pixels. */
    u16 target_room; /* ROOM_NO_CONNECTION until the route is confirmed. */
    Vect2D_s16 target_spawn;
    RoomDoorColor color;
    RoomDoorFacing facing;
} RoomDoorDef;

typedef struct room_def {
    const TileSet *tileset_fg, *tileset_bg;
    const Image *image_fg, *image_bg;
    const MapDefinition *map_fg, *map_bg;
    const Palette *palette_fg, *palette_bg;
    u16 map_width, map_height;
    u16 background_scroll_height; /* 0: fixed; nonzero: continuous camera Y, MAP wraps its resource. */
    const u8 *map_collision;
    Vect2D_s16 player_initial_pos;
    const MmCameraZone *camera_zones;
    u16 camera_zone_count;
    const RoomEnemySpawn *enemy_spawns;
    u16 enemy_spawn_count;
    const RoomDoorDef *doors;
    u16 door_count;
} room_def;
