"""Pack the original beam and its five orange impact frames into shared PAL3."""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
source = root / 'res/images/weapons-original-sgdk'
beam = Image.open(source / 'weapons_power_beam.png')
impact = Image.open(source / 'weapons_impact_r00.png').crop((0, 0, 80, 16))
colors = [tuple(beam.getpalette()[i:i+3]) for i in range(0, 48, 3)]
colors[15] = (255, 255, 255)
palette = [c for color in colors for c in color] + [0] * (768 - 48)
target = root / 'res/images/player'
for name, image in [('beam', beam), ('beam_impact', impact)]:
    rgb = image.convert('RGB')
    result = Image.new('P', image.size)
    result.putpalette(palette)
    result.putdata([colors.index(pixel) for pixel in rgb.get_flattened_data()])
    result.save(target / (name + '.png'))
