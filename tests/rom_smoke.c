/* Alternate ROM entry point: exercises production game/render/audio without
   timing-sensitive desktop key injection. Not linked into the release ROM. */
#include "game.h"
#include "player_anim.h"
#include "resources.h"

/* Read by scripts/smoke_test.py through BlastEm's GDB protocol: 0 running, 1 pass, 2 fail. */
volatile u16 mm_smoke_result;
volatile u32 mm_smoke_failure; /* Failure bitmask, printed through the GDB API. */
__attribute__((noinline, used, externally_visible)) void mm_smoke_finish(void)
{
    while (1) SYS_doVBlankProcess();
}

static int turns(Game *g)
{
    int side, air, t, failed = 0;
    for (side = 0; side < 2; ++side) for (air = 0; air < 2; ++air) {
        int seen = 0;
        g->player.facing_left = side;
        if (air) {
            for (t = 0; t < 12; ++t) {
                game_update(g, (MmInput){.jump_pressed = t == 0, .jump_held = 1});
                SYS_doVBlankProcess();
            }
        }
        for (t = 0; t < MM_TURN_POSE_TICKS; ++t) {
            game_update(g, (MmInput){.axis = side ? 1 : -1, .jump_held = 1});
            if (g->player.turn_pose_ticks) {
                if (g->animation != (air ? PLAYER_ANIM_TURN_AIR_R : PLAYER_ANIM_TURN_R) + side ||
                    g->sprite->frameInd != (MM_TURN_POSE_TICKS - g->player.turn_pose_ticks) / 2) failed = 1;
                seen |= 1 << g->sprite->frameInd;
            }
            SYS_doVBlankProcess();
        }
        /* Return to rest, ready for the next turn. */
        for (t = 0; t < 100; ++t) {
            game_update(g, (MmInput){0});
            SYS_doVBlankProcess();
        }
        if (seen != 7 || !g->player.grounded || g->player.turn_pose_ticks) failed = 1;
    }
    return failed;
}

int main(bool hardReset)
{
    static Game g;
    int frame, failed = 0, start, after_right = 0, after_left = 0, jumped = 0, poses_seen = 0;
    static u8 rows_seen[PLAYER_ANIM_COUNT];
    int kept = 0, stood = 0, released = 0, morphed = 0, unmorphed = 0, spin_sound = 0, beam_sound = 0, music = 0;
    int morph_frames = 0, unmorph_frames = 0;
    int hitboxes_seen = 0;
    /* A standing jump shows its takeoff, rise, fall and landing; a jump from a run spins and lands spinning. */
    const int standing_jump_poses = 1 << MM_POSE_TAKEOFF | 1 << MM_POSE_JUMP_UP |
                                    1 << MM_POSE_FALL | 1 << MM_POSE_LAND;
    const int spin_jump_poses = 1 << MM_POSE_SPIN | 1 << MM_POSE_SPIN_LAND;
    const int crouch_poses = 1 << MM_POSE_CROUCH_DOWN | 1 << MM_POSE_CROUCH | 1 << MM_POSE_STAND_UP;
    const int ball_poses = 1 << MM_POSE_MORPH | 1 << MM_POSE_BALL | 1 << MM_POSE_UNMORPH;
    (void)hardReset;
    if (!game_init(&g, 0)) {
        mm_smoke_result = 2;
        VDP_drawText("SMOKE INIT FAIL", 1, 1);
        mm_smoke_finish();
    }
    start = g.player.x;
    for (frame = 0; frame < 760; ++frame) {
        MmInput input = {0};
        if (frame >= 60 && frame < 180) input.axis = 1;
        if (frame >= 180 && frame < 300) input.axis = -1;
        if (frame >= 380 && frame < 420) input.axis = 1;
        /* Jumps hold the button: letting go during the takeoff keeps the body on the ground. The holds
           are short so the standing jump lands before 380 and the spin jump before 445. */
        input.jump_pressed = frame == 330 || frame == 400;
        input.jump_held = (frame >= 330 && frame < 345) || (frame >= 400 && frame < 410);
        /* A shot on the ground plays the beam sound (channel 3); the spin jump at 400 loops channel 2. */
        input.shoot_pressed = input.shoot_held = frame == 470;
        input.aim_y = frame >= 445 && frame < 460;
        if (frame >= 350 && frame < 355) input.aim_y = 1;
        if (frame == 465) input.aim_y = -1;
        /* Crouch persists after both a tap and a long hold. Up stands up.
           Ball: down twice morphs; up unmorphs to crouch, another up stands up. */
        if (frame >= 480 && frame < 483) input.aim_y = 1;
        if (frame == 540) input.aim_y = -1;
        if (frame >= 560 && frame < 580) input.aim_y = 1;
        if (frame == 590) input.aim_y = -1;
        if (frame >= 600 && frame < 603) input.aim_y = 1;
        if (frame >= 606 && frame < 609) input.aim_y = 1;
        if (frame == 621) input.aim_y = -1;
        input.down_pressed = frame == 480 || frame == 600 || frame == 606;
        input.up_pressed = frame == 465 || frame == 540 || frame == 590 || frame == 621 || frame == 650;
        /* Aiming (3.2.5): up held standing, a walk while shooting, a walk aiming up-forward, then a
           short running jump with a shot (the shot stops the spin) still aiming up-forward. */
        if (frame >= 650 && frame < 670) input.aim_y = -1;
        if (frame >= 680 && frame < 700) { input.axis = -1; input.shoot_held = 1; }
        input.shoot_pressed |= frame == 680 || frame == 722;
        if (frame >= 700 && frame < 740) { input.axis = 1; input.aim_y = -1; }
        input.jump_pressed |= frame == 720;
        input.jump_held |= frame >= 720 && frame < 726;
        if (frame >= 722 && frame < 726) input.shoot_held = 1;
        game_update(&g, input);
        {
            int height = g.player.hitbox.bottom - g.player.hitbox.top;
            if (!g.player.samus_hitboxes || g.player.hitbox.right - g.player.hitbox.left != 10) failed |= 16384;
            if (height == 42) hitboxes_seen |= 1;
            if (height == 38) hitboxes_seen |= 2;
            if (height == 24) hitboxes_seen |= 4;
            if (height == 32) hitboxes_seen |= 8;
            if (height == 14) hitboxes_seen |= 16;
            if (height == 20) hitboxes_seen |= 32;
        }
        if (frame == 100) music = XGM2_isPlayingPCM(SOUND_PCM_CH1_MSK) != 0;
        if (frame == 405) spin_sound = XGM2_isPlayingPCM(SOUND_PCM_CH2_MSK) != 0;
        if (frame == 472) beam_sound = XGM2_isPlayingPCM(SOUND_PCM_CH3_MSK) != 0;
        if (frame == 179) after_right = g.player.x;
        if (frame == 299) after_left = g.player.x;
        if (frame == 539) kept = g.player.crouching;
        if (frame == 559) stood = !g.player.crouching;
        if (frame == 589) released = g.player.crouching;
        if (frame == 620) morphed = g.player.ball;
        if (frame == 639) unmorphed = !g.player.ball && g.player.crouching;
        if ((g.animation & ~1) == PLAYER_ANIM_MORPH_R) morph_frames |= 1 << g.sprite->frameInd;
        if ((g.animation & ~1) == PLAYER_ANIM_UNMORPH_R) unmorph_frames |= 1 << g.sprite->frameInd;
        if (g.player.jumped) jumped = 1;
        poses_seen |= 1 << g.player.pose;
        rows_seen[g.animation] = 1;
        if (g.camera.y != 1024 || mm_blocked(&g.world, &g.player,
            mm_pixel(g.player.x), mm_pixel(g.player.y))) failed |= 1;
        SYS_doVBlankProcess();
    }
    if (after_right <= start || after_left >= after_right || !jumped || !g.player.grounded) failed |= 2;
    if ((poses_seen & standing_jump_poses) != standing_jump_poses) failed |= 4;
    if ((poses_seen & spin_jump_poses) != spin_jump_poses) failed |= 8;
    if ((poses_seen & crouch_poses) != crouch_poses) failed |= 16;
    if ((poses_seen & ball_poses) != ball_poses) failed |= 32;
    if (!kept || !stood || !released || !morphed || !unmorphed) failed |= 64;
    if (morph_frames != 3 || unmorph_frames != 3) failed |= 128;
    if (hitboxes_seen != 63) failed |= 16384;
    if (!spin_sound || !beam_sound || !music) failed |= 256;
    if (!rows_seen[PLAYER_ANIM_AIM_UP_R] || !rows_seen[PLAYER_ANIM_WALK_SHOOT_L] ||
        !rows_seen[PLAYER_ANIM_WALK_AIM_UP_R] || !rows_seen[PLAYER_ANIM_JUMP_AIM_DIAG_UP_R]) failed |= 512;
    /* PAL adds one tick in every five game_update calls: 760 calls become 912 ticks. */
    if (g.frame != 760 + (SYS_isPAL() ? 152 : 0)) failed |= 1024;
    /* Both facings keep the normal-jump strip throughout descent, with no frame loop. */
    {
        int side;
        for (side = 0; side < 2; ++side) {
            int seen = 0, last = 0;
            g.player.facing_left = side;
            for (frame = 0; frame < 110; ++frame) {
                game_update(&g, (MmInput){.jump_pressed = frame == 0, .jump_held = 1});
                if (g.player.pose == MM_POSE_FALL && !g.player.falling) {
                    if (g.animation != PLAYER_ANIM_JUMP_UP_R + side ||
                        g.sprite->frameInd != g.player.jump_frame || g.player.jump_frame < last) failed |= 2048;
                    last = g.player.jump_frame;
                    seen |= 1 << last;
                }
                SYS_doVBlankProcess();
            }
            if ((seen & 60) != 60 || !g.player.grounded) failed |= 2048;
        }
    }
    if (turns(&g)) failed |= 4096;
    /* A downward beam just above the floor must render all five impact frames, then hide. */
    mm_weapon_init(&g.weapon);
    g.weapon.beams[0] = (MmBeam){.active = 1, .x = g.player.x + 20 * MM_UNIT,
        .y = g.player.y + (g.player.hitbox.bottom - 8) * MM_UNIT,
        .vy = MM_BEAM_SPEED, .life = 60};
    game_update(&g, (MmInput){0});
    if (!g.weapon.beams[0].impact_ticks ||
        g.beam_sprites[0]->definition != &beam_impact_sprite) failed |= 8192;
    {
        int impact_frames = 0;
        for (frame = 0; frame < MM_IMPACT_TICKS; ++frame) {
            if (g.weapon.beams[0].impact_ticks)
                impact_frames |= 1 << g.beam_sprites[0]->frameInd;
            SYS_doVBlankProcess();
            game_update(&g, (MmInput){0});
        }
        if (impact_frames != 31 || g.weapon.beams[0].impact_ticks) failed |= 8192;
    }
    /* Window plane keeps the result on screen; plane A scrolls with the camera. */
    VDP_setWindowVPos(FALSE, 2);
    VDP_setTextPlane(WINDOW);
    VDP_drawText(failed ? "SMOKE FAIL" : "SMOKE PASS", 1, 0);
    VDP_drawText("MOVE L/R JUMP CAMERA", 1, 1);
    mm_smoke_failure = failed;
    mm_smoke_result = failed ? 2 : 1;
    mm_smoke_finish();
    return 0;
}
