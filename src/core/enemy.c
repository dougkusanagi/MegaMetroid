#include "core/enemy.h"

/* A walker turns before the front foot leaves the floor. The probe is the one pixel just below
   and ahead of the hitbox: solid floor keeps it walking, empty air turns it around. */
static int ledge_ahead(const MmWorld *w, const MmEnemy *e, int dir)
{
    int x = mm_pixel(e->x), y = mm_pixel(e->y);
    int px = dir > 0 ? x + e->hitbox.right : x + e->hitbox.left - 1;
    MmRect probe;
    probe.left = px;
    probe.right = px + 1;
    probe.top = y + e->hitbox.bottom;
    probe.bottom = probe.top + 1;
    return !mm_rect_blocked(w, probe);
}

/* Placeholder ground enemy for 5.1: gravity, walls, slopes and ledges through mm_move. Its
   speed and hitbox are generic; 5.2 replaces it with the measured Crateria behaviors. */
static void walker(const MmWorld *w, const MmBody *player, MmEnemy *e)
{
    (void)player;
    MmBody body = {0};
    int dir = e->facing_left ? -1 : 1;
    if (e->grounded && ledge_ahead(w, e, dir)) {
        e->facing_left = !e->facing_left;
        dir = -dir;
    }
    if (!e->grounded) {
        e->vy += MM_GRAVITY;
        if (e->vy > MM_MAX_FALL) e->vy = MM_MAX_FALL;
    }
    body.x = e->x;
    body.y = e->y;
    body.vx = dir * mm_enemy_defs[e->kind].speed;
    body.vy = e->vy;
    body.hitbox = e->hitbox;
    body.grounded = e->grounded;
    mm_move(w, &body);
    e->x = body.x;
    e->y = body.y;
    e->vx = body.vx;
    e->vy = body.vy;
    e->grounded = body.grounded;
    if (!body.vx) e->facing_left = !e->facing_left;
}

/* Skree hangs still until Samus is under it, prepares, then dives diagonally toward her
   current position. It burrows into the floor and disappears. */
static void skree(const MmWorld *w, const MmBody *player, MmEnemy *e)
{
    int cx = mm_pixel(e->x) + (e->hitbox.left + e->hitbox.right) / 2;
    int px = mm_pixel(player->x) + (player->hitbox.left + player->hitbox.right) / 2;
    switch (e->state) {
    case MM_ENEMY_STATE_IDLE: {
        int py = mm_pixel(player->y);
        int below = py - (mm_pixel(e->y) + e->hitbox.bottom);
        if (below >= 0 && below <= MM_ENEMY_SKREE_SIGHT_Y &&
            px >= cx - MM_ENEMY_SKREE_SIGHT_X && px <= cx + MM_ENEMY_SKREE_SIGHT_X) {
            e->state = MM_ENEMY_STATE_PREPARE;
            e->timer = MM_ENEMY_SKREE_PREPARE_TICKS;
        }
        break;
    }
    case MM_ENEMY_STATE_PREPARE:
        if (--e->timer <= 0) {
            e->state = MM_ENEMY_STATE_ATTACK;
            e->facing_left = px < cx;
            e->vx = px < cx ? -MM_ENEMY_SKREE_DIVE_X :
                    px > cx ? MM_ENEMY_SKREE_DIVE_X : 0;
            e->vy = MM_ENEMY_SKREE_DIVE_Y;
        }
        break;
    case MM_ENEMY_STATE_ATTACK: {
        MmRect probe;
        int nx = e->x + e->vx;
        probe.left = mm_pixel(nx) + e->hitbox.left;
        probe.right = mm_pixel(nx) + e->hitbox.right;
        probe.top = mm_pixel(e->y) + e->hitbox.top;
        probe.bottom = mm_pixel(e->y) + e->hitbox.bottom;
        if (mm_rect_blocked(w, probe)) e->vx = 0;
        else e->x = nx;
        e->y += e->vy;
        probe.left = mm_pixel(e->x) + e->hitbox.left;
        probe.right = mm_pixel(e->x) + e->hitbox.right;
        probe.top = mm_pixel(e->y) + e->hitbox.bottom;
        probe.bottom = probe.top + 1;
        if (mm_rect_blocked(w, probe)) {
            e->state = MM_ENEMY_STATE_BURROW;
            e->timer = MM_ENEMY_SKREE_BURROW_TICKS;
            e->vx = e->vy = 0;
        }
        break;
    }
    case MM_ENEMY_STATE_BURROW:
        e->y += MM_UNIT;
        if (--e->timer <= 0) e->active = 0;
        break;
    }
}

/* Ripper flies horizontally at a fixed height and reverses when its body would enter a wall. */
static void ripper(const MmWorld *w, const MmBody *player, MmEnemy *e)
{
    MmRect probe;
    int speed = mm_enemy_defs[e->kind].speed;
    int nx = e->x + (e->facing_left ? -speed : speed);
    (void)player;
    probe.left = mm_pixel(nx) + e->hitbox.left;
    probe.right = mm_pixel(nx) + e->hitbox.right;
    probe.top = mm_pixel(e->y) + e->hitbox.top;
    probe.bottom = mm_pixel(e->y) + e->hitbox.bottom;
    if (mm_rect_blocked(w, probe)) e->facing_left = !e->facing_left;
    else e->x = nx;
}

const MmEnemyDef mm_enemy_defs[MM_ENEMY_KIND_COUNT] = {
    {walker, 10, 10, {0, 0, 16, 16}, MM_ENEMY_WALK_SPEED, 0},
    /* Zoomer keeps the shared ground behavior; 5.2 adds its sprite and animation. */
    {walker, 15, 5, {0, 0, 16, 16}, MM_ENEMY_ZOOMER_SPEED, 0},
    /* Skree does not walk; the speed field belongs to the ground behaviors. */
    {skree, 15, 10, {0, 0, 8, 12}, 0, 0},
    /* Ripper starts left, as the original Crateria mainstreet placement does. */
    {ripper, 200, 5, {0, 0, 8, 4}, MM_ENEMY_RIPPER_SPEED, 1}
};

void mm_enemy_pool_init(MmEnemyPool *pool)
{
    pool->count = 0;
}

int mm_enemy_spawn(MmEnemyPool *pool, int kind, int x, int y)
{
    MmEnemy *e;
    const MmEnemyDef *def;
    if (pool->count >= MM_ENEMY_MAX || kind < 0 || kind >= MM_ENEMY_KIND_COUNT) return -1;
    def = &mm_enemy_defs[kind];
    e = &pool->enemies[pool->count];
    e->x = x * MM_UNIT;
    e->y = y * MM_UNIT;
    e->vx = e->vy = 0;
    e->hitbox = def->hitbox;
    e->kind = kind;
    e->health = def->health;
    e->damage = def->damage;
    e->grounded = 0;
    e->facing_left = def->facing_left;
    e->state = e->timer = 0;
    e->age = e->frame = 0;
    e->active = 1;
    return pool->count++;
}

void mm_enemy_tick(const MmWorld *w, const MmBody *player, MmEnemyPool *pool)
{
    int i;
    for (i = 0; i < pool->count; ++i) {
        MmEnemy *e = &pool->enemies[i];
        if (!e->active) continue;
        mm_enemy_defs[e->kind].behavior(w, player, e);
        ++e->age;
    }
}

MmRect mm_enemy_rect(const MmEnemy *e)
{
    MmRect r;
    r.left = mm_pixel(e->x) + e->hitbox.left;
    r.top = mm_pixel(e->y) + e->hitbox.top;
    r.right = mm_pixel(e->x) + e->hitbox.right;
    r.bottom = mm_pixel(e->y) + e->hitbox.bottom;
    return r;
}
