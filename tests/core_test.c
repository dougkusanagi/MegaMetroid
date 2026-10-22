#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "core/world.h"
#include "core/weapon.h"
#include "core/worldmap.h"
#include "core/doorlock.h"
#include "world_map.h"
#include "out/maps.h"
#include "out/sm_hitboxes.h"
#include "out/sm_beams.h"
#include "out/sm_missiles.h"
#include "out/sm_wall.h"

static unsigned char tiles[64 * 32];
static MmWorld world = {tiles, 64, 32, 0, 0};
static MmRect box = {0, 0, 8, 16};
static MmInput idle = {0};
static void reset(void) { memset(tiles, 1, sizeof tiles); }
static void floor_at(int row) {
    int x, y;
    for (y = row; y < 32; ++y) for (x = 0; x < 64; ++x) tiles[y * 64 + x] = 0;
}
/* Ticks once and returns how far the body moved, in Q24.8. */
static int step_dx(MmBody *b, MmInput input) {
    int x = b->x;
    mm_tick(&world, b, input);
    return b->x - x;
}
/* Compares the movement of count ticks against the Super Metroid trace, in 1/256 px. */
static void expect_dx(MmBody *b, MmInput input, const int *expected, int count) {
    int i, dx;
    for (i = 0; i < count; ++i) {
        dx = step_dx(b, input);
        if (dx != expected[i]) fprintf(stderr, "tick %d: moved %d, Super Metroid %d\n", i, dx, expected[i]);
        assert(dx == expected[i]);
    }
}
static void movement(void) {
    MmBody left, right;
    MmInput l = {.axis = -1}, r = {.axis = 1}, run = {.axis = 1, .run_held = 1};
    /* Super Metroid, measured: one pixel, +0.1875 a tick up to 1.875, then 2.75 (docs/FISICA.md). */
    static const int walk[] = {256, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 704, 704};
    /* Letting go slides 16 px: 0.5 less each tick, plus one pixel a tick. */
    static const int slide[] = {960, 832, 704, 576, 448, 320, 256, 0, 0};
    /* Turning around keeps the old speed for a tick, slows down to rest, then walks the new way. */
    static const int turn[] = {704, 576, 448, 320, 192, 64, 0, -256, -48, -96};
    /* From rest it wobbles the new way for 7 ticks before the first step. */
    static const int turn_rest[] = {0, 0, -192, 0, -192, 0, -192, -256, -48};
    int i, origin = 256 * MM_UNIT;
    reset(); floor_at(30);
    mm_body_init(&left, 256, 224, box); mm_tick(&world, &left, idle);
    right = left; left.facing_left = 1;
    for (i = 0; i < 80; ++i) {
        mm_tick(&world, &left, l); mm_tick(&world, &right, r);
        assert(origin - left.x == right.x - origin);
    }
    mm_body_init(&right, 256, 224, box); mm_tick(&world, &right, idle);
    expect_dx(&right, r, walk, 13);
    expect_dx(&right, idle, slide, 9);
    for (i = 0; i < 20; ++i) mm_tick(&world, &right, r);
    expect_dx(&right, l, turn, 10);
    assert(right.facing_left);
    mm_body_init(&right, 256, 224, box); mm_tick(&world, &right, idle);
    expect_dx(&right, l, turn_rest, 9);
    assert(right.facing_left);
    /* The dash adds 1/16 a tick from the first accelerating tick: 4.75 px/tick after 32 ticks. */
    mm_body_init(&right, 256, 224, box); mm_tick(&world, &right, idle);
    mm_tick(&world, &right, run);
    for (i = 1; i <= 32; ++i) { mm_tick(&world, &right, run); assert(right.run_extra == i * MM_RUN_ACCEL); }
    assert(step_dx(&right, run) == MM_SPEED + MM_RUN_EXTRA_MAX);
    assert(mm_pixel(-1) == -1 && mm_pixel(-256) == -1);
    puts("PASS walking, sliding, turning around and dashing as in Super Metroid; both ways alike");
}
/* Horizontal speed in the air and in the ball, from the Super Metroid traces. */
static void air_steering(void) {
    MmBody b;
    MmInput r = {.axis = 1}, l = {.axis = -1}, run = {.axis = 1, .run_held = 1};
    MmInput press = {.jump_pressed = 1, .jump_held = 1}, hold = {.jump_held = 1};
    MmInput r_press = {.axis = 1, .jump_pressed = 1, .jump_held = 1}, r_hold = {.axis = 1, .jump_held = 1};
    MmInput l_hold = {.axis = -1, .jump_held = 1}, l_press = {.axis = -1, .jump_pressed = 1, .jump_held = 1};
    MmInput run_press = {.axis = 1, .run_held = 1, .jump_pressed = 1, .jump_held = 1};
    MmInput down_press = {.aim_y = 1, .down_pressed = 1};
    static const int jump_right[] = {192, 320, 320};
    static const int spin_reverse[] = {352, 224, 96, 0, -192, -352};
    /* A normal jump or a fall turns around for 7 ticks: the old way, slowing down, then a wobble. */
    static const int jump_reverse[] = {320, 192, 64, 0, -192, 0, -192, -320, -320};
    static const int fall_reverse[] = {448, 320, 192, 64, 0, -192, 0, -192, -384, -256, -448};
    static const int spin_slide[] = {352, 352, 352};
    static const int turn_jump[] = {320, 192, 64, 0, -192, -352};
    static const int air_turn_rest[] = {192, 64, 0, -192, 0, -192, 0, -192, -320};
    static const int fall[] = {256, 448, 256, 448};
    static const int roll[] = {192, 384, 576, 768, 960, 832, 832};
    int i, ledge;
    reset(); floor_at(20);
    /* Normal jump: steering accelerates to 1.25 and letting go stops at once. */
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    mm_tick(&world, &b, press);
    for (i = 0; i < 5; ++i) assert(step_dx(&b, hold) == 0);
    expect_dx(&b, r_hold, jump_right, 3);
    assert(step_dx(&b, hold) == 0);
    expect_dx(&b, r_hold, jump_right, 3);
    expect_dx(&b, l_hold, jump_reverse, 9);
    /* Spin from a walk: 1.375 in the air, kept without a direction, reversed through rest. */
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    for (i = 0; i < 20; ++i) mm_tick(&world, &b, r);
    assert(step_dx(&b, r_press) == MM_SPEED && b.spin_jumping);
    assert(step_dx(&b, r_hold) == MM_SPIN_AIR_SPEED);
    assert(step_dx(&b, hold) == MM_SPIN_AIR_SPEED);
    expect_dx(&b, l_hold, spin_reverse, 6);
    /* Jumping during the slide, with no direction, is a spin at the slide speed, without the extra pixel. */
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    for (i = 0; i < 30; ++i) mm_tick(&world, &b, r);
    mm_tick(&world, &b, idle); mm_tick(&world, &b, idle);
    assert(step_dx(&b, press) == 448 && b.spin_jumping);
    expect_dx(&b, hold, spin_slide, 3);
    /* Jumping in the middle of a turnaround spins and turns the pressed way (turn_jump in the trace). */
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    for (i = 0; i < 30; ++i) mm_tick(&world, &b, r);
    for (i = 0; i < 3; ++i) mm_tick(&world, &b, l);
    assert(step_dx(&b, l_press) == turn_jump[0] && b.spin_jumping);
    expect_dx(&b, l_hold, turn_jump + 1, 5);
    /* A standing jump turns around in the air through a 0.75 nudge first (air_turn_rest in the trace). */
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    mm_tick(&world, &b, press);
    for (i = 0; i < 10; ++i) assert(step_dx(&b, hold) == 0);
    expect_dx(&b, l_hold, air_turn_rest, 9);
    /* A running spin keeps the dash bonus in the air, also without a direction. */
    mm_body_init(&b, 8, 144, box); mm_tick(&world, &b, idle);
    for (i = 0; i < 40; ++i) mm_tick(&world, &b, run);
    mm_tick(&world, &b, run_press);
    assert(step_dx(&b, hold) == MM_SPIN_AIR_SPEED + MM_RUN_EXTRA_MAX);
    /* Landing stops the body: it starts over from rest, even with the direction held. */
    for (i = 0; i < 200 && !b.grounded; ++i) mm_tick(&world, &b, r_hold);
    assert(b.grounded && step_dx(&b, r) == MM_UNIT && step_dx(&b, r) == MM_WALK_ACCEL);
    /* Walking off a ledge falls at 1 and 1.75 px/tick in turn and stops when the direction is let go. */
    reset(); for (i = 0; i < 12; ++i) tiles[20 * 64 + i] = 0;
    mm_body_init(&b, 40, 144, box); mm_tick(&world, &b, idle);
    for (ledge = 0; b.grounded && ledge < 100; ++ledge) mm_tick(&world, &b, r);
    expect_dx(&b, r, fall, 4);
    assert(step_dx(&b, idle) == 0);
    reset(); for (i = 0; i < 12; ++i) tiles[20 * 64 + i] = 0;
    mm_body_init(&b, 40, 144, box); mm_tick(&world, &b, idle);
    for (ledge = 0; b.grounded && ledge < 100; ++ledge) mm_tick(&world, &b, r);
    expect_dx(&b, r, fall, 3);
    expect_dx(&b, l, fall_reverse, 11);
    /* The ball rolls up to 3.25 px/tick and stops when the direction is let go. */
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    mm_tick(&world, &b, down_press); mm_tick(&world, &b, idle); mm_tick(&world, &b, down_press);
    assert(b.ball);
    expect_dx(&b, r, roll, 7);
    assert(step_dx(&b, idle) == 0 && step_dx(&b, l) == -MM_AIR_ACCEL);
    puts("PASS air and ball speeds: 1.25 jump, 1.375 spin kept and reversed, 1/1.75 fall, 3.25 roll");
}
static void collisions(void) {
    MmBody b;
    int i;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 20, box);
    for (i = 0; i < 120; ++i) mm_tick(&world, &b, idle);
    assert(b.grounded && mm_pixel(b.y) == 144 && b.vy == 0);
    b.vx = 0; b.vy = 100 * MM_UNIT; mm_move(&world, &b);
    assert(mm_pixel(b.y) == 144);
    for (i = 0; i < 20; ++i) tiles[i * 64 + 15] = 0;
    b.vx = 100 * MM_UNIT; mm_move(&world, &b);
    assert(mm_pixel(b.x) == 112 && b.vx == 0);
    mm_body_init(&b, 128, 144, box); b.vx = -100 * MM_UNIT; mm_move(&world, &b);
    assert(mm_pixel(b.x) == 128);
    mm_body_init(&b, 80, 0, box); b.vy = -100 * MM_UNIT; mm_move(&world, &b);
    assert(mm_pixel(b.y) == 0 && b.vy == 0);
    mm_body_init(&b, 0, 20, box); b.vx = -MM_SPEED; mm_move(&world, &b); assert(b.x == 0);
    mm_body_init(&b, 504, 20, box); b.vx = MM_SPEED; mm_move(&world, &b); assert(b.x == 504 * MM_UNIT);
    assert(mm_tile(&world, -1, 0) == 0 && mm_tile(&world, 64, 32) == 0);
    puts("PASS floors, walls, ceilings, high speed sweeps and map boundaries");
}
static void jump_and_ledge(void) {
    MmBody b; int i;
    MmInput jump = {.jump_pressed = 1, .jump_held = 1}, right = {.axis = 1};
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    assert(b.grounded);
    mm_tick(&world, &b, jump); assert(b.jumped && !b.grounded && b.takeoff);
    mm_tick(&world, &b, jump); assert(!b.jumped);
    for (i = 0; i < 100; ++i) mm_tick(&world, &b, idle);
    assert(b.grounded && mm_pixel(b.y) == 144);
    reset(); for (i = 0; i < 12; ++i) tiles[20 * 64 + i] = 0;
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    for (i = 0; i < 20; ++i) mm_tick(&world, &b, right);
    assert(!b.grounded && b.vy > 0 && mm_pixel(b.y) > 144);
    puts("PASS jump, landing and ledge release");
}
static void slopes(void) {
    MmBody b; int x, y, i;
    MmRect tiny = {0,0,4,8};
    MmInput right = {.axis = 1}, left = {.axis = -1};
    reset();
    /* Ascending staircase of slope tiles, with flat approaches. */
    for (x=0; x<64; ++x) {
        int row = x < 12 ? 24 : x < 20 ? 35-x : 16;
        for (y=row+1; y<32; ++y) tiles[y*64+x]=0;
        tiles[row*64+x] = x>=12 && x<20 ? 2 : 0;
    }
    mm_body_init(&b, 72, 184, tiny); mm_tick(&world,&b,idle);
    for (i=0;i<48;++i) mm_tick(&world,&b,right);
    assert(mm_pixel(b.x)>168 && mm_pixel(b.y)==120 && b.grounded);
    for (i=0;i<64;++i) mm_tick(&world,&b,left);
    assert(mm_pixel(b.x)<80 && mm_pixel(b.y)==184 && b.grounded);
    /* Mirror the entire terrain and traverse the opposite slope type. */
    for (y=0;y<32;++y) for(x=0;x<32;++x) {
        unsigned char a=tiles[y*64+x], z=tiles[y*64+63-x];
        tiles[y*64+x]=z==2?3:z; tiles[y*64+63-x]=a==2?3:a;
    }
    mm_body_init(&b, 436, 184, tiny); mm_tick(&world,&b,idle);
    for(i=0;i<48;++i) mm_tick(&world,&b,left);
    assert(mm_pixel(b.x)<340 && mm_pixel(b.y)==120 && b.grounded);
    for(i=0;i<64;++i) mm_tick(&world,&b,right);
    assert(mm_pixel(b.x)>430 && mm_pixel(b.y)==184 && b.grounded);
    puts("PASS both slope orientations uphill and downhill");
}
/* Jump from every subpixel/tile phase of an ascending 45-degree surface. */
static void slope_spin_takeoff(void) {
    int side, phase, bonus, x, y, t;
    for (side = -1; side <= 1; side += 2) {
        reset();
        for (x = 0; x < 64; ++x) {
            int col = side > 0 ? x : 63 - x;
            int row = col < 12 ? 24 : col < 20 ? 35 - col : 16;
            for (y = row + 1; y < 32; ++y) tiles[y * 64 + x] = 0;
            tiles[row * 64 + x] = col >= 12 && col < 20 ? (side > 0 ? 2 : 3) : 0;
        }
        for (phase = 0; phase < 8 * MM_UNIT; phase += 32) for (bonus = 0; bonus <= MM_RUN_EXTRA_MAX; bonus += MM_RUN_EXTRA_MAX) {
            MmBody b;
            MmInput held = {.axis = side, .jump_held = 1, .run_held = bonus != 0};
            int origin;
            mm_body_init(&b, side > 0 ? 104 : 384, 60, (MmRect){8, 8, 32, 48});
            b.x += phase;
            for (t = 0; t < 100 && !b.grounded; ++t) mm_tick(&world, &b, idle);
            assert(b.grounded);
            b.speed = MM_SPEED; b.run_extra = bonus; b.walking = 1;
            b.facing_left = side < 0; b.vx = side * (b.speed + bonus);
            held.jump_pressed = 1;
            mm_tick(&world, &b, held);
            assert(b.spin_jumping && b.vx * side > 0);
            origin = b.x;
            held.jump_pressed = 0;
            mm_tick(&world, &b, held);
            assert(b.spin_jumping && !b.grounded && b.vy < 0);
            assert((b.x - origin) * side > 0 && b.vx * side > 0);
            assert(b.run_extra == bonus);
            assert(!mm_blocked(&world, &b, mm_pixel(b.x), mm_pixel(b.y)));
        }
    }
    puts("PASS spin takeoff uphill: both slopes, 64 subpixel phases, walking and full dash");
}
/* A door is gone through by pushing into its far-edge strip, never by standing next to it. */
static void doors(void) {
    /* Left-facing door in the east wall of a 2304 px room: rect 2272..2304 x 352..416. */
    MmRect r = mm_door_rect(2272, 352, MM_DOOR_FACING_LEFT);
    MmRect vertical = mm_door_rect(100, 200, MM_DOOR_FACING_UP);
    MmRect body = {2264, 372, 2274, 414};
    assert(r.left == 2272 && r.top == 352 && r.right == 2304 && r.bottom == 416);
    assert(vertical.right - vertical.left == MM_DOOR_LENGTH && vertical.bottom - vertical.top == MM_DOOR_DEPTH);
    assert(!mm_door_touched(body, 2272, 352, MM_DOOR_FACING_LEFT)); /* In the doorway, not at the wall. */
    body.left = 2286; body.right = 2296;
    assert(!mm_door_touched(body, 2272, 352, MM_DOOR_FACING_LEFT)); /* Strip starts at 2296: touching is not enough. */
    body.right = 2297;
    assert(mm_door_touched(body, 2272, 352, MM_DOOR_FACING_LEFT));
    body.left = 2294; body.right = 2304;
    assert(mm_door_touched(body, 2272, 352, MM_DOOR_FACING_LEFT));
    body.top = 416; body.bottom = 458; /* Standing below the door. */
    assert(!mm_door_touched(body, 2272, 352, MM_DOOR_FACING_LEFT));
    body.top = 300; body.bottom = 352; /* Above it: the edges are exclusive. */
    assert(!mm_door_touched(body, 2272, 352, MM_DOOR_FACING_LEFT));
    /* Right-facing door in the west wall: strip 0..8. */
    body = (MmRect){0, 372, 10, 414};
    assert(mm_door_touched(body, 0, 352, MM_DOOR_FACING_RIGHT));
    body = (MmRect){8, 372, 18, 414};
    assert(!mm_door_touched(body, 0, 352, MM_DOOR_FACING_RIGHT));
    /* Up door (horizontal) in a floor: strip is its bottom 8 px; down door: its top 8 px. */
    body = (MmRect){110, 224, 120, 236};
    assert(mm_door_touched(body, 100, 200, MM_DOOR_FACING_UP));
    body = (MmRect){110, 200, 120, 220};
    assert(!mm_door_touched(body, 100, 200, MM_DOOR_FACING_UP));
    assert(mm_door_touched(body, 100, 200, MM_DOOR_FACING_DOWN));
    body = (MmRect){110, 200, 120, 208};
    assert(mm_door_touched(body, 100, 200, MM_DOOR_FACING_DOWN));
    body = (MmRect){164, 200, 170, 208}; /* Past the 64 px length. */
    assert(!mm_door_touched(body, 100, 200, MM_DOOR_FACING_DOWN));
    puts("PASS doors: only the far-edge strip triggers, for all four facings");
}

/* The Crateria room graph as it ships (rooms/world.json through the exporter). */
static void world_map(void) {
    int order[WORLD_NODE_COUNT], i, n;
    const MmWorldNode *site = &world_nodes[WORLD_NODE_91F8];
    assert(WORLD_NODE_COUNT == 34 && mm_world_consistent(world_nodes, WORLD_NODE_COUNT));
    /* Walking from the Landing Site through Crateria doors alone reaches 29 rooms, no repeats. */
    n = mm_world_reach(world_nodes, WORLD_NODE_COUNT, WORLD_NODE_91F8, order, WORLD_NODE_COUNT);
    assert(n == 29 && order[0] == WORLD_NODE_91F8);
    for (i = 0; i < n; ++i) { int j; for (j = 0; j < i; ++j) assert(order[j] != order[i]); }
    /* The other five (Forgotten Highway) hang off the Wrecked Ship and Maridia. */
    assert(mm_world_reach(world_nodes, WORLD_NODE_COUNT, WORLD_NODE_94FD, order, WORLD_NODE_COUNT) == 5);
    assert(mm_world_reach(world_nodes, WORLD_NODE_COUNT, WORLD_NODE_91F8, order, 4) == 4); /* Capacity respected. */
    assert(mm_world_reach(world_nodes, WORLD_NODE_COUNT, -1, order, 4) == 0 && mm_world_reach(world_nodes, WORLD_NODE_COUNT, 0, order, 0) == 0);
    /* Landing Site: nine by five screens, four doors, each answered from the other side. */
    assert(site->id == 0x91F8 && site->screens_w == 9 && site->screens_h == 5 && site->door_count == 4);
    assert(site->doors[3].wall == MM_WALL_RIGHT && site->doors[3].to == WORLD_NODE_93AA &&
           site->doors[3].at_x == 142 && site->doors[3].at_y == 22);
    assert(world_nodes[WORLD_NODE_93AA].doors[site->doors[3].peer].to == WORLD_NODE_91F8);
    /* Catalog rooms: Crateria 1 is the Landing Site, Crateria 2 stands in for room 93AA. */
    assert(mm_world_node_of_room(world_nodes, WORLD_NODE_COUNT, 0) == WORLD_NODE_91F8);
    assert(mm_world_node_of_room(world_nodes, WORLD_NODE_COUNT, 1) == WORLD_NODE_93AA);
    assert(mm_world_node_of_room(world_nodes, WORLD_NODE_COUNT, 2) == MM_NO_NODE && mm_world_node_of_room(world_nodes, WORLD_NODE_COUNT, -1) == MM_NO_NODE);
    /* Exits to other regions have a destination region and no room. */
    assert(world_nodes[WORLD_NODE_962A].doors[1].to == MM_NO_NODE && world_nodes[WORLD_NODE_962A].doors[1].leaves_area == MM_AREA_BRINSTAR);
    {   /* A graph whose door is not answered, or answered on the same wall, is refused. */
        MmWorldDoor a[1] = {{MM_WALL_RIGHT, 1, 1, 1, 0, MM_AREA_HERE}}, b[1] = {{MM_WALL_RIGHT, 1, 1, 0, 0, MM_AREA_HERE}};
        MmWorldNode two[2] = {{1, 1, 1, 0, 0, MM_NO_NODE, 1, a}, {2, 1, 1, 0, 0, MM_NO_NODE, 1, b}};
        assert(!mm_world_consistent(two, 2));
        b[0].wall = MM_WALL_LEFT;
        assert(mm_world_consistent(two, 2));
        b[0].peer = 1;
        assert(!mm_world_consistent(two, 2));
    }
    puts("PASS world map: 34 rooms, 29 walkable from the Landing Site, doors answered both ways");
}

/* Etapa 6.6: the cap blocks until the door has opened, and each colour takes its own shot. */
static void door_locks(void) {
    MmRect cap, solids[1];
    MmWorld w = {tiles, 64, 32, solids, 1};
    MmDoorLock lock;
    MmBody body;
    int i, steps;
    /* The cap is the half of the door on the room's side, for the four facings. */
    cap = mm_door_cap(2272, 352, MM_DOOR_FACING_LEFT);
    assert(cap.left == 2272 && cap.right == 2288 && cap.top == 352 && cap.bottom == 416);
    cap = mm_door_cap(0, 96, MM_DOOR_FACING_RIGHT);
    assert(cap.left == 16 && cap.right == 32 && cap.top == 96 && cap.bottom == 160);
    cap = mm_door_cap(100, 200, MM_DOOR_FACING_UP);
    assert(cap.left == 100 && cap.right == 164 && cap.top == 200 && cap.bottom == 216);
    cap = mm_door_cap(100, 200, MM_DOOR_FACING_DOWN);
    assert(cap.top == 216 && cap.bottom == 232);
    /* Blue: any shot, then the opening plays and only afterwards the cap lets go. */
    mm_lock_init(&lock, 0);
    assert(mm_lock_solid(&lock) && mm_lock_frame(&lock) == 0);
    assert(mm_lock_hit(&lock, MM_LOCK_BLUE, MM_SHOT_BEAM) && lock.state == MM_DOOR_OPENING);
    assert(!mm_lock_hit(&lock, MM_LOCK_BLUE, MM_SHOT_BEAM)); /* Already opening. */
    for (i = 0; i < MM_DOOR_OPEN_TICKS - 1; ++i) { mm_lock_tick(&lock); assert(mm_lock_solid(&lock)); }
    assert(mm_lock_frame(&lock) == MM_DOOR_STEPS && MM_DOOR_OPEN_TICKS == 18);
    mm_lock_tick(&lock);
    assert(!mm_lock_solid(&lock) && mm_lock_frame(&lock) == MM_DOOR_FRAMES && lock.state == MM_DOOR_OPEN);
    mm_lock_init(&lock, 1);
    assert(!mm_lock_solid(&lock));
    /* Red: five missiles or one super, never the beam. Green: a super only. */
    mm_lock_init(&lock, 0);
    assert(!mm_lock_hit(&lock, MM_LOCK_RED, MM_SHOT_BEAM) && lock.hits == 0);
    for (i = 1; i < MM_RED_DOOR_HITS; ++i) assert(!mm_lock_hit(&lock, MM_LOCK_RED, MM_SHOT_MISSILE) && lock.state == MM_DOOR_CLOSED);
    assert(mm_lock_hit(&lock, MM_LOCK_RED, MM_SHOT_MISSILE));
    mm_lock_init(&lock, 0);
    assert(mm_lock_hit(&lock, MM_LOCK_RED, MM_SHOT_SUPER));
    mm_lock_init(&lock, 0);
    assert(!mm_lock_hit(&lock, MM_LOCK_GREEN, MM_SHOT_BEAM) && !mm_lock_hit(&lock, MM_LOCK_GREEN, MM_SHOT_MISSILE));
    assert(mm_lock_hit(&lock, MM_LOCK_GREEN, MM_SHOT_SUPER));
    /* A beam that stopped on a cap finds its door by the impact point; an open door no longer catches it. */
    {
        MmDoorLock locks[2];
        MmRect caps[2] = {{100, 100, 116, 164}, {200, 100, 216, 164}};
        mm_lock_init(&locks[0], 0); mm_lock_init(&locks[1], 0);
        assert(mm_lock_at(locks, caps, 2, 100, 100) == 0 && mm_lock_at(locks, caps, 2, 115, 163) == 0);
        assert(mm_lock_at(locks, caps, 2, 210, 130) == 1);
        assert(mm_lock_at(locks, caps, 2, 190, 130) == -1 && mm_lock_at(locks, caps, 2, 116, 130) == -1);
        assert(mm_lock_at(locks, caps, 2, 110, 99) == -1 && mm_lock_at(locks, caps, 2, 110, 164) == -1);
        mm_lock_init(&locks[0], 1);
        assert(mm_lock_at(locks, caps, 2, 110, 130) == -1);
        assert(mm_lock_at(locks, caps, 1, 210, 130) == -1);
        /* A beam stops with its box on the cap's near edge, so the impact centre is outside it. */
        mm_lock_init(&locks[0], 0);
        assert(mm_lock_shot(locks, caps, 2, (MmRect){114, 130, 118, 134}) == 0);
        assert(mm_lock_shot(locks, caps, 2, (MmRect){197, 130, 201, 134}) == 1);
        assert(mm_lock_shot(locks, caps, 2, (MmRect){116, 130, 120, 134}) == -1);
        assert(mm_lock_shot(locks, caps, 2, (MmRect){190, 130, 194, 134}) == -1);
        assert(mm_lock_shot(locks, caps, 2, (MmRect){110, 96, 114, 100}) == -1);
        mm_lock_init(&locks[0], 1);
        assert(mm_lock_shot(locks, caps, 2, (MmRect){114, 130, 118, 134}) == -1);
    }
    /* Samus walking right on a floor stops at a closed cap and goes on once it is not a solid. */
    reset(); floor_at(20);
    solids[0] = (MmRect){200, 100, 216, 164};
    mm_samus_init(&body, 100, 100 - mm_samus_boxes[MM_BOX_STAND].top + 0);
    body.y = (20 * MM_TILE - mm_samus_boxes[MM_BOX_STAND].bottom) * MM_UNIT;
    body.grounded = 1;
    {   MmInput right = {0}; right.axis = 1; steps = 0;
        while (steps++ < 200) mm_tick(&w, &body, right);
        assert(mm_pixel(body.x) + body.hitbox.right <= 200 && mm_pixel(body.x) + body.hitbox.right >= 198);
        w.solid_count = 0; /* The door is open. */
        for (steps = 0; steps < 60; ++steps) mm_tick(&w, &body, right);
        assert(mm_pixel(body.x) + body.hitbox.right > 216);
    }
    /* A beam is stopped by a closed cap exactly like by a wall. */
    {   MmRect probe = {198, 120, 202, 124};
        w.solid_count = 1;
        assert(mm_rect_blocked(&w, probe));
        assert(!mm_rect_blocked(&w, (MmRect){190, 120, 198, 124}));
        w.solid_count = 0;
        assert(!mm_rect_blocked(&w, probe));
    }
    puts("PASS door locks: cap blocks until open; blue/red/green take the right shots; shots find their cap");
}

static void camera_and_input(void) {
    MmRect r = {0, 0, 2304, 1248}, small = {0,0,128,128};
    MmCameraZone zone = {{0,960,608,1248},{0,1024,768,1248}};
    MmCamera c; MmInputState s = {0}; MmInput in;
    int x,y;
    for(x=-100;x<2500;x+=17) for(y=-100;y<1400;y+=19) {
        c=mm_camera(x,y,256,224,r);
        assert(c.x>=0 && c.y>=0 && c.x+256<=2304 && c.y+224<=1248);
    }
    c=mm_camera(0,0,256,224,small); assert(c.x==0 && c.y==0);
    r=mm_camera_bounds(100,1100,r,&zone,1);
    c=mm_camera(100,1030,256,224,r); assert(c.y==1024);
    in=mm_input(&s,7,1,2,4); assert(in.axis==0 && in.jump_pressed);
    in=mm_input(&s,6,1,2,4); assert(in.axis==1 && !in.jump_pressed);
    in=mm_input(&s,1,1,2,4); assert(in.axis==-1 && !in.jump_pressed);
    in=mm_input(&s,4,1,2,4); assert(in.jump_pressed);
    puts("PASS camera bounds, rooms, undersized maps and input edges");
}
static void real_levels(void) {
    MmWorld maps[2] = {{&real_map_1[0][0],288,156,0,0},{&real_map_2[0][0],160,160,0,0}};
    int starts[2][2] = {{256,1176},{1168,80}};
    MmRect hitbox = {8,8,32,48};
    unsigned int seed = 12345;
    int profile, level, frame;
    for (profile = 0; profile < 2; ++profile) for (level=0;level<2;++level) {
        MmBody b;
        if (profile) mm_samus_init(&b, starts[level][0], starts[level][1]);
        else mm_body_init(&b,starts[level][0],starts[level][1],hitbox);
        assert(!mm_blocked(&maps[level],&b,mm_pixel(b.x),mm_pixel(b.y)));
        for(frame=0;frame<20000;++frame) {
            MmInput input = {0};
            seed=seed*1664525u+1013904223u;
            input.axis=(frame/180)%3-1;
            input.jump_pressed=(seed%71)==0;
            if (profile) {
                input.jump_pressed = frame % 127 == 0;
                input.jump_held = frame % 127 < 50;
                input.down_pressed = frame % 233 == 0 || frame % 233 == 6;
                input.aim_y = input.down_pressed ? 1 : 0;
                input.up_pressed = frame % 233 == 80 || frame % 233 == 100;
                input.shoot_pressed = input.shoot_held = frame % 109 == 0;
            }
            mm_tick(&maps[level],&b,input);
            assert(!mm_blocked(&maps[level],&b,mm_pixel(b.x),mm_pixel(b.y)));
            assert(mm_pixel(b.x) + b.hitbox.left >= 0 && mm_pixel(b.y) + b.hitbox.top >= 0);
            assert(mm_pixel(b.x) + b.hitbox.right <= maps[level].columns*8);
            assert(mm_pixel(b.y) + b.hitbox.bottom <= maps[level].rows*8);
        }
    }
    puts("PASS 80000 simulation ticks on actual Crateria maps: generic and production Samus profiles");
}
/* Ticks with the given input, appending each state change to log ("STAND WALK"). */
static void run_states(MmBody *b, char *log, MmInput input, int ticks) {
    int i;
    for (i = 0; i < ticks; ++i) {
        MmState before = b->state;
        mm_tick(&world, b, input);
        if (b->state != before) { strcat(log, " "); strcat(log, mm_state_name(b->state)); }
    }
}
static void states(void) {
    MmBody b;
    char log[256] = "STAND";
    MmInput right = {.axis = 1}, down = {.aim_y = 1};
    MmInput jump = {.jump_pressed = 1, .jump_held = 1};
    MmInput spin = {.axis = 1, .jump_pressed = 1, .jump_held = 1};
    MmInput down_press = {.aim_y = 1, .down_pressed = 1, .jump_held = 1}, up_press = {.up_pressed = 1};
    MmInput hold = {.jump_held = 1}, spin_hold = {.axis = 1, .jump_held = 1};
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box);
    run_states(&b, log, idle, 1);
    run_states(&b, log, right, 20);
    run_states(&b, log, idle, 10);
    run_states(&b, log, jump, 1);
    run_states(&b, log, hold, 20);
    run_states(&b, log, idle, 100);
    run_states(&b, log, right, 10);
    run_states(&b, log, spin, 1);
    run_states(&b, log, spin_hold, 20);
    run_states(&b, log, idle, 100);
    run_states(&b, log, down, 30);
    run_states(&b, log, idle, 2);
    run_states(&b, log, jump, 1);
    run_states(&b, log, hold, 4);
    run_states(&b, log, down_press, 1);
    run_states(&b, log, hold, 3);
    run_states(&b, log, down_press, 1);
    run_states(&b, log, down, 100);
    run_states(&b, log, up_press, 1);
    assert(strcmp(log, "STAND WALK STAND JUMP FALL STAND WALK SPIN WALK STAND CROUCH JUMP BALL CROUCH") == 0);
    printf("PASS state transitions: %s\n", log);
}
/* Measures a standing jump and a running jump; bounds follow docs/FISICA.md. */
static void physics(void) {
    MmBody b;
    MmInput jump = {.jump_pressed = 1, .jump_held = 1}, right = {.axis = 1};
    MmInput right_jump = {.axis = 1, .jump_pressed = 1, .jump_held = 1};
    MmInput hold = {.jump_held = 1}, right_hold = {.axis = 1, .jump_held = 1};
    int ground, apex, air, max_fall, height, range, x0;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    ground = b.y;
    mm_tick(&world, &b, jump); air = 1; apex = b.y; max_fall = 0;
    while (!b.grounded && air < 300) {
        mm_tick(&world, &b, hold); air += !b.grounded;
        if (b.y < apex) apex = b.y;
        if (b.vy > max_fall) max_fall = b.vy;
    }
    height = (ground - apex) / MM_UNIT;
    /* Super Metroid, measured in the original: 111.09 px high and 92 frames off the ground. */
    assert(height == 111);
    assert(air == 92);
    /* A jump lands before its fall reaches the cap; a long drop reaches 5.03 px/tick and stays. */
    assert(max_fall < MM_MAX_FALL);
    reset(); floor_at(31);
    mm_body_init(&b, 80, 0, box); max_fall = 0;
    while (!b.grounded) { mm_tick(&world, &b, idle); if (b.vy > max_fall) max_fall = b.vy; }
    assert(max_fall == MM_MAX_FALL + 8);
    /* Super Metroid: a jump from rest with the direction pressed on the same tick is a standing jump.
       It holds still for the 2 takeoff ticks, then moves 0.75 px and 1.25 px a tick for 90 more
       ticks, the landing tick included. */
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    x0 = b.x;
    mm_tick(&world, &b, right_jump);
    assert(!b.spin_jumping && b.x == x0);
    while (!b.grounded) mm_tick(&world, &b, right_hold);
    assert(b.x - x0 == MM_AIR_ACCEL + 90 * MM_JUMP_AIR_SPEED);
    /* A spin after 10 ticks of walking: 1.875 px on the takeoff tick, then 1.375 for 91 ticks. */
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    for (air = 0; air < 10; ++air) mm_tick(&world, &b, right);
    x0 = b.x;
    mm_tick(&world, &b, right_jump); air = 1;
    assert(b.spin_jumping && b.x - x0 == 10 * MM_WALK_ACCEL);
    while (!b.grounded && air < 300) { mm_tick(&world, &b, right_hold); ++air; }
    assert(air == 92 && b.x - x0 == 10 * MM_WALK_ACCEL + 91 * MM_SPIN_AIR_SPEED);
    range = (b.x - x0) / MM_UNIT;
    printf("PASS jump measured: apex %d px, airtime %d ticks, spin range %d px, fall cap %d px/tick\n",
           height, air, range, max_fall / MM_UNIT);
}
/* Height of a jump whose jump button is held for the first hold_ticks ticks. */
static int jump_height(int hold_ticks) {
    MmBody b;
    MmInput press = {.jump_pressed = 1, .jump_held = 1}, held = {.jump_held = 1};
    int ground, apex, t;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    ground = b.y;
    mm_tick(&world, &b, press); apex = b.y;
    for (t = 1; t < 300 && !b.grounded; ++t) {
        mm_tick(&world, &b, t < hold_ticks ? held : idle);
        if (b.y < apex) apex = b.y;
    }
    return (ground - apex) / MM_UNIT;
}
static void variable_jump(void) {
    /* Measured in Super Metroid: letting go within 2 frames stays on the ground, holding 3 rises
       4.9 px and holding 10 rises 35.9 px; the speed drops to 0 the moment the button is let go. */
    assert(jump_height(1) == 0 && jump_height(2) == 0);
    assert(jump_height(3) == 4 && jump_height(10) == 35 && jump_height(30) == 95);
    assert(jump_height(300) == 111);
    puts("PASS variable jump: 0, 4, 35, 95 and 111 px for 2, 3, 10, 30 and all ticks held");
}
/* As in Super Metroid, there is no jump once the body has left the ledge, not even on the next tick. */
static void ledge_jump(void) {
    MmBody a;
    MmInput press = {.jump_pressed = 1, .jump_held = 1}, right = {.axis = 1};
    int i;
    reset();
    for (i = 0; i < 12; ++i) tiles[20 * 64 + i] = 0;
    mm_body_init(&a, 80, 144, box); mm_tick(&world, &a, idle);
    for (i = 0; a.grounded && i < 100; ++i) mm_tick(&world, &a, right);
    assert(!a.grounded);
    mm_tick(&world, &a, press);
    assert(!a.jumped && !a.takeoff && a.vy > 0);
    puts("PASS no jump after leaving a ledge");
}
/* Walks dir for ticks, counting grounded stops and airborne frames over solid ground. A turnaround
   and a landing stop the body on purpose, so they are not stalls. */
static void sweep_walk(const MmWorld *cw, MmBody *b, int dir, int ticks, int *stalls, int *floats) {
    MmInput in = {0};
    int t, was_grounded;
    in.axis = dir;
    for (t = 0; t < ticks; ++t) {
        was_grounded = b->grounded;
        mm_tick(cw, b, in);
        if (t >= MM_TURN_TICKS && was_grounded && b->grounded && b->vx == 0 &&
            !mm_blocked(cw, b, mm_pixel(b->x) + dir, mm_pixel(b->y) - 1)) ++*stalls;
        if (!b->grounded && mm_blocked(cw, b, mm_pixel(b->x), mm_pixel(b->y) + 1)) ++*floats;
    }
}
/* Every 45-degree surface of Crateria 1 that the standing hitbox can reach. */
static void slope_sweep(void) {
    MmWorld crateria = {&real_map_1[0][0], 288, 156, 0, 0};
    MmRect hitbox = {8, 8, 32, 48};
    int c, r, walked = 0, too_narrow = 0, stalls = 0, floats = 0;
    for (c = 0; c < crateria.columns; ++c) {
        int top = -1;
        for (r = 1; r < crateria.rows && top < 0; ++r)
            if ((real_map_1[r][c] == 2 || real_map_1[r][c] == 3) && real_map_1[r - 1][c] == 1) top = r;
        if (top < 0) continue;
        {
            MmBody b;
            int x = c * MM_TILE - 12, y, t, placed = 0;
            /* Start in the air pocket above the slope, then drop onto it. */
            for (y = top * MM_TILE - 48; y >= 0 && !placed; y -= 4) {
                mm_body_init(&b, x, y, hitbox);
                placed = !mm_blocked(&crateria, &b, x, y);
            }
            for (t = 0; placed && t < 3000 && !b.grounded; ++t) mm_tick(&crateria, &b, (MmInput){0});
            if (!placed || !b.grounded) { ++too_narrow; continue; }
            ++walked;
            sweep_walk(&crateria, &b, 1, 48, &stalls, &floats);
            sweep_walk(&crateria, &b, -1, 96, &stalls, &floats);
            sweep_walk(&crateria, &b, 1, 48, &stalls, &floats);
        }
    }
    assert(walked >= 70 && stalls == 0 && floats == 0);
    printf("PASS 45-degree slopes of Crateria 1: %d surfaces walked both ways, %d too narrow for the hitbox\n",
           walked, too_narrow);
}

static void samus_slope_sweep(void) {
    MmWorld crateria = {&real_map_1[0][0], 288, 156, 0, 0};
    int c, r, walked = 0, stalls = 0, floats = 0;
    for (c = 0; c < crateria.columns; ++c) {
        int top = -1;
        for (r = 1; r < crateria.rows && top < 0; ++r)
            if ((real_map_1[r][c] == 2 || real_map_1[r][c] == 3) && real_map_1[r - 1][c] == 1) top = r;
        if (top >= 0) {
            MmBody b;
            int x = c * MM_TILE - 16, y, t, placed = 0;
            for (y = top * MM_TILE - 48; y >= 0 && !placed; y -= 4) {
                mm_samus_init(&b, x, y);
                placed = !mm_blocked(&crateria, &b, x, y);
            }
            for (t = 0; placed && t < 3000 && !b.grounded; ++t) mm_tick(&crateria, &b, idle);
            if (!placed || !b.grounded) continue;
            ++walked;
            sweep_walk(&crateria, &b, 1, 48, &stalls, &floats);
            sweep_walk(&crateria, &b, -1, 96, &stalls, &floats);
            sweep_walk(&crateria, &b, 1, 48, &stalls, &floats);
            assert(!mm_blocked(&crateria, &b, mm_pixel(b.x), mm_pixel(b.y)));
        }
    }
    assert(walked >= 70 && !stalls && !floats);
    printf("PASS production Samus slope sweep: %d surfaces, no stalls or floating\n", walked);
}
/* A ball crosses a 2-tile tunnel and cannot un-morph inside it. Crouch under a 3-tile ceiling holds. */
static void tunnels(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    MmInput idle_in = {0}, jump = {.jump_pressed = 1, .jump_held = 1}, right = {.axis = 1};
    MmInput down_press = {.aim_y = 1, .down_pressed = 1}, up_press = {.up_pressed = 1};
    int x, y, i, stuck = 0;
    reset(); floor_at(20);
    for (y = 0; y <= 17; ++y) for (x = 20; x <= 40; ++x) tiles[y * 64 + x] = 0;
    mm_body_init(&b, 40, 112, real);
    mm_tick(&world, &b, idle_in);
    assert(b.grounded);
    mm_tick(&world, &b, jump);
    mm_tick(&world, &b, down_press); mm_tick(&world, &b, idle_in); mm_tick(&world, &b, down_press);
    assert(b.ball && b.state == MM_STATE_BALL);
    for (i = 0; i < 80; ++i) mm_tick(&world, &b, idle_in);
    assert(b.grounded && b.ball);
    for (i = 0; i < 60; ++i) { mm_tick(&world, &b, right); if (!b.vx) ++stuck; }
    assert(stuck == 0 && mm_pixel(b.x) >= 160 && mm_pixel(b.x) < 320);
    mm_tick(&world, &b, up_press);
    assert(b.ball);
    for (i = 0; i < 60; ++i) { mm_tick(&world, &b, right); if (!b.vx) ++stuck; }
    assert(stuck == 0 && mm_pixel(b.x) > 330);
    mm_tick(&world, &b, up_press);
    assert(!b.ball && b.state == MM_STATE_CROUCH);
    for (i = 0; i < MM_UNMORPH_TICKS; ++i) mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, up_press);
    assert(b.state == MM_STATE_STAND);
    reset(); floor_at(20);
    for (y = 0; y <= 16; ++y) for (x = 20; x <= 40; ++x) tiles[y * 64 + x] = 0;
    mm_body_init(&b, 200, 112, real);
    b.grounded = 1; b.crouching = 1;
    for (i = 0; i < 11; ++i) mm_tick(&world, &b, idle_in);
    assert(b.crouching && b.hitbox.top == 24 && b.hitbox.bottom == 48 && b.state == MM_STATE_CROUCH);
    puts("PASS ball through a 2-tile tunnel; no standing up or un-morphing under the ceiling");
}
/* Climbs a 40 px shaft by alternating wall jumps: the body must rise from y 112 to 8. Each kick
   presses away from the wall it touches; the first jump spins because the body walks before it. */
static void wall_climb(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    int i, y, side = -1, wall_jumps = 0;
    reset(); floor_at(20);
    for (y = 0; y < 20; ++y) { tiles[y * 64 + 12] = 0; tiles[y * 64 + 18] = 0; }
    mm_body_init(&b, 104, 112, real);
    for (i = 0; i < 400 && mm_pixel(b.y) > 8; ++i) {
        MmInput in = {0};
        int left = mm_blocked(&world, &b, mm_pixel(b.x) - 1, mm_pixel(b.y));
        int right = mm_blocked(&world, &b, mm_pixel(b.x) + 1, mm_pixel(b.y));
        in.jump_held = 1;
        if (b.grounded) { in.axis = side; in.jump_pressed = b.vx != 0; }
        else if (left) { in.axis = 1; in.jump_pressed = 1; side = 1; }
        else if (right) { in.axis = -1; in.jump_pressed = 1; side = -1; }
        else in.axis = side;
        mm_tick(&world, &b, in);
        if (in.jump_pressed && b.jumped && b.pose == MM_POSE_WALL_KICK) ++wall_jumps;
    }
    assert(mm_pixel(b.y) <= 8 && wall_jumps >= 2);
    printf("PASS wall jumps: %d kicks climbed the 40 px shaft to y %d\n", wall_jumps, mm_pixel(b.y));
}
static const char *pose_name(MmPose pose) {
    static const char *const names[] = {"GROUND", "TAKEOFF", "JUMP_UP", "FALL", "SPIN", "WALL_KICK", "LAND", "SPIN_LAND",
                                        "CROUCH_DOWN", "CROUCH", "STAND_UP", "MORPH", "BALL", "UNMORPH"};
    return names[pose];
}
/* Ticks with the given input, appending each pose change to log ("GROUND TAKEOFF"). */
static void run_poses(MmBody *b, char *log, MmInput input, int ticks) {
    int i;
    for (i = 0; i < ticks; ++i) {
        MmPose before = b->pose;
        mm_tick(&world, b, input);
        if (b->pose != before) { strcat(log, " "); strcat(log, pose_name(b->pose)); }
    }
}
/* Ticks the pose stays on screen, counting the tick that set it. Input is held for all of them. */
static int hold_ticks(MmBody *b, MmInput input, MmPose pose) {
    int n = 0;
    while (b->pose == pose && n < 1000) { mm_tick(&world, b, input); ++n; }
    return n;
}
/* Standing jump, spin jump, wall kick and a standing landing that ends when walking starts.
   The spin sound is only for the jumps that spin: a standing jump is silent. */
static void poses(void) {
    MmBody b;
    char log[256];
    MmInput jump = {.jump_pressed = 1, .jump_held = 1}, held = {.jump_held = 1};
    MmInput right = {.axis = 1}, right_jump = {.axis = 1, .jump_pressed = 1, .jump_held = 1};
    MmInput right_held = {.axis = 1, .jump_held = 1};
    int i;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle, 1);
    run_poses(&b, log, jump, 1);
    assert(b.jumped && !b.spin_jumping);
    run_poses(&b, log, held, 200);
    assert(strcmp(log, "GROUND TAKEOFF JUMP_UP FALL LAND GROUND") == 0);
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    mm_tick(&world, &b, jump);
    assert(hold_ticks(&b, held, MM_POSE_TAKEOFF) == MM_TAKEOFF_TICKS);
    while (!b.grounded) mm_tick(&world, &b, held);
    assert(b.pose == MM_POSE_LAND && hold_ticks(&b, idle, MM_POSE_LAND) == 7);

    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle, 1);
    run_poses(&b, log, right, 10);
    run_poses(&b, log, right_jump, 1);
    assert(b.jumped && b.spin_jumping);
    run_poses(&b, log, right_held, 200);
    assert(strcmp(log, "GROUND SPIN SPIN_LAND GROUND") == 0);

    /* A spin jump into a wall at x 96, then a kick pressing away from it (left). */
    reset(); floor_at(20);
    for (i = 0; i < 20; ++i) tiles[i * 64 + 12] = 0;
    mm_body_init(&b, 72, 144, box);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle, 1);
    run_poses(&b, log, right, 1);
    run_poses(&b, log, right_jump, 1);
    run_poses(&b, log, right_held, 12);
    assert(!b.grounded && mm_pixel(b.x) == 88);
    run_poses(&b, log, (MmInput){.axis = -1, .jump_pressed = 1, .jump_held = 1}, 1);
    assert(b.jumped && b.spin_jumping && b.pose == MM_POSE_WALL_KICK && b.vx < 0);
    /* hold_ticks does not log, so the spin after the kick is checked here instead of in the log. */
    assert(hold_ticks(&b, idle, MM_POSE_WALL_KICK) == MM_WALL_KICK_TICKS && b.pose == MM_POSE_SPIN);
    run_poses(&b, log, idle, 200);
    assert(strcmp(log, "GROUND SPIN WALL_KICK SPIN_LAND GROUND") == 0);

    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle, 1);
    run_poses(&b, log, jump, 1);
    run_poses(&b, log, held, 40);
    run_poses(&b, log, right_held, 100);
    assert(strcmp(log, "GROUND TAKEOFF JUMP_UP FALL GROUND") == 0);
    puts("PASS poses: standing jump, spin jump, wall kick, landings and hold durations; spin sound only when spinning");
}
/* Crouch: the crouch-down frame, the crouched loop, then the stand-up frame. Walking out of a
   crouch ends the stand-up at once, as a standing landing does. */
static void crouch_poses(void) {
    MmBody b;
    char log[256];
    MmRect real = {8, 8, 32, 48};
    MmInput down = {.aim_y = 1}, up = {.aim_y = -1, .up_pressed = 1}, right = {.axis = 1}, idle_in = {0};
    /* The standing box is 48 px tall: y 112 puts its feet on the floor at y 160. */
    reset(); floor_at(20);
    mm_body_init(&b, 80, 112, real);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle_in, 1);
    run_poses(&b, log, down, 20);
    run_poses(&b, log, idle_in, 20);
    assert(b.crouching);
    run_poses(&b, log, up, 1);
    run_poses(&b, log, idle_in, 20);
    assert(strcmp(log, "GROUND CROUCH_DOWN CROUCH STAND_UP GROUND") == 0);
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down);
    assert(b.pose == MM_POSE_CROUCH_DOWN && hold_ticks(&b, down, MM_POSE_CROUCH_DOWN) == MM_CROUCH_DOWN_TICKS);
    assert(b.pose == MM_POSE_CROUCH);
    mm_tick(&world, &b, up);
    assert(b.pose == MM_POSE_STAND_UP && hold_ticks(&b, idle_in, MM_POSE_STAND_UP) == MM_STAND_UP_TICKS);
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    strcpy(log, "GROUND");
    run_poses(&b, log, down, 20);
    run_poses(&b, log, right, 20);
    assert(strcmp(log, "GROUND CROUCH_DOWN CROUCH GROUND") == 0);
    puts("PASS crouch: crouch-down, crouched loop and stand-up hold their frames; walking out ends the stand-up");
}
/* Both taps and long holds keep the crouch. Up, a direction or a jump ends it. */
static void crouch_tap(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    MmInput down_press = {.aim_y = 1, .down_pressed = 1}, down_held = {.aim_y = 1}, idle_in = {0};
    MmInput up_press = {.aim_y = -1, .up_pressed = 1}, right = {.axis = 1};
    MmInput jump = {.jump_pressed = 1, .jump_held = 1}, aim_down = {.aim_y = 1, .shoot_held = 1};
    int t;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press);
    for (t = 1; t < 12; ++t) mm_tick(&world, &b, down_held);
    mm_tick(&world, &b, idle_in);
    assert(b.crouching && b.crouch_latch && b.hitbox.top == 24 && b.state == MM_STATE_CROUCH);
    for (t = 0; t < 60; ++t) { mm_tick(&world, &b, idle_in); assert(b.crouching); }
    mm_tick(&world, &b, up_press);
    assert(!b.crouching && !b.crouch_latch && b.state == MM_STATE_STAND);

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press);
    for (t = 1; t <= 60; ++t) mm_tick(&world, &b, down_held);
    mm_tick(&world, &b, idle_in);
    assert(b.crouching && b.crouch_latch && b.state == MM_STATE_CROUCH);

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press); mm_tick(&world, &b, idle_in);
    for (t = 0; t < 20; ++t) mm_tick(&world, &b, idle_in);
    assert(b.crouching);
    mm_tick(&world, &b, right);
    assert(!b.crouching && !b.crouch_latch && b.state == MM_STATE_WALK);

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, jump);
    assert(b.jumped && !b.crouch_latch);
    for (t = 0; t < 100; ++t) mm_tick(&world, &b, idle_in);
    assert(b.grounded && !b.crouching && b.state == MM_STATE_STAND);

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, aim_down); mm_tick(&world, &b, aim_down);
    mm_tick(&world, &b, idle_in);
    assert(!b.crouching && !b.crouch_latch);
    puts("PASS crouch: taps and long holds persist; up, direction or jump ends the crouch");
}
/* Two presses of down less than MM_DOUBLE_TAP_TICKS apart morph, on the ground or in the air, and so does
   a press while crouched. One press does nothing, a late second press after standing up starts a new
   count, and a shot aiming down is no press. */
static void double_tap(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    MmInput down_press = {.aim_y = 1, .down_pressed = 1}, up_press = {.aim_y = -1, .up_pressed = 1};
    MmInput aim_down = {.aim_y = 1, .down_pressed = 1, .shoot_held = 1}, idle_in = {0};
    int t;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press);
    for (t = 0; t < 2; ++t) mm_tick(&world, &b, idle_in);
    assert(!b.ball && b.crouch_latch);
    mm_tick(&world, &b, down_press);
    assert(b.ball && !b.crouch_latch && b.state == MM_STATE_BALL);
    mm_tick(&world, &b, up_press);
    assert(!b.ball);
    assert(b.crouching);
    for (t = 0; t < 10; ++t) mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, up_press);
    for (t = 0; t < 30; ++t) { mm_tick(&world, &b, idle_in); assert(!b.ball && !b.crouching); }

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    /* A long hold also stays crouched; a new up press first stands up. */
    mm_tick(&world, &b, down_press);
    for (t = 1; t < MM_DOUBLE_TAP_TICKS; ++t) mm_tick(&world, &b, (MmInput){.aim_y = 1});
    mm_tick(&world, &b, idle_in);
    assert(b.crouching);
    mm_tick(&world, &b, up_press);
    assert(!b.crouching);
    mm_tick(&world, &b, down_press);
    assert(!b.ball);
    mm_tick(&world, &b, idle_in); mm_tick(&world, &b, down_press);
    assert(b.ball);

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, aim_down); mm_tick(&world, &b, idle_in); mm_tick(&world, &b, aim_down);
    assert(!b.ball);

    mm_body_init(&b, 80, 60, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press);
    assert(!b.ball && !b.crouching);
    mm_tick(&world, &b, idle_in); mm_tick(&world, &b, down_press);
    assert(b.ball && !b.grounded && b.state == MM_STATE_BALL);
    for (t = 0; t < 200; ++t) mm_tick(&world, &b, idle_in);
    assert(b.ball && b.grounded);

    /* A kept crouch morphs on the next press of down, long after the window closed. */
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press); mm_tick(&world, &b, idle_in);
    for (t = 0; t < 120; ++t) mm_tick(&world, &b, idle_in);
    assert(b.crouching && !b.ball);
    mm_tick(&world, &b, down_press);
    assert(b.ball && b.state == MM_STATE_BALL);
    puts("PASS double tap: two presses of down morph on the ground or in the air; one press, a shot or a late second press do not");
}
/* Morph rules. Down morphs in the air, but not while aiming down with the shot. A morphed body
   does not wall jump; it can stand up again in the air where the standing box fits. */
static void morph_rules(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    MmInput idle_in = {0}, aim_down = {.aim_y = 1, .down_pressed = 1, .shoot_held = 1, .shoot_pressed = 1};
    MmInput down_press = {.aim_y = 1, .down_pressed = 1};
    MmInput kick = {.axis = -1, .jump_pressed = 1, .jump_held = 1}, right = {.axis = 1};
    int y, t;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 100, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, aim_down);
    assert(!b.ball);
    mm_tick(&world, &b, down_press);
    assert(!b.ball);
    mm_tick(&world, &b, idle_in); mm_tick(&world, &b, down_press);
    assert(b.ball);
    /* A wall at x 96. Standing, the body fits flush at x 64 (its box is 4 px wider than the ball's
       at each side); a ball rolls up to x 68 and touches the wall there. */
    reset(); floor_at(20);
    for (y = 0; y < 20; ++y) tiles[y * 64 + 12] = 0;
    mm_body_init(&b, 64, 20, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press); mm_tick(&world, &b, idle_in); mm_tick(&world, &b, down_press);
    for (t = 0; t < 8; ++t) mm_tick(&world, &b, right);
    assert(b.ball && mm_pixel(b.x) == 68);
    b.spin_jumping = 1;
    mm_tick(&world, &b, kick);
    assert(b.ball && !b.jumped);
    mm_body_init(&b, 64, 20, real); mm_tick(&world, &b, idle_in);
    b.spin_jumping = 1;
    mm_tick(&world, &b, kick);
    assert(!b.ball && b.jumped && b.spin_jumping && b.pose == MM_POSE_WALL_KICK);
    puts("PASS morph: shot aims down in the air without morphing; a ball does not wall jump");
}
/* A 45 degree slope (tile 2 steps rising to the right): a ball that lands on it stands up again. */
static void ball_stands_on_slope(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    MmInput morph = {.down_pressed = 1, .aim_y = 1}, up = {.up_pressed = 1, .aim_y = -1}, idle_in = {0};
    int x, y, t;
    reset();
    for (x = 0; x < 64; ++x) {
        int row = x < 12 ? 24 : x < 20 ? 35 - x : 16;
        for (y = row + 1; y < 32; ++y) tiles[y * 64 + x] = 0;
        tiles[row * 64 + x] = x >= 12 && x < 20 ? 2 : 0;
    }
    /* Starts in the air above the slope (its feet at y 88, the surface at y 136), then drops onto it. */
    mm_body_init(&b, 120, 40, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, morph); mm_tick(&world, &b, idle_in); mm_tick(&world, &b, morph);
    for (t = 0; t < 300 && !b.grounded; ++t) mm_tick(&world, &b, idle_in);
    assert(b.grounded && b.ball);
    mm_tick(&world, &b, up);
    assert(!b.ball);
    for (t = 0; t < 30; ++t) { mm_tick(&world, &b, idle_in); assert(b.grounded); }
    puts("PASS ball on a 45 degree slope stands up and stays on the surface");
}
/* Morphs in the air over every reachable landing of Crateria 1 and presses up. The standing box
   can be blocked by a wall a few pixels away or by a ceiling, so a few spots keep the ball. */
static void ball_stand_real(void) {
    MmWorld crateria = {&real_map_1[0][0], 288, 156, 0, 0};
    MmRect real = {8, 8, 32, 48};
    MmInput morph = {.down_pressed = 1, .aim_y = 1}, up = {.up_pressed = 1, .aim_y = -1}, idle_in = {0};
    int x, y, t, slopes = 0, stuck = 0;
    for (x = 16; x < 288 * MM_TILE - 64; x += 6) for (y = 0; y < 156 * MM_TILE - 48; y += 80) {
        MmBody b;
        int c, row, on_slope = 0;
        mm_body_init(&b, x, y, real);
        if (mm_blocked(&crateria, &b, x, y)) continue;
        mm_tick(&crateria, &b, morph); mm_tick(&crateria, &b, idle_in); mm_tick(&crateria, &b, morph);
        if (!b.ball) continue;
        for (t = 0; t < 600 && !b.grounded; ++t) mm_tick(&crateria, &b, idle_in);
        if (!b.grounded) continue;
        row = (mm_pixel(b.y) + real.bottom) / MM_TILE;
        for (c = (mm_pixel(b.x) + real.left) / MM_TILE; c <= (mm_pixel(b.x) + real.right - 1) / MM_TILE; ++c)
            if (mm_tile(&crateria, c, row) == 2 || mm_tile(&crateria, c, row) == 3) on_slope = 1;
        if (!on_slope) continue;
        ++slopes;
        mm_tick(&crateria, &b, up);
        stuck += b.ball;
    }
    assert(slopes >= 100 && stuck * 10 <= slopes);
    printf("PASS ball stands up on Crateria 1 slopes: %d of %d landings still ball (narrow spots)\n", stuck, slopes);
}
/* Ball poses: curling up, rolling, and uncurling to crouch without a jump impulse. */
static void ball_poses(void) {
    MmBody b;
    int t;
    char log[256];
    MmRect real = {8, 8, 32, 48};
    MmInput down_press = {.aim_y = 1, .down_pressed = 1}, up_press = {.aim_y = -1, .up_pressed = 1};
    MmInput idle_in = {0}, jump = {.jump_pressed = 1, .jump_held = 1};
    reset(); floor_at(20);
    mm_body_init(&b, 80, 112, real);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle_in, 1);
    run_poses(&b, log, down_press, 1);
    run_poses(&b, log, idle_in, 10);
    run_poses(&b, log, down_press, 1);
    assert(b.pose == MM_POSE_MORPH && hold_ticks(&b, idle_in, MM_POSE_MORPH) == MM_MORPH_TICKS);
    run_poses(&b, log, jump, 1);
    run_poses(&b, log, idle_in, 100);
    assert(!b.ball && b.grounded && b.pose == MM_POSE_CROUCH);
    /* Jump while in a grounded ball unmorphs, without leaving the floor. */
    mm_tick(&world, &b, down_press);
    for (t = 0; t < MM_MORPH_TICKS; ++t) mm_tick(&world, &b, idle_in);
    run_poses(&b, log, up_press, 1);
    assert(b.pose == MM_POSE_UNMORPH && hold_ticks(&b, idle_in, MM_POSE_UNMORPH) == MM_UNMORPH_TICKS);
    mm_body_init(&b, 80, 112, real);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle_in, 1);
    run_poses(&b, log, down_press, 1);
    run_poses(&b, log, idle_in, 10);
    run_poses(&b, log, down_press, 1);
    run_poses(&b, log, idle_in, 20);
    run_poses(&b, log, jump, 1);
    run_poses(&b, log, idle_in, 100);
    run_poses(&b, log, up_press, 1);
    run_poses(&b, log, idle_in, 20);
    assert(strcmp(log, "GROUND CROUCH_DOWN CROUCH MORPH BALL UNMORPH CROUCH STAND_UP GROUND") == 0);
    puts("PASS ball poses: morph and unmorph take six ticks; jump unmorphs to crouch without lifting");
}
/* A wall jump needs all three: a spin jump, a wall touched (up to MM_WALL_REACH px behind) and the
   d-pad pressed away from it. Pressing into the wall, no direction or a jump without spin do nothing. */
static void wall_jump_rules(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    MmInput away = {.axis = -1, .jump_pressed = 1, .jump_held = 1};
    MmInput into = {.axis = 1, .jump_pressed = 1, .jump_held = 1};
    MmInput none = {.jump_pressed = 1, .jump_held = 1};
    int y, gap;
    reset(); floor_at(20);
    for (y = 0; y < 20; ++y) tiles[y * 64 + 12] = 0;
    /* In the air with the standing box flush against the wall at x 96 (body x 64). */
#define AIRBORNE(x, spin) (mm_body_init(&b, (x), 40, real), b.spin_jumping = (spin), b.vy = -MM_UNIT)
    AIRBORNE(64, 1); mm_tick(&world, &b, away);
    /* The kick faces away from the wall and accelerates from rest, as a spin does. */
    assert(b.jumped && b.facing_left && b.vx == -MM_AIR_ACCEL && b.pose == MM_POSE_WALL_KICK);
    mm_tick(&world, &b, (MmInput){.axis = -1, .jump_held = 1});
    assert(b.vx == -REFERENCE_WALL_SPEED);
    mm_tick(&world, &b, (MmInput){.jump_held = 1});
    assert(b.vx == -REFERENCE_WALL_SPEED); /* Releasing the d-pad preserves spin momentum. */
    AIRBORNE(64, 0); mm_tick(&world, &b, away);
    assert(!b.jumped && b.pose != MM_POSE_WALL_KICK);
    AIRBORNE(64, 1); mm_tick(&world, &b, into);
    assert(!b.jumped && b.pose != MM_POSE_WALL_KICK);
    AIRBORNE(64, 1); mm_tick(&world, &b, none);
    assert(!b.jumped && b.pose != MM_POSE_WALL_KICK);
    for (gap = 1; gap < MM_WALL_REACH; ++gap) {
        AIRBORNE(64 - gap, 1); mm_tick(&world, &b, away);
        assert(b.jumped && b.pose == MM_POSE_WALL_KICK);
    }
    AIRBORNE(64 - MM_WALL_REACH, 1); mm_tick(&world, &b, away);
    assert(!b.jumped && b.pose != MM_POSE_WALL_KICK);
#undef AIRBORNE
    puts("PASS wall jump rules: only in a spin, touching a wall and pressing away from it");
}
/* The aim follows the d-pad while it is held, shot or not. Down crouches on the ground and aims
   down only in the air or, with a direction, down and forward while walking. */
static void aiming(void) {
    MmBody b;
    MmRect real = {8, 8, 32, 48};
    MmInput up = {.aim_y = -1}, up_right = {.aim_y = -1, .axis = 1}, down = {.aim_y = 1};
    MmInput down_right = {.aim_y = 1, .axis = 1}, shoot = {.shoot_held = 1, .shoot_pressed = 1};
    int t;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle);
    assert(b.aim == MM_AIM_FORWARD && !b.shoot_pose_timer);
    mm_tick(&world, &b, up);
    assert(b.aim == MM_AIM_UP && !b.shoot_pose_timer);
    mm_tick(&world, &b, up_right);
    assert(b.aim == MM_AIM_DIAG_UP && b.vx > 0);
    mm_tick(&world, &b, down_right);
    assert(b.aim == MM_AIM_DIAG_DOWN && b.vx > 0 && !b.crouching);
    mm_tick(&world, &b, down);
    assert(b.aim == MM_AIM_FORWARD && b.crouching);
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle);
    mm_tick(&world, &b, shoot);
    for (t = 0; t < 30; ++t) mm_tick(&world, &b, (MmInput){.shoot_held = 1});
    assert(b.shoot_pose_timer == MM_SHOOT_POSE_TICKS);
    for (t = 0; t < MM_SHOOT_POSE_TICKS; ++t) mm_tick(&world, &b, idle);
    assert(!b.shoot_pose_timer);
    mm_body_init(&b, 80, 40, real); mm_tick(&world, &b, idle);
    mm_tick(&world, &b, down);
    assert(!b.grounded && b.aim == MM_AIM_DOWN);
    /* Pressing the other way in the air turns around first, as in Super Metroid. */
    for (t = 0; t < MM_TURN_TICKS; ++t) mm_tick(&world, &b, (MmInput){.aim_y = 1, .axis = -1});
    assert(!b.grounded && b.aim == MM_AIM_DIAG_DOWN && b.facing_left);
    puts("PASS aim: up, up-forward, down-forward walking, down in the air; the shooting pose holds while firing");
}
/* The beam leaves from the given muzzle along the aim, and not when the cannon is inside a wall. */
static void beam_from_muzzle(void) {
    MmBody b;
    MmWeapon w;
    MmRect real = {8, 8, 32, 48};
    MmCamera camera = {0, 0};
    MmInput shoot = {.shoot_held = 1, .shoot_pressed = 1};
    int y;
    reset(); floor_at(20);
    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle);
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, shoot, camera, 33, 21);
    assert(w.fired && w.beams[0].active && w.beams[0].vy == 0 && w.beams[0].vx == 1040);
    assert(w.beams[0].x == (80 + 33) * MM_UNIT + 1040 && w.beams[0].y == (112 + 21) * MM_UNIT);
    b.aim = MM_AIM_DIAG_UP; b.facing_left = 1;
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, shoot, camera, 2, 1);
    assert(w.beams[0].active && w.beams[0].vx == -699 && w.beams[0].vy == -699);
    b.aim = MM_AIM_DOWN;
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, shoot, camera, 10, 35);
    assert(w.beams[0].active && w.beams[0].vx == 0 && w.beams[0].vy == 1040);
    /* A wall at x 112-119, between the body centre (x 100) and a forward muzzle (x 113). */
    for (y = 0; y < 20; ++y) tiles[y * 64 + 14] = 0;
    b.aim = MM_AIM_FORWARD; b.facing_left = 0;
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, shoot, camera, 33, 21);
    assert(!w.beams[0].active);
    puts("PASS beam leaves from the muzzle along the aim, never from inside a wall");
}
/* Reference: Mesen crouch_morph, down_hold, morph_jump and morph_up_hold. */
static void measured_postures(void) {
    MmBody b;
    MmInput down = {.aim_y = 1, .down_pressed = 1};
    MmInput up = {.aim_y = -1, .up_pressed = 1};
    MmInput jump = {.jump_pressed = 1, .jump_held = 1};
    MmRect real = {8, 8, 32, 48};
    int side, t, x, y;
    for (side = 0; side < 2; ++side) {
        reset(); floor_at(20);
        mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle);
        b.facing_left = side;
        mm_tick(&world, &b, down);
        assert(hold_ticks(&b, idle, MM_POSE_CROUCH_DOWN) == 3);
        mm_tick(&world, &b, down);
        assert(hold_ticks(&b, idle, MM_POSE_MORPH) == 6);
        y = b.y;
        mm_tick(&world, &b, jump);
        assert(!b.ball && b.crouching && b.grounded && !b.jumped && b.y == y && b.vy == 0);
        assert(hold_ticks(&b, (MmInput){.jump_held = 1}, MM_POSE_UNMORPH) == 6);
        for (t = 0; t < 30; ++t) mm_tick(&world, &b, (MmInput){.jump_held = 1});
        assert(b.crouching && b.grounded);
        mm_tick(&world, &b, up);
        assert(hold_ticks(&b, idle, MM_POSE_STAND_UP) == 3);
        mm_tick(&world, &b, down); mm_tick(&world, &b, idle); mm_tick(&world, &b, down);
        for (t = 0; t < 10; ++t) mm_tick(&world, &b, idle);
        /* Ceiling allows crouch (24 px) but not standing (40 px). */
        for (t = 0; t <= 16; ++t) for (x = 8; x <= 16; ++x) tiles[t * 64 + x] = 0;
        mm_tick(&world, &b, up);
        assert(!b.ball && b.crouching && !mm_blocked(&world, &b, mm_pixel(b.x), mm_pixel(b.y)));
        for (t = 0; t < 30; ++t) mm_tick(&world, &b, (MmInput){.aim_y = -1});
        assert(b.crouching); /* Held up does not become another press. */
        mm_tick(&world, &b, up);
        assert(b.crouching); /* Standing is still blocked. */
        mm_tick(&world, &b, jump);
        assert(!b.jumped && b.grounded && !mm_blocked(&world, &b, mm_pixel(b.x), mm_pixel(b.y)));
        reset(); floor_at(30);
        mm_body_init(&b, 100, 192, real); mm_tick(&world, &b, idle);
        for (t = 0; t < 12; ++t) mm_tick(&world, &b, (MmInput){.axis = 1});
        mm_tick(&world, &b, (MmInput){.axis = 1, .jump_pressed = 1, .jump_held = 1});
        for (t = 0; t < 150 && !b.grounded; ++t) mm_tick(&world, &b, (MmInput){.jump_held = 1});
        assert(b.pose == MM_POSE_SPIN_LAND && hold_ticks(&b, idle, MM_POSE_SPIN_LAND) == 1);
        /* In the air jump also unmorphs without adding upward speed. */
        reset(); floor_at(30);
        mm_body_init(&b, 80, 40, real);
        mm_tick(&world, &b, down); mm_tick(&world, &b, idle); mm_tick(&world, &b, down);
        for (t = 0; t < 10; ++t) mm_tick(&world, &b, idle);
        y = b.vy;
        mm_tick(&world, &b, jump);
        assert(!b.ball && !b.jumped && !b.grounded && b.vy >= y);
    }
    puts("PASS measured postures: 3/6/6/3 tick transitions, jump unmorphs without lift, crouch fits under low ceilings");
}
static void turn_animation(void) {
    int side, air, t;
    for (side = 0; side < 2; ++side) for (air = 0; air < 2; ++air) {
        MmBody b;
        MmInput opposite = {.axis = side ? 1 : -1, .jump_held = 1};
        reset(); floor_at(30);
        mm_body_init(&b, 100, 192, (MmRect){8, 8, 32, 48});
        mm_tick(&world, &b, idle); b.facing_left = side;
        if (air) {
            mm_tick(&world, &b, (MmInput){.jump_pressed = 1, .jump_held = 1});
            for (t = 0; t < 12; ++t) mm_tick(&world, &b, (MmInput){.jump_held = 1});
        }
        for (t = 0; t < MM_TURN_POSE_TICKS; ++t) {
            mm_tick(&world, &b, opposite);
            assert(b.turn_pose_ticks == MM_TURN_POSE_TICKS - t && b.turn_pose_left == side);
            assert(b.grounded == !air);
        }
        mm_tick(&world, &b, opposite);
        assert(!b.turn_pose_ticks && b.facing_left != side);
        /* Spin and morph cancel a pending normal turn instead of displaying it. */
        b.turn_pose_ticks = 3; b.spin_jumping = 1; b.grounded = 0; b.vy = -MM_JUMP;
        mm_tick(&world, &b, (MmInput){.jump_held = 1});
        assert(!b.turn_pose_ticks);
        b.turn_pose_ticks = 3; b.ball = 1;
        mm_tick(&world, &b, idle);
        assert(!b.turn_pose_ticks);
    }
    puts("PASS turn animation: six ticks, old-facing strips on ground and in air; spin/ball cancel");
}
static void normal_jump_animation(void) {
    int side, short_hop;
    for (side = 0; side < 2; ++side) for (short_hop = 0; short_hop < 2; ++short_hop) {
        MmBody b;
        int t, descent[6] = {0}, seen_ascent_hold = 0, previous = 0;
        reset(); floor_at(30);
        mm_body_init(&b, 100, 192, (MmRect){8, 8, 32, 48});
        mm_tick(&world, &b, idle);
        b.facing_left = side;
        mm_tick(&world, &b, (MmInput){.jump_pressed = 1, .jump_held = 1});
        for (t = 0; t < 150 && !b.grounded; ++t) {
            mm_tick(&world, &b, (MmInput){.jump_held = !short_hop || t < 10});
            if (b.grounded) break;
            if (b.pose == MM_POSE_JUMP_UP && b.jump_frame == 1) seen_ascent_hold = 1;
            if (b.vy > 0) {
                assert(b.pose == MM_POSE_FALL && !b.falling);
                assert(b.jump_frame >= 2 && b.jump_frame <= 5 && b.jump_frame >= previous);
                previous = b.jump_frame;
                ++descent[b.jump_frame];
            }
        }
        assert(b.grounded && seen_ascent_hold);
        assert(descent[2] == 4 && descent[3] == 4 && descent[4] == 4 && descent[5] > 4);
        assert(b.jump_frame == 0);
    }
    puts("PASS normal jump frames: held ascent, four ticks each on descent 2/3/4, held final frame; both facings and short hop");
}
static void measured_beams(void) {
    const MmAim aims[] = {MM_AIM_FORWARD, MM_AIM_UP, MM_AIM_DOWN, MM_AIM_DIAG_UP, MM_AIM_DIAG_DOWN};
    int side, a, t;
    MmBody b;
    MmWeapon w;
    MmCamera camera = {0, 0};
    reset();
    mm_body_init(&b, 108, 88, (MmRect){8, 8, 32, 48});
    for (side = 0; side < 2; ++side) for (a = 0; a < 5; ++a) {
        int dx = a == 1 || a == 2 ? 0 : side ? -1 : 1;
        int dy = a == 1 || a == 3 ? -1 : a == 2 || a == 4 ? 1 : 0;
        int x = 128 * MM_UNIT, y = 112 * MM_UNIT;
        b.aim = aims[a]; b.facing_left = side;
        mm_weapon_init(&w);
        for (t = 0; t < 16; ++t) {
            int speed = a >= 3 ? reference_beam_up_right[t][0] : reference_beam_up[t][1];
            mm_weapon_tick(&w, &world, &b, t ? idle : (MmInput){.shoot_held = 1}, camera, 20, 24);
            x += dx * speed; y += dy * speed;
            assert(w.beams[0].active && w.beams[0].vx == dx * speed && w.beams[0].vy == dy * speed);
            assert(w.beams[0].x == x && w.beams[0].y == y);
        }
    }
    b.aim = MM_AIM_FORWARD; b.facing_left = 0;
    mm_weapon_init(&w);
    for (t = 0; t < 70; ++t) {
        mm_weapon_tick(&w, &world, &b, (MmInput){.shoot_held = 1}, camera, 20, 24);
        assert(w.fired == (t == 0 || t == 15 || t == 40 || t == 65));
    }
    mm_weapon_init(&w);
    for (t = 0; t < 40; ++t) {
        mm_weapon_tick(&w, &world, &b, (MmInput){.shoot_held = !(t & 1), .shoot_pressed = !(t & 1)}, camera, 20, 24);
        assert(w.fired == (t == 0 || t == 16 || t == 32));
    }
    mm_weapon_init(&w);
    for (t = 0; t < MM_BEAM_COUNT; ++t)
        w.beams[t] = (MmBeam){.active = 1, .x = 128 * MM_UNIT, .y = 112 * MM_UNIT, .life = 60};
    mm_weapon_tick(&w, &world, &b, (MmInput){.shoot_held = 1}, camera, 20, 24);
    assert(!w.fired && !w.cooldown);
    puts("PASS beam: 16 exact original velocity samples/trajectory, eight directions, 15/25 cadence, rapid taps and full pool");
}
static void beam_impacts(void) {
    MmBody b;
    MmWeapon w;
    MmCamera camera = {0, 0};
    int y, t;
    reset();
    mm_body_init(&b, 80, 64, (MmRect){8, 8, 32, 48});
    for (y = 0; y < 20; ++y) tiles[y * 64 + 16] = 0;
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, (MmInput){.shoot_held = 1}, camera, 33, 21);
    for (t = 0; t < 3; ++t) mm_weapon_tick(&w, &world, &b, idle, camera, 33, 21);
    assert(!w.beams[0].active && w.beams[0].impact_ticks == MM_IMPACT_TICKS);
    assert(w.beams[0].impact_x == 127 && w.beams[0].impact_y == 85);
    for (t = 0; t < MM_IMPACT_TICKS; ++t) mm_weapon_tick(&w, &world, &b, idle, camera, 33, 21);
    assert(!w.beams[0].impact_ticks);
    /* Leaving the screen is not a wall impact. */
    mm_weapon_init(&w);
    w.beams[0] = (MmBeam){.active = 1, .x = 260 * MM_UNIT, .y = 40 * MM_UNIT,
                         .vx = MM_BEAM_SPEED, .life = 60};
    mm_weapon_tick(&w, &world, &b, idle, camera, 33, 21);
    assert(!w.beams[0].active && !w.beams[0].impact_ticks);
    /* Accelerating beams still sweep thin walls even above their normal speed. */
    mm_weapon_init(&w);
    w.beams[0] = (MmBeam){.active = 1, .x = 80 * MM_UNIT, .y = 85 * MM_UNIT,
                         .vx = 80 * MM_UNIT, .life = 60};
    mm_weapon_tick(&w, &world, &b, idle, camera, 33, 21);
    assert(!w.beams[0].active && w.beams[0].impact_ticks == MM_IMPACT_TICKS && w.beams[0].impact_x == 127);
    puts("PASS beam impact at swept wall contact, expires after 10 ticks; no impact off screen");
}
/* Reference: Mesen missile_right, missile_up_right and super_right, in the Landing Site.
   Every trajectory is compared against the archived original velocities, Q8.8. */
static void measured_missiles(void) {
    const int (*traces[3])[2] = {reference_missile_right, reference_missile_up_right, reference_super_right};
    const int weapons[3] = {MM_WEAPON_MISSILE, MM_WEAPON_MISSILE, MM_WEAPON_SUPER};
    MmBody b;
    MmWeapon w;
    MmCamera camera = {0, 0};
    int s, t;
    for (s = 0; s < 3; ++s) {
        reset();
        mm_body_init(&b, 60, 88, (MmRect){8, 8, 32, 48});
        b.aim = s == 1 ? MM_AIM_DIAG_UP : MM_AIM_FORWARD;
        mm_weapon_init(&w);
        w.missiles = w.supers = 1;
        w.selected = weapons[s];
        for (t = 0; t < 16; ++t) {
            mm_weapon_tick(&w, &world, &b, t ? idle : (MmInput){.shoot_held = 1, .shoot_pressed = 1},
                           camera, 20, 24);
            assert(w.beams[0].active && w.beams[0].vx == traces[s][t][0] && w.beams[0].vy == traces[s][t][1]);
        }
        assert(w.beams[0].kind == (s == 2 ? MM_SHOT_SUPER : MM_SHOT_MISSILE));
        assert(w.beams[0].damage == (s == 2 ? MM_SUPER_DAMAGE : MM_MISSILE_DAMAGE));
    }
    puts("PASS missiles: 16 exact original velocity samples each for missile, diagonal missile and super");
}

static void missile_loadout(void) {
    MmRect real = {8, 8, 32, 48};
    MmBody b;
    MmWeapon w;
    MmCamera camera = {0, 0};
    MmInput press = {.shoot_held = 1, .shoot_pressed = 1};
    MmInput hold = {.shoot_held = 1};
    int t, fired;
    reset();
    mm_body_init(&b, 60, 88, real);
    mm_weapon_init(&w);
    assert(w.selected == MM_WEAPON_BEAM && w.missiles == MM_START_MISSILES && w.supers == MM_START_SUPERS);
    /* The selector skips the empty super bin and waits for the shot to leave the screen. */
    mm_weapon_cycle(&w); mm_weapon_tick(&w, &world, &b, idle, camera, 20, 24);
    assert(w.selected == MM_WEAPON_MISSILE);
    mm_weapon_cycle(&w); mm_weapon_tick(&w, &world, &b, idle, camera, 20, 24);
    assert(w.selected == MM_WEAPON_BEAM);
    w.missiles = 2; w.selected = MM_WEAPON_MISSILE;
    mm_weapon_tick(&w, &world, &b, press, camera, 20, 24);
    assert(w.fired && w.fired_kind == MM_SHOT_MISSILE && w.missiles == 1 && w.cooldown == MM_MISSILE_COOLDOWN);
    mm_weapon_cycle(&w);
    for (t = 0; t < MM_MISSILE_COOLDOWN - 1; ++t) {
        mm_weapon_tick(&w, &world, &b, hold, camera, 20, 24);
        assert(!w.fired && w.selected == MM_WEAPON_MISSILE && w.cycle_pending); /* Held A does not repeat. */
    }
    mm_weapon_tick(&w, &world, &b, press, camera, 20, 24);
    assert(w.fired && w.missiles == 0 && w.selected == MM_WEAPON_BEAM);
    /* Nothing left: the trigger is dead until ammo comes back. */
    fired = w.fired;
    for (t = 0; t < 30; ++t) mm_weapon_tick(&w, &world, &b, press, camera, 20, 24);
    assert(w.selected == MM_WEAPON_BEAM && !w.missiles && fired);
    /* A super has its own cooldown, and the pool never takes a fourth shot. */
    mm_weapon_init(&w); w.supers = 4; w.selected = MM_WEAPON_SUPER;
    mm_weapon_tick(&w, &world, &b, press, camera, 20, 24);
    assert(w.fired && w.supers == 3 && w.cooldown == MM_SUPER_COOLDOWN);
    for (t = 0; t < MM_SUPER_COOLDOWN - 1; ++t) {
        mm_weapon_tick(&w, &world, &b, hold, camera, 20, 24);
        assert(!w.fired);
    }
    mm_weapon_tick(&w, &world, &b, press, camera, 20, 24);
    assert(w.fired && w.supers == 2);
    puts("PASS missile loadout: SELECT cycles past empty bins, press-only fire, 10/20 cooldowns, auto-deselect at zero");
}

static void expect_samus_box(const MmBody *b, MmHitboxKind kind) {
    assert(b->hitbox.right - b->hitbox.left == reference_hitboxes[kind][0]);
    assert(b->hitbox.bottom - b->hitbox.top == reference_hitboxes[kind][1]);
}

static void samus_hitboxes(void) {
    MmBody b;
    MmInput down = {.aim_y = 1, .down_pressed = 1}, up = {.aim_y = -1, .up_pressed = 1};
    MmInput jump = {.jump_pressed = 1, .jump_held = 1};
    int kind, side, t, foot, centre;
    assert(sizeof(reference_hitboxes) / sizeof(reference_hitboxes[0]) == MM_BOX_COUNT);
    for (kind = 0; kind < MM_BOX_COUNT; ++kind) {
        MmRect r = mm_samus_boxes[kind];
        assert(r.right - r.left == reference_hitboxes[kind][0] && r.bottom - r.top == reference_hitboxes[kind][1]);
    }
    for (side = 0; side < 2; ++side) {
        reset(); floor_at(30);
        mm_samus_init(&b, 100, 192); mm_tick(&world, &b, idle);
        b.facing_left = side;
        expect_samus_box(&b, MM_BOX_STAND);
        foot = b.y + b.hitbox.bottom * MM_UNIT;
        centre = b.y + (b.hitbox.top + b.hitbox.bottom) * MM_UNIT / 2;
        mm_tick(&world, &b, down);
        expect_samus_box(&b, MM_BOX_CROUCH);
        assert(b.y + b.hitbox.bottom * MM_UNIT == foot);
        assert(b.y + (b.hitbox.top + b.hitbox.bottom) * MM_UNIT / 2 == centre + 5 * MM_UNIT);
        mm_tick(&world, &b, idle); mm_tick(&world, &b, down);
        expect_samus_box(&b, MM_BOX_BALL);
        assert(b.y + b.hitbox.bottom * MM_UNIT == foot);
        for (t = 0; t < 6; ++t) mm_tick(&world, &b, idle);
        mm_tick(&world, &b, up); expect_samus_box(&b, MM_BOX_CROUCH);
        for (t = 0; t < 6; ++t) mm_tick(&world, &b, idle);
        mm_tick(&world, &b, up); expect_samus_box(&b, MM_BOX_STAND);
        assert(b.y + b.hitbox.bottom * MM_UNIT == foot);
        mm_tick(&world, &b, jump);
        for (t = 0; t < 6; ++t) mm_tick(&world, &b, (MmInput){.jump_held = 1});
        expect_samus_box(&b, MM_BOX_JUMP);
        mm_tick(&world, &b, (MmInput){.aim_y = 1, .jump_held = 1});
        expect_samus_box(&b, MM_BOX_AIR_DOWN);
        mm_tick(&world, &b, (MmInput){.jump_held = 1});
        expect_samus_box(&b, MM_BOX_JUMP);
        for (t = 0; t < 150 && !b.grounded; ++t) mm_tick(&world, &b, idle);
        expect_samus_box(&b, MM_BOX_STAND);
        assert(b.grounded && b.y + b.hitbox.bottom * MM_UNIT == foot);
        for (t = 0; t < 12; ++t) mm_tick(&world, &b, (MmInput){.axis = side ? -1 : 1});
        mm_tick(&world, &b, (MmInput){.axis = side ? -1 : 1, .jump_pressed = 1, .jump_held = 1});
        mm_tick(&world, &b, (MmInput){.jump_held = 1});
        expect_samus_box(&b, MM_BOX_SPIN);
        mm_tick(&world, &b, (MmInput){.shoot_pressed = 1, .shoot_held = 1, .jump_held = 1});
        expect_samus_box(&b, MM_BOX_JUMP);
        assert(!mm_blocked(&world, &b, mm_pixel(b.x), mm_pixel(b.y)));
    }
    puts("PASS production Samus boxes match archived original radii; grounded feet, air transitions and both facings");
}

static void samus_jump_measure(void) {
    MmBody b;
    int ground, apex, air = 1;
    reset(); floor_at(30);
    mm_samus_init(&b, 100, 192); mm_tick(&world, &b, idle);
    ground = apex = b.y;
    mm_tick(&world, &b, (MmInput){.jump_pressed = 1, .jump_held = 1});
    while (!b.grounded && air < 300) {
        mm_tick(&world, &b, (MmInput){.jump_held = 1});
        air += !b.grounded;
        if (b.y < apex) apex = b.y;
    }
    printf("Production Samus jump: apex %d px, %d ticks airborne\n", (ground - apex) / MM_UNIT, air);
    assert((ground - apex) / MM_UNIT == 111 && air == 92 && b.y == ground);
}

static void samus_tunnels(void) {
    MmBody b;
    MmInput down = {.aim_y = 1, .down_pressed = 1}, up = {.up_pressed = 1};
    int gap, x, y, t;
    for (gap = 2; gap <= 4; gap += 2) {
        reset(); floor_at(30);
        for (y = 0; y < 30 - gap; ++y) for (x = 10; x < 30; ++x) tiles[y * 64 + x] = 0;
        mm_samus_init(&b, 40, 192); mm_tick(&world, &b, idle);
        mm_tick(&world, &b, down); mm_tick(&world, &b, idle); mm_tick(&world, &b, down);
        for (t = 0; t < 25; ++t) mm_tick(&world, &b, (MmInput){.axis = 1});
        assert(b.ball && mm_pixel(b.x) > 90);
        mm_tick(&world, &b, up);
        assert(b.ball == (gap == 2));
        if (gap == 4) {
            expect_samus_box(&b, MM_BOX_CROUCH);
            for (t = 0; t < 6; ++t) mm_tick(&world, &b, idle);
            mm_tick(&world, &b, up);
            assert(b.crouching);
            mm_tick(&world, &b, (MmInput){.jump_pressed = 1, .jump_held = 1});
            assert(b.grounded && !b.jumped);
        }
        assert(!mm_blocked(&world, &b, mm_pixel(b.x), mm_pixel(b.y)));
    }
    /* Unspinning under a ceiling defers growth instead of entering solid tiles. */
    reset(); floor_at(30);
    tiles[13 * 64 + 14] = tiles[13 * 64 + 15] = 0;
    mm_samus_init(&b, 100, 100);
    b.hitbox = mm_samus_boxes[MM_BOX_SPIN]; b.spin_jumping = 1; b.pose = MM_POSE_SPIN;
    mm_tick(&world, &b, (MmInput){.shoot_pressed = 1, .shoot_held = 1});
    expect_samus_box(&b, MM_BOX_SPIN);
    assert(!mm_blocked(&world, &b, mm_pixel(b.x), mm_pixel(b.y)));
    for (t = 0; t < 30; ++t) {
        mm_tick(&world, &b, (MmInput){.axis = 1});
        assert(!mm_blocked(&world, &b, mm_pixel(b.x), mm_pixel(b.y)));
    }
    expect_samus_box(&b, b.grounded ? MM_BOX_STAND : MM_BOX_JUMP);
    puts("PASS production Samus: 14px ball tunnels, 32px crouch clearance, no clipping on blocked air expansion");
}

static void diagonal_controls(void)
{
    MmBody b;
    int side, down, crouch;
    reset(); floor_at(20);
    for (side = 0; side < 2; ++side) for (down = 0; down < 2; ++down)
        for (crouch = 0; crouch < 2; ++crouch) {
            mm_samus_init(&b, 100, 112);
            mm_tick(&world, &b, idle);
            b.facing_left = side;
            if (crouch) mm_tick(&world, &b, (MmInput){.aim_y = 1, .down_pressed = 1});
            mm_tick(&world, &b, (MmInput){.aim_diagonal = down ? 1 : -1});
            assert(b.aim == (down ? MM_AIM_DIAG_DOWN : MM_AIM_DIAG_UP));
            assert(b.crouching == crouch && !b.ball && !b.vx && b.facing_left == side);
            mm_tick(&world, &b, idle);
            assert(b.aim == MM_AIM_FORWARD && b.crouching == crouch);
        }
    puts("PASS six-button diagonal aim: both facings, stand/crouch, release without movement");
}

int main(void) {
    diagonal_controls();
    samus_jump_measure();
    samus_slope_sweep();
    samus_hitboxes(); samus_tunnels();
    measured_postures();
    turn_animation();
    normal_jump_animation();
    slope_spin_takeoff();
    beam_impacts();
    measured_beams();
    measured_missiles();
    missile_loadout();
    movement(); air_steering(); collisions(); jump_and_ledge(); states(); physics(); variable_jump(); ledge_jump();
    slopes(); slope_sweep(); tunnels(); wall_climb(); wall_jump_rules(); poses(); crouch_poses(); crouch_tap(); double_tap(); ball_poses(); aiming(); beam_from_muzzle(); morph_rules(); ball_stands_on_slope();
    ball_stand_real(); camera_and_input(); doors(); world_map(); door_locks(); real_levels();
    puts("All core regression tests passed."); return 0;
}
