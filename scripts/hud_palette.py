"""Reserve duplicate/unused PAL0 colors without changing room hardware pixels."""
from PIL import Image
import numpy as np

HUD_COLORS = {1: (0, 0, 0), 2: (32, 32, 128), 3: (224, 224, 224),
              4: (224, 32, 160), 8: (96, 64, 96), 13: (160, 128, 160)}


def reserve(image):
    palette = image.getpalette() + [0] * (768 - len(image.getpalette()))
    pixels = np.array(image)
    before = np.array(image.convert('RGB')) // 32
    if all(tuple(palette[i*3:i*3+3]) == color for i, color in HUD_COLORS.items()):
        return image
    # Landing Site's quantizer already chooses the first of duplicate colors.
    if not any(np.any(pixels == i) for i in (2, 3, 4, 8, 13)):
        pass
    else:
        # Crateria 2: move four terrain entries into unused slots; move its white
        # into the HUD's white slot. Compare at VDP precision before accepting.
        mapping = {2: 12, 3: 14, 4: 15, 8: 11, 11: 3}
        original = pixels.copy()
        old = palette.copy()
        for source, target in mapping.items():
            pixels[original == source] = target
            palette[target*3:target*3+3] = old[source*3:source*3+3]
    for i, color in HUD_COLORS.items():
        palette[i*3:i*3+3] = color
    result = Image.fromarray(pixels).convert('P')
    result.putpalette(palette)
    assert np.array_equal(before, np.array(result.convert('RGB')) // 32), 'HUD reservation changed terrain colors'
    return result
