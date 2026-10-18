#include "game.h"
#include "resources.h"
#include "player_anim.h"

#define VIEW_WIDTH 256
#define VIEW_HEIGHT 224
/* The 64x64 frame has the body on column 32 and the feet on row 58. These offsets put
   the body on the hitbox centre (x + 20) and the feet on its bottom row (y + 47). */
#define SAMUS_DRAW_X 12
#define SAMUS_DRAW_Y 11

/* Both definitions reserve four tiles, so switching to an impact never reallocates VRAM. */
static SpriteDefinition beam_definition;

/* Build with -DMM_DEBUG_OVERLAY=1 to draw position, velocity, floor and foot tiles. */
#ifndef MM_DEBUG_OVERLAY
#define MM_DEBUG_OVERLAY 0
#endif
#define OVERLAY_ROWS 4

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
    sprintf(line, "FOOT L %d R %d  MUSIC %s", mm_tile(&g->world, left / MM_TILE, foot_row),
            mm_tile(&g->world, right / MM_TILE, foot_row), g->music_looping ? "LOOP " : "INTRO");
    VDP_drawText(line, 0, 3);
}
#endif

/* Right-facing row of the body's pose with no aim or shot. */
static u16 plain_row(const MmBody *p)
{
    switch (p->pose) {
    case MM_POSE_TAKEOFF: return PLAYER_ANIM_TAKEOFF_R;
    case MM_POSE_JUMP_UP: return PLAYER_ANIM_JUMP_UP_R;
    case MM_POSE_FALL: return p->falling ? PLAYER_ANIM_FALL_R : PLAYER_ANIM_JUMP_UP_R;
    case MM_POSE_SPIN: return PLAYER_ANIM_SPIN_R;
    case MM_POSE_WALL_KICK: return PLAYER_ANIM_WALL_KICK_R;
    case MM_POSE_LAND: return PLAYER_ANIM_LAND_R;
    case MM_POSE_SPIN_LAND: return PLAYER_ANIM_SPIN_LAND_R;
    case MM_POSE_CROUCH_DOWN: return PLAYER_ANIM_CROUCH_DOWN_R;
    case MM_POSE_CROUCH: return PLAYER_ANIM_CROUCH_R;
    case MM_POSE_STAND_UP: return PLAYER_ANIM_STAND_UP_R;
    case MM_POSE_MORPH: return PLAYER_ANIM_MORPH_R;
    case MM_POSE_BALL: return PLAYER_ANIM_BALL_R;
    case MM_POSE_UNMORPH: return PLAYER_ANIM_UNMORPH_R;
    default: return p->vx ? PLAYER_ANIM_WALK_R : PLAYER_ANIM_STAND_R;
    }
}

/* Row of the body on screen, facing included (the left row is the right row + 1). Aiming or
   shooting replaces the plain pose, one-shot poses included, except for the ball and its
   transitions, the spin and the wall kick. */
static u16 player_row(const MmBody *p)
{
    /* Air rows by aim, as {rising, falling}: forward (shooting), up, up-forward, down, down-forward. */
    static const u16 air[5][2] = {
        {PLAYER_ANIM_JUMP_SHOOT_R, PLAYER_ANIM_FALL_SHOOT_R},
        {PLAYER_ANIM_JUMP_AIM_UP_R, PLAYER_ANIM_FALL_AIM_UP_R},
        {PLAYER_ANIM_JUMP_AIM_DIAG_UP_R, PLAYER_ANIM_FALL_AIM_DIAG_UP_R},
        {PLAYER_ANIM_JUMP_AIM_DOWN_R, PLAYER_ANIM_FALL_AIM_DOWN_R},
        {PLAYER_ANIM_JUMP_AIM_DIAG_DOWN_R, PLAYER_ANIM_FALL_AIM_DIAG_DOWN_R},
    };
    u16 side = p->facing_left ? 1 : 0, row;
    bool up = p->aim == MM_AIM_UP || p->aim == MM_AIM_DIAG_UP;
    /* Turn strips are indexed by the OLD facing. Aim/shoot keep their cannon pose,
       and a ball, spin or wall kick keeps its own animation. */
    if (p->turn_pose_ticks && !p->ball && !p->spin_jumping && p->pose != MM_POSE_WALL_KICK &&
        p->pose != MM_POSE_UNMORPH && p->aim == MM_AIM_FORWARD && !p->shoot_pose_timer)
        return (p->grounded ? PLAYER_ANIM_TURN_R : PLAYER_ANIM_TURN_AIR_R) + p->turn_pose_left;
    if ((p->aim == MM_AIM_FORWARD && !p->shoot_pose_timer) || p->ball || p->pose == MM_POSE_UNMORPH ||
        p->pose == MM_POSE_SPIN || p->pose == MM_POSE_WALL_KICK)
        row = plain_row(p);
    else if (p->grounded && p->crouching)
        row = up ? PLAYER_ANIM_CROUCH_AIM_UP_R : PLAYER_ANIM_CROUCH_R;
    else if (p->grounded && p->vx)
        row = p->aim == MM_AIM_DIAG_UP ? PLAYER_ANIM_WALK_AIM_UP_R :
              p->aim == MM_AIM_DIAG_DOWN ? PLAYER_ANIM_WALK_AIM_DOWN_R :
              p->aim == MM_AIM_UP ? PLAYER_ANIM_AIM_UP_R : PLAYER_ANIM_WALK_SHOOT_R;
    else if (p->grounded)
        row = up ? PLAYER_ANIM_AIM_UP_R : PLAYER_ANIM_STAND_R;
    else
        row = air[p->aim][p->vy < 0 ? 0 : 1];
    return row + side;
}

/* Walking rows share the leg cycle: switching between them keeps the frame. */
static bool walk_row(u16 row)
{
    row &= ~1;
    return row == PLAYER_ANIM_WALK_R || row == PLAYER_ANIM_WALK_SHOOT_R ||
           row == PLAYER_ANIM_WALK_AIM_UP_R || row == PLAYER_ANIM_WALK_AIM_DOWN_R;
}

static void render(Game *g)
{
    int i;
    int x = mm_pixel(g->player.x), y = mm_pixel(g->player.y);
    MmRect world_bounds = {0, 0, g->level->map_width, g->level->map_height};
    MmRect bounds = mm_camera_bounds(x + 20, y + 28, world_bounds,
        g->level->camera_zones, g->level->camera_zone_count);
    /* Left is its own row, not a flip: the facing picks the row of the action. */
    u16 side = g->player.facing_left ? 1 : 0;
    u16 animation = player_row(&g->player);
    bool rolling = animation == PLAYER_ANIM_BALL_R + side;
    g->camera = mm_camera(x + 20, y + 28, VIEW_WIDTH, VIEW_HEIGHT, bounds);
    MAP_scrollTo(g->foreground, g->camera.x, g->camera.y);
    MAP_scrollTo(g->background, 0, 0);
    SPR_setPosition(g->sprite, x - g->camera.x - SAMUS_DRAW_X, y - g->camera.y - SAMUS_DRAW_Y);
    if (animation != g->animation) {
        if (g->animation != 0xFFFF && walk_row(animation) && walk_row(g->animation))
            SPR_setAnimAndFrame(g->sprite, animation, g->sprite->frameInd);
        else
            SPR_setAnim(g->sprite, animation);
        g->animation = animation;
    }
    if ((animation & ~1) == PLAYER_ANIM_MORPH_R || (animation & ~1) == PLAYER_ANIM_UNMORPH_R) {
        SPR_setAutoAnimation(g->sprite, FALSE);
        SPR_setFrame(g->sprite, (MM_MORPH_TICKS - g->player.pose_timer) / 3);
    } else if ((animation & ~1) == PLAYER_ANIM_LAND_R || (animation & ~1) == PLAYER_ANIM_SPIN_LAND_R) {
        SPR_setAutoAnimation(g->sprite, FALSE);
        SPR_setFrame(g->sprite, g->player.pose == MM_POSE_LAND && g->player.pose_timer <= 2 ? 1 : 0);
    } else if ((animation & ~1) == PLAYER_ANIM_TURN_R || (animation & ~1) == PLAYER_ANIM_TURN_AIR_R) {
        SPR_setAutoAnimation(g->sprite, FALSE);
        SPR_setFrame(g->sprite, (MM_TURN_POSE_TICKS - g->player.turn_pose_ticks) / 2);
    } else if (animation == PLAYER_ANIM_JUMP_UP_R + side) {
        /* These frames follow the simulation, including PAL's extra ticks, rather than looping. */
        SPR_setAutoAnimation(g->sprite, FALSE);
        SPR_setFrame(g->sprite, g->player.jump_frame);
    } else if (rolling) {
        /* The ball only turns while it moves, and a turn takes about the time it rolls its own
           circumference (16 px * pi at 2.75 px/tick): 2 ticks a frame. */
        SPR_setAutoAnimation(g->sprite, g->player.vx != 0);
        if (g->sprite->timer > 2) g->sprite->timer = 2;
    } else {
        SPR_setAutoAnimation(g->sprite, TRUE);
    }
    /* Shorten the walk cycle as dash momentum builds. */
    if (walk_row(animation) && g->player.run_extra && g->sprite->timer > 3)
        g->sprite->timer = 3;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        const MmBeam *beam = &g->weapon.beams[i];
        Sprite *sprite = g->beam_sprites[i];
        bool hit = beam->impact_ticks != 0;
        const SpriteDefinition *definition = hit ? &beam_impact_sprite : &beam_definition;
        SPR_setVisibility(sprite, beam->active || hit ? AUTO_FAST : HIDDEN);
        if (beam->active || hit) {
            if (sprite->definition != definition) SPR_setDefinition(sprite, definition);
            SPR_setAutoAnimation(sprite, FALSE);
            /* Source order: up-left, up, up-right, left, right, down-left, down, down-right. */
            SPR_setFrame(sprite, hit ? (MM_IMPACT_TICKS - beam->impact_ticks) / 2 :
                beam->vy < 0 ? (beam->vx < 0 ? 0 : beam->vx > 0 ? 2 : 1) :
                beam->vy > 0 ? (beam->vx < 0 ? 5 : beam->vx > 0 ? 7 : 6) :
                beam->vx < 0 ? 3 : 4);
            SPR_setPosition(sprite, (hit ? beam->impact_x : mm_pixel(beam->x)) - g->camera.x - (hit ? 8 : 4),
                            (hit ? beam->impact_y : mm_pixel(beam->y)) - g->camera.y - (hit ? 8 : 4));
        }
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
    g->music_looping = FALSE;
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
    PAL_setPalette(PAL2, pal_samus_power_suit.data, CPU);
    g->sprite = SPR_addSprite(&player_sprite, 0, 0, TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    if (!g->foreground || !g->background || !g->sprite) {
        if (g->foreground) MEM_free(g->foreground);
        if (g->background) MEM_free(g->background);
        if (g->sprite) SPR_releaseSprite(g->sprite);
        SYS_enableInts();
        return FALSE;
    }
    PAL_setPalette(PAL3, beam_sprite.palette->data, CPU);
    beam_definition = beam_sprite;
    beam_definition.maxNumTile = beam_impact_sprite.maxNumTile;
    for (i = 0; i < MM_BEAM_COUNT; ++i) {
        /* Reserve the larger impact up front; changing definition then needs no larger block. */
        g->beam_sprites[i] = SPR_addSprite(&beam_impact_sprite, 0, 0, TILE_ATTR(PAL3, TRUE, FALSE, FALSE));
        if (!g->beam_sprites[i]) {
            while (i > 0) SPR_releaseSprite(g->beam_sprites[--i]);
            SPR_releaseSprite(g->sprite);
            MEM_free(g->foreground); MEM_free(g->background);
            SYS_enableInts();
            return FALSE;
        }
        SPR_setVisibility(g->beam_sprites[i], HIDDEN);
    }
    SYS_enableInts();
#if MM_DEBUG_OVERLAY
    /* Window plane stays put while plane A scrolls, so the overlay does not move. */
    VDP_setWindowVPos(FALSE, OVERLAY_ROWS);
    VDP_setTextPlane(WINDOW);
#endif
    XGM2_playPCMEx(music_crateria_arrival_intro, sizeof(music_crateria_arrival_intro),
                   SOUND_PCM_CH1, 15, FALSE, FALSE);
    render(g);
    SYS_doVBlankProcess();
    return TRUE;
}

/* PCM sound effects from res/sound/Sound-Effects. Channel 1 stays free for the music; the spin loop
   and the landing share channel 2, since a landing ends the spin. */
static void play_sounds(const Game *g, MmPose pose_before)
{
    MmPose pose = g->player.pose;
    if (g->weapon.fired)
        XGM2_playPCMEx(beam_sfx, sizeof(beam_sfx), SOUND_PCM_CH3, 15, FALSE, FALSE);
    /* Only the spin and the wall kick make the spin sound, looped while spinning; a standing jump is silent. */
    if (g->player.jumped && g->player.spin_jumping)
        XGM2_playPCMEx(spin_sfx, sizeof(spin_sfx), SOUND_PCM_CH2, 14, FALSE, TRUE);
    if (pose != pose_before && (pose == MM_POSE_LAND || pose == MM_POSE_SPIN_LAND))
        XGM2_playPCMEx(land_sfx, sizeof(land_sfx), SOUND_PCM_CH2, 15, FALSE, FALSE);
    else if (!g->player.spin_jumping && pose_before == MM_POSE_SPIN && pose != MM_POSE_SPIN)
        XGM2_stopPCM(SOUND_PCM_CH2);
}

/* Crateria music: the intro once, then the loop body with the driver's loop on. The driver takes a
   moment to report a new sample as playing, so the intro is not polled in its first half second. */
static void update_music(Game *g)
{
    if (g->music_looping || g->frame < 30 || XGM2_isPlayingPCM(SOUND_PCM_CH1_MSK)) return;
    XGM2_playPCMEx(music_crateria_arrival_loop, sizeof(music_crateria_arrival_loop),
                   SOUND_PCM_CH1, 15, FALSE, TRUE);
    g->music_looping = TRUE;
}

void game_update(Game *g, MmInput input)
{
    u16 steps = 1, row;
    /* A 60 Hz simulation also runs at the same speed on a 50 Hz console. */
    if (SYS_isPAL() && ++g->pal_phase == 5) { steps = 2; g->pal_phase = 0; }
    while (steps--) {
        MmPose pose_before = g->player.pose;
        ++g->frame;
        mm_tick(&g->world, &g->player, input);
        /* The shot leaves from the cannon of the pose that will be on screen. */
        row = player_row(&g->player);
        mm_weapon_tick(&g->weapon, &g->world, &g->player, input, g->camera,
                       player_muzzle[row][0], player_muzzle[row][1]);
        play_sounds(g, pose_before);
        /* Presses count once, even when a PAL frame runs two ticks. */
        input.jump_pressed = 0;
        input.shoot_pressed = 0;
        input.down_pressed = 0;
        input.up_pressed = 0;
    }
    update_music(g);
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
