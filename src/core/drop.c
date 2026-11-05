#include "core/drop.h"

void mm_drop_init(MmDropPool *pool)
{
    int i;
    for (i = 0; i < MM_DROP_MAX; ++i) pool->drops[i].active = 0;
    pool->kills = 0;
}

int mm_drop_spawn(MmDropPool *pool, const MmWeapon *weapon, MmRect box)
{
    int i, kind = (pool->kills++ & 1) ? MM_DROP_MISSILE : MM_DROP_ENERGY;
    if (kind == MM_DROP_MISSILE && weapon->missiles_max <= 0) kind = MM_DROP_ENERGY;
    for (i = 0; i < MM_DROP_MAX; ++i) {
        MmDrop *d = &pool->drops[i];
        if (d->active) continue;
        d->active = 1; d->kind = kind; d->life = MM_DROP_LIFE_TICKS;
        d->x = (box.left + box.right - MM_DROP_BOX) / 2;
        d->y = (box.top + box.bottom - MM_DROP_BOX) / 2;
        return i;
    }
    return -1;
}

void mm_drop_tick(MmDropPool *pool)
{
    int i;
    for (i = 0; i < MM_DROP_MAX; ++i)
        if (pool->drops[i].active && --pool->drops[i].life <= 0) pool->drops[i].active = 0;
}

MmRect mm_drop_rect(const MmDrop *d)
{
    MmRect r = {d->x, d->y, d->x + MM_DROP_BOX, d->y + MM_DROP_BOX};
    return r;
}

int mm_drop_collect(MmDropPool *pool, MmRect b, MmHealth *health, MmWeapon *weapon)
{
    int i, taken = 0;
    for (i = 0; i < MM_DROP_MAX; ++i) {
        MmDrop *d = &pool->drops[i];
        MmRect r;
        if (!d->active) continue;
        r = mm_drop_rect(d);
        if (r.left >= b.right || r.right <= b.left || r.top >= b.bottom || r.bottom <= b.top) continue;
        if (d->kind == MM_DROP_ENERGY) {
            mm_health_heal(health, MM_DROP_ENERGY_AMOUNT);
        } else {
            weapon->missiles += MM_DROP_MISSILE_AMOUNT;
            if (weapon->missiles > weapon->missiles_max) weapon->missiles = weapon->missiles_max;
        }
        d->active = 0;
        ++taken;
    }
    return taken;
}
