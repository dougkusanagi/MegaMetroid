"""Builds res/images/objects/drops.png: 8x8 drops, frames 0..3 energy (animated) and 4 missile.

The energy frames are the original small health pickup (objects_item_r04 frames 3..6, the purple
spinning orb), centred into 8x8 and remapped onto the room PAL0 palette, which holds the purples
(the Samus PAL2 has only one visible purple). The missile orb is still provisional (a plain red
orb in PAL2), until its original frames are identified.
"""
import os
from PIL import Image

root = os.path.join(os.path.dirname(__file__), "..")
pal = Image.open(os.path.join(root, "res/images/samus-original-sgdk/palettes/pal_samus_power_suit.png")).getpalette()[:48]
SOURCE = os.path.join(root, "res/images/objects-original-sgdk/objects_item_r04.png")
FG = os.path.join(root, "res/images/maps/landing_site/foreground.png")
TRANSPARENT = (0, 136, 255)
# Original purple orb -> room PAL0 (its purples): dark muted outline, magenta body/highlight.
ENERGY_MAP = {(80, 0, 96): 8, (160, 0, 176): 4, (232, 0, 248): 4}
ROWS = ["..rrrr..", ".rbbbbr.", "rbhbbbbr", "rbbbbbbr", "rbbbbbbr", "rbbbbbbr", ".rbbbbr.", "..rrrr.."]


def orb(ring, body, hi):
    m = {".": 0, "r": ring, "b": body, "h": hi}
    return [[m[c] for c in r] for r in ROWS]


source = Image.open(SOURCE).convert("RGB")
frames = []
for cell in range(3, 7):
    part = source.crop((cell * 16, 0, cell * 16 + 16, 16))
    # The animated orb changes size while spinning; centre what is inside the cell.
    points = [(x, y) for y in range(16) for x in range(16)
              if part.getpixel((x, y)) != TRANSPARENT]
    left = min(x for x, _ in points)
    right = max(x for x, _ in points) + 1
    top = min(y for _, y in points)
    bottom = max(y for _, y in points) + 1
    crop = part.crop((left, top, right, bottom))
    if crop.width > 8 or crop.height > 8:
        raise ValueError(f'energy frame is {crop.width}x{crop.height}')
    frame = [[0] * 8 for _ in range(8)]
    for y in range(crop.height):
        for x in range(crop.width):
            pixel = crop.getpixel((x, y))
            if pixel == TRANSPARENT:
                continue
            if pixel not in ENERGY_MAP:
                raise ValueError(f'unexpected energy drop colour {pixel}')
            frame[y + (8 - crop.height) // 2][x + (8 - crop.width) // 2] = ENERGY_MAP[pixel]
    frames.append(frame)
frames.append(orb(15, 9, 2))  # provisional red missile orb
os.makedirs(os.path.join(root, "res/images/objects"), exist_ok=True)
missile = Image.new("P", (8, 8), 0)
missile.putpalette(pal + [0] * (768 - 48))
for y in range(8):
    for x in range(8):
        missile.putpixel((x, y), frames[4][y][x])
missile.save(os.path.join(root, "res/images/objects/drops.png"))
energy = Image.new("P", (32, 8), 0)
energy.putpalette(Image.open(FG).getpalette()[:48] + [0] * (768 - 48))
for f in range(4):
    for y in range(8):
        for x in range(8):
            energy.putpixel((f * 8 + x, y), frames[f][y][x])
energy.save(os.path.join(root, "res/images/objects/drops_energy.png"))
print(f"drops.png (missile) + drops_energy.png: {4} animated frames of 8x8")
