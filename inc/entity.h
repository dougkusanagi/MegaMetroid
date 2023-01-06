#pragma once

#include <genesis.h>
#include "types.h"

typedef struct {
    Vect2D_s16 position;
    Vect2D_f16 velocity;

	AABB box_collision;

    s16 tile_collision_width;
    s16 tile_collision_height;

    s16 tile_width;
    s16 tile_height;
    
    bool is_on_floor;
    bool is_flipped;

    Sprite* sprite;
	u16 current_animation;
} Entity;

void ENTITY_setAnimation(Entity * const entity, u16 animation);
void ENTITY_setPosition(Entity * const entity, s16 x, s16 y);
void ENTITY_moveSprite(Entity * const entity, s16 x, s16 y);
AABB ENTITY_getCollisionPosition(Entity * const entity);
