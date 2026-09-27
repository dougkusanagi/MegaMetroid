"""Extract actual level collision fixtures; validate dimensions before C tests."""
from pathlib import Path
import re
import struct

root = Path(__file__).resolve().parents[1]
fixtures = []
for index, rows, cols, image in [(1,156,288,'origina-smallerl-no-sprites-v2.png'), (2,160,160,'tilemap2.png')]:
    source = (root / f'src/map_crateria_{index}.c').read_text()
    match = re.search(r'const u8 map_collision_crateria_\d+\[\d+\]\[\d+\] = (\{.*?\});', source, re.S)
    assert match, 'Missing collision array'
    data_rows = re.findall(r'\{([^{}]+)\}', match[1])
    assert len(data_rows) == rows
    assert all(len(re.findall(r'\d+', row)) == cols for row in data_rows)
    assert all(int(n) in range(4) for n in re.findall(r'\d+', match[1]))
    png = (root / 'res/images/maps/crateria' / image).read_bytes()
    assert struct.unpack('>II', png[16:24]) == (cols * 8, rows * 8)
    fixtures.append(f'static const unsigned char real_map_{index}[{rows}][{cols}] = {match[1]};')
(root / 'tests/out/maps.h').write_text('\n'.join(fixtures))
print('PASS real level dimensions, tile values and image alignment')
