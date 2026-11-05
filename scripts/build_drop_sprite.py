"""Builds res/images/objects/drops.png: PROVISIONAL 8x8 drops (frame 0 energy, frame 1 missile).

The original small drop sprites are not in the repository, so these are plain orbs drawn with
entries of the Samus PAL2 palette (no palette bank of their own). Replace with the original art.
"""
import os
from PIL import Image

root = os.path.join(os.path.dirname(__file__), "..")
pal = Image.open(os.path.join(root, "res/images/samus-original-sgdk/palettes/pal_samus_power_suit.png")).getpalette()[:48]
ROWS = ["..rrrr..", ".rbbbbr.", "rbhbbbbr", "rbbbbbbr", "rbbbbbbr", "rbbbbbbr", ".rbbbbr.", "..rrrr.."]


def orb(ring, body, hi):
    m = {".": 0, "r": ring, "b": body, "h": hi}
    return [[m[c] for c in r] for r in ROWS]


frames = [orb(8, 4, 6), orb(15, 9, 2)]  # green energy orb, red missile orb
im = Image.new("P", (16, 8), 0)
im.putpalette(pal + [0] * (768 - 48))
for f, px in enumerate(frames):
    for y in range(8):
        for x in range(8):
            im.putpixel((f * 8 + x, y), px[y][x])
os.makedirs(os.path.join(root, "res/images/objects"), exist_ok=True)
im.save(os.path.join(root, "res/images/objects/drops.png"))
