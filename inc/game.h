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
/* The room's two tilesets fit in the user VRAM, below the sprite tiles and the font. */
bool game_room_fits_vram(const room_def *room);
/* Fades out, swaps the room's tiles, maps, palettes and collision, puts Samus at spawn (the
   room's pixel coordinates, as for room_def.player_initial_pos) and fades in. Samus keeps her
   facing. Returns FALSE, with the old room already released, only if the new maps do not fit. */
bool game_change_room(Game *game, u16 room_index, Vect2D_s16 spawn);
/* One video frame: read input, simulate (1 tick, or 2 every fifth PAL frame), draw. */
void game_frame(Game *game);

/* Input injection supports deterministic replays on the real target. */
void game_update(Game *game, MmInput input);
