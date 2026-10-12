#include "game.h"

int main(bool hardReset)
{
    static Game game;
    (void)hardReset;
    if (!game_init(&game, 0)) {
        VDP_drawText("Unable to initialize level", 1, 1);
        while (1) SYS_doVBlankProcess();
    }
    while (1) {
        game_frame(&game);
        SYS_doVBlankProcess();
    }
    return 0;
}
