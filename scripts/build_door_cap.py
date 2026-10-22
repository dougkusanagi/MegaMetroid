"""Build the door cap sprite from the original door sheet.

`doors_door_r00` frame 0 is a closed door in a left wall: the metal frame is
columns 0..15 and the cap (the blue half on the room's side) is columns 16..31.
Frames 1..3 retract the cap and frame 4 leaves the open jamb, so the 16x64
strip x=16..31 is the only part that changes.  The room art keeps the closed
door baked in; this sprite is drawn on top of that strip and must be opaque
(index 0 is transparent on the Mega Drive, so black uses index 1) to cover it
while the door opens.

The 16 colors are the Landing Site PAL1 (background) palette, built by
scripts/build_landing_site.py: the room has no other bank with the three door
blues.  Original SNES colors are reduced to the VDP's 3 bits per channel, and
the white highlight (source index 12) uses the closest PAL1 slot (index 14).
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'res/images/doors-original-sgdk/doors_door_r00.png'
PALETTE = ROOT / 'res/images/maps/landing_site/background.png'
OUT = ROOT / 'res/images/doors/door_cap.png'
FRAMES = 5

# Original sheet index -> Landing Site PAL1 index.  Only these indices occur in
# the changing strip; the rest of the sheet (red doors, pink trim) is refused.
COLORS = {
    0: 1,   # outside the door shape: black, matching the cave wall behind it
    1: 1,   # black
    2: 3,   # 32,32,32
    3: 9,   # 0,64,128  dark blue
    5: 12,  # 96,64,64
    6: 13,  # 128,96,96
    7: 10,  # 0,96,224  blue
    9: 11,  # 96,160,224 light blue
    10: 14, # 192,160,160 pink highlight
    12: 14, # white highlight, closest available slot
}


def main():
    sheet = Image.open(SOURCE)
    pal1 = Image.open(PALETTE).getpalette()[:48]
    result = Image.new('P', (16 * FRAMES, 64))
    result.putpalette(list(pal1) + [0] * (768 - len(pal1)))
    for frame in range(FRAMES):
        cap = sheet.crop((frame * 32 + 16, 0, frame * 32 + 32, 64))
        mapped = []
        for index in cap.get_flattened_data():
            if index not in COLORS:
                raise ValueError(f'frame {frame}: unexpected source index {index}')
            mapped.append(COLORS[index])
        image = Image.new('P', cap.size)
        image.putdata(mapped)
        result.paste(image, (frame * 16, 0))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    # No transparency metadata: index 0 would be transparent on screen and is
    # deliberately unused.
    result.save(OUT)
    print(f'{OUT.relative_to(ROOT)}: {FRAMES} frames of 16x64, PAL1, opaque')


if __name__ == '__main__':
    main()
