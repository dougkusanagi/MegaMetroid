#pragma once
#include <genesis.h>

typedef struct {
    Vect2D_s16 position;
} Camera;

typedef struct {
	u8 x;
	u8 y;
} Vect2D_u8;

typedef struct {
	s8 x;
	s8 y;
} Vect2D_s8;

typedef struct {
	Vect2D_s16 min;
	Vect2D_s16 max;
} AABB;

struct {
    Vect2D_s16 d_pad;
    bool a;
    bool b;
    bool c;
    bool x;
    bool y;
    bool z;
} control;

typedef struct level_def
{
	TileSet *tileset_fg;
	TileSet *tileset_bg;
	Image *image_fg;
	Image *image_bg;
	MapDefinition *map_fg;
	MapDefinition *map_bg;
	Palette *palette_fg;
	Palette *palette_bg;
	
	u16 map_width;
	u16 map_height;
  
	// 1D array with all collisions in 16x16
	// should be converted to 2D array when loaded to curr_collision_map
	u8 *map_collision;
	AABB room_size;

	Sprite *level_elements;
	u8 *num_level_elements;
	Sprite enemies;
	u8 num_enemies;

	Vect2D_s16 player_initial_pos;
} level_def;

// Empty array
// typedef struct InstanceData{
//     s8 type;
//     s16 x, y;
// }InstanceData;

// Level definition
// typedef struct Level
// {
//     InstanceData instances[];
// }Level;

// Then in level03.c
// const Level level03 = {
//     .instances = {
        // {PLAYER, 80, 152}, {GAME_MANAGER, 16, 16}, {ENEMY, 152, 128}
//     }
// };

AABB newAABB(s16 x1, s16 x2, s16 y1, s16 y2);

Vect2D_f16 newVector2D_f16(f16 x, f16 y);
Vect2D_f32 newVector2D_f32(f32 x, f32 y);

Vect2D_s8 newVector2D_s8(s8 x, s8 y);
Vect2D_s16 newVector2D_s16(s16 x, s16 y);
Vect2D_s32 newVector2D_s32(s32 x, s32 y);

Vect2D_u8 newVector2D_u8(u8 x, u8 y);
Vect2D_u16 newVector2D_u16(u16 x, u16 y);
Vect2D_u32 newVector2D_u32(u32 x, u32 y);

