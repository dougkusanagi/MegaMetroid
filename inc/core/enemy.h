#ifndef MM_ENEMY_H
#define MM_ENEMY_H
/* Portable C: enemy instances, their behavior table and the shared physics. No SGDK here; the
   game layer owns sprites, palettes and animation (5.2). */
#include "world.h"

/* Pool size for one room; enter_room spawns up to this many and ignores extra room spawns. */
#define MM_ENEMY_MAX 16

/* Kinds index mm_enemy_defs. Geemer is the purple crawler that shares the Zoomer AI. */
enum { MM_ENEMY_WALKER, MM_ENEMY_ZOOMER, MM_ENEMY_SKREE, MM_ENEMY_RIPPER, MM_ENEMY_GEEMER, MM_ENEMY_KIND_COUNT };

/* Behavior state shared by the enemies that need more than walking. */
enum {
    MM_ENEMY_STATE_IDLE, MM_ENEMY_STATE_PREPARE, MM_ENEMY_STATE_ATTACK, MM_ENEMY_STATE_BURROW
};

/* Placeholder until 5.2 measures each original enemy: 0.5 px/tick, slow enough to read on screen. */
#define MM_ENEMY_WALK_SPEED 128
/* Zoomer crawls at the same provisional 0.5 px/tick; its 16x16 box is the body inside the
   22x17 visible pixels of the original sprite. */
#define MM_ENEMY_ZOOMER_SPEED 128
/* Skree waits for Samus under it, then dives diagonally and burrows into the floor. The
   8x12 body, 15 HP and 10 damage are the original enemy header values. */
#define MM_ENEMY_SKREE_SIGHT_X 64 /* Half-width of the trigger area, px. */
#define MM_ENEMY_SKREE_SIGHT_Y 256 /* How far below the hanging body Samus can trigger it, px. */
#define MM_ENEMY_SKREE_PREPARE_TICKS 16
#define MM_ENEMY_SKREE_DIVE_X 128 /* 0.5 px/tick, enough to reach Samus across the shaft. */
#define MM_ENEMY_SKREE_DIVE_Y 512 /* 2 px/tick downward. */
#define MM_ENEMY_SKREE_BURROW_TICKS 16
/* Ripper flies horizontally and turns at walls. The original linear speed table entry is
   1.0 px/tick; its 8x4 body, 200 HP and 5 damage are the original header values. */
#define MM_ENEMY_RIPPER_SPEED 256

typedef struct {
    int x, y; /* Q24.8 world position of the hitbox origin (its left/top pixel). */
    int vx, vy;
    MmRect hitbox; /* Half-open local pixel bounds, relative to x and y. */
    int kind;
    int health, damage; /* damage is the contact damage applied in 5.3. */
    int grounded, facing_left;
    int state, timer; /* Behavior state and its countdown, set by the enemy behavior. */
    int age, frame; /* Ticks alive and the animation frame chosen by the game layer. */
    int active;
} MmEnemy;

typedef struct MmEnemyPool {
    MmEnemy enemies[MM_ENEMY_MAX];
    int count;
} MmEnemyPool;

typedef void (*MmEnemyBehavior)(const MmWorld *world, const MmBody *player, MmEnemy *enemy);

typedef struct {
    MmEnemyBehavior behavior;
    int health, damage;
    MmRect hitbox;
    int speed; /* Q24.8 px/tick along the facing direction. */
    int facing_left; /* Initial direction; ground enemies start facing right. */
} MmEnemyDef;

extern const MmEnemyDef mm_enemy_defs[MM_ENEMY_KIND_COUNT];

void mm_enemy_pool_init(MmEnemyPool *pool);
/* Adds an enemy whose hitbox origin is at the room pixel (x, y). Returns its index, or -1 when
   the pool is full or the kind is unknown. */
int mm_enemy_spawn(MmEnemyPool *pool, int kind, int x, int y);
/* Runs one simulation tick for every active enemy. */
void mm_enemy_tick(const MmWorld *world, const MmBody *player, MmEnemyPool *pool);
/* World-space bounds of the enemy's hitbox, for contact and shot checks. */
MmRect mm_enemy_rect(const MmEnemy *enemy);
#endif
