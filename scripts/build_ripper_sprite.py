"""Remap the original Ripper strip onto the Landing Site PAL1 palette.

PAL1 is the room background palette. Its last entry is reserved for enemy
highlights, so the original greens and oranges are approximated by the
existing background and door shades instead of using a separate palette bank.
"""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'res/images/enemies-original-sgdk/enemies_left_r06.png'
PALETTE_SOURCE = ROOT / 'res/images/maps/landing_site/background.png'
OUT = ROOT / 'res/images/enemies/ripper.png'
PREVIEW = ROOT / 'tests/out/ripper-preview.png'

REMAP = {
    0: 0,
    1: 3,
    2: 12,
    3: 2,
    4: 12,
    5: 7,
    6: 12,
    7: 12,
    8: 13,
    9: 15,
    10: 13,
    11: 14,
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
    print(f'Ripper remapped to PAL1: {OUT} ({out.width}x{out.height})')


if __name__ == '__main__':
    main()
