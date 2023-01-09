#include "entity.h"
#include "types.h"
#include "physics.h"

void Entity_setCollisionPosition(Entity * const entity, s16 x, s16 y)
{
    Entity_setPosition(entity, x - 8, y - 8);
}

void Entity_setPosition(Entity * const entity, s16 x, s16 y)
{
    entity->position.x = x;
    entity->position.y = y;
}

void Entity_moveSprite(Entity * const entity, s16 x, s16 y)
{
    SPR_setPosition(entity->sprite, x, y);
}

void Entity_setAnimation(Entity * const entity, u16 animation)
{
    SPR_setAnim(entity->sprite, animation);
    entity->current_animation = animation;
}