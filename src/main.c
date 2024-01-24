#include <genesis.h>

#include "../inc/types.h"
#include "../inc/map.h"
#include "../inc/entity.h"
#include "../inc/physics.h"
#include "../res/resources.h"
#include "../res/crateria_1.h"
#include "../res/crateria_2.h"

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 224

#define ANIM_STAND 0
#define ANIM_WALK 1
#define ANIM_JUMP 2

#define GRAVITY FIX16(0.22)
#define GRAVITY_MAX 300
#define JUMP FIX16(6.6L)

#define TILEMAP_PLANE BG_A
#define BACKGROUND_PLANE BG_B
#define LEVEL_PALETTE PAL0
#define BG_PALETTE PAL1
#define PLAYER_PALETTE PAL2

Entity player;
Camera camera;
Map *current_map;
Map *current_map_bg;
AABB roomSize;

const level_def *all_level_defs[2];
u8 curr_level_index;

// Forwarding functions
void boot(void);
u16 levelInit(u16 vram_index);

void cameraInit();
void updateCamera();

void playerInit();
void playerUpdate();
void playerUpdateAnimation();
void playerApplyGravity();

void checkTileCollisions();
void setCameraPosition(s16 x, s16 y);
void handleInput(u16 joy, u16 changed, u16 state);

int main()
{
    boot();

    while (1)
    {
        playerUpdate();
        SPR_update();
        SYS_doVBlankProcess();
    }

    return (0);
}

void boot(void)
{
    u16 VDPTilesFilled = TILE_USER_INDEX;

    SYS_disableInts();
    VDP_setScreenWidth256();
    SPR_init();
    JOY_init();
    JOY_setEventHandler(&handleInput);

    // need to increase a bit DMA buffer size to init both plan tilemap and sprites
    DMA_setBufferSize(10000);
    DMA_setMaxTransferSize(10000);

    // MEM_getAllocated();
    // MEM_getFree();

    DMA_setMaxQueueSize(120);

    curr_level_index = 0;
    all_level_defs[curr_level_index] = &level_crateria_1;

    KLog_S1("all_level_defs[0]->map_width = ", all_level_defs[curr_level_index]->map_width);

    VDPTilesFilled += levelInit(VDPTilesFilled);
    playerInit();
    cameraInit();

    // can restore default DMA buffer size
    DMA_setBufferSizeToDefault();
    DMA_setMaxTransferSizeToDefault();
}

void cameraInit()
{
    // camera position (force refresh)
    camera.position.x = -1;
    camera.position.y = -1;

    MAP_scrollTo(current_map, camera.position.x, camera.position.y);
}

u16 levelInit(u16 vram_index)
{
    u16 index = vram_index;

    roomSize = newAABB(
        0, all_level_defs[curr_level_index]->map_width,
        0, all_level_defs[curr_level_index]->map_height);

    PAL_setPalette(LEVEL_PALETTE, all_level_defs[curr_level_index]->palette_fg->data, DMA);
    VDP_loadTileSet(all_level_defs[curr_level_index]->tileset_fg, index, DMA);
    current_map = MAP_create(all_level_defs[curr_level_index]->map_fg, TILEMAP_PLANE, TILE_ATTR_FULL(LEVEL_PALETTE, FALSE, FALSE, FALSE, index));
    index += all_level_defs[curr_level_index]->tileset_fg->numTile;

    PAL_setPalette(BG_PALETTE, all_level_defs[curr_level_index]->palette_bg->data, DMA);
    VDP_loadTileSet(all_level_defs[curr_level_index]->tileset_bg, index, DMA);
    current_map_bg = MAP_create(all_level_defs[curr_level_index]->map_bg, BACKGROUND_PLANE, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, index));
    index += all_level_defs[curr_level_index]->tileset_bg->numTile;

    return index;
}

void playerInit()
{
    player.is_on_floor = FALSE;

    // calculated in tiles
    player.tile_width = 5;
    player.tile_height = 6;

    player.collision_size = newAABB(
        8, 32, 8, 48);

    player.velocity.x = 0;
    player.velocity.y = 0;

    // initial player position
    Entity_setPosition(
        &player,
        tileToPixel(32),
        all_level_defs[curr_level_index]->map_height - tileToPixel(player.tile_height) - 24);

    // Setup the jump SFX with an index between 64 and 255
    XGM_setPCM(64, jump_sfx, sizeof(jump_sfx));

    PAL_setPalette(PLAYER_PALETTE, player_sprite.palette->data, DMA);

    player.sprite = SPR_addSprite(&player_sprite,
                                  player.position.x - camera.position.x,
                                  player.position.y - camera.position.y,
                                  TILE_ATTR(PLAYER_PALETTE, TRUE, FALSE, FALSE));

    Entity_setAnimation(&player, ANIM_STAND);
}

void playerApplyGravity()

{
    if (player.velocity.y < GRAVITY_MAX)
    {
        player.velocity.y = fix16Add(player.velocity.y, GRAVITY);
    }
}

void playerUpdate()
{
    // Only after check collision with tiles
    playerApplyGravity();

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

    Entity_setPosition(
        &player,
        player.position.x + fix16ToInt(player.velocity.x),
        player.position.y + fix16ToInt(player.velocity.y));

    checkTileCollisions();

    Entity_moveSprite(&player,
                      player.position.x - camera.position.x,
                      player.position.y - camera.position.y);

    // Update the player animations
    playerUpdateAnimation();

    updateCamera();
}

void checkTileCollisions()
{
    AABB levelLimits = roomSize;

    player.collision_position = newAABB(
        player.position.x + player.collision_size.min.x,
        player.position.x + player.collision_size.max.x,
        player.position.y + player.collision_size.min.y,
        player.position.y + player.collision_size.max.y);

    // Skin width (yIntVelocity) changes depending on the vertical velocity
    s16 yIntVelocity = fix16ToRoundedInt(player.velocity.y);
    s16 playerHeadPos = player.collision_size.min.y - yIntVelocity + player.position.y;
    s16 playerFeetPos = player.collision_size.max.y - yIntVelocity + player.position.y;

    // Positions in tiles
    Vect2D_u16 minTilePos = posToTile(newVector2D_s16(player.collision_position.min.x, player.collision_position.min.y));
    Vect2D_u16 maxTilePos = posToTile(newVector2D_s16(player.collision_position.max.x, player.collision_position.max.y));

    // Used to limit how many tiles we have to check for collision
    Vect2D_u16 tileBoundDifference = newVector2D_u16(maxTilePos.x - minTilePos.x, maxTilePos.y - minTilePos.y);

    // First we check for horizontal collisions
    for (u16 i = 0; i <= tileBoundDifference.y; i++)
    {
        // Height position constant as a helper
        const u16 y = minTilePos.y + i;

        // Right position constant as a helper
        const u16 rx = maxTilePos.x;

        // u16 rTileValue = getTileValue(rx, y);
        u16 rTileValue = map_collision[y][rx];

        // After getting the tile value, we check if that is one of whom we can collide/trigger with horizontally
        if (rTileValue == GROUND_TILE)
        {
            AABB tileBounds = getTileBounds(rx, y);

            // Before taking that tile as a wall, we have to check if that is within the player hitbox, e.g. not seeing ground as a wall
            if (tileBounds.min.x < levelLimits.max.x &&
                tileBounds.min.y < playerFeetPos &&
                tileBounds.max.y > playerHeadPos)
            {
                levelLimits.max.x = tileBounds.min.x;
                break;
            }
        }

        // Left position constant as a helper
        const s16 lx = minTilePos.x;

        // u16 lTileValue = getTileValue(lx, y);
        u16 lTileValue = map_collision[y][lx];

        // We do the same here but for the left side
        if (lTileValue == GROUND_TILE)
        {
            AABB tileBounds = getTileBounds(lx, y);

            if (tileBounds.max.x > levelLimits.min.x &&
                tileBounds.min.y < playerFeetPos &&
                tileBounds.max.y > playerHeadPos)
            {
                levelLimits.min.x = tileBounds.max.x;
                break;
            }
        }

        else if (map_collision[y][rx] == 2)
        {
            AABB tileBounds = getTileBounds(rx, y);

            s16 x_dif = (player.collision_position.max.x - tileBounds.min.x) << 1;

            if (x_dif > 8)
                x_dif = 8;

            levelLimits.max.y = tileBounds.max.y - x_dif;
        }

        else if (map_collision[y][lx] == 3)
        {
            AABB tileBounds = getTileBounds(lx, y);

            s16 x_dif = ((player.collision_position.min.x - tileBounds.max.x) << 1) * -1;

            if (x_dif > 8)
                x_dif = 8;

            KLog_S1("tileBounds.max.y = ", tileBounds.max.y);
            KLog_S1("x_dif = ", x_dif);
            KLog_S1("new_y = ", tileBounds.max.y - x_dif);

            levelLimits.max.y = tileBounds.max.y - x_dif;
        }
    }

    // After checking for horizontal positions we can modify the positions if the player is colliding
    if (levelLimits.max.x < player.collision_position.max.x)
    {
        player.position.x = levelLimits.max.x - player.collision_size.max.x;
    }
    if (levelLimits.min.x > player.collision_position.min.x)
    {
        player.position.x = levelLimits.min.x - player.collision_size.min.x;
    }

    player.collision_position = newAABB(
        player.position.x + player.collision_size.min.x,
        player.position.x + player.collision_size.max.x,
        player.position.y + player.collision_size.min.y,
        player.position.y + player.collision_size.max.y);

    // And do the same to the variables that are used to check for them
    minTilePos = posToTile(newVector2D_s16(player.collision_position.min.x, player.collision_position.min.y));
    maxTilePos = posToTile(newVector2D_s16(player.collision_position.max.x - 1, player.collision_position.max.y));

    // Used to limit how many tiles we have to check for collision
    tileBoundDifference = newVector2D_u16(maxTilePos.x - minTilePos.x, maxTilePos.y - minTilePos.y);

    if (yIntVelocity >= 0)
    {
        for (u16 i = 0; i <= tileBoundDifference.x; i++)
        {
            s16 x = minTilePos.x + i;
            u16 y = maxTilePos.y;

            // This is the exact same method that we use for horizontal collisions
            //  u16 bottomTileValue = getTileValue(x, y);
            u16 bottomTileValue = map_collision[y][x];

            if (bottomTileValue == GROUND_TILE)
            {
                if (getTileRightEdge(x) == levelLimits.min.x || getTileLeftEdge(x) == levelLimits.max.x)
                    continue;

                u16 bottomEdgePos = getTileTopEdge(y);

                // The error correction is used to add some extra width pixels in case the player isn't high enough by just some of them
                if (bottomEdgePos < levelLimits.max.y)
                {
                    levelLimits.max.y = bottomEdgePos;
                }
            }
        }
    }
    else
    {
        for (u16 i = 0; i <= tileBoundDifference.x; i++)
        {
            s16 x = minTilePos.x + i;
            u16 y = minTilePos.y;

            // And the same once again
            //  u16 topTileValue = getTileValue(x, y);
            u16 topTileValue = map_collision[y][x];

            if (topTileValue == GROUND_TILE)
            {
                if (getTileRightEdge(x) == levelLimits.min.x || getTileLeftEdge(x) == levelLimits.max.x)
                    continue;

                u16 upperEdgePos = getTileBottomEdge(y);

                if (upperEdgePos < levelLimits.max.y)
                {
                    levelLimits.min.y = upperEdgePos;
                    break;
                }
            }
        }
    }

    // Now we modify the player position and some properties if necessary
    if (levelLimits.min.y > player.collision_position.min.y)
    {
        player.position.y = levelLimits.min.y - player.collision_size.min.y;
        player.velocity.y = 0;
    }

    if (levelLimits.max.y <= player.collision_position.max.y)
    {
        if (levelLimits.max.y == roomSize.max.y)
        {
            player.is_on_floor = FALSE;
        }
        else
        {
            player.is_on_floor = TRUE;
            player.position.y = levelLimits.max.y - player.collision_size.max.y;
            player.velocity.y = 0;
        }
    }
    else
    {
        player.is_on_floor = FALSE;
    }
}

void playSoundJump()
{
    if (!XGM_isPlayingPCM(SOUND_PCM_CH2_MSK))
        XGM_startPlayPCM(64, 15, SOUND_PCM_CH2);
}

void stopSoundJump()
{
    if (XGM_isPlayingPCM(SOUND_PCM_CH2_MSK))
        stopSoundJump(SOUND_PCM_CH2);
}

void playerUpdateAnimation()
{
    // jumping
    if (!player.is_on_floor)
    {
        Entity_setAnimation(&player, ANIM_JUMP);

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
            Entity_setAnimation(&player, ANIM_WALK);
            SPR_setHFlip(player.sprite, FALSE);
        }
        else if (player.velocity.x < 0)
        {
            Entity_setAnimation(&player, ANIM_WALK);
            SPR_setHFlip(player.sprite, TRUE);
        }
        else
        {
            Entity_setAnimation(&player, ANIM_STAND);
        }
    }
}

void updateCamera()
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
    else if (new_camera_position_x > (all_level_defs[curr_level_index]->map_width - SCREEN_WIDTH))
    {
        new_camera_position_x = (all_level_defs[curr_level_index]->map_width - SCREEN_WIDTH);
    }

    if (new_camera_position_y < 0)
    {
        new_camera_position_y = 0;
    }
    else if (new_camera_position_y > (all_level_defs[curr_level_index]->map_height - SCREEN_HEIGHT))
    {
        new_camera_position_y = (all_level_defs[curr_level_index]->map_height - SCREEN_HEIGHT);
    }

    // KLog_S1("new_camera_position_y=", new_camera_position_y);

    // set new camera position
    setCameraPosition(new_camera_position_x, new_camera_position_y);
}

void setCameraPosition(s16 x, s16 y)
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

void handleInput(u16 joy, u16 changed, u16 state)
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
