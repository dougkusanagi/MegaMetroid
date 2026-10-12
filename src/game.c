#include "game.h"
#include "resources.h"
#include "beam_sprite.h"

#define VIEW_WIDTH 256
#define VIEW_HEIGHT 224

/* Build with -DMM_DEBUG_OVERLAY=1 to draw position, velocity, floor and foot tiles. */
#ifndef MM_DEBUG_OVERLAY
#define MM_DEBUG_OVERLAY 0
#endif
#define OVERLAY_ROWS 4

enum { ANIM_STAND, ANIM_WALK, ANIM_JUMP };

#if MM_DEBUG_OVERLAY
/* Q24.8 as fixed-width signed text with two decimals, e.g. "+ 2.75". */
static void fix_text(char *out, int value)
{
    int a = value < 0 ? -value : value;
    sprintf(out, "%c%4d.%02d", value < 0 ? '-' : '+', a / MM_UNIT, a % MM_UNIT * 100 / MM_UNIT);
}

/* Foot tiles are the tiles just below the hitbox corners: 0 solid, 1 air, 2/3 slopes. */
static void draw_debug_overlay(const Game *g)
{
    char x[12], y[12], vx[12], vy[12], line[40];
    int left = mm_pixel(g->player.x) + g->player.hitbox.left;
    int right = mm_pixel(g->player.x) + g->player.hitbox.right - 1;
    int foot_row = (mm_pixel(g->player.y) + g->player.hitbox.bottom) / MM_TILE;
    fix_text(x, g->player.x); fix_text(y, g->player.y);
    fix_text(vx, g->player.vx); fix_text(vy, g->player.vy);
    sprintf(line, "X %s  Y %s", x, y);
    VDP_drawText(line, 0, 0);
    sprintf(line, "VX %s  VY %s", vx, vy);
    VDP_drawText(line, 0, 1);
    sprintf(line, "%-6s %s", mm_state_name(g->player.state), g->player.grounded ? "GROUND" : "AIR   ");
    VDP_drawText(line, 0, 2);
    sprintf(line, "FOOT L %d R %d", mm_tile(&g->world, left / MM_TILE, foot_row),
            mm_tile(&g->world, right / MM_TILE, foot_row));
    VDP_drawText(line, 0, 3);
}
#endif

static void render(Game *g)
{
    int i;
    int x = mm_pixel(g->player.x), y = mm_pixel(g->player.y);
    MmRect world_bounds = {0, 0, g->level->map_width, g->level->map_height};
    MmRect bounds = mm_camera_bounds(x + 20, y + 28, world_bounds,
        g->level->camera_zones, g->level->camera_zone_count);
    bool shooting_pose = g->player.shoot_pose_timer != 0;
    bool spin = !g->player.grounded && g->player.spin_jumping && !shooting_pose;
    u16 animation = spin ? ANIM_JUMP :
                    (!g->player.grounded || shooting_pose) ? ANIM_STAND :
                    g->player.vx ? ANIM_WALK : ANIM_STAND;
    g->camera = mm_camera(x + 20, y + 28, VIEW_WIDTH, VIEW_HEIGHT, bounds);
    MAP_scrollTo(g->foreground, g->camera.x, g->camera.y);
    MAP_scrollTo(g->background, 0, 0);
    SPR_setPosition(g->sprite, x - g->camera.x, y - g->camera.y);
    SPR_setHFlip(g->sprite, g->player.facing_left);
    if (animation != g->animation) {
        SPR_setAnim(g->sprite, animation);
        g->animation = animation;
    }
    if (shooting_pose) {
        SPR_setAutoAnimation(g->sprite, FALSE);
        SPR_setFrame(g->sprite, g->player.shoot_frame);
    } else if (!g->player.grounded && !spin) {
        /* A neutral jump keeps the standing pose even if direction is
           pressed after takeoff. Only jumping while already moving spins. */
        SPR_setAutoAnimation(g->sprite, FALSE);
        SPR_setFrame(g->sprite, 0);
    } else {
        SPR_setAutoAnimation(g->sprite, TRUE);
    }
    /* Shorten the walk cycle as dash momentum builds. */
    if (animation == ANIM_WALK && g->player.run_extra && g->sprite->timer > 3)
        g->sprite->timer = 3;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        const MmBeam *beam = &g->weapon.beams[i];
        SPR_setVisibility(g->beam_sprites[i], beam->active ? AUTO_FAST : HIDDEN);
        if (beam->active)
            SPR_setPosition(g->beam_sprites[i], mm_pixel(beam->x) - g->camera.x - 4,
                            mm_pixel(beam->y) - g->camera.y - 4);
    }
    SPR_update();
#if MM_DEBUG_OVERLAY
    draw_debug_overlay(g);
#endif
}

bool game_init(Game *g, u16 level_index)
{
    int i;
    u16 tile = TILE_USER_INDEX;
    MmRect hitbox = {8, 8, 32, 48};
    g->level = level_get(level_index);
    if (!g->level) return FALSE;
    g->input.previous = 0; g->pal_phase = 0; g->frame = 0;
    g->beam_sound_timer = 0;
    mm_weapon_init(&g->weapon);
    g->animation = 0xFFFF;
    g->world.tiles = g->level->map_collision;
    g->world.columns = g->level->map_width / MM_TILE;
    g->world.rows = g->level->map_height / MM_TILE;
    mm_body_init(&g->player, g->level->player_initial_pos.x,
                 g->level->player_initial_pos.y, hitbox);
    SYS_disableInts();
    VDP_setScreenWidth256();
    VDP_setScreenHeight224();
    JOY_init();
    SPR_init();
    /* CPU transfers during setup avoid queuing data across buffer resizing. */
    PAL_setPalette(PAL0, g->level->palette_fg->data, CPU);
    VDP_loadTileSet(g->level->tileset_fg, tile, CPU);
    g->foreground = MAP_create(g->level->map_fg, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, tile));
    tile += g->level->tileset_fg->numTile;
    PAL_setPalette(PAL1, g->level->palette_bg->data, CPU);
    VDP_loadTileSet(g->level->tileset_bg, tile, CPU);
    g->background = MAP_create(g->level->map_bg, BG_B, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, tile));
    PAL_setPalette(PAL2, player_sprite.palette->data, CPU);
    g->sprite = SPR_addSprite(&player_sprite, 0, 0, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    if (!g->foreground || !g->background || !g->sprite) {
        if (g->foreground) MEM_free(g->foreground);
        if (g->background) MEM_free(g->background);
        if (g->sprite) SPR_releaseSprite(g->sprite);
        SYS_enableInts();
        return FALSE;
    }
    PAL_setPalette(PAL3, beam_palette, CPU);
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        g->beam_sprites[i] = SPR_addSprite(&beam_sprite, 0, 0, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
        if (!g->beam_sprites[i]) {
            while (i > 0) SPR_releaseSprite(g->beam_sprites[--i]);
            SPR_releaseSprite(g->sprite);
            MEM_free(g->foreground); MEM_free(g->background);
            SYS_enableInts();
            return FALSE;
        }
        SPR_setVisibility(g->beam_sprites[i], HIDDEN);
    }
    PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
    SYS_enableInts();
#if MM_DEBUG_OVERLAY
    /* Window plane stays put while plane A scrolls, so the overlay does not move. */
    VDP_setWindowVPos(FALSE, OVERLAY_ROWS);
    VDP_setTextPlane(WINDOW);
#endif
    render(g);
    SYS_doVBlankProcess();
    return TRUE;
}

void game_update(Game *g, MmInput input)
{
    u16 steps = 1;
    /* A 60 Hz simulation also runs at the same speed on a 50 Hz console. */
    if (SYS_isPAL() && ++g->pal_phase == 5) { steps = 2; g->pal_phase = 0; }
    while (steps--) {
        ++g->frame;
        mm_tick(&g->world, &g->player, input);
        mm_weapon_tick(&g->weapon, &g->world, &g->player, input, g->camera);
        if (g->weapon.fired) g->beam_sound_timer = 8;
        if (g->beam_sound_timer) {
            --g->beam_sound_timer;
            PSG_setTone(0, 25 + (8 - g->beam_sound_timer) * 22);
            PSG_setEnvelope(0, g->beam_sound_timer ? 3 + (8 - g->beam_sound_timer) : PSG_ENVELOPE_MIN);
        }
        if (g->player.jumped)
            XGM2_playPCMEx(jump_sfx, sizeof(jump_sfx), SOUND_PCM_CH2, 15, FALSE, TRUE);
        input.jump_pressed = 0;
        input.shoot_pressed = 0;
    }
    if (g->player.grounded && XGM2_isPlayingPCM(SOUND_PCM_CH2_MSK))
        XGM2_stopPCM(SOUND_PCM_CH2);
    render(g);
}

void game_frame(Game *g)
{
    u16 buttons = JOY_readJoypad(JOY_1);
    u16 pressed = buttons & ~g->input.previous;
    MmInput input = mm_input(&g->input, buttons, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_C);
    input.run_held = (buttons & BUTTON_B) != 0;
    input.shoot_held = (buttons & BUTTON_A) != 0;
    input.shoot_pressed = (pressed & BUTTON_A) != 0;
    input.aim_y = ((buttons & BUTTON_DOWN) != 0) - ((buttons & BUTTON_UP) != 0);
    input.jump_held = (buttons & BUTTON_C) != 0;
    input.down_pressed = (pressed & BUTTON_DOWN) != 0;
    input.up_pressed = (pressed & BUTTON_UP) != 0;
    game_update(g, input);
}
