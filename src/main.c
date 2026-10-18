#include "game.h"

int main(bool hardReset)
{
    static Game game;
    (void)hardReset;
    if (!game_init(&game, ROOM_CRATERIA_1)) {
        VDP_drawText("Unable to initialize room", 1, 1);
        while (1) SYS_doVBlankProcess();
    }
    /* Single loop: simulate and draw, then wait for VBlank to flush DMA and sprites. */
    while (1) {
        game_frame(&game);
        SYS_doVBlankProcess();
    }
    return 0;
}
