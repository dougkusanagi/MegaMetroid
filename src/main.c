#include <genesis.h>

#include "../inc/types.h"
#include "../inc/map.h"
#include "../inc/physics.h"
#include "../res/resources.h"

#define SCREEN_WIDTH        256
#define SCREEN_HEIGHT       224

#define ANIM_STAND          0
#define ANIM_WALK           1
#define ANIM_JUMP           2

#define GRAVITY             FIX16(0.22)
#define GRAVITY_MAX         300
#define JUMP                FIX16(6.6L)

#define TILEMAP_PLANE       BG_A
#define BACKGROUND_PLANE    BG_B
#define LEVEL_PALETTE       PAL0
#define BG_PALETTE          PAL1
#define PLAYER_PALETTE      PAL2

Player player;
Camera camera;
Map *map;
Map *map_bg;

u16 VDPTilesFilled = TILE_USER_INDEX;
bool active_gravity = TRUE;

// Forwarding functions
static void cameraInit();
static void levelInit();
static void playerInit();
static void setPlayerAnimation(u16 animation);
static void updatePlayer();
static void updatePlayerAnim();
static void checkTileCollisions();
static void setCameraPosition(s16 x, s16 y);
static void updateCamera();
static void handleInput(u16 joy, u16 changed, u16 state);
static void applyGravity();
static s16 getPlayerFeetPosition();

int main(bool resetType)
{
    VDP_setScreenWidth256();
    SPR_init();
    JOY_init();
    JOY_setEventHandler(&handleInput);

    if (!resetType)
        SYS_hardReset();

    // need to increase a bit DMA buffer size to init both plan tilemap and sprites
    DMA_setBufferSize(13000);
    DMA_setMaxTransferSize(13000);
    // DMA_setBufferSize(9000);

    // MEM_getAllocated();
    // MEM_getFree();
    KLog_S1("Allocated Memory = ", MEM_getAllocated());
    KLog_S1("Available Memory = ", MEM_getFree());

    DMA_setMaxQueueSize(120);

    playerInit();
    levelInit();

    while (1)
    {
        updatePlayer();
        SPR_update();
        SYS_doVBlankProcess();
    }

    return (0);
}

static void cameraInit()
{
    // camera position (force refresh)
    camera.position.x = -1;
    camera.position.y = -1;

    MAP_scrollTo(map, camera.position.x, camera.position.y);
}

static void levelInit()
{
    PAL_setPalette(LEVEL_PALETTE, level_palette.data, DMA);
    VDP_loadTileSet(&level_tileset, VDPTilesFilled, DMA);
    map = MAP_create(&level_map, TILEMAP_PLANE, TILE_ATTR_FULL(LEVEL_PALETTE, FALSE, FALSE, FALSE, VDPTilesFilled));

    // Update the number of tiles filled in order to avoid overlaping them when loading more
    VDPTilesFilled += level_tileset.numTile;

    PAL_setPalette(BG_PALETTE, bg_palette.data, DMA);
    VDP_loadTileSet(&bg_tileset, VDPTilesFilled, DMA);
    map_bg = MAP_create(&bg_map, BACKGROUND_PLANE, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, VDPTilesFilled));

    // Update the number of tiles filled in order to avoid overlaping them when loading more
    VDPTilesFilled += bg_tileset.numTile;

    cameraInit();
}

static void playerInit()
{
    // var * 8
    player.tile_width = 5;
    player.tile_height = 6;

    player.pixel_width = 5 << 3;
    player.pixel_height = 6 << 3;

    // initial player position
    player.position.x = 0;
    player.position.y = MAP_HEIGHT - (12 << 3) - player.pixel_height;

    player.velocity.x = 0;
    player.velocity.y = 0;

    player.is_on_floor = FALSE;

	//Setup the jump SFX with an index between 64 and 255
	// XGM_setPCM(64, jump_sfx, sizeof(jump_sfx));
    SND_setPCM_XGM(64, jump_sfx, sizeof(jump_sfx));

    PAL_setPalette(PLAYER_PALETTE, player_sprite.palette->data, DMA);
    player.sprite = SPR_addSprite(
        &player_sprite,
        fix32ToInt(player.position.x) - camera.position.x,
        fix32ToInt(player.position.y) - camera.position.y,
        TILE_ATTR(PLAYER_PALETTE, TRUE, FALSE, FALSE));

    setPlayerAnimation(ANIM_STAND);
}

static void setPlayerAnimation(u16 animation)
{
    SPR_setAnim(player.sprite, animation);
    player.current_animation = animation;
}

static void applyGravity()
{
    if (player.velocity.y < GRAVITY_MAX)
    {
        player.velocity.y += GRAVITY;
    }
}

static void updatePlayer()
{
    // Only after check collision with tiles
    applyGravity();

    if (player.control.d_pad.x > 0)
    {
        player.velocity.x = FIX16(2.3);
    }
    else if (player.control.d_pad.x < 0)
    {
        player.velocity.x = -FIX16(2.3);
    }
    else
    {
        player.velocity.x = 0;
    }

    if (player.control.d_pad.y > 0)
    {
        player.velocity.y = FIX16(2.3);
    }
    else if (player.control.d_pad.y < 0)
    {
        player.velocity.y = -FIX16(2.3);
    }

    // KLog_S1("player.velocity.y = ", player.velocity.y);

    checkTileCollisions();

    SPR_setPosition(player.sprite,
                    player.position.x - camera.position.x,
                    player.position.y - camera.position.y);

    // Update the player animations
    updatePlayerAnim();

    updateCamera();
}

static bool checkHorizontalTile(s16 tile_y)
{
    return tile_y + 8 > player.position.y && tile_y < player.position.y + player.pixel_height;
}

static bool checkVerticalTile(s16 tile_x)
{
    return tile_x + 8 > player.position.x && tile_x < player.position.x + player.pixel_width;
}

static void checkTileCollisions()
{
    if (!player.velocity.x && !player.velocity.y)
        return;

    s16 player_x_tile = pixelToTile(player.position.x + fix16ToInt(player.velocity.x));
    s16 player_y_tile = pixelToTile(player.position.y + fix16ToInt(player.velocity.y));

    s16 player_left_tile = player_x_tile;
    s16 player_right_tile = player_x_tile + player.tile_width;
    s16 player_top_tile = player_y_tile;
    s16 player_bottom_tile = player_y_tile + player.tile_height;

    s16 player_right = player.position.x + player.pixel_width;

    // active_gravity = FALSE;

    player.position.x += fix16ToInt(player.velocity.x);
    player.position.y += fix16ToInt(player.velocity.y);

    // s16 player_feet_pos = getPlayerFeetPosition();
    // u16 tile_value = MAP_getTile(map, fix32ToInt(player.position.x) >> 3, player_feet_pos >> 3);

    // if (tile_value == TILE_SLOPE)
    // {
    //     u16 left_tile_value = MAP_getTile(map, (fix32ToInt(player.position.x) >> 3) - 1, player_feet_pos >> 3);
    //     u16 right_tile_value = MAP_getTile(map, (fix32ToInt(player.position.x) >> 3) + 1, player_feet_pos >> 3);

    //     fix16 slope = fix16Div(FIX16(1), fix16FromInt(right_tile_value - left_tile_value));

    //     player.position.y = fix32Add(player.position.y, fix16Mul(fix16Sub(fix16FromInt(right_tile_value), fix16FromInt(tile_value)), slope));
    // }

    for (s16 x = player_left_tile; x <= player_right_tile; x++)
    {
        for (s16 y = player_top_tile; y <= player_bottom_tile; y++)
        {
            if (map_collision[y][x] == 0)
            {
                if (checkHorizontalTile(tileToPixel(y + fix16ToInt(player.velocity.y))))
                {
                    if (player.velocity.x > 0)
                    {
                        if (player_right >= tileToPixel(x))
                        {
                            player.velocity.x = 0;
                            player.position.x = (tileToPixel(x)) - player.pixel_width;
                        }
                    }
                    else if (player.velocity.x < 0)
                    {
                        if (player.position.x <= (tileToPixel(x)) + 8)
                        {
                            player.velocity.x = 0;
                            player.position.x = (tileToPixel(x)) + 8;
                        }
                    }
                }

                if (checkVerticalTile(tileToPixel(x + fix16ToInt(player.velocity.x))))
                {
                    if (player.velocity.y > 0)
                    {
                        if (getPlayerFeetPosition() > tileToPixel(y))
                        {
                            player.position.y = tileToPixel(y) - player.pixel_height;
                            player.velocity.y = 0;
                            player.is_on_floor = FALSE;
                        }
                    }
                    else if (player.velocity.y < 0)
                    {
                        if (player.position.y < tileToPixel(y) + 8)
                        {
                            player.position.y = tileToPixel(y) + 8;
                            player.velocity.y = 0;
                            player.is_on_floor = FALSE;
                        }
                    }
                    else
                    {
                        player.is_on_floor = TRUE;
                    }
                }
            }
        }
    }
}

static void playSoundJump()
{
    if (!SND_isPlayingPCM_XGM(SOUND_PCM_CH2_MSK)) SND_startPlayPCM_XGM(64, 15, SOUND_PCM_CH2);
}

static void stopSoundJump()
{
    if (SND_isPlayingPCM_XGM(SOUND_PCM_CH2_MSK)) SND_stopPlayPCM_XGM(SOUND_PCM_CH2);
}

static void updatePlayerAnim()
{
    // jumping
    if (!player.is_on_floor)
    {
        setPlayerAnimation(ANIM_JUMP);

        playSoundJump();

        if (player.velocity.x > 0)
        {
            SPR_setHFlip(player.sprite, FALSE);
        }
        else if (player.velocity.x < 0)
        {
            SPR_setHFlip(player.sprite, TRUE);
        }
    }
    else
    {
        stopSoundJump();

        if (player.velocity.x > 0)
        {
            setPlayerAnimation(ANIM_WALK);
            SPR_setHFlip(player.sprite, FALSE);
        }
        else if (player.velocity.x < 0)
        {
            setPlayerAnimation(ANIM_WALK);
            SPR_setHFlip(player.sprite, TRUE);
        }
        else
        {
            setPlayerAnimation(ANIM_STAND);
        }
    }
}

static void updateCamera()
{
    s16 new_camera_position_x;
    s16 new_camera_position_y;

    // w >> 1 = w / 2
    new_camera_position_x = player.position.x - (SCREEN_WIDTH >> 1) + (player.pixel_width >> 1);
    new_camera_position_y = player.position.y - (SCREEN_HEIGHT >> 1) + (player.pixel_height >> 1);

    // Limit camera to screen size
    if (new_camera_position_x < 0)
    {
        new_camera_position_x = 0;
    }
    else if (new_camera_position_x > (MAP_WIDTH - SCREEN_WIDTH))
    {
        new_camera_position_x = (MAP_WIDTH - SCREEN_WIDTH);
    }

    if (new_camera_position_y < 0)
    {
        new_camera_position_y = 0;
    }
    else if (new_camera_position_y > (MAP_HEIGHT - SCREEN_HEIGHT))
    {
        new_camera_position_y = (MAP_HEIGHT - SCREEN_HEIGHT);
    }

    // KLog_S1("new_camera_position_y=", new_camera_position_y);

    // set new camera position
    setCameraPosition(new_camera_position_x, new_camera_position_y);
}

static void setCameraPosition(s16 x, s16 y)
{
    if ((x != camera.position.x) || (y != camera.position.y))
    {
        camera.position.x = x;
        camera.position.y = y;

        // scroll maps
        MAP_scrollTo(map, x, y);

        // scroll maps
        MAP_scrollTo(map_bg, 0, 0);
    }
}

static void handleInput(u16 joy, u16 changed, u16 state)
{
    if (joy == JOY_1)
    {
        // Update x velocity
        if (state & BUTTON_RIGHT)
        {
            player.control.d_pad.x = 1;
        }
        else if (state & BUTTON_LEFT)
        {
            player.control.d_pad.x = -1;
        }
        else if ((changed & BUTTON_RIGHT) | (changed & BUTTON_LEFT))
        {
            player.control.d_pad.x = 0;
        }

        // Update x velocity
        if (state & BUTTON_DOWN)
        {
            player.control.d_pad.y = 1;
        }
        else if (state & BUTTON_UP)
        {
            player.control.d_pad.y = -1;
        }
        else if ((changed & BUTTON_DOWN) | (changed & BUTTON_UP))
        {
            player.control.d_pad.y = 0;
        }

        if (state & BUTTON_C)
        {
            if (player.is_on_floor)
            {
                player.velocity.y = -JUMP;
                player.is_on_floor = FALSE;
            }
        }
    }
}

static s16 getPlayerFeetPosition()
{
    return player.position.y + player.pixel_height;
}
