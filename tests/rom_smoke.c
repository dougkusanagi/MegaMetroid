/* Alternate ROM entry point: exercises production game/render/audio without
   timing-sensitive desktop key injection. Not linked into the release ROM. */
#include "game.h"

int main(bool hardReset)
{
    static Game g;
    int frame, failed = 0, start, after_right = 0, after_left = 0, jumped = 0, poses_seen = 0;
    /* A standing jump shows its takeoff, rise, fall and landing; a jump from a run spins and lands spinning. */
    const int standing_jump_poses = 1 << MM_POSE_TAKEOFF | 1 << MM_POSE_JUMP_UP |
                                    1 << MM_POSE_FALL | 1 << MM_POSE_LAND;
    const int spin_jump_poses = 1 << MM_POSE_SPIN | 1 << MM_POSE_SPIN_LAND;
    const int crouch_poses = 1 << MM_POSE_CROUCH_DOWN | 1 << MM_POSE_CROUCH | 1 << MM_POSE_STAND_UP;
    (void)hardReset;
    if (!game_init(&g, 0)) {
        VDP_drawText("SMOKE INIT FAIL", 1, 1);
        while (1) SYS_doVBlankProcess();
    }
    start = g.player.x;
    for (frame = 0; frame < 480; ++frame) {
        MmInput input = {0};
        if (frame >= 60 && frame < 180) input.axis = 1;
        if (frame >= 180 && frame < 300) input.axis = -1;
        if (frame >= 380 && frame < 420) input.axis = 1;
        input.jump_pressed = frame == 330 || frame == 400;
        input.aim_y = frame >= 445 && frame < 460;
        game_update(&g, input);
        if (frame == 179) after_right = g.player.x;
        if (frame == 299) after_left = g.player.x;
        if (g.player.jumped) jumped = 1;
        poses_seen |= 1 << g.player.pose;
        if (g.camera.y != 1024 || mm_blocked(&g.world, &g.player,
            mm_pixel(g.player.x), mm_pixel(g.player.y))) failed = 1;
        SYS_doVBlankProcess();
    }
    if (after_right <= start || after_left >= after_right || !jumped || !g.player.grounded) failed = 1;
    if ((poses_seen & standing_jump_poses) != standing_jump_poses) failed = 1;
    if ((poses_seen & spin_jump_poses) != spin_jump_poses) failed = 1;
    if ((poses_seen & crouch_poses) != crouch_poses) failed = 1;
    /* PAL adds one tick in every five game_update calls: 480 calls become 576 ticks. */
    if (g.frame != 480 + (SYS_isPAL() ? 96 : 0)) failed = 1;
    /* Window plane keeps the result on screen; plane A scrolls with the camera. */
    VDP_setWindowVPos(FALSE, 2);
    VDP_setTextPlane(WINDOW);
    VDP_drawText(failed ? "SMOKE FAIL" : "SMOKE PASS", 1, 0);
    VDP_drawText("MOVE L/R JUMP CAMERA", 1, 1);
    while (1) SYS_doVBlankProcess();
    return 0;
}
