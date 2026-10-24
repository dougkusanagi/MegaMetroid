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

# A door must not paint black outside its shape or leave a closed cap in the map.
from sys import path as module_path
module_path.insert(0, str(root/'scripts'))
from build_door_cap import COLORS
source = np.array(Image.open(root/'res/images/doors-original-sgdk/doors_door_r00.png'))[:,:160]
converted = np.array(Image.open(root/'res/images/doors/door_cap.png'))
assert converted.shape == (64,160)
assert np.array_equal(converted, np.vectorize(COLORS.__getitem__)(source))
assert np.array_equal(converted == 0, source == 0), 'Door transparency was replaced with opaque black'
original = np.array(Image.open(root/'tests/reference/super_metroid/landing_site/foreground.png').convert('RGB'))
for x,y in [(0,608),(0,1120),(2272,352),(2272,1120)]:
    sky = np.all(original[y:y+64,x:x+32] == [53,133,186],axis=2)
    assert sky.any() and np.all(fg[y:y+64,x:x+32][sky] == 0), 'Door baked into original sky underlay'
    cap = np.isin(source[:,:32], [3,7,9])
    if x: cap = cap[:,::-1]
    assert np.all(fg[y:y+64,x:x+32][cap] == 0), 'Closed cap remains baked in map'
print('PASS full door frames, transparent masks and preserved room underlay')
