"""Recolor the Crateria cave background (bg-2.bmp) into the PAL1 of the Landing Site.

Door caps and enemy sprites are drawn with the colors of the Landing Site's PAL1
(res/images/maps/landing_site/background.png), so every room must carry that palette in PAL1 or
its doors and enemies come out in the wrong colors. bg-2.bmp has four colors, and each one exists
in that palette at the same 3-bit value; only the indices change.

  python scripts/build_crateria_bg.py          write res/images/maps/crateria/bg-2-pal1.png
  python scripts/build_crateria_bg.py --check  fail if it is out of date
"""
from pathlib import Path
import sys

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'res/images/maps/crateria/bg-2.bmp'
PALETTE = ROOT / 'res/images/maps/landing_site/background.png'
OUT = ROOT / 'res/images/maps/crateria/bg-2-pal1.png'


def build():
    source, target = Image.open(SOURCE), Image.open(PALETTE)
    src_pal, dst_pal = source.getpalette(), target.getpalette()
    dst_slots = {tuple(c // 32 for c in dst_pal[i * 3:i * 3 + 3]): i for i in range(15, 0, -1)}
    remap = np.zeros(256, dtype='uint8')
    for index in set(np.array(source).flatten().tolist()):
        color = tuple(c // 32 for c in src_pal[index * 3:index * 3 + 3])
        assert color in dst_slots, f'{color} is not in the Landing Site PAL1'
        remap[index] = dst_slots[color]
    image = Image.fromarray(remap[np.array(source)], 'P')
    image.putpalette(dst_pal)
    return image


if __name__ == '__main__':
    image = build()
    if '--check' in sys.argv:
        if not OUT.exists() or not np.array_equal(np.array(Image.open(OUT)), np.array(image)):
            sys.exit(f'{OUT.relative_to(ROOT)} is out of date; run scripts/build_crateria_bg.py')
        print('PASS Crateria background carries the Landing Site PAL1')
    else:
        image.save(OUT)
        print(f'wrote {OUT.relative_to(ROOT)}')
