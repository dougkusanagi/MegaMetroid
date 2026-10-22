#ifndef MM_WEAPON_H
#define MM_WEAPON_H
#include "world.h"
#include "projectile.h"

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

/* Selected shot. The original cycles Beam -> Missile -> Super with SELECT. */
enum { MM_WEAPON_BEAM, MM_WEAPON_MISSILE, MM_WEAPON_SUPER, MM_WEAPON_COUNT };
/* Every shot leaves along the aim at 1 px/tick per moving axis (original $0100). */
#define MM_SHOT_START_SPEED 256
/* Measured in the original: missile $0040 (cardinal) / $0036 (diagonal) plus the
   generic $0010 projectile acceleration, so 80 and 70 Q8.8. Supers use
   SuperMissileAccelerations alone: $0100 cardinal, $00B6 diagonal. */
#define MM_MISSILE_ACCEL 80
#define MM_MISSILE_DIAGONAL_ACCEL 70
#define MM_SUPER_ACCEL 256
#define MM_SUPER_DIAGONAL_ACCEL 182
#define MM_MISSILE_DAMAGE 100
#define MM_SUPER_DAMAGE 300
#define MM_MISSILE_COOLDOWN 10
#define MM_SUPER_COOLDOWN 20
/* Provisional loadout until item pickups (7.2); supers are 0 until then. */
#define MM_START_MISSILES 5
#define MM_START_SUPERS 0

typedef struct {
    int active, x, y, vx, vy, life, damage, kind; /* kind: MM_SHOT_*; life 0 = no timer. */
    int impact_ticks, impact_x, impact_y;
} MmBeam;
typedef struct {
    MmBeam beams[MM_BEAM_COUNT];
    int selected; /* MM_WEAPON_* */
    int missiles, supers;
    int cooldown, repeating, fired, fired_kind;
    int cycle_pending; /* A weapon change waits until no shot is on screen. */
} MmWeapon;

void mm_weapon_init(MmWeapon *weapon);
/* Requests the next usable weapon; an empty missile/super bin is skipped. */
void mm_weapon_cycle(MmWeapon *weapon);
/* Drops the shots in flight when a room loads; ammo and selection are kept. */
void mm_weapon_clear_shots(MmWeapon *weapon);
/* Fires along player->aim from the cannon tip, given in pixels from the body position
   (player_muzzle[] in player_anim.h for the pose on screen). */
void mm_weapon_tick(MmWeapon *weapon, const MmWorld *world, const MmBody *player,
                    MmInput input, MmCamera camera, int muzzle_x, int muzzle_y);
#endif
