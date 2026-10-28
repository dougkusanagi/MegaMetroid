#include "core/weapon.h"

static int beam_blocked(const MmWorld *w, int x, int y)
{
    MmRect bounds = {x - MM_BEAM_RADIUS, y - MM_BEAM_RADIUS,
                     x + MM_BEAM_RADIUS, y + MM_BEAM_RADIUS};
    return mm_rect_blocked(w, bounds);
}

static void impact(MmBeam *b, int x, int y)
{
    b->active = 0;
    b->impact_ticks = MM_IMPACT_TICKS;
    b->impact_x = x; b->impact_y = y;
}

/* Original projectile acceleration happens before the movement of the same tick. Missiles
   add the generic $0010 to their table ($0040 cardinal, $0036 diagonal); supers use their
   own $0100/$00B6 table. Beams keep $0010. */
static void accelerate(MmBeam *b)
{
    int diagonal = b->vx && b->vy, accel;
    switch (b->kind) {
    case MM_SHOT_MISSILE: accel = diagonal ? MM_MISSILE_DIAGONAL_ACCEL : MM_MISSILE_ACCEL; break;
    case MM_SHOT_SUPER: accel = diagonal ? MM_SUPER_DIAGONAL_ACCEL : MM_SUPER_ACCEL; break;
    default: accel = MM_BEAM_ACCEL; break;
    }
    if (b->vx) b->vx += b->vx > 0 ? accel : -accel;
    if (b->vy) b->vy += b->vy > 0 ? accel : -accel;
}

static void advance(MmBeam *b, const MmWorld *w, MmCamera camera)
{
    int x = mm_pixel(b->x), y = mm_pixel(b->y);
    int nx, ny;
    /* The original accelerates before moving, including the spawn tick. */
    accelerate(b);
    nx = mm_pixel(b->x + b->vx); ny = mm_pixel(b->y + b->vy);
    /* Sweep every crossed pixel so the beam cannot skip a thin wall. */
    while (x != nx || y != ny) {
        if (x != nx) x += x < nx ? 1 : -1;
        if (y != ny) y += y < ny ? 1 : -1;
        if (beam_blocked(w, x, y)) { impact(b, x, y); return; }
    }
    b->x += b->vx; b->y += b->vy;
    /* Missiles have no timer: like the original, they go until a wall or off screen. */
    if (b->life && --b->life <= 0) { b->active = 0; return; }
    if (x < camera.x - 8 || x >= camera.x + 264 ||
        y < camera.y - 8 || y >= camera.y + 232) b->active = 0;
}

static MmBeam *free_beam(MmWeapon *w)
{
    int i;
    for (i = 0; i < MM_BEAM_COUNT; ++i)
        if (!w->beams[i].active && !w->beams[i].impact_ticks) return &w->beams[i];
    return 0;
}

static int shot_on_screen(const MmWeapon *w)
{
    int i;
    for (i = 0; i < MM_BEAM_COUNT; ++i)
        if (w->beams[i].active || w->beams[i].impact_ticks) return 1;
    return 0;
}

static int ammo_for(const MmWeapon *w, int weapon)
{
    return weapon == MM_WEAPON_MISSILE ? w->missiles :
           weapon == MM_WEAPON_SUPER ? w->supers : 1;
}

/* The palette changes with the shot on screen, so a change waits until it is gone. */
static void apply_cycle(MmWeapon *w)
{
    int next, i;
    if (!w->cycle_pending || shot_on_screen(w)) return;
    w->cycle_pending = 0;
    next = w->selected;
    for (i = 0; i < MM_WEAPON_COUNT; ++i) {
        next = (next + 1) % MM_WEAPON_COUNT;
        if (ammo_for(w, next)) break;
    }
    w->selected = next;
}

void mm_weapon_init(MmWeapon *w)
{
    w->selected = MM_WEAPON_BEAM;
    w->missiles = MM_START_MISSILES;
    w->supers = MM_START_SUPERS;
    w->missiles_max = MM_START_MISSILES;
    w->supers_max = MM_START_SUPERS;
    mm_weapon_clear_shots(w);
}

void mm_weapon_clear_shots(MmWeapon *w)
{
    int i;
    w->cooldown = w->repeating = w->fired = w->cycle_pending = 0;
    w->fired_kind = -1;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        MmBeam empty = {0};
        w->beams[i] = empty;
    }
}

void mm_weapon_cycle(MmWeapon *w)
{
    w->cycle_pending = 1;
}

/* The body centre the cannon is traced from, in pixels from the body position. */
#define CANNON_BASE_X 20
#define CANNON_BASE_Y 24

void mm_weapon_tick(MmWeapon *w, const MmWorld *world, const MmBody *player,
                    MmInput input, MmCamera camera, int muzzle_x, int muzzle_y)
{
    int i, kind;
    MmBeam *b;
    w->fired = 0;
    if (w->cooldown) --w->cooldown;
    if (!input.shoot_held || input.shoot_pressed) w->repeating = 0;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        if (w->beams[i].impact_ticks) --w->beams[i].impact_ticks;
        if (w->beams[i].active) advance(&w->beams[i], world, camera);
    }
    apply_cycle(w);
    kind = w->selected;
    /* The morph ball has no shot. Missiles fire on a new press only, as in the original. */
    if (player->ball) return;
    if (kind == MM_WEAPON_BEAM ? (!input.shoot_held || w->cooldown) :
                                 (!input.shoot_pressed || w->cooldown)) return;
    if (!ammo_for(w, kind)) { w->selected = MM_WEAPON_BEAM; return; }
    b = free_beam(w);
    if (!b) return;
    {
        MmAim aim = player->aim;
        int dx = aim == MM_AIM_UP || aim == MM_AIM_DOWN ? 0 : player->facing_left ? -1 : 1;
        int dy = aim == MM_AIM_UP || aim == MM_AIM_DIAG_UP ? -1 :
                 aim == MM_AIM_DOWN || aim == MM_AIM_DIAG_DOWN ? 1 : 0;
        int px = mm_pixel(player->x), py = mm_pixel(player->y);
        int ex = muzzle_x - CANNON_BASE_X, ey = muzzle_y - CANNON_BASE_Y;
        int steps = ex < 0 ? -ex : ex, step, x = px + muzzle_x, y = py + muzzle_y, speed;
        if ((ey < 0 ? -ey : ey) > steps) steps = ey < 0 ? -ey : ey;
        speed = kind != MM_WEAPON_BEAM ? MM_SHOT_START_SPEED :
                dx && dy ? MM_BEAM_DIAGONAL_SPEED : MM_BEAM_SPEED;
        b->active = 1;
        /* Trace from the body centre to the muzzle; never spawn beyond a wall. */
        for (step = 1; step <= steps; ++step) {
            if (beam_blocked(world, px + CANNON_BASE_X + ex * step / steps,
                             py + CANNON_BASE_Y + ey * step / steps)) {
                impact(b, px + CANNON_BASE_X + ex * step / steps,
                       py + CANNON_BASE_Y + ey * step / steps);
                break;
            }
        }
        b->x = x * MM_UNIT; b->y = y * MM_UNIT;
        b->vx = dx * speed; b->vy = dy * speed;
        b->kind = kind;
        b->life = kind == MM_WEAPON_BEAM ? 60 : 0;
        b->damage = kind == MM_WEAPON_MISSILE ? MM_MISSILE_DAMAGE :
                    kind == MM_WEAPON_SUPER ? MM_SUPER_DAMAGE : MM_BEAM_DAMAGE;
        /* Measured in the original: a beam moves on its spawn tick, a missile does not; the
           sampled missile velocity is still $0100 on the frame it appears. */
        if (b->active && kind == MM_WEAPON_BEAM) advance(b, world, camera);
    }
    w->cooldown = kind == MM_WEAPON_MISSILE ? MM_MISSILE_COOLDOWN :
                  kind == MM_WEAPON_SUPER ? MM_SUPER_COOLDOWN :
                  w->repeating ? MM_BEAM_REPEAT : MM_BEAM_COOLDOWN;
    w->repeating = kind == MM_WEAPON_BEAM;
    if (kind == MM_WEAPON_MISSILE) { if (!--w->missiles) w->selected = MM_WEAPON_BEAM; }
    else if (kind == MM_WEAPON_SUPER) { if (!--w->supers) w->selected = MM_WEAPON_BEAM; }
    w->fired = 1;
    w->fired_kind = kind;
}
