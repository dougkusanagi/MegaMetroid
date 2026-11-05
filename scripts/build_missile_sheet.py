"""Build the missile sprite sheet in its own 12-colour palette.

The original `weapons_missile` and `weapons_super_missile` sheets each hold the
eight directions in the same order as the beam sheet: up-left, up, up-right,
left, right, down-left, down, down-right. Frames 0..7 are missiles and 8..15
are supers, all 16x16.

Both sheets share most hardware colours; the union is 11 opaque colours.
The sheet uses only indices 0..9, except its two greens, which take the same
slots (14, 15) as the beam (and ship) palette, so selecting a missile never
colours the ship tones in PAL3 indices 10..13. Colours are reduced to the
VDP's 3 bits per channel, as in scripts/build_beam_sheet.py.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'res/images/weapons-original-sgdk'
OUT = ROOT / 'res/images/player/missile.png'
TRANSPARENT = (0, 136, 255)
SHEETS = ['weapons_missile.png', 'weapons_super_missile.png']


def hardware(color):
    return tuple((channel >> 5) * 32 for channel in color)


def main():
    images = [Image.open(SOURCE / name).convert('RGB') for name in SHEETS]
    # PAL3 entries 10..15 already belong to the beam (and ship) palette; the missile
    # sheet starts from it, so those tones never change while it is selected.
    beam = ROOT / 'res/images/player/beam.png'
    beam_palette = [tuple(Image.open(beam).getpalette()[index * 3:index * 3 + 3]) for index in range(16)]
    colors = []
    for image in images:
        for pixel in image.get_flattened_data():
            if pixel == TRANSPARENT:
                continue
            color = hardware(pixel)
            if color not in colors:
                colors.append(color)
    if len(colors) > 15:
        raise ValueError(f'{len(colors)} opaque colours do not fit one VDP palette')

    index_of = {}
    for color in colors:
        if color == beam_palette[14]:
            index_of[color] = 14
        elif color == beam_palette[15]:
            index_of[color] = 15
        else:
            free = next(index for index in range(1, 10) if index not in index_of.values())
            index_of[color] = free
    palette = list(beam_palette)
    palette[0] = TRANSPARENT
    for color, index in index_of.items():
        palette[index] = color
    assert palette[10:14] == beam_palette[10:14] and palette[14:16] == [beam_palette[14], beam_palette[15]]

    result = Image.new('P', (16 * 16, 16))
    result.putpalette([value for color in palette for value in color] + [0] * (768 - len(palette) * 3))
    for sheet, image in enumerate(images):
        for frame in range(8):
            cell = image.crop((frame * 16, 0, frame * 16 + 16, 16))
            mapped = []
            for pixel in cell.get_flattened_data():
                mapped.append(0 if pixel == TRANSPARENT else index_of[hardware(pixel)])
            tile = Image.new('P', cell.size)
            tile.putdata(mapped)
            result.paste(tile, ((sheet * 8 + frame) * 16, 0))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    result.save(OUT, transparency=0)
    print(f'{OUT.relative_to(ROOT)}: 16 frames of 16x16, {len(colors)} opaque colours')


if __name__ == '__main__':
    main()
