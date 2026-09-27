#pragma once
#include <genesis.h>
#include "core/world.h"

typedef struct level_def {
    const TileSet *tileset_fg, *tileset_bg;
    const Image *image_fg, *image_bg;
    const MapDefinition *map_fg, *map_bg;
    const Palette *palette_fg, *palette_bg;
    u16 map_width, map_height;
    const u8 *map_collision;
    Vect2D_s16 player_initial_pos;
    const MmCameraZone *camera_zones;
    u16 camera_zone_count;
} level_def;
