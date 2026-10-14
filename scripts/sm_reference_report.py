"""Resume as medidas do Super Metroid original gravadas por scripts/sm_reference.ps1.

Uso: python scripts/sm_reference_report.py [tests/out/sm_reference.csv]
Velocidades em px/quadro (60 Hz, o mesmo ritmo do tick do port).
"""
import csv
import sys
from collections import defaultdict

AIR = {'2', '3', '4', '6', '8', '20'}  # movement type: pulo, giro, bola no ar, queda, pulo de parede


def load(path):
    scenarios = defaultdict(list)
    with open(path, newline='') as f:
        for row in csv.DictReader(f):
            scenarios[row['scenario']].append(row)
    return scenarios


def x(r):
    return int(r['x']) + int(r['xsub']) / 65536


def y(r):
    return int(r['y']) + int(r['ysub']) / 65536


def vy(r):
    speed = int(r['vy']) + int(r['vysub']) / 65536
    return -speed if r['ydir'] == '1' else speed


def steps(rows, first):
    return [r for r in rows if int(r['step']) >= first]


def dx(rows):
    return [round(x(b) - x(a), 4) for a, b in zip(rows, rows[1:])]


def runs(values):
    """Sequência de (valor, repetições)."""
    out = []
    for v in values:
        if out and out[-1][0] == v:
            out[-1][1] += 1
        else:
            out.append([v, 1])
    return out


def pose_runs(rows):
    return ' '.join(f"{int(p):02X}x{n}" for p, n in runs([r['pose'] for r in rows]))


def jump(rows):
    top = min(rows, key=y)
    air = [r for r in rows if r['movement'] in AIR]
    return y(rows[0]) - y(top), rows.index(top), len(air)


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    s = load(sys.argv[1] if len(sys.argv) > 1 else 'tests/out/sm_reference.csv')
    print('## Chão')
    walk = steps(s['walk_stop'], 3)
    start = [v for v in dx([r for r in walk if r['step'] == '3'])]
    print('caminhada, dx por quadro ao sair do repouso:', start[:14])
    stop = [r for r in walk if r['step'] == '4']
    print(f"soltar o direcional: desliza {x(stop[-1]) - x(walk[[r['step'] for r in walk].index('4') - 1]):.2f} px,"
          f" dx {dx([walk[[r['step'] for r in walk].index('4') - 1]] + stop)[:8]}")
    print('virar (poses):', pose_runs(steps(s['turn'], 3))[:60])
    extra = [int(r['vx_extra']) + int(r['vx_extrasub']) / 65536 for r in s['run_left']]
    first = next(i for i, e in enumerate(extra) if e > 0)
    full = extra.index(max(extra))
    print(f"corrida: bônus +{extra[first]:.4f} por quadro, {full - first + 1} quadros até +{max(extra):.2f}")
    roll = [v for v in dx(steps(s['morph_roll'], 5)) if v]
    print('bola rolando, dx:', roll[:8])

    print('## Pulo')
    height, peak, air = jump(steps(s['jump_stand'], 3))
    rows = steps(s['jump_stand'], 3)
    gravity = vy(rows[3]) - vy(rows[2])
    print(f"parado: saída {vy(rows[0]):.4f}, gravidade {gravity:.6f}, altura {height:.2f} px no quadro {peak}, {air} quadros no ar")
    print('  poses:', pose_runs(rows))
    fall = max(vy(r) for r in s['fall_from_ship'])
    print(f"queda máxima {fall:.4f}")
    holds = []
    for n in range(1, 31):
        h, _, a = jump(steps(s[f'jump_hold_{n}'], 3))
        holds.append(f"{n}:{h:.1f}")
    print('altura por quadros segurando o pulo:', ' '.join(holds))
    release = steps(s['jump_hold_10'], 4)
    print('vy depois de soltar o pulo:', [round(vy(r), 4) for r in release[:3]])
    for name, first in (('jump_then_right', 3), ('spin_right', 3), ('walk_spin', 4), ('run_jump', 2)):
        rows = steps(s[name], first)
        air_dx = [abs(v) for v, r in zip(dx(rows), rows[1:]) if r['movement'] in AIR]
        print(f"{name}: dx no ar {sorted(set(air_dx))[-3:]}, poses {pose_runs(rows)[:50]}")
    wall = steps(s['wall_jump'], 6)
    print(f"pulo de parede: saída {vy(wall[0]):.4f}, dx {dx(wall)[:6]}, poses {pose_runs(wall)[:30]}")

    print('## Agachar e bola')
    rows = steps(s['crouch_morph'], 3)
    print('↓, ↓, ↑, ↑:', ' '.join(f"{int(k[0]):02X}(r{k[1]},{k[2]})x{n}"
                                  for k, n in runs([(r['pose'], r['xradius'], r['yradius']) for r in rows])))
    print('↓ segurado 30 quadros e solto:', pose_runs(steps(s['down_hold'], 3)))
    print('↓ uma vez no ar:', pose_runs(steps(s['morph_air'], 4)))
    print('↓ duas vezes no ar:', pose_runs(steps(s['morph_air_twice'], 4)))

    print('## Tiro')
    rows = steps(s['shoot_single'], 3)
    xs = [float(r['shots'].split(':')[0]) for r in rows if r['shots']]
    print('feixe, dx por quadro:', [round(b - a, 4) for a, b in zip(xs, xs[1:])][:28])
    for name in ('shoot_hold', 'shoot_mash'):
        rows = steps(s[name], 3)
        shots, previous = [], 0
        for i, r in enumerate(rows):
            if int(r['cooldown']) > previous:
                shots.append(i)
            previous = int(r['cooldown'])
        print(f"{name}: tiros nos quadros {shots}, até {max(int(r['projectiles']) for r in rows)} ao mesmo tempo")


if __name__ == '__main__':
    main()
