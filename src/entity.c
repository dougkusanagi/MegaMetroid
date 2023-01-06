#include "entity.h"
#include "types.h"
#include "physics.h"

AABB ENTITY_getCollisionPosition(Entity * const entity)
{
    return newAABB(
        entity->position.x + 8,
        entity->position.x + tileToPixel(entity->tile_collision_width),
        entity->position.y + 8,
        entity->position.y + tileToPixel(entity->tile_collision_height)
    );
}

void ENTITY_setCollisionPosition(Entity * const entity, s16 x, s16 y)
{
    ENTITY_setPosition(entity, x - 8, y - 8);
}

void ENTITY_setPosition(Entity * const entity, s16 x, s16 y)
{
    entity->position.x = x;
    entity->position.y = y;

    entity->box_collision = ENTITY_getCollisionPosition(entity);
}

void ENTITY_moveSprite(Entity * const entity, s16 x, s16 y)
{
    SPR_setPosition(entity->sprite, x, y);
}

void ENTITY_setAnimation(Entity * const entity, u16 animation)
{
    SPR_setAnim(entity->sprite, animation);
    entity->current_animation = animation;
}