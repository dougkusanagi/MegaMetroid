#include <genesis.h>
#include <gfx.h>
#include <sprite.h>

#define MAP_WIDTH 2304
#define MAP_HEIGHT 1280

#define SCREEN_LIMIT_W_L    0          //limites pantalla para el sprite
#define SCREEN_LIMIT_W_R    MAP_WIDTH - 320          //limites pantalla para el sprite
#define SCREEN_LIMIT_H_T    0          //limites pantalla para el sprite
#define SCREEN_LIMIT_H_D    MAP_HEIGHT - 224          //limites pantalla para el sprite

#define ANIM_STAND          0
#define ANIM_WALK           1
#define ANIM_JUMP           2

// Sonic Animations
// #define ANIM_STAND          0
// #define ANIM_WALK           2
// #define ANIM_JUMP           7

#define JUMP_SPEED_MIN      FIX32(4L)
#define JUMP_SPEED_MAX      FIX32(22L)
#define JUMP_SPEED_DEFAULT  FIX32(5.7L)

#define GRAVITY_MIN         FIX32(0.15)
#define GRAVITY_MAX         FIX32(0.8)
#define GRAVITY_DEFAULT     FIX32(0.15)

#define MIN_POSX            FIX32(0)
#define MAX_POSX            FIX32(MAP_WIDTH - 100)
#define MAX_POSY            FIX32(MAP_HEIGHT - 94)
// #define MAX_POSY            FIX32(MAP_HEIGHT - 124)

// Sonic max position Y
// #define MAX_POSY            FIX32(MAP_HEIGHT - 88)

Sprite* player;

Map *bga;
Map *bgb;

s16 camera_offset_x = 0;
s16 camera_offset_y = SCREEN_LIMIT_H_D;

fix32 player_pos_x;
fix32 player_pos_y;

fix32 player_vel_x;
fix32 player_vel_y;

fix32 player_speed_x = FIX32(2.3);
fix32 player_speed_y = FIX32(2.3);

fix32 player_gravity_min = FIX32(1);
fix32 player_gravity_max = FIX32(3);

fix32 player_jump_max = FIX32(22L);

s16 player_input_x = 0;
s16 player_input_y = 0;

bool player_is_moving;
bool player_is_on_floor;

int player_flipped = 0;

bool paused = FALSE;

int dump = 0;
char str_dump[8] = "0";

// Forward functions
static void handleInput(u16 joy, u16 changed, u16 state);
static void setCameraPosition(s16 x, s16 y);
static void setSpritePosition(Sprite* sprite, s16 x, s16 y);
static void updateCamera();
static void updatePhysic();
static void updateAnim();

int main(){
    SPR_init();

    JOY_init();
    JOY_setEventHandler(&handleInput);

    VDP_setScreenWidth256();

    u16 ind = TILE_USERINDEX;
    u16 numTile;
	
    VDP_setPalette(PAL0, bga_palette.data);
    VDP_loadTileSet(&bga_tileset, TILE_USERINDEX, DMA);
    bga = MAP_create(&bga_map, BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind));
    ind += bga_map.tileset->numTile;

    // VDP_setPalette(PAL1, bgb_palette.data);
    // VDP_loadTileSet(&bgb_tileset, ind, DMA);
    // bgb = MAP_create(&bgb_map, BG_B, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, ind));
    // ind += bgb_map.tileset->numTile;

    camera_offset_x = -1;
    camera_offset_y = -1;

    // player_pos_x = FIX32(160);
    player_pos_y = MAX_POSY;
    player_pos_x = FIX32(0);
    // player_pos_y = FIX32(0);

    VDP_setPalette(PAL2, imgsamus.palette->data);
    player = SPR_addSprite(
        &imgsamus,
        fix32ToInt(player_pos_x) - camera_offset_x,
        fix32ToInt(player_pos_y) - camera_offset_y,
        TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
        
    ind += numTile;
    
	SPR_setAnim(player, ANIM_STAND);
	
    while(1){
        if (!paused)
        {
            // update internal sprite position
            updatePhysic();
            updateAnim();
        }

        SPR_update();

        SYS_doVBlankProcess();
    }
	
    return (0);
}


static void handleInput(u16 joy, u16 changed, u16 state){
    if (changed & state & BUTTON_START){
        paused = !paused;
    }

    if (!paused){
        if (state & BUTTON_RIGHT){
            player_input_x = 1;
        }
        else if (state & BUTTON_LEFT){
            player_input_x = -1;
        }
        else{
            if( (changed & BUTTON_RIGHT) | (changed & BUTTON_LEFT) ){
                player_input_x = 0;
            }
        }

        if (state & BUTTON_UP){
            player_input_y = -1;
        }
        else if (state & BUTTON_DOWN){
            player_input_y = 1;
        }
        else{
            if( (changed & BUTTON_UP) | (changed & BUTTON_DOWN) ){
                player_input_y = 0;
            }
        }

        if (state & BUTTON_C){
            if (player_vel_y == 0){
                player_vel_y = -JUMP_SPEED_DEFAULT;
            }
        }
    }
}

static void updateAnim(){
    // jumping
    if (player_vel_y){
        SPR_setAnim(player, ANIM_JUMP);
        
        if (player_vel_x > 0){
            SPR_setHFlip(player, FALSE);
        }
        else if (player_vel_x < 0){
            SPR_setHFlip(player, TRUE);
        }
    }else{
        if (player_vel_x > 0){
            SPR_setAnim(player, ANIM_WALK);
            SPR_setHFlip(player, FALSE);
        }
        else if (player_vel_x < 0){
            SPR_setAnim(player, ANIM_WALK);
            SPR_setHFlip(player, TRUE);
        }
        else{
            SPR_setAnim(player, ANIM_STAND);
        }
    }


}

static void updatePhysic(){
    if (player_input_x > 0){
        player_vel_x = player_speed_x;
    }
    else if (player_input_x < 0){
        player_vel_x = -player_speed_x;
    }
    else{
        player_vel_x = 0;
    }

    // if (player_input_y > 0){
    //     player_vel_y = player_speed_y;
    // }
    // else if (player_input_y < 0){
    //     player_vel_y = -player_speed_y;
    // }
    // else{
    //     player_vel_y = 0;
    // }

    // if (player_input_y > 0){
    //     player_vel_y = player_speed_y;
    // }
    // else if (player_input_y < 0){
    //     player_vel_y = -player_speed_y;
    // }
    // else{
    //     player_vel_y = 0;
    // }

    player_pos_x += player_vel_x;
    player_pos_y += player_vel_y;

    if (player_pos_x >= MAX_POSX){
        player_pos_x = MAX_POSX;
        player_vel_x = 0;
    }
    else if (player_pos_x <= MIN_POSX){
        player_pos_x = MIN_POSX;
        player_vel_x = 0;
    }

    if (player_vel_y){
        if (player_pos_y > MAX_POSY) {
            player_pos_y = MAX_POSY;
            player_vel_y = 0;
        }
        else{
            player_vel_y += GRAVITY_DEFAULT;
        }
    }

    updateCamera();
    
    setSpritePosition(
        player,
        fix32ToInt(player_pos_x) - camera_offset_x,
        fix32ToInt(player_pos_y) - camera_offset_y);
}

static void setSpritePosition(Sprite* sprite, s16 x, s16 y){
    // clip out of screen sprites
    if ((x < -100) || (x > 320) || (y < -100) || (y > 240)) SPR_setVisibility(sprite, HIDDEN);
    else
    {
        SPR_setVisibility(sprite, VISIBLE);
        SPR_setPosition(sprite, x, y);
    }
}

static void updateCamera(){
    // get player position (pixel)
    s16 px = fix32ToInt(player_pos_x);
    s16 py = fix32ToInt(player_pos_y);

    // current sprite position on screen
    s16 px_scr = px - camera_offset_x;
    s16 py_scr = py - camera_offset_y;

    s16 np_cam_x, np_cam_y;

    // adjust new camera position
    // if (px_scr > 240) np_cam_x = px - 240;
    // else if (px_scr < 40) np_cam_x = px - 40;
    // else np_cam_x = camera_offset_x;
    
    // if (py_scr > 140) np_cam_y = py - 140;
    // else if (py_scr < 60) np_cam_y = py - 60;
    // else np_cam_y = camera_offset_y;

    // centralized Camera
    // if(px_scr > 148) np_cam_x = px - 148;
    // else if(px_scr < 148) np_cam_x = px - 148;

    if(player_input_x > 0){
        if(px_scr > 92) np_cam_x += 4;
        else if(px_scr >= 88 && px_scr <= 92) np_cam_x = px - 88;
    }
    else if(player_input_x < 0){
        if(px_scr < 140) np_cam_x -= 4;
        else if(px_scr <= 144 && px_scr >= 140) np_cam_x = px - 144;
    }else{
        if (px_scr > 200) np_cam_x = px - 200;
        else if (px_scr < 80) np_cam_x = px - 80;
    }
    
    if (py_scr > 110) np_cam_y = py - 110;
    else if (py_scr < 90) np_cam_y = py - 90;

    // clip camera position
    if (np_cam_x < 0) np_cam_x = 0;
    else if (np_cam_x > (MAP_WIDTH - 320)) np_cam_x = (MAP_WIDTH - 320);
    if (np_cam_y < 0) np_cam_y = 0;
    else if (np_cam_y > (MAP_HEIGHT - 224)) np_cam_y = (MAP_HEIGHT - 224);

    char *str = "cam_y=";
    KLog_S1(str, np_cam_y);

    str = "player_y=";
    KLog_S1(str, py);

    // set new camera position
    setCameraPosition(np_cam_x, np_cam_y);
}

static void setCameraPosition(s16 x, s16 y){
    if ((x != camera_offset_x) || (y != camera_offset_y)){
        camera_offset_x = x;
        camera_offset_y = y;

        // scroll maps
        MAP_scrollTo(bga, x, y);
        // scrolling is slower on BGB
        // MAP_scrollTo(bgb, x >> 3, y >> 5);
        // MAP_scrollTo(bgb, 0, 0);
    }
}
