#include "beam_sprite.h"

/* Native 8x8 tile: orange rim, yellow glow and white core. */
static const u32 pixels[] = {
    0x00000000, 0x00111100, 0x01222210, 0x12333321,
    0x12333321, 0x01222210, 0x00111100, 0x00000000
};
const u16 beam_palette[16] = {
    0x0000, 0x004E, 0x00EE, 0x0EEE, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};
static const TileSet tiles = {COMPRESSION_NONE, 1, (u32 *)pixels};
static const AnimationFrame frame = {
    1, 0, (TileSet *)&tiles, NULL, {{0, 0, SPRITE_SIZE(1, 1), 0, 0, 1}}
};
static const AnimationFrame * const frames[] = {&frame};
static const Animation animation = {1, 0, (AnimationFrame **)frames};
static const Animation * const animations[] = {&animation};
const SpriteDefinition beam_sprite = {8, 8, NULL, 1, (Animation **)animations, 1, 1};
