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
  solid          half or more of the 8x8 tile is rock (grass and moss never block); lone 8 px
                 bumps and notches in a floor or ceiling are smoothed away
  ramp           the open tile at the foot of every 1-tile floor step: the 45 degree triangle
                 of mm_rect_blocked, so Samus walks up (walking climbs 1 px per px; any 8 px
                 step would stop her). Steps of 2+ tiles stay walls
  open           everything else, and every tile inside a door's footprint

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
# The saved map draws each blue door cap a few pixels wider than the 32 px footprint of the door.
# Those pixels are never rock: they stay open, so the cap does not wall off the doorway.
DOOR_CAP = [(56, 112, 224), (144, 168, 224), (24, 64, 152)]
# Grass and moss: decoration drawn over the rock, never solid.
GRASS = [(48, 64, 24), (72, 96, 56)]
# Frame pixels that are not rock take the commonest fully opaque tile, aligned to the tile grid, so whole
# filled tiles repeat (every distinct tile costs VRAM and heap, and the room has ~920 tiles of budget).
ROCK = (0, 32, 0)


SOLID, OPEN, RISE_RIGHT, RISE_LEFT = 0, 1, 2, 3


def smooth(values):
    """Remove lone 8 px bumps and notches in floors and ceilings: the 8 px grid makes them where
    the art's edge is a smooth curve, and Samus cannot walk over an 8 px step."""
    rows, cols = values.shape
    for _ in range(3):
        for ty in range(1, rows - 1):
            for tx in range(1, cols - 1):
                l, r = values[ty, tx - 1], values[ty, tx + 1]
                if values[ty, tx] == SOLID and l == OPEN and r == OPEN and (
                        values[ty + 1, tx - 1] == SOLID and values[ty + 1, tx + 1] == SOLID or
                        values[ty - 1, tx - 1] == SOLID and values[ty - 1, tx + 1] == SOLID):
                    values[ty, tx] = OPEN
                elif values[ty, tx] == OPEN and l == SOLID and r == SOLID and (
                        values[ty - 1, tx - 1] == OPEN and values[ty - 1, tx + 1] == OPEN or
                        values[ty + 1, tx - 1] == OPEN and values[ty + 1, tx + 1] == OPEN):
                    values[ty, tx] = SOLID


def add_ramps(values):
    """Walking climbs at most 1 px per px, so every 1-tile (8 px) step of a floor would stop Samus.
    The open tile at the foot of such a step becomes a 45 degree ramp. A step of 2+ tiles is a wall
    and stays; so does the edge of a floating platform (no ground under the foot tile)."""
    rows, cols = values.shape
    for ty in range(1, rows - 1):
        for tx in range(cols):
            if values[ty, tx] != OPEN or values[ty + 1, tx] != SOLID:
                continue  # the ramp sits in open air on top of solid ground
            left = tx > 0 and values[ty, tx - 1] == SOLID and (ty == 0 or values[ty - 1, tx - 1] != SOLID)
            right = tx < cols - 1 and values[ty, tx + 1] == SOLID and (ty == 0 or values[ty - 1, tx + 1] != SOLID)
            if right and not left:
                values[ty, tx] = RISE_RIGHT
            elif left and not right:
                values[ty, tx] = RISE_LEFT


def floor_steps(values):
    """Solid floor tiles beside an open tile that stands on solid ground: an 8 px step."""
    rows, cols = values.shape
    steps = []
    for ty in range(1, rows - 1):
        for tx in range(1, cols - 1):
            if values[ty, tx] != SOLID or values[ty - 1, tx] == SOLID:
                continue
            for dx in (-1, 1):
                if values[ty, tx + dx] == OPEN and values[ty + 1, tx + dx] == SOLID:
                    steps.append((tx * TILE, ty * TILE))
    return steps


def door_rects(room):
    layer = next(l for l in room['layers'] if l['name'] == 'doors')
    for obj in layer['objects']:
        facing = next(p['value'] for p in obj['properties'] if p['name'] == 'facing')
        w, h = (64, 32) if facing in ('up', 'down') else (32, 64)
        yield round(obj['x']), round(obj['y']), w, h



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
    for color in DOOR_CAP:
        doors |= (crop == color).all(axis=2)
    metal = (pixels == 0) & ~sky & ~doors
    # The commonest fully opaque tile (deep rock) is the fill pattern.
    th, tw = height // TILE, width // TILE
    tiles = pixels.reshape(th, TILE, tw, TILE).swapaxes(1, 2).reshape(-1, TILE * TILE)
    rock_only = tiles[(tiles != 0).all(axis=1)]
    patterns, counts = np.unique(rock_only, axis=0, return_counts=True)
    pattern = patterns[counts.argmax()].reshape(TILE, TILE)
    ys, xs = np.nonzero(metal)
    pixels[ys, xs] = pattern[ys % TILE, xs % TILE]
    image = Image.fromarray(pixels, 'P')
    image.putpalette(palette)

    grass = np.zeros((height, width), bool)
    for color in GRASS:
        grass |= (crop == color).all(axis=2)
    # Grass and moss are decoration on top of the rock: counting them made 1-tile bumps that stop
    # Samus while walking. They never block.
    rows, cols = height // TILE, width // TILE
    solid = ((pixels != 0) & ~grass).reshape(rows, TILE, cols, TILE).swapaxes(1, 2)
    fraction = solid.mean(axis=(2, 3))
    values = np.where(fraction >= 0.5, SOLID, OPEN)
    smooth(values)
    for x, y, w, h in door_rects(room):
        values[y // TILE:(y + h) // TILE, x // TILE:(x + w) // TILE] = OPEN
    add_ramps(values)
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
        steps = floor_steps(values)
        if steps:
            sys.exit(f'Crateria 4 floor has 8 px steps that stop Samus at {steps[:8]}')
        print('PASS Crateria 4 art and collision match the saved map; no 8 px floor steps')
    else:
        image.save(OUT, transparency=0)
        source = ROOM.read_text(encoding='utf-8')
        ROOM.write_text(source[:match.start(2)] + text + source[match.end(2):], encoding='utf-8', newline='\n')
        print(f'wrote {OUT.relative_to(ROOT)} and the collision layer of {ROOM.relative_to(ROOT)}')
        print('collision values', np.bincount(values.flatten(), minlength=4).tolist())
