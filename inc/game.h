#pragma once
#include <genesis.h>
#include "map.h"
#include "core/doorlock.h"
#include "core/weapon.h"

/* More than any current room; the exporter refuses rooms that would overflow these tables. */
#define MM_ROOM_MAX_DOORS 8

typedef struct {
    const room_def *room;
    u16 room_index;
    MmWorld world;
    MmBody player;
    MmCamera camera;
    MmInputState input;
    Map *foreground, *background;
    Sprite *sprite;
    MmWeapon weapon;
    Sprite *beam_sprites[MM_BEAM_COUNT];
    bool missile_palette; /* PAL3 holds missile_sprite's palette instead of the beam's. */
    const char *door_prompt; /* Text on the window's first row while Samus is at a door; NULL otherwise. */
    Sprite *door_sprites[MM_ROOM_MAX_DOORS]; /* Full transparent door art; hidden for doors the room does not have. */
    MmDoorLock door_locks[MM_ROOM_MAX_DOORS];
    MmRect door_caps[MM_ROOM_MAX_DOORS]; /* Cap of every door, open or not; for beam hits. */
    MmRect door_solids[MM_ROOM_MAX_DOORS]; /* Caps that still block; world.solids points here. */
    u16 door_lock_count;
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
   facing. Doors start shut, except the one paired with from_door (a door index of the room
   being left, or -1): walking into a room leaves the door behind Samus open. Returns FALSE,
   with the old room already released, only if the new maps do not fit. */
bool game_change_room(Game *game, u16 room_index, Vect2D_s16 spawn, int from_door);
/* One video frame: read input, simulate (1 tick, or 2 every fifth PAL frame), draw. */
void game_frame(Game *game);

/* Input injection supports deterministic replays on the real target. */
void game_update(Game *game, MmInput input);
