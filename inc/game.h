#pragma once
#include <genesis.h>
#include "map.h"
#include "core/weapon.h"

typedef struct {
    const level_def *level;
    MmWorld world;
    MmBody player;
    MmCamera camera;
    MmInputState input;
    Map *foreground, *background;
    Sprite *sprite;
    MmWeapon weapon;
    Sprite *beam_sprites[MM_BEAM_COUNT];
    u16 beam_sound_timer;
    u16 animation;
    u16 pal_phase;
    u32 frame; /* 60 Hz simulation ticks since game_init, same on NTSC and PAL. */
} Game;

bool game_init(Game *game, u16 level_index);
/* One video frame: read input, simulate (1 tick, or 2 every fifth PAL frame), draw. */
void game_frame(Game *game);

/* Input injection supports deterministic replays on the real target. */
void game_update(Game *game, MmInput input);
