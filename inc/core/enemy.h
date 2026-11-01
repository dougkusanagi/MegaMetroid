#ifndef MM_ENEMY_H
#define MM_ENEMY_H
/* Portable C: enemy instances, their behavior table and the shared physics. No SGDK here; the
   game layer owns sprites, palettes and animation (5.2). */
#include "world.h"

/* Pool size for one room; enter_room spawns up to this many and ignores extra room spawns. */
#define MM_ENEMY_MAX 16

/* Kinds index mm_enemy_defs. Zoomer, Skree, Ripper and Geemer replace the placeholder in 5.2. */
enum { MM_ENEMY_WALKER, MM_ENEMY_KIND_COUNT };

/* Placeholder until 5.2 measures each original enemy: 0.5 px/tick, slow enough to read on screen. */
#define MM_ENEMY_WALK_SPEED 128

typedef struct {
    int x, y; /* Q24.8 world position of the hitbox origin (its left/top pixel). */
    int vx, vy;
    MmRect hitbox; /* Half-open local pixel bounds, relative to x and y. */
    int kind;
    int health, damage; /* damage is the contact damage applied in 5.3. */
    int grounded, facing_left;
    int age, frame; /* Ticks alive and the animation frame chosen by the game layer. */
    int active;
} MmEnemy;

typedef struct MmEnemyPool {
    MmEnemy enemies[MM_ENEMY_MAX];
    int count;
} MmEnemyPool;

typedef void (*MmEnemyBehavior)(const MmWorld *world, MmEnemy *enemy);

typedef struct {
    MmEnemyBehavior behavior;
    int health, damage;
    MmRect hitbox;
    int speed; /* Q24.8 px/tick along the facing direction. */
} MmEnemyDef;

extern const MmEnemyDef mm_enemy_defs[MM_ENEMY_KIND_COUNT];

void mm_enemy_pool_init(MmEnemyPool *pool);
/* Adds an enemy whose hitbox origin is at the room pixel (x, y). Returns its index, or -1 when
   the pool is full or the kind is unknown. */
int mm_enemy_spawn(MmEnemyPool *pool, int kind, int x, int y);
/* Runs one simulation tick for every active enemy. */
void mm_enemy_tick(const MmWorld *world, MmEnemyPool *pool);
/* World-space bounds of the enemy's hitbox, for contact and shot checks. */
MmRect mm_enemy_rect(const MmEnemy *enemy);
#endif
