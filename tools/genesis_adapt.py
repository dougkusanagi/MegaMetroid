"""Adapta paleta SNES -> Mega Drive preservando contraste entre cores vizinhas.

Otimiza (recozimento simulado) a escolha de N cores distintas da paleta de 512
cores do MD (niveis medidos) para que:
  - cores vizinhas na arte mantenham distancia perceptual proporcional a original
    (ganho `gain` compensa a perda de faixa dinamica nos escuros);
  - cada cor fique perto de um alvo "levantado" (gamma/saturacao) da original;
  - a ordem de luminosidade seja mantida.
"""
import itertools, random, sys
import numpy as np
from PIL import Image

LV = np.array([0, 52, 87, 116, 144, 172, 206, 255], float)
GEN = np.array(list(itertools.product(LV, LV, LV)))

def srgb2lin(c):
    c = np.asarray(c, float) / 255.0
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)

M1 = np.array([[0.4122214708, 0.5363325363, 0.0514459929],
               [0.2119034982, 0.6806995451, 0.1073969566],
               [0.0883024619, 0.2817188376, 0.6299787005]])
M2 = np.array([[0.2104542553, 0.7936177850, -0.0040720468],
               [1.9779984951, -2.4285922050, 0.4505937099],
               [0.0259040371, 0.7827717662, -0.8086757660]])

def oklab(rgb):
    return np.cbrt(srgb2lin(rgb) @ M1.T) @ M2.T

GLAB = oklab(GEN)

def adjacency(idx, n, skip):
    """Frequencia de pares vizinhos (4-conexo) de indices diferentes."""
    w = np.zeros((n, n))
    for a, b in ((idx[:, :-1], idx[:, 1:]), (idx[:-1, :], idx[1:, :])):
        m = (a != b) & (a != skip) & (b != skip)
        np.add.at(w, (a[m], b[m]), 1)
    return w + w.T

def optimize(cols, counts, adj, gamma=0.65, chroma=1.6, gain=1.6,
             lam=1.0, order_pen=3.0, iters=60000, seed=0, fixed=None, ab_w=3.0, hue_pen=6.0, chroma_pen=40.0):
    rng = random.Random(seed)
    n = len(cols)
    olab = oklab(np.array(cols, float))
    tgt = olab.copy()
    tgt[:, 0] = np.clip(tgt[:, 0], 0, None) ** gamma * (olab[:, 0].max() ** (1 - gamma))
    tgt[:, 1:] *= chroma
    dorig = np.linalg.norm(olab[:, None] - olab[None], axis=2)
    freq = counts / counts.sum()
    wpair = np.sqrt(adj / adj.sum())
    order = np.argsort(olab[:, 0])
    fixed = fixed or {}

    def cost(a):
        g = GLAB[a]
        dg = np.linalg.norm(g[:, None] - g[None], axis=2)
        c = (wpair * (dg - gain * dorig) ** 2).sum() * 40
        wl = np.array([1.0, ab_w, ab_w])
        c += lam * (np.sqrt(freq)[:, None] * wl * (g - tgt) ** 2).sum() * 40
        # matiz: angulo de (a,b) deve acompanhar o alvo quando ha croma relevante
        ct = np.linalg.norm(tgt[:, 1:], axis=1); cg = np.linalg.norm(g[:, 1:], axis=1)
        cosd = (g[:, 1:] * tgt[:, 1:]).sum(1) / (ct * cg + 1e-6)
        c += hue_pen * (np.clip(ct, 0, 0.06) / 0.06 * np.clip(0.6 - cosd, 0, None) ** 2).sum() * 10
        gl = g[order, 0]
        c += order_pen * np.clip(gl[:-1] - gl[1:], 0, None).sum() * 10
        # croma: neutros do original nao podem virar cores saturadas
        c += chroma_pen * (np.clip(cg - (ct * 1.6 + 0.035), 0, None) ** 2).sum() * 10
        # cores distintas: penaliza distancia minima muito pequena
        dg2 = dg + np.eye(n) * 9
        c += (np.clip(0.06 - dg2, 0, None) ** 2).sum() * 4000
        return c

    # inicial: vizinho mais proximo do alvo
    a = [int(np.argmin(((GLAB - t) ** 2).sum(1))) for t in tgt]
    for k, v in fixed.items(): a[k] = v
    a = np.array(a)
    cur = cost(a); best, bestc = a.copy(), cur
    T0 = cur * 0.05 + 1
    for it in range(iters):
        T = T0 * (1 - it / iters) ** 2 + 1e-6
        i = rng.randrange(n)
        if i in fixed: continue
        b = a.copy()
        if rng.random() < 0.8:  # passo de +-1 nivel num canal
            r, g_, bl = [int(v) for v in np.unravel_index(a[i], (8, 8, 8))]
            ch = rng.randrange(3); d = rng.choice((-1, 1))
            v = [r, g_, bl]; v[ch] += d
            if not all(0 <= x < 8 for x in v): continue
            b[i] = np.ravel_multi_index(v, (8, 8, 8))
        else:
            b[i] = rng.randrange(512)
        if b[i] in b[np.arange(n) != i]: continue
        c = cost(b)
        if c < cur or rng.random() < np.exp((cur - c) / T):
            a, cur = b, c
            if c < bestc: best, bestc = b.copy(), c
    return [tuple(int(v) for v in GEN[j]) for j in best], bestc

if __name__ == '__main__':
    print('modulo de biblioteca; use tools/adapt_crateria.py')
