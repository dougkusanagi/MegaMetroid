#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "core/world.h"
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
static void movement(void) {
    MmBody left, right;
    int i, origin = 256 * MM_UNIT;
    reset(); floor_at(30);
    mm_body_init(&left, 256, 224, box); right = left;
    for (i = 0; i < 80; ++i) {
        MmInput l = {.axis = -1}, r = {.axis = 1};
        mm_tick(&world, &left, l); mm_tick(&world, &right, r);
        assert(origin - left.x == right.x - origin);
        assert(right.x - origin == (i + 1) * MM_SPEED);
    }
    {
        MmBody b;
        MmInput l = {.axis = -1}, r = {.axis = 1};
        mm_body_init(&b,256,224,box);
        for(i=0;i<50;++i) { mm_tick(&world,&b,r); mm_tick(&world,&b,idle); }
        for(i=0;i<50;++i) { mm_tick(&world,&b,l); mm_tick(&world,&b,idle); }
        assert(b.x == origin);
    }
    assert(mm_pixel(-1) == -1 && mm_pixel(-256) == -1);
    puts("PASS symmetric subpixel movement (80 ticks)");
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
    mm_tick(&world, &b, jump); assert(b.jumped && !b.grounded && b.vy < 0);
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
    for (i=0;i<48;++i) mm_tick(&world,&b,left);
    assert(mm_pixel(b.x)<80 && mm_pixel(b.y)==184 && b.grounded);
    /* Mirror the entire terrain and traverse the opposite slope type. */
    for (y=0;y<32;++y) for(x=0;x<32;++x) {
        unsigned char a=tiles[y*64+x], z=tiles[y*64+63-x];
        tiles[y*64+x]=z==2?3:z; tiles[y*64+63-x]=a==2?3:a;
    }
    mm_body_init(&b, 436, 184, tiny); mm_tick(&world,&b,idle);
    for(i=0;i<48;++i) mm_tick(&world,&b,left);
    assert(mm_pixel(b.x)<340 && mm_pixel(b.y)==120 && b.grounded);
    for(i=0;i<48;++i) mm_tick(&world,&b,right);
    assert(mm_pixel(b.x)>430 && mm_pixel(b.y)==184 && b.grounded);
    puts("PASS both slope orientations uphill and downhill");
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
    MmInput down_press = {.aim_y = 1, .down_pressed = 1}, up_press = {.up_pressed = 1};
    reset(); floor_at(20);
    mm_body_init(&b, 80, 144, box);
    run_states(&b, log, idle, 1);
    run_states(&b, log, right, 20);
    run_states(&b, log, idle, 2);
    run_states(&b, log, jump, 1);
    run_states(&b, log, idle, 100);
    run_states(&b, log, right, 10);
    run_states(&b, log, spin, 1);
    run_states(&b, log, idle, 100);
    run_states(&b, log, down, 5);
    run_states(&b, log, idle, 2);
    run_states(&b, log, jump, 1);
    run_states(&b, log, down_press, 1);
    run_states(&b, log, down, 100);
    run_states(&b, log, up_press, 1);
    assert(strcmp(log, "STAND WALK STAND JUMP FALL STAND WALK SPIN STAND CROUCH STAND JUMP BALL STAND") == 0);
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
        mm_tick(&world, &b, hold); ++air;
        if (b.y < apex) apex = b.y;
        if (b.vy > max_fall) max_fall = b.vy;
    }
    height = (ground - apex) / MM_UNIT;
    /* Discrete ticks give 96 px; the continuous v^2/2g value, 99.6 px, overstates it. */
    assert(height >= 95 && height <= 97);
    assert(air >= 58 && air <= 62);
    assert(max_fall == MM_MAX_FALL);
    mm_body_init(&b, 80, 144, box); mm_tick(&world, &b, idle);
    for (air = 0; air < 10; ++air) mm_tick(&world, &b, right);
    x0 = b.x;
    mm_tick(&world, &b, right_jump); air = 1;
    while (!b.grounded && air < 300) { mm_tick(&world, &b, right_hold); ++air; }
    range = (b.x - x0) / MM_UNIT;
    assert(range >= 160 && range <= 170);
    printf("PASS jump measured: apex %d px, airtime %d ticks, running range %d px, fall cap %d px/tick\n",
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
    int tap = jump_height(1), full = jump_height(300);
    assert(full == 96);
    assert(tap >= 20 && tap <= 30 && tap < full / 3);
    printf("PASS variable jump: tap %d px, held %d px\n", tap, full);
}
/* A grounded jump still starts for 5 ticks after walking off a ledge, then no longer. */
static void coyote_jump(void) {
    MmBody a, c;
    MmInput press = {.jump_pressed = 1, .jump_held = 1}, right = {.axis = 1};
    int i;
    reset();
    for (i = 0; i < 12; ++i) tiles[20 * 64 + i] = 0;
    mm_body_init(&a, 80, 144, box); mm_tick(&world, &a, idle);
    for (i = 0; a.grounded && i < 100; ++i) mm_tick(&world, &a, right);
    assert(!a.grounded);
    for (i = 0; i < 4; ++i) mm_tick(&world, &a, idle);
    mm_tick(&world, &a, press);
    assert(a.jumped && a.vy < 0);
    mm_body_init(&c, 80, 144, box); mm_tick(&world, &c, idle);
    for (i = 0; c.grounded && i < 100; ++i) mm_tick(&world, &c, right);
    for (i = 0; i < 5; ++i) mm_tick(&world, &c, idle);
    mm_tick(&world, &c, press);
    assert(!c.jumped && c.vy > 0);
    puts("PASS coyote time: jump starts within 5 ticks of leaving a ledge, not after");
}
/* Walks dir for ticks, counting grounded stops and airborne frames over solid ground. */
static void sweep_walk(const MmWorld *cw, MmBody *b, int dir, int ticks, int *stalls, int *floats) {
    MmInput in = {0};
    int t;
    in.axis = dir;
    for (t = 0; t < ticks; ++t) {
        mm_tick(cw, b, in);
        if (b->grounded && b->vx == 0 && !mm_blocked(cw, b, mm_pixel(b->x) + dir, mm_pixel(b->y) - 1)) ++*stalls;
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
int main(void) {
    movement(); collisions(); jump_and_ledge(); states(); physics(); variable_jump(); coyote_jump();
    slopes(); slope_sweep(); camera_and_input(); real_levels();
    puts("All core regression tests passed."); return 0;
}
