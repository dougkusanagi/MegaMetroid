/* Alternate ROM entry point: exercises production game/render/audio without
   timing-sensitive desktop key injection. Not linked into the release ROM. */
#include "game.h"
#include "player_anim.h"
#include "resources.h"
#include "world_map.h"

#ifndef MM_SMOKE_ROOM
#define MM_SMOKE_ROOM ROOM_CRATERIA_1
#endif

/* Read by scripts/smoke_test.py through BlastEm's GDB protocol: 0 running, 1 pass, 2 fail. */
volatile u16 mm_smoke_result;
volatile u32 mm_smoke_failure; /* Failure bitmask, printed through the GDB API. */
/* Chunked VDP readback stays within RAM. The GDB runner optionally saves these
   checkpoints to reconstruct video from the emulator's VRAM, CRAM and SAT. */
volatile u16 mm_smoke_video[4096];
volatile u16 mm_smoke_video_header[76];
volatile u16 mm_smoke_video_chunk;
/* VRAM tiles available, tiles used by each room, then heap bytes free at the start and after each
   trip through the doors, and the largest free block at the end. */
volatile u16 mm_smoke_memory[10];
volatile u16 mm_smoke_perf[10]; /* CPU/DMA/missed VInts per jump, then initial/minimum camera Y. */

/* Samus placed at pos stands in the open, inside the room. */
static int spawn_valid(const room_def *room, Vect2D_s16 pos)
{
    MmWorld world;
    MmBody player;
    world.tiles = room->map_collision;
    world.columns = room->map_width / MM_TILE;
    world.rows = room->map_height / MM_TILE;
    mm_samus_init(&player, pos.x, pos.y);
    return !mm_blocked(&world, &player, mm_pixel(player.x), mm_pixel(player.y)) &&
        mm_pixel(player.x) + player.hitbox.right <= room->map_width &&
        mm_pixel(player.y) + player.hitbox.bottom <= room->map_height;
}

/* Etapa 6.5: the room graph in ROM is consistent, and every catalog room is a node whose doors it draws;
   a door is connected exactly when the room beyond it is built. */
static int world_map_valid(void)
{
    u16 i;
    if (WORLD_NODE_COUNT != 34 || !mm_world_consistent(world_nodes, WORLD_NODE_COUNT)) return 0;
    for (i = 0; i < ROOM_COUNT; ++i) {
        const room_def *room = room_get(i);
        int node = mm_world_node_of_room(world_nodes, WORLD_NODE_COUNT, i), door, connected = 0, expected = 0;
        if (node < 0 || world_nodes[node].door_count != room->door_count) return 0;
        for (door = 0; door < room->door_count; ++door) connected += room->doors[door].target_room != ROOM_NO_CONNECTION;
        for (door = 0; door < world_nodes[node].door_count; ++door) {
            int to = world_nodes[node].doors[door].to;
            expected += to != MM_NO_NODE && world_nodes[to].room != MM_NO_NODE;
        }
        if (connected != expected) return 0;
    }
    return mm_world_reach(world_nodes, WORLD_NODE_COUNT, WORLD_NODE_91F8, (int[WORLD_NODE_COUNT]){0}, WORLD_NODE_COUNT) == 29;
}

static int room_catalog_valid(void)
{
    u16 i;
    for (i = 0; i < ROOM_COUNT; ++i) {
        const room_def *room = room_get(i);
        int door;
        if (!room || !room->tileset_fg || !room->tileset_bg || !room->map_fg || !room->map_bg ||
            !room->palette_fg || !room->palette_bg || !room->map_collision ||
            !room->map_width || !room->map_height || room->map_width % MM_TILE || room->map_height % MM_TILE ||
            (room->camera_zone_count && !room->camera_zones) ||
            (room->enemy_spawn_count && !room->enemy_spawns) || (room->door_count && !room->doors)) return 0;
        if (!spawn_valid(room, room->player_initial_pos)) return 0;
        for (door = 0; door < room->door_count; ++door) {
            const RoomDoorDef *entry = &room->doors[door];
            MmRect rect = mm_door_rect(entry->position.x, entry->position.y, entry->facing);
            if (entry->position.x < 0 || entry->position.y < 0 ||
                rect.right > room->map_width || rect.bottom > room->map_height ||
                entry->color > ROOM_DOOR_GREEN || entry->facing > ROOM_DOOR_FACING_DOWN) return 0;
            if (entry->target_room != ROOM_NO_CONNECTION &&
                (entry->target_room >= ROOM_COUNT || !spawn_valid(room_get(entry->target_room), entry->target_spawn))) return 0;
        }
    }
    return room_get(ROOM_COUNT) == NULL && room_get(ROOM_CRATERIA_1)->door_count == 4 && world_map_valid();
}

static int jump_performance(Game *g)
{
    int spin, frame, i, failed = 0;
    for (i = 0; i < 10; ++i) mm_smoke_perf[i] = 0;
    for (spin = 0; spin < 2; ++spin) {
        int start_y, min_y, saw_spin = 0;
        mm_samus_init(&g->player, 1493, 1176);
        mm_weapon_init(&g->weapon);
        for (frame = 0; frame < 40; ++frame) {
            game_update(g, (MmInput){.axis = spin && frame >= 32 ? 1 : 0});
            SYS_doVBlankProcess();
        }
        start_y = min_y = g->camera.y;
        for (frame = 0; frame < 110; ++frame) {
            u32 start = vtimer;
            u16 bytes, load;
            game_update(g, (MmInput){.axis = spin, .jump_pressed = frame == 0, .jump_held = 1});
            if (g->camera.y < min_y) min_y = g->camera.y;
            if (g->player.spin_jumping) saw_spin = 1;
            bytes = DMA_getQueueTransferSize();
            SYS_doVBlankProcess();
            load = SYS_getCPULoad();
            if (load > mm_smoke_perf[spin * 3]) mm_smoke_perf[spin * 3] = load;
            if (bytes > mm_smoke_perf[spin * 3 + 1]) mm_smoke_perf[spin * 3 + 1] = bytes;
            if (vtimer > start + 1) mm_smoke_perf[spin * 3 + 2] += vtimer - start - 1;
        }
        /* Ensure the probe actually jumped through the 1024 boundary, rather
           than passing while stuck on a slope or below a ceiling. */
        mm_smoke_perf[6 + spin * 2] = start_y;
        mm_smoke_perf[7 + spin * 2] = min_y;
        if (start_y != 1024 || start_y - min_y < 24 || saw_spin != spin ||
            mm_smoke_perf[spin * 3 + 2] || mm_smoke_perf[spin * 3 + 1] > 2048) failed = 1;
    }
    return failed;
}
/* VRAM from first_tile on holds exactly the tiles of set (read back through the VDP). */
static int vram_holds(const TileSet *set, u16 first_tile)
{
    vu32 *control = (vu32 *)VDP_CTRL_PORT;
    vu16 *data = (vu16 *)VDP_DATA_PORT;
    TileSet *raw = set->compression == COMPRESSION_NONE ? NULL : unpackTileSet(set, NULL);
    const u16 *expected = (const u16 *)(raw ? raw->tiles : set->tiles);
    u16 i, ok = raw || set->compression == COMPRESSION_NONE;
    if (!ok) return 0;
    SYS_disableInts();
    VDP_setAutoInc(2);
    *control = VDP_READ_VRAM_ADDR((u32)first_tile * 32);
    for (i = 0; i < set->numTile * 16; ++i) if (*data != expected[i]) { ok = 0; break; }
    SYS_enableInts();
    if (raw) MEM_free(raw);
    return ok;
}

/* The room on screen is the room in memory: tiles in VRAM, palettes in CRAM, collision and maps. */
static int room_loaded(const Game *g)
{
    const room_def *room = g->room;
    return g->world.tiles == room->map_collision && g->world.columns == room->map_width / MM_TILE &&
        g->world.rows == room->map_height / MM_TILE && g->foreground && g->background &&
        PAL_getColor(1) == room->palette_fg->data[1] && PAL_getColor(17) == room->palette_bg->data[1] &&
        PAL_getColor(33) == pal_samus_power_suit.data[1] &&
        vram_holds(room->tileset_fg, TILE_USER_INDEX) &&
        vram_holds(room->tileset_bg, TILE_USER_INDEX + room->tileset_fg->numTile) &&
        !mm_blocked(&g->world, &g->player, mm_pixel(g->player.x), mm_pixel(g->player.y));
}

/* Walks until the room changes (or gives up after 240 frames); returns the frames it took. */
static int walk_through(Game *g, int axis)
{
    const room_def *start = g->room;
    int frame;
    for (frame = 0; frame < 240 && g->room == start; ++frame) {
        game_update(g, (MmInput){.axis = axis});
        SYS_doVBlankProcess();
    }
    return g->room == start ? -1 : frame;
}

static void idle(Game *g, int frames)
{
    while (frames--) {
        game_update(g, (MmInput){0});
        SYS_doVBlankProcess();
    }
}

/* Etapa 6.6.3: the cap sprite is visible, shows the lock's frame and covers the room's
   16x64 cap strip, mirrored for a door in the right wall. */
static int door_art(const Game *g, int door)
{
    const Sprite *sprite = g->door_sprites[door];
    const RoomDoorDef *def = &g->room->doors[door];
    int left = def->position.x + (def->facing == ROOM_DOOR_FACING_RIGHT ? MM_DOOR_DEPTH / 2 : 0);
    /* Sprite coordinates carry the engine's +0x80 offset for negative screen positions. */
    if (!sprite || SPR_getVisibility((Sprite *)sprite) != AUTO_FAST ||
        sprite->frameInd != mm_lock_frame(&g->door_locks[door]) ||
        sprite->x != left - g->camera.x + 0x80 ||
        sprite->y != def->position.y - g->camera.y + 0x80)
        return 0;
    return ((sprite->attribut & TILE_ATTR_HFLIP_MASK) != 0) ==
        (def->facing == ROOM_DOOR_FACING_LEFT);
}

/* Walks into the room's shut door at lock_index and shoots it; returns the tick that started
   the opening, or -1 if it stayed shut. */
static int shoot_door(Game *g, int lock_index)
{
    int frame;
    for (frame = 0; frame < 30 && g->door_locks[lock_index].state == MM_DOOR_CLOSED; ++frame) {
        game_update(g, (MmInput){.shoot_pressed = frame == 0, .shoot_held = 1});
        SYS_doVBlankProcess();
    }
    return g->door_locks[lock_index].state == MM_DOOR_OPENING ? frame : -1;
}

/* Etapa 6.6.2, with the real doors: a shut cap stops Samus and her beams; a beam opens a blue
   door after its animation, and only then can she cross. Arriving in a room leaves the paired
   door open behind her. Etapa 6.4: three round trips return every byte of heap, and each room
   fits in VRAM. */
static int room_transitions(Game *g)
{
    int failed = 0, trip, i;
    int px, py;
    u16 free_start;
    MmRect cap;
    for (i = 0; i < ROOM_COUNT; ++i)
        mm_smoke_memory[1 + i] = room_get(i)->tileset_fg->numTile + room_get(i)->tileset_bg->numTile;
    mm_smoke_memory[0] = TILE_USER_LENGTH;
    for (i = 0; i < ROOM_COUNT; ++i) if (!game_room_fits_vram(room_get(i))) failed |= 32;
    /* The west door of the Landing Site has no route: shut, its cap stops Samus before the
       strip; one beam opens it, but the open door still leads nowhere and she stays. */
    mm_samus_init(&g->player, 40, 624);
    idle(g, 40);
    cap = mm_door_cap(0, 608, ROOM_DOOR_FACING_RIGHT);
    for (i = 0; i < 90; ++i) { game_update(g, (MmInput){.axis = -1}); SYS_doVBlankProcess(); }
    if (g->door_locks[0].state != MM_DOOR_CLOSED || g->world.solid_count != 4 ||
        mm_pixel(g->player.x) + g->player.hitbox.left != cap.right) failed |= 1;
    if (shoot_door(g, 0) < 0) failed |= 1;
    /* Frame 1 as soon as the shot lands, then one shape every six ticks, then the open jamb.
       Depth grows towards the back, so Samus draws over the cap. */
    if (!door_art(g, 0) || g->door_sprites[0]->depth <= g->sprite->depth) failed |= 1024;
    idle(g, MM_DOOR_STEP_TICKS);
    if (g->door_locks[0].state != MM_DOOR_OPENING || !door_art(g, 0)) failed |= 1024;
    idle(g, MM_DOOR_STEP_TICKS);
    if (g->door_locks[0].state != MM_DOOR_OPENING || !door_art(g, 0)) failed |= 1024;
    idle(g, MM_DOOR_STEP_TICKS);
    if (!door_art(g, 0)) failed |= 1024;
    if (g->door_locks[0].state != MM_DOOR_OPEN || g->world.solid_count != 3) failed |= 1;
    for (i = g->door_lock_count; i < MM_ROOM_MAX_DOORS; ++i)
        if (SPR_getVisibility(g->door_sprites[i]) != HIDDEN) failed |= 1024;
    if (walk_through(g, -1) != -1 || g->room != room_get(ROOM_CRATERIA_1) ||
        mm_pixel(g->player.x) + g->player.hitbox.left >= MM_DOOR_EDGE) failed |= 2;
    mm_samus_init(&g->player, 2210, 368);
    idle(g, 30);
    free_start = MEM_getFree();
    mm_smoke_memory[3] = free_start;
    for (trip = 0; trip < 3; ++trip) {
        if (trip == 0) {
            /* East door: shut, its cap stops Samus; a beam opens it after the 18-tick animation. */
            cap = mm_door_cap(2272, 352, ROOM_DOOR_FACING_LEFT);
            for (i = 0; i < 90; ++i) { game_update(g, (MmInput){.axis = 1}); SYS_doVBlankProcess(); }
            if (g->door_locks[2].state != MM_DOOR_CLOSED ||
                mm_pixel(g->player.x) + g->player.hitbox.right != cap.left) failed |= 512;
            if (!g->door_prompt) failed |= 2048;
            if (shoot_door(g, 2) < 0) failed |= 512;
            if (!door_art(g, 2)) failed |= 1024;
            idle(g, MM_DOOR_OPEN_TICKS);
            if (g->door_locks[2].state != MM_DOOR_OPEN || !door_art(g, 2)) failed |= 512;
            if (g->door_prompt) failed |= 2048;
        } else if (g->door_locks[2].state != MM_DOOR_OPEN) failed |= 512;
        /* East door: Samus emerges in Crateria 2 facing the way she was walking, and the
           paired door there is already open. */
        if (walk_through(g, 1) < 0 || g->room != room_get(ROOM_CRATERIA_2)) return failed | 4;
        idle(g, 40);
        px = mm_pixel(g->player.x); py = mm_pixel(g->player.y);
        if (!room_loaded(g) || g->player.facing_left || !g->player.grounded ||
            px != 24 || py + g->player.hitbox.bottom != 160 || g->camera.x != 0) failed |= 4;
        if (g->door_locks[0].state != MM_DOOR_OPEN || g->world.solid_count != 0 ||
            !door_art(g, 0)) failed |= 128;
        /* Crateria 2 has a single door, so the other cap sprites must stay hidden. */
        for (i = g->door_lock_count; i < MM_ROOM_MAX_DOORS; ++i)
            if (SPR_getVisibility(g->door_sprites[i]) != HIDDEN) failed |= 1024;
        /* And back through Crateria 2's west door, now walking left. */
        if (walk_through(g, -1) < 0 || g->room != room_get(ROOM_CRATERIA_1)) return failed | 8;
        idle(g, 40);
        px = mm_pixel(g->player.x); py = mm_pixel(g->player.y);
        if (!room_loaded(g) || !g->player.facing_left || !g->player.grounded ||
            px != 2240 || py + g->player.hitbox.bottom != 416) failed |= 16;
        if (g->door_locks[2].state != MM_DOOR_OPEN || g->world.solid_count != 3 ||
            !door_art(g, 2) || !door_art(g, 3)) failed |= 256;
        mm_smoke_memory[4 + trip] = MEM_getFree();
        if (MEM_getFree() != free_start) failed |= 64;
    }
    /* The walk the player makes from the spawn: along the floor to the lower east door (3), which
       links to a room that is not built. Shot open, it lets Samus to the far edge, and nothing
       changes except the prompt. */
    mm_samus_init(&g->player, 256, 1176);
    idle(g, 30);
    for (i = 0; i < 800; ++i) { game_update(g, (MmInput){.axis = 1}); SYS_doVBlankProcess(); }
    cap = mm_door_cap(2272, 1120, ROOM_DOOR_FACING_LEFT);
    if (g->door_locks[3].state != MM_DOOR_CLOSED || g->door_prompt ||
        mm_pixel(g->player.x) + g->player.hitbox.right != cap.left) failed |= 4096;
    if (shoot_door(g, 3) < 0) failed |= 4096;
    idle(g, MM_DOOR_OPEN_TICKS);
    if (g->door_locks[3].state != MM_DOOR_OPEN) failed |= 4096;
    for (i = 0; i < 90; ++i) { game_update(g, (MmInput){.axis = 1}); SYS_doVBlankProcess(); }
    if (g->room != room_get(ROOM_CRATERIA_1) || !g->door_prompt ||
        mm_pixel(g->player.x) + g->player.hitbox.right != 2304) failed |= 8192;
    mm_samus_init(&g->player, 256, 1176);
    idle(g, 60);
    mm_smoke_memory[7] = MEM_getLargestFreeBlock();
    return failed;
}

__attribute__((noinline, used, externally_visible)) void mm_smoke_video_ready(void)
{
    __asm__ volatile ("nop");
}

static void room_video(Game *g)
{
    static const s16 views[][2] = {{1024, 976}, {0, 1024}, {2048, 1024},
                                  {0, 512}, {2048, 256}, {1664, 1024}};
    vu32 *control = (vu32 *)VDP_CTRL_PORT;
    vu16 *data = (vu16 *)VDP_DATA_PORT;
    int scene, i, chunk;
    for (scene = 0; scene < 6; ++scene) {
        mm_samus_init(&g->player, views[scene][0] + 108, views[scene][1] + 84);
        mm_weapon_init(&g->weapon);
        /* Load all newly visible rows/columns, then finish sprite DMA. */
        game_update(g, (MmInput){0});
        SYS_doVBlankProcess();
        game_update(g, (MmInput){0});
        SYS_doVBlankProcess();
        SYS_disableInts();
        VDP_setAutoInc(2);
        *control = VDP_READ_CRAM_ADDR(0);
        for (i = 0; i < 64; ++i) mm_smoke_video_header[i] = *data;
        mm_smoke_video_header[64] = VDP_getBGAAddress();
        mm_smoke_video_header[65] = VDP_getBGBAddress();
        mm_smoke_video_header[66] = VDP_getPlaneWidth();
        mm_smoke_video_header[67] = VDP_getPlaneHeight();
        mm_smoke_video_header[68] = g->camera.x;
        mm_smoke_video_header[69] = g->camera.y;
        mm_smoke_video_header[70] = VDP_getSpriteListAddress();
        mm_smoke_video_header[71] = scene;
        mm_smoke_video_header[72] = VDP_getHScrollTableAddress();
        *control = VDP_READ_VSRAM_ADDR(0);
        mm_smoke_video_header[73] = *data;
        mm_smoke_video_header[74] = *data;
        mm_smoke_video_header[75] = 0;
        for (chunk = 0; chunk < 8; ++chunk) {
            *control = VDP_READ_VRAM_ADDR((u32)chunk * 8192);
            for (i = 0; i < 4096; ++i) mm_smoke_video[i] = *data;
            mm_smoke_video_chunk = chunk;
            mm_smoke_video_ready();
        }
        SYS_enableInts();
    }
}
__attribute__((noinline, used, externally_visible)) void mm_smoke_finish(void)
{
    while (1) SYS_doVBlankProcess();
}

static int beam_motion(void)
{
    static unsigned char air[32 * 32];
    MmWorld world = {air, 32, 32, 0, 0};
    MmBody b;
    MmWeapon w;
    MmCamera camera = {0, 0};
    int side, diagonal, t;
    memset(air, 1, sizeof air);
    mm_body_init(&b, 108, 88, (MmRect){8, 8, 32, 48});
    for (side = 0; side < 2; ++side) for (diagonal = 0; diagonal < 2; ++diagonal) {
        int x = 128 * MM_UNIT, y = 112 * MM_UNIT;
        b.aim = diagonal ? MM_AIM_DIAG_UP : MM_AIM_FORWARD;
        b.facing_left = side;
        mm_weapon_init(&w);
        for (t = 0; t < 16; ++t) {
            int speed = (diagonal ? 699 : 1040) + t * 16;
            int vx = side ? -speed : speed, vy = diagonal ? -speed : 0;
            mm_weapon_tick(&w, &world, &b, (MmInput){.shoot_held = t == 0}, camera, 20, 24);
            x += vx; y += vy;
            if (!w.beams[0].active || w.beams[0].vx != vx || w.beams[0].vy != vy ||
                w.beams[0].x != x || w.beams[0].y != y) return 1;
        }
    }
    return 0;
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

/* Etapa 4.3.2: the ROM wiring of missiles and supers. START cycles the weapon (skipping the
   empty super bin), the missile sheet replaces the beam PAL3 while it is selected or in flight,
   the 16x16 frame follows the direction, ammo drops on a press, a missile opens a blue door,
   and ammo plus selection survive the room change. */
static int missile_weapon(Game *g)
{
    int failed = 0, i;
    MmBeam *shot;
    Sprite *sprite;
    /* The southwest door is still shut after the transitions test, so it has a live lock to hit.
       Stand clear of the cap: firing flush against it would spawn the missile inside the door. */
    mm_weapon_init(&g->weapon);
    mm_samus_init(&g->player, 80, 1136);
    idle(g, 30);
    g->player.facing_left = TRUE;
    g->player.turn_pose_ticks = 0;
    /* START: beam -> missile, skipping the empty super bin. */
    game_update(g, (MmInput){.weapon_pressed = 1});
    SYS_doVBlankProcess();
    if (g->weapon.selected != MM_WEAPON_MISSILE || g->weapon.missiles != MM_START_MISSILES ||
        PAL_getColor(49) != missile_sprite.palette->data[1]) failed = 1;
    /* One press fires one missile, without advancing on its spawn tick. */
    game_update(g, (MmInput){.shoot_pressed = 1, .shoot_held = 1});
    SYS_doVBlankProcess();
    shot = &g->weapon.beams[0];
    sprite = g->beam_sprites[0];
    if (!g->weapon.fired || g->weapon.fired_kind != MM_WEAPON_MISSILE ||
        g->weapon.missiles != MM_START_MISSILES - 1 || !shot->active ||
        shot->kind != MM_SHOT_MISSILE || shot->vx != -MM_SHOT_START_SPEED || shot->vy ||
        !sprite || SPR_getVisibility(sprite) != AUTO_FAST || sprite->definition != &missile_sprite ||
        sprite->frameInd != 3) failed = 1;
    game_update(g, (MmInput){0});
    SYS_doVBlankProcess();
    if (shot->vx != -(MM_SHOT_START_SPEED + MM_MISSILE_ACCEL)) failed = 1;
    /* Let it reach the cap: the blue lock opens on the missile hit. */
    for (i = 0; i < 60 && g->door_locks[1].state == MM_DOOR_CLOSED; ++i) {
        game_update(g, (MmInput){0});
        SYS_doVBlankProcess();
    }
    if (g->door_locks[1].state != MM_DOOR_OPENING || shot->active || !shot->impact_ticks) failed = 1;
    idle(g, MM_DOOR_OPEN_TICKS);
    if (g->door_locks[1].state != MM_DOOR_OPEN) failed = 1;
    /* Granting a super lets the next START select it; frame 11 is the left-facing super. */
    g->weapon.supers = 1;
    game_update(g, (MmInput){.weapon_pressed = 1});
    SYS_doVBlankProcess();
    if (g->weapon.selected != MM_WEAPON_SUPER) failed = 1;
    game_update(g, (MmInput){.shoot_pressed = 1, .shoot_held = 1});
    SYS_doVBlankProcess();
    shot = &g->weapon.beams[0];
    if (!g->weapon.fired || g->weapon.fired_kind != MM_WEAPON_SUPER || g->weapon.supers ||
        !shot->active || shot->kind != MM_SHOT_SUPER || shot->vx != -MM_SHOT_START_SPEED ||
        g->beam_sprites[0]->definition != &missile_sprite || g->beam_sprites[0]->frameInd != 11 ||
        /* The empty bin auto-deselects, but the shot on screen keeps the missile palette. */
        g->weapon.selected != MM_WEAPON_BEAM ||
        PAL_getColor(49) != missile_sprite.palette->data[1]) failed = 1;
    idle(g, 60);
    if (shot->active || PAL_getColor(49) != beam_sprite.palette->data[1]) failed = 1;
    /* Select missiles again and carry the remaining four through a room change. */
    game_update(g, (MmInput){.weapon_pressed = 1});
    SYS_doVBlankProcess();
    if (g->weapon.selected != MM_WEAPON_MISSILE) failed = 1;
    if (!game_change_room(g, ROOM_CRATERIA_2, room_crateria_2.player_initial_pos, -1)) failed = 1;
    SYS_doVBlankProcess();
    if (g->weapon.missiles != MM_START_MISSILES - 1 || g->weapon.selected != MM_WEAPON_MISSILE ||
        g->weapon.beams[0].active || g->weapon.beams[0].impact_ticks ||
        PAL_getColor(49) != missile_sprite.palette->data[1]) failed = 1;
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
    if (!room_catalog_valid() || !game_init(&g, MM_SMOKE_ROOM)) {
        mm_smoke_result = 2;
        VDP_drawText("SMOKE INIT FAIL", 1, 1);
        mm_smoke_finish();
    }
#if MM_SMOKE_ROOM == ROOM_CRATERIA_2
    /* Separate smoke build: prove room 2 loads its art, spawn and collision in production code. */
    VDP_setWindowVPos(FALSE, 2);
    VDP_setTextPlane(WINDOW);
    VDP_drawText("CRATERIA 2 LOADED", 1, 0);
    for (frame = 0; frame < 60; ++frame) {
        game_update(&g, (MmInput){0});
        SYS_doVBlankProcess();
    }
    failed = g.room != room_get(ROOM_CRATERIA_2) ||
        g.world.tiles != room_crateria_2.map_collision ||
        g.world.columns != room_crateria_2.map_width / MM_TILE ||
        g.world.rows != room_crateria_2.map_height / MM_TILE ||
        mm_blocked(&g.world, &g.player, mm_pixel(g.player.x), mm_pixel(g.player.y));
    VDP_drawText(failed ? "SMOKE FAIL" : "SMOKE PASS", 1, 1);
    mm_smoke_failure = failed ? 1 : 0;
    mm_smoke_result = failed ? 2 : 1;
    mm_smoke_finish();
#endif
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
    /* 3.3.1: stationary diagonal poses, muzzle and beam direction on the target. */
    {
        MmBody saved_player = g.player;
        int side, down, crouch;
        for (side = 0; side < 2; ++side) for (down = 0; down < 2; ++down)
            for (crouch = 0; crouch < 2; ++crouch) {
                u16 expected;
                mm_samus_init(&g.player, mm_pixel(g.player.x),
                              mm_pixel(g.player.y) + g.player.hitbox.bottom - 48);
                game_update(&g, (MmInput){0});
                g.player.facing_left = side;
                if (crouch) game_update(&g, (MmInput){.aim_y = 1, .down_pressed = 1});
                mm_weapon_init(&g.weapon);
                game_update(&g, (MmInput){.aim_diagonal = down ? 1 : -1,
                                         .shoot_held = 1, .shoot_pressed = 1});
                expected = (crouch ? (down ? PLAYER_ANIM_CROUCH_AIM_DIAG_DOWN_R : PLAYER_ANIM_CROUCH_AIM_DIAG_UP_R) :
                                    (down ? PLAYER_ANIM_AIM_DIAG_DOWN_R : PLAYER_ANIM_AIM_DIAG_UP_R)) + side;
                if (g.animation != expected || g.player.crouching != crouch || g.player.vx ||
                    !g.weapon.fired || (g.weapon.beams[0].vx < 0) != side ||
                    (g.weapon.beams[0].vy > 0) != down) failed |= 1UL << 22;
                SYS_doVBlankProcess();
                game_update(&g, (MmInput){0});
                if (g.player.aim != MM_AIM_FORWARD) failed |= 1UL << 22;
            }
        g.player = saved_player;
        mm_weapon_init(&g.weapon);
    }
    if (turns(&g)) failed |= 4096;
    if (beam_motion()) failed |= 32768;
    /* A downward beam just above the floor must render all five impact frames, then hide. */
    mm_weapon_init(&g.weapon);
    g.weapon.beams[0] = (MmBeam){.active = 1, .x = g.player.x + 20 * MM_UNIT,
        .y = g.player.y + (g.player.hitbox.bottom - 8) * MM_UNIT,
        .vy = 12 * MM_UNIT, .life = 60}; /* Fast fixture also exercises swept collision. */
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
    if (jump_performance(&g)) failed |= 65536;
    failed |= room_transitions(&g) << 17; /* Bits 17-30: which room/door check failed. */
    room_video(&g);
    if (missile_weapon(&g)) failed |= 1UL << 27;
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
