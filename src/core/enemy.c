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
static void walker(const MmWorld *w, MmEnemy *e)
{
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

const MmEnemyDef mm_enemy_defs[MM_ENEMY_KIND_COUNT] = {
    {walker, 10, 10, {0, 0, 16, 16}, MM_ENEMY_WALK_SPEED},
    /* Zoomer keeps the shared ground behavior; 5.2 adds its sprite and animation. */
    {walker, 15, 5, {0, 0, 16, 16}, MM_ENEMY_ZOOMER_SPEED}
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
    e->grounded = e->facing_left = 0;
    e->age = e->frame = 0;
    e->active = 1;
    return pool->count++;
}

void mm_enemy_tick(const MmWorld *w, MmEnemyPool *pool)
{
    int i;
    for (i = 0; i < pool->count; ++i) {
        MmEnemy *e = &pool->enemies[i];
        if (!e->active) continue;
        mm_enemy_defs[e->kind].behavior(w, e);
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
