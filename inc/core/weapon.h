#ifndef MM_WEAPON_H
#define MM_WEAPON_H
#include "world.h"

#define MM_BEAM_COUNT 3
#define MM_BEAM_SPEED (12 * MM_UNIT)
#define MM_BEAM_DIAGONAL_SPEED 2172
#define MM_BEAM_DAMAGE 20
#define MM_BEAM_COOLDOWN 15
#define MM_BEAM_REPEAT 25

typedef struct {
    int active, x, y, vx, vy, life, damage;
} MmBeam;
typedef struct {
    MmBeam beams[MM_BEAM_COUNT];
    int cooldown, repeating, fired;
} MmWeapon;

void mm_weapon_init(MmWeapon *weapon);
void mm_weapon_tick(MmWeapon *weapon, const MmWorld *world,
                    const MmBody *player, MmInput input, MmCamera camera);
#endif
