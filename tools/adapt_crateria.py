"""Gera variantes do mapa de Crateria 1 com paleta MD otimizada e um painel de comparacao."""
import sys, numpy as np
from PIL import Image
sys.path.insert(0, 'tools')
from genesis_adapt import optimize, adjacency

D = 'res/images/maps/crateria/'
T = sys.argv[1] if len(sys.argv) > 1 else '.'
src = np.array(Image.open(D + 'original-whithout-sprites.png').convert('RGB'))[1:1249]
KEY = (53, 133, 186)
flat = src.reshape(-1, 3)
cols_all, inv, counts = np.unique(flat, axis=0, return_inverse=True, return_counts=True)
inv = inv.reshape(src.shape[:2])
cols = [tuple(c) for c in cols_all]
key_i = cols.index(KEY)
keep = [i for i in range(len(cols)) if i != key_i]
remap = {o: n for n, o in enumerate(keep)}
idx = np.full(inv.shape, -1)
for o, n in remap.items(): idx[inv == o] = n
ccols = [cols[i] for i in keep]; ccnt = counts[keep].astype(float)
adj = adjacency(np.where(idx < 0, len(ccols), idx), len(ccols) + 1, len(ccols))[:-1, :-1]
print(len(ccols), 'cores', ccols)

variants = {}
def gi(r, g, b):
    L = [0, 52, 87, 116, 144, 172, 206, 255]
    return L.index(r) * 64 + L.index(g) * 8 + L.index(b)
FIX = {1: gi(0, 0, 52), 3: gi(0, 0, 87)}
for name, kw in {'E': dict(gain=1.2, gamma=0.75, chroma=1.5, lam=2.0),
                 'H': dict(gain=1.2, gamma=0.75, chroma=1.5, lam=2.0, fixed=FIX),
                 'I': dict(gain=1.4, gamma=0.75, chroma=1.4, lam=2.0, fixed=FIX, ab_w=4.0),
                 'J': dict(gain=1.0, gamma=0.7, chroma=1.5, lam=2.5, fixed=FIX)}.items():
    pal, c = optimize(ccols, ccnt, adj, iters=40000, seed=1, **kw)
    variants[name] = pal
    print(name, round(c, 3))
    for o, g in zip(ccols, pal): print('   ', tuple(int(x) for x in o), '->', g)

def render(pal, box):
    out = np.zeros(src.shape, np.uint8); out[:] = (24, 24, 40)  # cor do "ceu" so p/ visualizar
    for n in range(len(ccols)):
        out[idx == n] = pal[n]
    return Image.fromarray(out).crop(box)

box = (1640, 640, 1800, 740)
box2 = (60, 1090, 220, 1190)
rows = []
orig = Image.fromarray(src).crop(box)
panels = [('orig', None)] + list(variants.items())
W = 160 * 4
sheet = Image.new('RGB', (W * len(panels), 400 * 2 + 10), (40, 40, 40))
for k, (nm, pal) in enumerate(panels):
    for r, bx in enumerate((box, box2)):
        im = Image.fromarray(src).crop(bx) if pal is None else render(pal, bx)
        sheet.paste(im.resize((640, 400), 0), (k * 640, r * 410))
sheet.save(T + '/variants.png')
import json; json.dump({k: v for k, v in variants.items()}, open(T + '/variants.json', 'w'))
