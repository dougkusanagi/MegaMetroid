"""Load and validate rooms/world.json, the Crateria room graph (etapa 6.5)."""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
OPPOSITE = {'right': 'left', 'left': 'right', 'bottom': 'top', 'top': 'bottom'}
START = '91F8'


class WorldError(Exception):
    pass


def need(cond, message):
    if not cond:
        raise WorldError(message)


def load(root=ROOT):
    return json.loads((root / 'rooms/world.json').read_text(encoding='utf-8'))


def nodes_by_id(world):
    return {n['id']: n for n in world['nodes']}


def validate(world):
    """Structural invariants; returns the nodes by id."""
    nodes = nodes_by_id(world)
    need(len(nodes) == len(world['nodes']), 'duplicate room ids')
    need(world['nodes'][0]['id'] == START, 'the Landing Site must be node 0')
    size = world['screen_blocks']
    for n in world['nodes']:
        w, h = n['screens'][0] * size, n['screens'][1] * size
        for i, d in enumerate(n['doors']):
            where = f'{n["id"]} door {i}'
            need(d['wall'] in OPPOSITE, f'{where}: bad wall')
            if d['to'] is None:
                need(d['peer'] is None and d['at'] is None and d['leaves']['area'] != world['area'],
                     f'{where}: a door out of the area must have no peer')
                continue
            need(d['to'] in nodes, f'{where}: leads to unknown room {d["to"]}')
            other = nodes[d['to']]
            back = other['doors'][d['peer']]
            need(back['to'] == n['id'] and back['peer'] == i, f'{where}: peer is not mutual')
            need(back['wall'] == OPPOSITE[d['wall']], f'{where}: peer is not on the opposite wall')
            need(d['arrive'] == back['at'] and d['at'] == back['arrive'], f'{where}: arrival does not match the peer')
            # (0,0) marks the two doorless transitions of the original (no door cap).
            for (x, y), room, name in ((d['at'], n, 'door'), (d['arrive'], other, 'arrival')):
                rw, rh = room['screens'][0] * size, room['screens'][1] * size
                need(0 <= x < rw and 0 <= y < rh, f'{where}: {name} {x},{y} outside {room["id"]}')
            need(w > 0 and h > 0, f'{where}: empty room')
    return nodes


def reachable(world, start=START):
    """Rooms reachable from start without leaving the area, in discovery order."""
    nodes = nodes_by_id(world)
    order, seen = [start], {start}
    for room in order:
        for d in nodes[room]['doors']:
            if d['to'] and d['to'] not in seen:
                seen.add(d['to'])
                order.append(d['to'])
    return order
