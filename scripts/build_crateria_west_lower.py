"""Rebuild the art and the collision of Crateria 4 (room 92FD) from the saved Crateria map.

The room is the 1280x1296 px region at (3328, 1024) of
res/images/referencias-spriters-resource/139029-crateria-map.png (the map is five screens
left of the room coordinates of rooms/world.json; 92B3 sits at (3328, 512)).

Art (res/images/maps/crateria/crateria_west_lower_foreground.png): the original terrain
pixels, reduced to the 3-bit colors of the VDP and to the palette slots that Crateria 3
already uses (they avoid the HUD slots of PAL0). Sky and cave air stay transparent, so the
background plane shows through. Metal that is not sky (door frames, the bottom frame) becomes
rock, except inside a door's footprint, which the door sprites draw.

Collision (the "collision" layer of rooms/crateria_4.tmj), from the same pixels:
  solid          half or more of the 8x8 tile is terrain (grass and moss do not count)
  slope          a floor tile whose ground rises or falls by 3+ px across the tile, in steps of
                 at most 2 px per column, standing on solid rock (the physics treats it as
                 the 45 degree triangle of mm_rect_blocked; the art's slopes are gentler)
  open           everything else

  python scripts/build_crateria_west_lower.py          write the PNG and the collision layer
  python scripts/build_crateria_west_lower.py --check  fail if either is out of date
"""
from pathlib import Path
import json
import re
import sys

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
MAP = ROOT / 'res/images/referencias-spriters-resource/139029-crateria-map.png'
OUT = ROOT / 'res/images/maps/crateria/crateria_west_lower_foreground.png'
ROOM = ROOT / 'rooms/crateria_4.tmj'
PALETTE_SOURCE = ROOT / 'res/images/maps/crateria/crateria_west_foreground.png'
BOX = (3328, 1024, 3328 + 1280, 1024 + 1296)
TILE = 8
COLLISION_COLUMNS_PER_LINE = 160  # Tiled writes one map row per line
# Original terrain color -> VDP color. Slot 1 of PAL0 is the HUD's black: the same color.
TERRAIN = {
    (0, 8, 8): (0, 0, 0),
    (8, 32, 24): (0, 32, 0), (24, 40, 24): (0, 32, 0),
    (40, 48, 80): (32, 32, 64), (48, 64, 24): (32, 64, 0),
    (72, 64, 88): (64, 64, 64), (72, 96, 56): (64, 96, 32),
    (112, 64, 88): (96, 64, 64),
}
SKY = [(53, 133, 186), (2, 74, 122)]
# Grass tufts and moss stay in the art, but never block: Samus walks through them.
GRASS = [(48, 64, 24), (72, 96, 56)]
ROCK = (0, 32, 0)


def door_rects(room):
    layer = next(l for l in room['layers'] if l['name'] == 'doors')
    for obj in layer['objects']:
        facing = next(p['value'] for p in obj['properties'] if p['name'] == 'facing')
        w, h = (64, 32) if facing in ('up', 'down') else (32, 64)
        yield round(obj['x']), round(obj['y']), w, h


def ground_profile(tile):
    """Opaque pixels counted up from the bottom of each column of an 8x8 mask."""
    heights = []
    for column in range(TILE):
        n = 0
        while n < TILE and tile[TILE - 1 - n, column]:
            n += 1
        heights.append(n)
    return np.array(heights)


def build():
    """Returns (art image, collision values as a rows x columns array)."""
    room = json.loads(ROOM.read_text(encoding='utf-8'))
    reference = Image.open(PALETTE_SOURCE)
    palette = reference.getpalette()
    slot = {tuple(palette[i * 3:i * 3 + 3]): i for i in range(15, 0, -1)}  # lowest slot wins
    crop = np.array(Image.open(MAP).convert('RGB').crop(BOX))
    height, width = crop.shape[:2]
    pixels = np.zeros((height, width), dtype='uint8')
    for source, target in TERRAIN.items():
        pixels[(crop == source).all(axis=2)] = slot[target]
    sky = np.zeros((height, width), bool)
    for color in SKY:
        sky |= (crop == color).all(axis=2)
    doors = np.zeros((height, width), bool)
    for x, y, w, h in door_rects(room):
        doors[y:y + h, x:x + w] = True
    metal = (pixels == 0) & ~sky & ~doors
    pixels[metal] = slot[ROCK]
    image = Image.fromarray(pixels, 'P')
    image.putpalette(palette)

    grass = np.zeros((height, width), bool)
    for color in GRASS:
        grass |= (crop == color).all(axis=2)
    rows, cols = height // TILE, width // TILE
    solid = ((pixels != 0) & ~grass).reshape(rows, TILE, cols, TILE).swapaxes(1, 2)
    fraction = solid.mean(axis=(2, 3))
    values = np.where(fraction >= 0.5, 0, 1)
    for ty in range(rows - 1):
        for tx in range(cols):
            if not 0.25 <= fraction[ty, tx] <= 0.95 or fraction[ty + 1, tx] < 0.9:
                continue
            h = ground_profile(solid[ty, tx])
            step = np.diff(h)
            if h[7] - h[0] >= 3 and step.min() >= -1 and step.max() <= 2:
                values[ty, tx] = 2
            elif h[0] - h[7] >= 3 and step.max() <= 1 and step.min() >= -2:
                values[ty, tx] = 3
    return image, values


def collision_text(values):
    """The "data" lines of the collision layer: gid = value + 1, one row per line."""
    flat = [str(int(v) + 1) for v in values.flatten()]
    lines = [','.join(flat[i:i + COLLISION_COLUMNS_PER_LINE]) for i in range(0, len(flat), COLLISION_COLUMNS_PER_LINE)]
    return ',\n'.join(lines)


COLLISION_PATTERN = re.compile(r'("name": "collision",.*?"data": \[\n)(.*?)(\n\s*\])', re.S)


def read_collision_text():
    match = COLLISION_PATTERN.search(ROOM.read_text(encoding='utf-8'))
    if not match:
        raise SystemExit('collision layer not found in rooms/crateria_4.tmj')
    return match


if __name__ == '__main__':
    image, values = build()
    match = read_collision_text()
    text = collision_text(values)
    if '--check' in sys.argv:
        current = Image.open(OUT)
        if current.size != image.size or not np.array_equal(np.array(current), np.array(image)):
            sys.exit(f'{OUT.relative_to(ROOT)} is out of date; run scripts/build_crateria_west_lower.py')
        if match[2].strip() != text:
            sys.exit('the collision layer of rooms/crateria_4.tmj is out of date; run scripts/build_crateria_west_lower.py')
        print('PASS Crateria 4 art and collision match the saved map')
    else:
        image.save(OUT, transparency=0)
        source = ROOM.read_text(encoding='utf-8')
        ROOM.write_text(source[:match.start(2)] + text + source[match.end(2):], encoding='utf-8', newline='\n')
        print(f'wrote {OUT.relative_to(ROOT)} and the collision layer of {ROOM.relative_to(ROOT)}')
        print('collision values', np.bincount(values.flatten(), minlength=4).tolist())
