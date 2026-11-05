"""Build original-size HUD tiles in reserved PAL0 slots (see hud_palette.py)."""
import argparse
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
BAR = ROOT / 'res/images/status-bar-original-sgdk'
OUT = ROOT / 'res/images/hud'
PALETTE = ROOT / 'res/images/maps/landing_site/foreground.png'
# Reserved PAL0 colors; terrain pixels retain their hardware colors.
BLACK, BLUE, WHITE, GREY_LIGHT, GREY = 1, 2, 3, 13, 8
TILE_COUNT = 37

# Source index -> HUD index, per sheet. Index 0 is the bar's black background.
DIGIT_MAP = {0: BLACK, 1: BLUE, 2: WHITE}
ENERGY_MAP = {0: BLACK, 1: BLUE, 2: BLACK, 3: WHITE}
TANK_MAP = {0: BLACK, 2: BLUE, 5: 4}
# Selected icons use pink/white; inactive icons use muted pink.
# ENERGY index 2 is the sheet's green separator, not part of the glyph.
ICON_ON_MAP = {0: BLACK, 1: BLACK, 2: 4, 3: WHITE}
ICON_OFF_MAP = {0: BLACK, 1: BLACK, 2: GREY, 3: GREY_LIGHT}


def remap(image, box, table):
    crop = image.crop(box)
    out = Image.new('P', crop.size)
    try:
        out.putdata([table[i] for i in crop.get_flattened_data()])
    except KeyError as error:
        raise ValueError(f'unexpected source index {error} in {box}') from None
    return out


def tiles_of(image):
    """Row-major 8x8 tiles of an image whose size is a multiple of 8."""
    return [image.crop((x, y, x + 8, y + 8))
            for y in range(0, image.height, 8) for x in range(0, image.width, 8)]


def palette():
    flat = Image.open(PALETTE).getpalette()[:48]
    return flat + [0] * (768 - len(flat))


def build_tiles():
    digits = Image.open(BAR / 'status_bar_digits.png')
    energy = Image.open(BAR / 'status_bar_energy_text.png')
    reserve = Image.open(BAR / 'status_bar_auto_reserve.png')
    icons_on = Image.open(BAR / 'status_bar_item_icons_on.png')
    icons_off = Image.open(BAR / 'status_bar_item_icons_off.png')
    tiles = [Image.new('P', (8, 8), BLACK)]
    # The sheet runs 1..9 then 0, one glyph every 9 px.
    glyph = [remap(digits, (9 * k, 0, 9 * k + 8, 8), DIGIT_MAP) for k in range(10)]
    tiles += [glyph[9]] + glyph[:9]
    tiles += tiles_of(remap(energy, (0, 0, 32, 8), ENERGY_MAP))
    # Tanks: 6x6 squares cut from the reserve tank swatches, with a gap so a row of them
    # does not merge into a bar.
    for y in (0, 8):
        tank = Image.new('P', (8, 8), BLACK)
        tank.paste(remap(reserve, (18, y, 24, y + 6), TANK_MAP), (1, 1))
        tiles.append(tank)
    # The selected sheet sits one row lower than the other.
    for on in (False, True):
        sheet, table, top = (icons_on, ICON_ON_MAP, 1) if on else (icons_off, ICON_OFF_MAP, 0)
        tiles += tiles_of(remap(sheet, (0, top, 24, top + 16), table))
    for on in (False, True):
        sheet, table, top = (icons_on, ICON_ON_MAP, 1) if on else (icons_off, ICON_OFF_MAP, 0)
        tiles += tiles_of(remap(sheet, (32, top, 48, top + 16), table))
    assert len(tiles) == TILE_COUNT, len(tiles)
    strip = Image.new('P', (8 * len(tiles), 8))
    strip.putpalette(palette())
    for i, tile in enumerate(tiles):
        strip.paste(tile, (8 * i, 0))
    return strip


def build_font(source):
    """SGDK's font with an opaque background: glyph 15 -> white, 0 -> black."""
    font = Image.open(source)
    if set(font.get_flattened_data()) - {0, 15}:
        raise ValueError('font must use only indices 0 and 15')
    out = remap(font, (0, 0, font.width, font.height), {0: BLACK, 15: WHITE})
    out.putpalette(palette())
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font', default='D:/apps/sgdk/res/image/font_default.png',
                        help="SGDK's res/image/font_default.png")
    args = parser.parse_args()
    from hud_palette import reserve
    for path in (ROOT / 'res/images/maps/landing_site/foreground.png',
                 ROOT / 'res/images/maps/crateria/tilemap2.png'):
        reserve(Image.open(path)).save(path, transparency=0)
    OUT.mkdir(parents=True, exist_ok=True)
    build_tiles().save(OUT / 'hud_tiles.png')
    build_font(args.font).save(OUT / 'hud_font.png')
    print(f'{(OUT / "hud_tiles.png").relative_to(ROOT)}: {TILE_COUNT} tiles in the reserved PAL0 palette')
    print(f'{(OUT / "hud_font.png").relative_to(ROOT)}: opaque font, 96 glyphs')


if __name__ == '__main__':
    main()
