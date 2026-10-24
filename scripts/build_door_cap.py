"""Build full transparent 32x64 door frames (metal frame plus animated cap).
The room keeps its original terrain/sky underlay, so index 0 must remain
transparent. The complete door sprite sorts in front of Samus.
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
    0: 0,   # reveal the original room underlay outside the door shape
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
    result = Image.new('P', (32 * FRAMES, 64))
    result.putpalette(list(pal1) + [0] * (768 - len(pal1)))
    for frame in range(FRAMES):
        cap = sheet.crop((frame * 32, 0, frame * 32 + 32, 64))
        mapped = []
        for index in cap.get_flattened_data():
            if index not in COLORS:
                raise ValueError(f'frame {frame}: unexpected source index {index}')
            mapped.append(COLORS[index])
        image = Image.new('P', cap.size)
        image.putdata(mapped)
        result.paste(image, (frame * 32, 0))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    result.save(OUT, transparency=0)
    print(f'{OUT.relative_to(ROOT)}: {FRAMES} frames of 32x64, PAL1, transparent')


if __name__ == '__main__':
    main()
