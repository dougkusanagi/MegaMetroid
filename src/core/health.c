#include "core/health.h"
#include "core/hud.h"

void mm_health_init(MmHealth *h)
{
    h->energy = h->energy_max = MM_START_ENERGY;
    h->invulnerable = h->stun = h->dead = 0;
}

int mm_health_hit(MmHealth *h, MmBody *body, int damage, int from_left)
{
    if (h->dead || h->invulnerable > 0 || damage <= 0) return 0;
    h->energy -= damage;
    if (h->energy <= 0) {
        h->energy = 0;
        h->dead = 1;
        return 1;
    }
    h->invulnerable = MM_HIT_INVULNERABLE_TICKS;
    h->stun = MM_HIT_STUN_TICKS;
    body->vx = from_left ? MM_HIT_KNOCK_VX : -MM_HIT_KNOCK_VX;
    body->vy = MM_HIT_KNOCK_VY;
    body->grounded = 0;
    body->stun = MM_HIT_STUN_TICKS;
    body->speed = body->run_extra = body->turn = body->slide = body->walking = 0;
    return 1;
}

void mm_health_tick(MmHealth *h)
{
    if (h->invulnerable > 0) --h->invulnerable;
    if (h->stun > 0) --h->stun;
}

int mm_health_heal(MmHealth *h, int amount)
{
    int before = h->energy;
    if (h->dead || amount <= 0) return 0;
    h->energy += amount;
    if (h->energy > h->energy_max) h->energy = h->energy_max;
    return h->energy - before;
}

void mm_health_add_tank(MmHealth *h)
{
    h->energy_max += MM_ENERGY_PER_TANK;
    h->energy = h->energy_max;
}
