#include "core/weapon.h"

static int beam_blocked(const MmWorld *w, int x, int y)
{
    MmRect bounds = {x - 2, y - 2, x + 2, y + 2};
    return mm_rect_blocked(w, bounds);
}

static void advance(MmBeam *b, const MmWorld *w, MmCamera camera)
{
    int x = mm_pixel(b->x), y = mm_pixel(b->y);
    int nx = mm_pixel(b->x + b->vx), ny = mm_pixel(b->y + b->vy);
    /* Sweep every crossed pixel so the beam cannot skip a thin wall. */
    while (x != nx || y != ny) {
        if (x != nx) x += x < nx ? 1 : -1;
        if (y != ny) y += y < ny ? 1 : -1;
        if (beam_blocked(w, x, y)) { b->active = 0; return; }
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

void mm_weapon_tick(MmWeapon *w, const MmWorld *world,
                    const MmBody *player, MmInput input, MmCamera camera)
{
    int i;
    w->fired = 0;
    if (w->cooldown) --w->cooldown;
    if (!input.shoot_held || input.shoot_pressed) w->repeating = 0;
    for (i = 0; i < MM_BEAM_COUNT; ++i)
        if (w->beams[i].active) advance(&w->beams[i], world, camera);
    /* The morph ball has no beam. */
    if (!input.shoot_held || w->cooldown || player->ball) return;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        if (!w->beams[i].active) {
            MmBeam *b = &w->beams[i];
            int dx = player->facing_left ? -1 : 1;
            int dy = input.aim_y < 0 ? -1 : input.aim_y > 0 ? 1 : 0;
            int x = mm_pixel(player->x) + 20;
            int y = mm_pixel(player->y) + 22;
            int step, speed;
            if (dy > 0 && player->grounded) dy = 0;
            if (dy && !input.axis) dx = 0;
            speed = dx && dy ? MM_BEAM_DIAGONAL_SPEED : MM_BEAM_SPEED;
            b->active = 1;
            /* Trace from cannon to muzzle; never spawn beyond a wall. */
            for (step = 0; step < 14; ++step) {
                x += dx; y += dy;
                if (beam_blocked(world, x, y)) { b->active = 0; break; }
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
