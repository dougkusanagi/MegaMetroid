#include "core/weapon.h"

static int beam_blocked(const MmWorld *w, int x, int y)
{
    MmRect bounds = {x - 2, y - 2, x + 2, y + 2};
    return mm_rect_blocked(w, bounds);
}

static void impact(MmBeam *b, int x, int y)
{
    b->active = 0;
    b->impact_ticks = MM_IMPACT_TICKS;
    b->impact_x = x; b->impact_y = y;
}

static void advance(MmBeam *b, const MmWorld *w, MmCamera camera)
{
    int x = mm_pixel(b->x), y = mm_pixel(b->y);
    int nx = mm_pixel(b->x + b->vx), ny = mm_pixel(b->y + b->vy);
    /* Sweep every crossed pixel so the beam cannot skip a thin wall. */
    while (x != nx || y != ny) {
        if (x != nx) x += x < nx ? 1 : -1;
        if (y != ny) y += y < ny ? 1 : -1;
        if (beam_blocked(w, x, y)) { impact(b, x, y); return; }
    }
    b->x += b->vx; b->y += b->vy;
    if (--b->life <= 0 || x < camera.x - 8 || x >= camera.x + 264 ||
        y < camera.y - 8 || y >= camera.y + 232) b->active = 0;
}

void mm_weapon_init(MmWeapon *w)
{
    int i;
    w->cooldown = w->repeating = w->fired = 0;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        MmBeam empty = {0};
        w->beams[i] = empty;
    }
}

/* The body centre the cannon is traced from, in pixels from the body position. */
#define CANNON_BASE_X 20
#define CANNON_BASE_Y 24

void mm_weapon_tick(MmWeapon *w, const MmWorld *world, const MmBody *player,
                    MmInput input, MmCamera camera, int muzzle_x, int muzzle_y)
{
    int i;
    w->fired = 0;
    if (w->cooldown) --w->cooldown;
    if (!input.shoot_held || input.shoot_pressed) w->repeating = 0;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        if (w->beams[i].impact_ticks) --w->beams[i].impact_ticks;
        if (w->beams[i].active) advance(&w->beams[i], world, camera);
    }
    /* The morph ball has no beam. */
    if (!input.shoot_held || w->cooldown || player->ball) return;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        if (!w->beams[i].active && !w->beams[i].impact_ticks) {
            MmBeam *b = &w->beams[i];
            MmAim aim = player->aim;
            int dx = aim == MM_AIM_UP || aim == MM_AIM_DOWN ? 0 : player->facing_left ? -1 : 1;
            int dy = aim == MM_AIM_UP || aim == MM_AIM_DIAG_UP ? -1 :
                     aim == MM_AIM_DOWN || aim == MM_AIM_DIAG_DOWN ? 1 : 0;
            int px = mm_pixel(player->x), py = mm_pixel(player->y);
            int ex = muzzle_x - CANNON_BASE_X, ey = muzzle_y - CANNON_BASE_Y;
            int steps = ex < 0 ? -ex : ex, step, x = px + muzzle_x, y = py + muzzle_y, speed;
            if ((ey < 0 ? -ey : ey) > steps) steps = ey < 0 ? -ey : ey;
            speed = dx && dy ? MM_BEAM_DIAGONAL_SPEED : MM_BEAM_SPEED;
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
            b->life = 60; b->damage = MM_BEAM_DAMAGE;
            w->cooldown = w->repeating ? MM_BEAM_REPEAT : MM_BEAM_COOLDOWN;
            w->repeating = w->fired = 1;
            return;
        }
    }
}
