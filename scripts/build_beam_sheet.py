"""Pack original beam/impact pixels into Landing Site's shared PAL3.

The nine distinct VDP beam colors retain their original hardware values;
duplicate SNES shades leave six slots for scenery.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def build(colors):
    target = ROOT / 'res/images/player'
    source = ROOT / 'res/images/weapons-original-sgdk'
    for name, filename in [('beam', 'weapons_power_beam.png'), ('beam_impact', 'weapons_impact_r00.png')]:
        image = Image.open(source / filename)
        if name == 'beam_impact': image = image.crop((0, 0, 80, 16))
        values = []
        for pixel in image.convert('RGB').get_flattened_data():
            if pixel == (0, 136, 255): values.append(0)
            else:
                hardware = tuple((channel >> 5) * 32 for channel in pixel)
                values.append(colors.index(hardware))
        result = Image.new('P', image.size)
        result.putpalette([v for c in colors for v in c] + [0] * (768 - len(colors)*3))
        result.putdata(values)
        result.save(target / (name + '.png'), transparency=0)


if __name__ == '__main__':
    palette = Image.open(ROOT / 'res/images/maps/landing_site/foreground.png').getpalette()[144:192]
    build([tuple(palette[i:i+3]) for i in range(0,48,3)])
