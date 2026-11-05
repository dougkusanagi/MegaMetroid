#pragma once
#include <genesis.h>
#include "map.h"
#include "core/doorlock.h"
#include "core/weapon.h"
#include "core/hud.h"
#include "core/health.h"
#include "core/drop.h"
#include "core/enemy.h"

/* More than any current room; the exporter refuses rooms that would overflow these tables. */
#define MM_ROOM_MAX_DOORS 8
/* The 24x24 Zoomer frame has its 22x17 body on rows 7..23 and columns 1..22, so these
   offsets centre it on the 16x16 hitbox and put its feet on the hitbox bottom. */
#define ENEMY_DRAW_X 4
#define ENEMY_DRAW_Y 8
#define ZOOMER_FRAME_COUNT 6
#define ZOOMER_FRAME_TICKS 8
/* The 32x32 Skree frame keeps its body on the lower rows; the 8x12 hitbox covers its
   lower centre. Frames 0..3 spin while hanging, 4..7 show the dive. */
#define SKREE_FRAME_COUNT 8
#define SKREE_FRAME_TICKS 8
#define SKREE_ATTACK_FRAME_TICKS 4
#define SKREE_DRAW_X 11
#define SKREE_DRAW_Y 20
/* Ripper's 24x16 strip centres its 8x4 body; its four-step loop is A, B, A, C. */
#define RIPPER_FRAME_COUNT 3
#define RIPPER_FRAME_TICKS 8
#define RIPPER_DRAW_X 8
#define RIPPER_DRAW_Y 6
/* Geemer's 24x32 strip has three walk frames; its 16x16 body sits in the middle. */
#define GEEMER_FRAME_COUNT 3
#define GEEMER_FRAME_TICKS 8
#define GEEMER_DRAW_X 4
#define GEEMER_DRAW_Y 8

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
    const char *door_prompt; /* Text on the HUD message row while Samus is at a door; NULL otherwise. */
    u16 door_prompt_ticks; /* Transient notice: 120 rendered frames, rearmed after leaving. */
    MmDropPool drops; /* Small energy and missile drops from dead enemies (5.4). */
    MmHealth health; /* Energy, tanks and hit invulnerability (5.3); damage sources come with the enemies (5.2). */
    MmEnemyPool enemies; /* Simulated enemies; art and animation arrive in 5.2. */
    Sprite *drop_sprites[MM_DROP_MAX]; /* One per drop slot, hidden while the slot is free. */
    Sprite *enemy_sprites[MM_ENEMY_MAX]; /* NULL for enemies without art yet. */
    u8 hud_cells[MM_HUD_ICON_ROWS][MM_HUD_COLUMNS]; /* Status bar tiles on the window, redrawn when they change. */
    MmHudState hud_state; /* The state hud_cells was built from; a frame with the same state skips the rebuild. */
    u16 hud_tile; /* VRAM index of the first status bar tile, just below the sprite tiles. */
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
/* The room's two tilesets fit in the user VRAM, below the status bar, sprite tiles and font. */
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
