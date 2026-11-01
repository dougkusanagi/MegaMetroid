#ifndef MM_HEALTH_H
#define MM_HEALTH_H
/* Portable C: Samus's energy, energy tanks, damage and the invulnerability after a hit. */
#include "world.h"

/* Super Metroid gives 96 ticks (1.6 s) of invulnerability after a hit. */
#define MM_HIT_INVULNERABLE_TICKS 96
/* Knockback: horizontal push away from the source, and the upward kick, in Q24.8 px/tick. */
#define MM_HIT_KNOCK_VX 512
#define MM_HIT_KNOCK_VY (-768)
/* Ticks the body is thrown before Samus has control again. */
#define MM_HIT_STUN_TICKS 8

typedef struct {
    int energy, energy_max; /* energy_max = 99 + 100 per tank. */
    int invulnerable;       /* Ticks left in which hits are ignored. */
    int stun;               /* Ticks left of knockback; the game ignores the d-pad meanwhile. */
    int dead;               /* Energy reached 0; the game restarts the room. */
} MmHealth;

void mm_health_init(MmHealth *health);
/* Applies damage unless invulnerable or dead. from_left is 1 when the source is on Samus's left,
   so she is thrown to the right. Returns 1 if the hit landed. A hit starts the invulnerability
   and the stun; a hit that empties the energy sets dead and no invulnerability. */
int mm_health_hit(MmHealth *health, MmBody *body, int damage, int from_left);
/* Counts the invulnerability and the stun down by one tick. */
void mm_health_tick(MmHealth *health);
/* Refills energy, capped at energy_max (drops, 5.4). Returns the amount added. */
int mm_health_heal(MmHealth *health, int amount);
/* Adds a tank: +100 maximum and a full refill (7.2). */
void mm_health_add_tank(MmHealth *health);
#endif
