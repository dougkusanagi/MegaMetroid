#pragma once

#include <genesis.h>

typedef struct
{
    Vect2D_s16 position;
    Vect2D_f16 velocity;

    bool is_on_floor;
    bool is_flipped;

    u8 current_animation;
    Sprite *sprite;
} Entity;
