"""World graph (etapa 6.5): rooms/world.json is consistent and agrees with the Tiled rooms."""
from pathlib import Path
import copy
import json
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import world_graph as wg  # noqa: E402

world = wg.load()
nodes = wg.validate(world)

# Crateria as extracted: 34 rooms, 29 of them connected to the Landing Site by Crateria doors alone;
# the other five (Forgotten Highway) are reached from the Wrecked Ship and Maridia.
assert len(world['nodes']) == 34
walk = wg.reachable(world)
assert len(walk) == 29 and walk[0] == '91F8'
assert sorted(set(nodes) - set(walk)) == ['94CC', '94FD', '9552', '957D', '95A8']
assert sum(1 for n in world['nodes'] for d in n['doors'] if d['to'] is None) == 12  # exits to other regions

# The Landing Site has the four doors the port draws, in the places the port draws them.
site = nodes['91F8']
assert site['name'] == 'Landing Site' and site['screens'] == [9, 5]
assert [(d['wall'], d['at'], d['to']) for d in site['doors']] == [
    ('left', [1, 70], '92FD'), ('right', [142, 70], '95D4'), ('left', [1, 38], '92B3'), ('right', [142, 22], '93AA')]
tmj = json.loads((ROOT / 'rooms/crateria_1.tmj').read_text())
placed = next(l for l in tmj['layers'] if l['name'] == 'doors')['objects']
cap = sorted((o['x'] // 16, o['y'] // 16) for o in placed)
expected = sorted((d['at'][0] - (1 if d['wall'] == 'left' else 0), d['at'][1]) for d in site['doors'])
assert cap == expected, (cap, expected)


def rejects(mutate, text):
    doc = copy.deepcopy(world)
    mutate(doc)
    try:
        wg.validate(doc)
    except wg.WorldError as error:
        assert text in str(error), f'{error} does not mention {text!r}'
        return
    raise AssertionError(f'accepted invalid world ({text})')


rejects(lambda w: w['nodes'][0]['doors'][0].update(to='ZZZZ'), 'unknown room')
rejects(lambda w: w['nodes'][0]['doors'][0].update(peer=0), 'peer is not mutual')
rejects(lambda w: w['nodes'][0]['doors'][0].update(wall='right'), 'opposite wall')
rejects(lambda w: w['nodes'][0]['doors'][0].update(at=[500, 1]), 'does not match')
rejects(lambda w: w['nodes'].append(copy.deepcopy(w['nodes'][1])), 'duplicate')
print('PASS world graph: 34 rooms, 29 connected to the Landing Site')
