#include <genesis.h>

#include "../inc/global.h"
#include "../res/resources.h"

typedef struct
{
	Vect2D_s16 position;
	Vect2D_f16 velocity;

	bool is_on_floor;
	bool is_flipped;

	u8 current_animation;
	Sprite *sprite;
} Entity;

typedef struct
{
	Vect2D_s16 position;
} Camera;

typedef struct
{
	Map *fg;
	Map *bg;
} MapInfo;

Entity player;
Camera camera;

MapInfo map;

Sprite *player_sprite_test;

static void boot(void);
static void PLAYER_init(void);
static void CAMERA_init(void);
static void LEVEL_init(void);
static void handleInput(u16 joy, u16 changed, u16 state);

int main()
{
	// boot();

	SPR_init();

	PAL_setPalette(PLAYER_PALETTE, player_sprite.palette->data, DMA);
	player_sprite_test = SPR_addSprite(
		&player_sprite,
		100, 100,
		TILE_ATTR(PLAYER_PALETTE, 0, FALSE, FALSE));

	while (1)
	{
		// process input
		// update
		// render

		SPR_update();
		SYS_doVBlankProcess();
	}

	return (0);
}

static void PLAYER_init(void)
{
	PAL_setPalette(PLAYER_PALETTE, player_sprite.palette->data, DMA);

	player.position = (Vect2D_s16){0, 0};
	player.velocity = (Vect2D_f16){0, 0};
	player.is_on_floor = FALSE;
	player.is_flipped = FALSE;
	player.current_animation = PLAYER_ANIM_IDLE;

	player.sprite = SPR_addSprite(&player_sprite,
								  player.position.x,
								  player.position.y,
								  TILE_ATTR(PLAYER_PALETTE, TRUE, FALSE, FALSE));

	SPR_setPriority(player.sprite, 0);
}

static void CAMERA_init()
{
	// camera position (force refresh)
	camera.position.x = -1;
	camera.position.y = -1;

	MAP_scrollTo(map.fg, camera.position.x, camera.position.y);
}

static void LEVEL_init(void)
{
	CAMERA_init();
}

static void handleInput(u16 joy, u16 changed, u16 state)
{
	// TODO: Implement
}

static void boot(void)
{
	VDP_setScreenWidth256();

	JOY_init();
	JOY_setEventHandler(&handleInput);

	SPR_init();

	PLAYER_init();
	LEVEL_init();
}
