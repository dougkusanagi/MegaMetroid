#pragma once
#include <genesis.h>

typedef struct {
    Vect2D_s16 position;
    Vect2D_f16 velocity;

    s16 tile_width;
    s16 tile_height;

    s16 pixel_width;
    s16 pixel_height;
    
    bool is_on_floor;
    bool is_flipped;
	bool is_jumping;
	bool is_falling;

    Sprite* sprite;
	u16 current_animation;

    struct {
        Vect2D_s16 d_pad;
        bool a;
        bool b;
        bool c;
        bool x;
        bool y;
        bool z;
    } control;
} Player;


typedef struct {
    Vect2D_s16 position;
} Camera;

Vect2D_u16 newVector2D_u16(u16 x, u16 y);
Vect2D_s16 newVector2D_s16(s16 x, s16 y);

