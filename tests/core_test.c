#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "core/world.h"
#include "core/weapon.h"
#include "out/maps.h"

static unsigned char tiles[64 * 32];
static MmWorld world = {tiles, 64, 32};
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
    MmWorld maps[2] = {{&real_map_1[0][0],288,156},{&real_map_2[0][0],160,160}};
    int starts[2][2] = {{256,1176},{1168,80}};
    MmRect hitbox = {8,8,32,48};
    unsigned int seed = 12345;
    int level, frame;
    for (level=0;level<2;++level) {
        MmBody b;
        mm_body_init(&b,starts[level][0],starts[level][1],hitbox);
        assert(!mm_blocked(&maps[level],&b,mm_pixel(b.x),mm_pixel(b.y)));
        for(frame=0;frame<20000;++frame) {
            MmInput input = {0};
            seed=seed*1664525u+1013904223u;
            input.axis=(frame/180)%3-1;
            input.jump_pressed=(seed%71)==0;
            mm_tick(&maps[level],&b,input);
            assert(!mm_blocked(&maps[level],&b,mm_pixel(b.x),mm_pixel(b.y)));
            assert(b.x>=-8*MM_UNIT && b.y>=-8*MM_UNIT);
            assert(mm_pixel(b.x)+32<=maps[level].columns*8);
            assert(mm_pixel(b.y)+48<=maps[level].rows*8);
        }
    }
    puts("PASS 40000 simulation ticks on actual Crateria maps");
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
    run_states(&b, log, down, MM_TAP_TICKS + 1);
    run_states(&b, log, idle, 2);
    run_states(&b, log, jump, 1);
    run_states(&b, log, hold, 4);
    run_states(&b, log, down_press, 1);
    run_states(&b, log, hold, 3);
    run_states(&b, log, down_press, 1);
    run_states(&b, log, down, 100);
    run_states(&b, log, up_press, 1);
    assert(strcmp(log, "STAND WALK STAND JUMP FALL STAND WALK SPIN WALK STAND CROUCH STAND JUMP BALL STAND") == 0);
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
    MmWorld crateria = {&real_map_1[0][0], 288, 156};
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
    assert(!b.ball && b.state == MM_STATE_STAND);
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
    assert(b.pose == MM_POSE_LAND && hold_ticks(&b, idle, MM_POSE_LAND) == MM_LAND_TICKS);

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
    MmInput down = {.aim_y = 1}, up = {.aim_y = -1}, right = {.axis = 1}, idle_in = {0};
    /* The standing box is 48 px tall: y 112 puts its feet on the floor at y 160. */
    reset(); floor_at(20);
    mm_body_init(&b, 80, 112, real);
    strcpy(log, "GROUND");
    run_poses(&b, log, idle_in, 1);
    run_poses(&b, log, down, 20);
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
/* A tap of down keeps the crouch after release (up to MM_TAP_TICKS held); one tick more is a hold
   that stands up on release. Up, a direction or a jump ends a kept crouch, and aiming down with the
   shot is never a tap. */
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
    for (t = 1; t < MM_TAP_TICKS; ++t) mm_tick(&world, &b, down_held);
    mm_tick(&world, &b, idle_in);
    assert(b.crouching && b.crouch_latch && b.hitbox.top == 24 && b.state == MM_STATE_CROUCH);
    for (t = 0; t < 60; ++t) { mm_tick(&world, &b, idle_in); assert(b.crouching); }
    mm_tick(&world, &b, up_press);
    assert(!b.crouching && !b.crouch_latch && b.state == MM_STATE_STAND);

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    mm_tick(&world, &b, down_press);
    for (t = 1; t <= MM_TAP_TICKS; ++t) mm_tick(&world, &b, down_held);
    mm_tick(&world, &b, idle_in);
    assert(!b.crouching && !b.crouch_latch && b.state == MM_STATE_STAND);

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
    puts("PASS crouch tap: a tap keeps the crouch, one tick more is a hold; up, a direction or a jump ends it");
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
    for (t = 0; t < 30; ++t) { mm_tick(&world, &b, idle_in); assert(!b.ball && !b.crouching); }

    mm_body_init(&b, 80, 112, real); mm_tick(&world, &b, idle_in);
    /* A hold stands up on release, so a late second press finds the body standing: no morph. */
    mm_tick(&world, &b, down_press);
    for (t = 1; t < MM_DOUBLE_TAP_TICKS; ++t) mm_tick(&world, &b, (MmInput){.aim_y = 1});
    mm_tick(&world, &b, idle_in);
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
    MmWorld crateria = {&real_map_1[0][0], 288, 156};
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
/* Ball poses: curling up (also straight from a crouch), the rolling loop, uncurling. Landing as a
   ball and jumping as a ball keep the ball pose. */
static void ball_poses(void) {
    MmBody b;
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
    assert(b.ball && b.grounded && b.pose == MM_POSE_BALL);
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
    assert(strcmp(log, "GROUND CROUCH_DOWN CROUCH MORPH BALL UNMORPH GROUND") == 0);
    puts("PASS ball poses: morph (also from a crouch), rolling loop through a jump and a landing, unmorph");
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
    assert(w.fired && w.beams[0].active && w.beams[0].vy == 0 && w.beams[0].vx == MM_BEAM_SPEED);
    assert(mm_pixel(w.beams[0].x) == 80 + 33 && mm_pixel(w.beams[0].y) == 112 + 21);
    b.aim = MM_AIM_DIAG_UP; b.facing_left = 1;
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, shoot, camera, 2, 1);
    assert(w.beams[0].active && w.beams[0].vx == -MM_BEAM_DIAGONAL_SPEED && w.beams[0].vy == -MM_BEAM_DIAGONAL_SPEED);
    b.aim = MM_AIM_DOWN;
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, shoot, camera, 10, 43);
    assert(w.beams[0].active && w.beams[0].vx == 0 && w.beams[0].vy == MM_BEAM_SPEED);
    /* A wall at x 112-119, between the body centre (x 100) and a forward muzzle (x 113). */
    for (y = 0; y < 20; ++y) tiles[y * 64 + 14] = 0;
    b.aim = MM_AIM_FORWARD; b.facing_left = 0;
    mm_weapon_init(&w);
    mm_weapon_tick(&w, &world, &b, shoot, camera, 33, 21);
    assert(!w.beams[0].active);
    puts("PASS beam leaves from the muzzle along the aim, never from inside a wall");
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
    for (t = 0; t < 2; ++t) mm_weapon_tick(&w, &world, &b, idle, camera, 33, 21);
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
    puts("PASS beam impact at swept wall contact, expires after 10 ticks; no impact off screen");
}
int main(void) {
    normal_jump_animation();
    slope_spin_takeoff();
    beam_impacts();
    movement(); air_steering(); collisions(); jump_and_ledge(); states(); physics(); variable_jump(); ledge_jump();
    slopes(); slope_sweep(); tunnels(); wall_climb(); wall_jump_rules(); poses(); crouch_poses(); crouch_tap(); double_tap(); ball_poses(); aiming(); beam_from_muzzle(); morph_rules(); ball_stands_on_slope();
    ball_stand_real(); camera_and_input(); real_levels();
    puts("All core regression tests passed."); return 0;
}
