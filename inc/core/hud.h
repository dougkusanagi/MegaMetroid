#ifndef MM_HUD_H
#define MM_HUD_H
/* Original-size glyphs: icons on rows 1-2, counts/energy on row 3, tanks on row 2. */
#define MM_HUD_COLUMNS 32
#define MM_HUD_ROWS 4
#define MM_HUD_ICON_ROWS 4
#define MM_HUD_MESSAGE_ROW 4
/* One row of tanks; the original's second row comes with more tanks than Crateria has. */
#define MM_HUD_MAX_TANKS 7
#define MM_HUD_TANK_COLUMN 1
#define MM_HUD_ENERGY_COLUMN 1
#define MM_HUD_ENERGY_DIGITS_COLUMN 6
#define MM_HUD_MISSILE_COLUMN 10
#define MM_HUD_MISSILE_COUNT_COLUMN 10
#define MM_HUD_SUPER_COLUMN 14
#define MM_HUD_SUPER_COUNT_COLUMN 14

/* Energy of a new game: no tanks, 99 units. */
#define MM_START_ENERGY 99
#define MM_ENERGY_PER_TANK 100

/* Tile order of res/images/hud/hud_tiles.png (scripts/build_hud.py). Icons are row-major. */
enum {
    MM_HUD_BLANK,
    MM_HUD_DIGIT0,
    MM_HUD_ENERGY = MM_HUD_DIGIT0 + 10,
    MM_HUD_TANK_EMPTY = MM_HUD_ENERGY + 4,
    MM_HUD_TANK_FULL,
    MM_HUD_MISSILE_OFF,
    MM_HUD_MISSILE_ON = MM_HUD_MISSILE_OFF + 6,
    MM_HUD_SUPER_OFF = MM_HUD_MISSILE_ON + 6,
    MM_HUD_SUPER_ON = MM_HUD_SUPER_OFF + 4,
    MM_HUD_TILE_COUNT = MM_HUD_SUPER_ON + 4
};

typedef struct {
    int energy, energy_max;
    int missiles, missiles_max, supers, supers_max;
    int selected; /* MM_WEAPON_* */
} MmHudState;

/* Tiles of rows 0-3. Energy shows energy % 100 with one square per tank, full while the
   energy covers it. An ammo icon appears once its capacity (or count) is above 0, lit while
   selected; missiles show three digits and supers two, as in the original. */
void mm_hud_cells(const MmHudState *state, unsigned char cells[MM_HUD_ICON_ROWS][MM_HUD_COLUMNS]);
/* True if both states give the same cells; lets a frame skip the work of mm_hud_cells. */
int mm_hud_state_equal(const MmHudState *a, const MmHudState *b);
#endif
