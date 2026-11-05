#ifndef MM_DROP_H
#define MM_DROP_H
/* Portable C: small energy and missile drops left by dead enemies (5.4). */
#include "world.h"
#include "health.h"
#include "weapon.h"

#define MM_DROP_MAX 8
/* Provisional until the original drop tables are measured: values and lifetime are not from the game. */
#define MM_DROP_ENERGY_AMOUNT 5
#define MM_DROP_MISSILE_AMOUNT 2
#define MM_DROP_LIFE_TICKS 600
#define MM_DROP_BOX 8 /* Side of the square pickup box, px. */

enum { MM_DROP_ENERGY, MM_DROP_MISSILE };

typedef struct { int active, kind, x, y, life; } MmDrop; /* x, y: top-left pixel of the box. */
typedef struct { MmDrop drops[MM_DROP_MAX]; int kills; } MmDropPool;

void mm_drop_init(MmDropPool *pool);
/* Leaves a drop centered on the box of a dead enemy. Kills alternate energy and missile; a missile
   drop becomes energy while the missile bin has no capacity. Returns the slot, or -1 when full. */
int mm_drop_spawn(MmDropPool *pool, const MmWeapon *weapon, MmRect enemy_box);
/* Ages the drops by one tick and removes the expired ones. */
void mm_drop_tick(MmDropPool *pool);
MmRect mm_drop_rect(const MmDrop *drop);
/* Collects every drop that overlaps body. Returns how many were taken. */
int mm_drop_collect(MmDropPool *pool, MmRect body, MmHealth *health, MmWeapon *weapon);
#endif
