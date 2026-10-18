#pragma once
#include <genesis.h>
#include "map.h"
#include "core/weapon.h"

typedef struct {
    const room_def *room;
    MmWorld world;
    MmBody player;
    MmCamera camera;
    MmInputState input;
    Map *foreground, *background;
    Sprite *sprite;
    MmWeapon weapon;
    Sprite *beam_sprites[MM_BEAM_COUNT];
    u16 animation;
    bool music_looping; /* The music intro has ended and the loop body is playing. */
    u16 pal_phase;
    u32 frame; /* 60 Hz simulation ticks since game_init, same on NTSC and PAL. */
} Game;

bool game_init(Game *game, u16 room_index);
/* One video frame: read input, simulate (1 tick, or 2 every fifth PAL frame), draw. */
void game_frame(Game *game);

/* Input injection supports deterministic replays on the real target. */
void game_update(Game *game, MmInput input);
