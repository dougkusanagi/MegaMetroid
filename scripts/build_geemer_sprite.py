"""Remap the original Geemer strip onto the Landing Site PAL1 palette.

PAL1 is the room background palette. Its last entry is reserved for enemy
highlights, so the original Geemer colors are approximated by the existing
background and door shades instead of using a separate palette bank.
"""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'res/images/enemies-original-sgdk/enemies_right_r12_0.png'
PALETTE_SOURCE = ROOT / 'res/images/maps/landing_site/background.png'
OUT = ROOT / 'res/images/enemies/geemer.png'
WALL_OUT = ROOT / 'res/images/enemies/geemer_wall.png'
PREVIEW = ROOT / 'tests/out/geemer-preview.png'

REMAP = {
    0: 0,
    1: 4,
    2: 5,
    3: 4,
    4: 4,
    5: 12,
    6: 9,
    7: 13,
    8: 7,
    9: 13,
    10: 11,
    11: 13,
    12: 7,
    13: 11,
    14: 11,
    15: 14,
}


def main():
    source = Image.open(SOURCE)
    palette = Image.open(PALETTE_SOURCE).getpalette()[:48]
    pixels = np.array(source)
    remapped = np.zeros_like(pixels)
    for old, new in REMAP.items():
        remapped[pixels == old] = new
    out = Image.fromarray(remapped, mode='P')
    out.putpalette(palette + [0] * (768 - len(palette)))
    out.save(OUT, transparency=0)
    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    preview = Image.new('RGBA', out.size, (0, 0, 0, 255))
    rgba = out.convert('RGBA')
    preview.alpha_composite(rgba)
    preview.resize((out.width * 6, out.height * 6), Image.Resampling.NEAREST).save(PREVIEW)
    print(f'Geemer remapped to PAL1: {OUT} ({out.width}x{out.height})')
    # Wall strip: the floor frames rotated 90 degrees, so the legs point at the wall and the
    # crown points away from it. The mirrored (horizontal flip) sheet serves the other wall.
    wall = Image.new('P', (out.width * 32 // 24, 24), 0)
    wall.putpalette(palette + [0] * (768 - 48))
    for frame in range(3):
        part = out.crop((frame * 24, 0, (frame + 1) * 24, 32)).transpose(Image.ROTATE_90)
        wall.paste(part, (frame * 32, 0))
    wall.save(WALL_OUT, transparency=0)
    print(f'Geemer wall strip: {WALL_OUT} ({wall.width}x{wall.height})')


if __name__ == '__main__':
    main()
