"""Converte os mapas SNES de referencia para PNG indexado com paleta Mega Drive.

Mapeamento cor-a-cor (pixels intactos): cada cor do SNES vira uma cor distinta da
paleta de 512 cores do MD (niveis medidos 0,52,87,116,144,172,206,255). As cores
SNES de Crateria sao muito escuras (ex.: 8,32,24) e o MD so tem 8 niveis por canal,
entao os tons foram escolhidos a mao (ajudados por tools/genesis_adapt.py) para
manter a ordem de luminosidade, o matiz e o contraste entre vizinhos (dither, "+").
Indice 0 = transparente (ceu / vazio).

Uso: python tools/make_md_assets.py
"""
import numpy as np
from PIL import Image

D = 'res/images/maps/crateria/'
KEY = (53, 133, 186)  # cor de "ceu" nos PNGs de referencia

# FG: SNES (R,G,B 8 bits) -> Mega Drive
FG = {
    (0, 0, 0):      (0, 0, 0),       # preto
    (0, 0, 8):      (0, 0, 0),       # interior das cavernas (quase preto no original)
    (8, 8, 16):     (0, 0, 52),      # cavernas: relevo
    (0, 8, 8):      (0, 52, 0),      # rocha: dither escuro
    (8, 32, 24):    (0, 52, 52),     # rocha: corpo
    (24, 24, 32):   (52, 52, 52),    # contorno cinza escuro
    (32, 32, 40):   (52, 52, 87),    # contorno cinza
    (24, 40, 24):   (52, 52, 0),     # musgo escuro
    (40, 48, 80):   (52, 52, 116),   # rocha: "+" indigo
    (48, 64, 24):   (52, 87, 0),     # grama
    (72, 64, 88):   (87, 87, 116),   # borda lilas
    (112, 64, 88):  (144, 52, 87),   # borda rosada
    (72, 96, 56):   (87, 116, 52),   # metal / vegetacao
    (96, 128, 96):  (116, 172, 116), # metal claro
    # so na sala 2
    (64, 64, 64):   (87, 87, 87),
    (248, 248, 248): (255, 255, 255),
}

# BG da sala 2: mais escuro que o FG, para separar os planos
BG2 = {
    (0, 8, 8):      (0, 0, 0),       # base (indice 0 = transparente sobre o backdrop preto)
    (8, 32, 24):    (0, 52, 0),
    (40, 48, 80):   (0, 0, 87),
    (72, 64, 88):   (52, 52, 87),
}

def to_indexed(rgba, mapping):
    """RGBA -> imagem 'P'; alfa 0 ou cor KEY -> indice 0; cores do mapping a partir do 1
    (ou no 0 se a cor mapeada for o preto da base, como no BG2)."""
    flat = rgba.reshape(-1, 4)
    rgb = flat[:, :3]
    clear = (flat[:, 3] == 0) | (rgb == KEY).all(1)
    out = np.zeros(len(flat), np.uint8)
    pal = [(0, 0, 0)]
    done = clear.copy()
    for snes, md in mapping.items():
        m = (rgb == snes).all(1) & ~clear
        if not m.any():
            continue
        if md == (0, 0, 0) and mapping is BG2:
            done |= m
            continue
        pal.append(md)
        out[m] = len(pal) - 1
        done |= m
    if not done.all():
        raise SystemExit('cores sem mapeamento: %s' % sorted({tuple(c) for c in rgb[~done]}))
    h, w = rgba.shape[:2]
    im = Image.fromarray(out.reshape(h, w), 'P')
    flat_pal = [c for p in pal for c in p]
    assert len(pal) <= 16, 'mais de 16 cores'
    im.putpalette(flat_pal + [0] * (48 - len(flat_pal)))  # 16 entradas -> PNG de 4 bits
    return im

def load(name, crop=None):
    im = Image.open(D + name).convert('RGBA')
    return np.asarray(im.crop(crop) if crop else im)

if __name__ == '__main__':
    to_indexed(load('original-whithout-sprites.png', (0, 1, 2304, 1249)), FG) \
        .save(D + 'crateria1-fg-md.png', transparency=0)
    to_indexed(load('tilemap2.png'), FG).save(D + 'crateria2-fg-md.png', transparency=0)
    to_indexed(load('bg-2.bmp'), BG2).save(D + 'crateria2-bg-md.png')
    print('ok')
