#include <genesis.h>

#include "../inc/types.h"
#include "../inc/map.h"
#include "../inc/entity.h"
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

Entity player;
Camera camera;
Map *current_map;
Map *current_map_bg;

u16 VDPTilesFilled = TILE_USER_INDEX;

// Forwarding functions
static void levelInit();

static void cameraInit();
static void updateCamera();

static void playerInit();
static void playerUpdate();
static void playerUpdateAnimation();
static void playerApplyGravity();
static s16 playerGetFeetPosition();

static void checkTileCollisions();
static void setCameraPosition(s16 x, s16 y);
static void handleInput(u16 joy, u16 changed, u16 state);

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
        playerUpdate();
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

    MAP_scrollTo(current_map, camera.position.x, camera.position.y);
}

static void levelInit()
{
    PAL_setPalette(LEVEL_PALETTE, level_palette.data, DMA);
    VDP_loadTileSet(&level_tileset, VDPTilesFilled, DMA);
    current_map = MAP_create(&level_map, TILEMAP_PLANE, TILE_ATTR_FULL(LEVEL_PALETTE, FALSE, FALSE, FALSE, VDPTilesFilled));

    // Update the number of tiles filled in order to avoid overlaping them when loading more
    VDPTilesFilled += level_tileset.numTile;

    PAL_setPalette(BG_PALETTE, bg_palette.data, DMA);
    VDP_loadTileSet(&bg_tileset, VDPTilesFilled, DMA);
    current_map_bg = MAP_create(&bg_map, BACKGROUND_PLANE, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, VDPTilesFilled));

    // Update the number of tiles filled in order to avoid overlaping them when loading more
    VDPTilesFilled += bg_tileset.numTile;

    cameraInit();
}

static void playerInit()
{
    player.is_on_floor = FALSE;

    // calculated in tiles
    player.tile_width = 5;
    player.tile_height = 6;

    // calculated in tiles
    player.tile_collision_width = 3;
    player.tile_collision_height = 5;

    player.velocity.x = 0;
    player.velocity.y = 0;

    // initial player position
    ENTITY_setPosition(
        &player,
        tileToPixel(32),
        MAP_HEIGHT - tileToPixel(player.tile_height) - 8);

	//Setup the jump SFX with an index between 64 and 255
    SND_setPCM_XGM(64, jump_sfx, sizeof(jump_sfx));

    PAL_setPalette(PLAYER_PALETTE, player_sprite.palette->data, DMA);

    player.sprite = SPR_addSprite(&player_sprite,
        player.position.x - camera.position.x,
        player.position.y - camera.position.y,
        TILE_ATTR(PLAYER_PALETTE, TRUE, FALSE, FALSE));

    ENTITY_setAnimation(&player, ANIM_STAND);
}


static void playerApplyGravity()

{
    if (player.velocity.y < GRAVITY_MAX)
    {
        player.velocity.y += GRAVITY;
    }
}

static void playerUpdate()
{
    playerApplyGravity();
    // Only after check collision with tiles

    if (control.d_pad.x > 0)
    {
        player.velocity.x = FIX16(2.3);
    }
    else if (control.d_pad.x < 0)
    {
        player.velocity.x = -FIX16(2.3);
    }
    else
    {
        player.velocity.x = 0;
    }

    if (control.d_pad.y > 0)
    {
        player.velocity.y = FIX16(2.3);
    }
    else if (control.d_pad.y < 0)
    {
        player.velocity.y = -FIX16(2.3);
    }

    // KLog_S1("player.velocity.y = ", player.velocity.y);

    checkTileCollisions();

    ENTITY_moveSprite(&player,
        player.position.x - camera.position.x,
        player.position.y - camera.position.y);

    // Update the player animations
    playerUpdateAnimation();

    updateCamera();
}

static bool checkHorizontalTile(s16 tile_y)
{
    return tile_y + 8 > player.position.y && tile_y < player.position.y + tileToPixel(player.tile_height);
}

static bool checkVerticalTile(s16 tile_x)
{
    return tile_x + 8 > player.position.x && tile_x < player.position.x + tileToPixel(player.tile_width);
}

static void checkTileCollisions()
{
    if (!player.velocity.x && !player.velocity.y)
        return;
    
    ENTITY_setPosition(
        &player,
        player.position.x + fix16ToInt(player.velocity.x),
        player.position.y + fix16ToInt(player.velocity.y)
    );

    s16 player_x_tile = pixelToTile(player.position.x);
    s16 player_y_tile = pixelToTile(player.position.y);

    s16 player_left_tile = player_x_tile;
    s16 player_right_tile = player_x_tile + player.tile_width;
    s16 player_top_tile = player_y_tile;
    s16 player_bottom_tile = player_y_tile + player.tile_height;

    s16 player_right = player.position.x + tileToPixel(player.tile_width);

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
                            player.position.x = (tileToPixel(x)) - tileToPixel(player.tile_width);
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
                        if (playerGetFeetPosition() > tileToPixel(y))
                        {
                            player.position.y = tileToPixel(y) - tileToPixel(player.tile_height);
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

static void playerUpdateAnimation()
{
    // jumping
    if (!player.is_on_floor)
    {
        ENTITY_setAnimation(&player, ANIM_JUMP);

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
            ENTITY_setAnimation(&player, ANIM_WALK);
            SPR_setHFlip(player.sprite, FALSE);
        }
        else if (player.velocity.x < 0)
        {
            ENTITY_setAnimation(&player, ANIM_WALK);
            SPR_setHFlip(player.sprite, TRUE);
        }
        else
        {
            ENTITY_setAnimation(&player, ANIM_STAND);
        }
    }
}

static void updateCamera()
{
    s16 new_camera_position_x;
    s16 new_camera_position_y;

    // w >> 1 = w / 2
    new_camera_position_x = player.position.x - (SCREEN_WIDTH >> 1) + (tileToPixel(player.tile_width) >> 1);
    new_camera_position_y = player.position.y - (SCREEN_HEIGHT >> 1) + (tileToPixel(player.tile_height) >> 1);

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
        MAP_scrollTo(current_map, x, y);

        // scroll maps
        MAP_scrollTo(current_map_bg, 0, 0);
    }
}

static void handleInput(u16 joy, u16 changed, u16 state)
{
    if (joy == JOY_1)
    {
        // Update x velocity
        if (state & BUTTON_RIGHT)
        {
            control.d_pad.x = 1;
        }
        else if (state & BUTTON_LEFT)
        {
            control.d_pad.x = -1;
        }
        else if ((changed & BUTTON_RIGHT) | (changed & BUTTON_LEFT))
        {
            control.d_pad.x = 0;
        }

        // Update x velocity
        if (state & BUTTON_DOWN)
        {
            control.d_pad.y = 1;
        }
        else if (state & BUTTON_UP)
        {
            control.d_pad.y = -1;
        }
        else if ((changed & BUTTON_DOWN) | (changed & BUTTON_UP))
        {
            control.d_pad.y = 0;
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

static s16 playerGetFeetPosition()
{
    return player.position.y + tileToPixel(player.tile_height);
}
