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
} Game;

bool game_init(Game *game, u16 level_index);
void game_frame(Game *game);

/* Input injection supports deterministic replays on the real target. */
void game_update(Game *game, MmInput input);
