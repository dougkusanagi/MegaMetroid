"""Extract actual level collision fixtures; validate dimensions before C tests."""
from pathlib import Path
import re
import struct
from PIL import Image
import numpy as np

root = Path(__file__).resolve().parents[1]
fixtures = []
for index, rows, cols, image in [(1,156,288,'landing_site/foreground.png'), (2,160,160,'crateria/tilemap2.png')]:
    source = (root / f'src/map_crateria_{index}.c').read_text()
    match = re.search(r'const u8 map_collision_crateria_\d+\[\d+\]\[\d+\] = (\{.*?\});', source, re.S)
    assert match, 'Missing collision array'
    data_rows = re.findall(r'\{([^{}]+)\}', match[1])
    assert len(data_rows) == rows
    assert all(len(re.findall(r'\d+', row)) == cols for row in data_rows)
    assert all(int(n) in range(4) for n in re.findall(r'\d+', match[1]))
    png = (root / 'res/images/maps' / image).read_bytes()
    assert struct.unpack('>II', png[16:24]) == (cols * 8, rows * 8)
    fixtures.append(f'static const unsigned char real_map_{index}[{rows}][{cols}] = {match[1]};')
(root / 'tests/out/maps.h').write_text('\n'.join(fixtures))
print('PASS real level dimensions, tile values and image alignment')

# The VDP selects one of four palettes for each tile, never per pixel.
fg = np.array(Image.open(root / 'res/images/maps/landing_site/foreground.png'))
tiles = fg.reshape(156,8,288,8).transpose(0,2,1,3).reshape(-1,64)
for tile in tiles:
    opaque = tile[(tile & 15) != 0]
    assert len(set((opaque >> 4).tolist())) <= 1, 'Mixed palette banks in one tile'
assert fg.max() < 64
for target, source in [('beam', 'weapons_power_beam.png'), ('beam_impact', 'weapons_impact_r00.png')]:
    original = Image.open(root / 'res/images/weapons-original-sgdk' / source)
    if target == 'beam_impact': original = original.crop((0,0,80,16))
    converted = Image.open(root / f'res/images/player/{target}.png')
    a = np.array(original.convert('RGB'))
    b = np.array(converted.convert('RGB'))
    visible = np.any(a != [0,136,255], axis=2)
    assert np.array_equal((a//32)[visible], (b//32)[visible]), 'Changed beam hardware colors'
    assert np.array_equal(np.array(converted) != 0, visible), 'Changed beam transparency'
print('PASS four palette banks and unchanged beam/impact hardware colors')
