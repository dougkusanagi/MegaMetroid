#ifndef MM_WEAPON_H
#define MM_WEAPON_H
#include "world.h"

#define MM_BEAM_COUNT 3
#define MM_BEAM_SPEED (4 * MM_UNIT)
/* Original Power Beam tables: $0400 cardinal, $02AB per diagonal axis (Q8.8). */
#define MM_BEAM_DIAGONAL_SPEED 683
#define MM_BEAM_ACCEL 16
#define MM_BEAM_DAMAGE 20
#define MM_BEAM_COOLDOWN 15
#define MM_BEAM_REPEAT 25
#define MM_IMPACT_TICKS 10
/* Half-size of the box a beam checks against the world, and of the one it leaves on a cap. */
#define MM_BEAM_RADIUS 2

typedef struct {
    int active, x, y, vx, vy, life, damage;
    int impact_ticks, impact_x, impact_y;
} MmBeam;
typedef struct {
    MmBeam beams[MM_BEAM_COUNT];
    int cooldown, repeating, fired;
} MmWeapon;

void mm_weapon_init(MmWeapon *weapon);
/* Fires along player->aim from the cannon tip, given in pixels from the body position
   (player_muzzle[] in player_anim.h for the pose on screen). */
void mm_weapon_tick(MmWeapon *weapon, const MmWorld *world, const MmBody *player,
                    MmInput input, MmCamera camera, int muzzle_x, int muzzle_y);
#endif
