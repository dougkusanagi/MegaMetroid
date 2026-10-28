#include "core/hud.h"
#include "core/weapon.h"

static void digits(unsigned char *cell, int value, int count)
{
    while (count--) {
        cell[count] = (unsigned char)(MM_HUD_DIGIT0 + value % 10);
        value /= 10;
    }
}

static void icon(unsigned char cells[MM_HUD_ICON_ROWS][MM_HUD_COLUMNS], int column, int width, int first)
{
    int row, i;
    for (row = 0; row < MM_HUD_ICON_ROWS; ++row)
        for (i = 0; i < width; ++i) cells[row][column + i] = (unsigned char)(first + row * width + i);
}

void mm_hud_cells(const MmHudState *s, unsigned char cells[MM_HUD_ICON_ROWS][MM_HUD_COLUMNS])
{
    int row, i, tanks, full, energy;
    for (row = 0; row < MM_HUD_ICON_ROWS; ++row)
        for (i = 0; i < MM_HUD_COLUMNS; ++i) cells[row][i] = MM_HUD_BLANK;
    energy = s->energy < 0 ? 0 : s->energy;
    tanks = s->energy_max / MM_ENERGY_PER_TANK;
    if (tanks > MM_HUD_MAX_TANKS) tanks = MM_HUD_MAX_TANKS;
    full = energy / MM_ENERGY_PER_TANK;
    for (i = 0; i < tanks; ++i)
        cells[0][MM_HUD_TANK_COLUMN + i] = (unsigned char)(i < full ? MM_HUD_TANK_FULL : MM_HUD_TANK_EMPTY);
    for (i = 0; i < 4; ++i) cells[1][MM_HUD_ENERGY_COLUMN + i] = (unsigned char)(MM_HUD_ENERGY + i);
    digits(&cells[1][MM_HUD_ENERGY_DIGITS_COLUMN], energy % MM_ENERGY_PER_TANK, 2);
    if (s->missiles_max > 0 || s->missiles > 0) {
        icon(cells, MM_HUD_MISSILE_COLUMN, 3,
             s->selected == MM_WEAPON_MISSILE ? MM_HUD_MISSILE_ON : MM_HUD_MISSILE_OFF);
        digits(&cells[1][MM_HUD_MISSILE_COUNT_COLUMN], s->missiles % 1000, 3);
    }
    if (s->supers_max > 0 || s->supers > 0) {
        icon(cells, MM_HUD_SUPER_COLUMN, 2,
             s->selected == MM_WEAPON_SUPER ? MM_HUD_SUPER_ON : MM_HUD_SUPER_OFF);
        digits(&cells[1][MM_HUD_SUPER_COUNT_COLUMN], s->supers % 100, 2);
    }
}
