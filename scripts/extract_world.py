"""Extract the Crateria room graph of Super Metroid into rooms/world.json (etapa 6.5).

Reads only the room headers (bank $8F) and door tables (bank $83) of the original ROM:
size in screens, position on the area map and, per door, where it leads. No ROM bytes,
art or collision are stored. Walks the doors from the Landing Site through every area, so
a Crateria room that is only reachable from another region is still found.

Door facts, from the original data model: a door entry belongs to the room Samus leaves
and describes the *arrival* (the door cap in the destination room). So the position of a
door in its own room is the arrival cap of the matching door in the destination room
("peer"). Several doors between the same pair of rooms are matched in order along the wall.

  python scripts/extract_world.py [--rom file.sfc|file.zip]   write rooms/world.json
  python scripts/extract_world.py --check [--rom ...]         fail if rooms/world.json is stale

The ROM is only needed to regenerate; tests/check_world.py validates the committed file.
"""
from pathlib import Path
import argparse
import json
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = Path('D:/roms/snes/Super Metroid (Japan, USA) (En,Ja).zip')
AREAS = ['Crateria', 'Brinstar', 'Norfair', 'Wrecked Ship', 'Maridia', 'Tourian', 'Ceres', 'Debug']
START = 0x91F8
CRATERIA = 0
NAMES = {0x91F8: 'Landing Site'}  # Only names that are certain; the others stay identified by address.
# Travel direction of a door entry (low two bits): the wall of the leaving room the door is on.
WALLS = ['right', 'left', 'bottom', 'top']
OPPOSITE = {'right': 'left', 'left': 'right', 'bottom': 'top', 'top': 'bottom'}
ELEVATOR_FLAG = 0x40


class WorldError(Exception):
    pass


def need(cond, message):
    if not cond:
        raise WorldError(message)


def load_rom(path):
    path = Path(path)
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as z:
            names = [n for n in z.namelist() if n.lower().endswith(('.sfc', '.smc'))]
            need(names, f'{path}: no .sfc/.smc inside')
            data = z.read(names[0])
    else:
        data = path.read_bytes()
    need(len(data) % 0x8000 == 0, f'{path}: size {len(data)} is not a multiple of 32 KB (copier header?)')
    need(len(data) >= 0x300000, f'{path}: too small for Super Metroid')
    return data


def pc(bank, addr):
    return ((bank & 0x7F) << 15) | (addr & 0x7FFF)


class Rom:
    def __init__(self, data):
        self.data = data

    def room(self, addr):
        o = pc(0x8F, addr)
        index, area, mx, my, w, h, _up, _down, _cre, door_list = struct.unpack('<BBBBBBBBBH', self.data[o:o + 11])
        return dict(addr=addr, index=index, area=area, map=[mx, my], screens=[w, h], door_list=door_list)

    def plausible(self, addr):
        if not 0x8000 <= addr < 0x10000:
            return False
        r = self.room(addr)
        return r['area'] < len(AREAS) and 1 <= r['screens'][0] <= 32 and 1 <= r['screens'][1] <= 32

    def doors(self, room):
        """Door entries of a room. The list has no stored length: stop at the first entry that is
        not a door table pointer to a plausible room (checked afterwards by symmetry)."""
        out = []
        o = pc(0x8F, room['door_list'])
        while True:
            ptr = struct.unpack('<H', self.data[o:o + 2])[0]
            if not 0x8000 <= ptr < 0x10000 - 12:
                break
            d = self.data[pc(0x83, ptr):pc(0x83, ptr) + 12]
            dest = struct.unpack('<H', d[:2])[0]
            if not self.plausible(dest):
                break
            direction = d[3]
            if direction & 0x78:
                break
            out.append(dict(dest=dest, flags=d[2], travel=WALLS[direction & 3], close=bool(direction & 4),
                            cap=[d[4], d[5]], screen=[d[6], d[7]]))
            o += 2
        return out


def walk(rom):
    """Every room reachable from the Landing Site through any area: addr -> room with doors."""
    seen, pending = {}, [START]
    while pending:
        addr = pending.pop()
        if addr in seen:
            continue
        room = rom.room(addr)
        room['doors'] = rom.doors(room)
        seen[addr] = room
        pending += [d['dest'] for d in room['doors']]
    return seen


def pair_doors(rooms):
    """Fill peer (index of the matching door in the destination room) and at (cap in this room)."""
    for a in rooms.values():
        for d in a['doors']:
            d['peer'] = None
    for a in rooms.values():
        if a['area'] != CRATERIA:
            continue
        groups = {}
        for i, d in enumerate(a['doors']):
            b = rooms[d['dest']]
            if b['area'] == CRATERIA:
                groups.setdefault((d['dest'], d['travel']), []).append(i)
        for (dest, travel), mine in groups.items():
            back = [j for j, e in enumerate(rooms[dest]['doors'])
                    if e['dest'] == a['addr'] and e['travel'] == OPPOSITE[travel]]
            need(len(mine) == len(back),
                 f'{a["addr"]:04X}->{dest:04X} {travel}: {len(mine)} doors but {len(back)} on the way back')
            axis = 1 if travel in ('right', 'left') else 0  # coordinate along the wall
            mine.sort(key=lambda i: a['doors'][i]['cap'][axis])
            back.sort(key=lambda j: rooms[dest]['doors'][j]['cap'][axis])
            for i, j in zip(mine, back):
                a['doors'][i]['peer'] = j


def build(rom):
    rooms = walk(rom)
    pair_doors(rooms)
    nodes = sorted((r for r in rooms.values() if r['area'] == CRATERIA), key=lambda r: r['addr'])
    need(nodes and nodes[0]['addr'] == START, 'the Landing Site must be the first node')
    out = []
    for r in nodes:
        doors = []
        for d in r['doors']:
            dest = rooms[d['dest']]
            entry = dict(wall=d['travel'], close=d['close'], flags=d['flags'])
            if dest['area'] == CRATERIA:
                peer = dest['doors'][d['peer']]
                entry.update(at=peer['cap'], to=f'{d["dest"]:04X}', peer=d['peer'], arrive=d['cap'],
                             arrive_screen=d['screen'])
            else:
                entry.update(at=None, to=None, peer=None, arrive=d['cap'], arrive_screen=d['screen'],
                             leaves=dict(area=AREAS[dest['area']], room=f'{d["dest"]:04X}'))
            doors.append(entry)
        node = dict(id=f'{r["addr"]:04X}', screens=r['screens'], map=r['map'], doors=doors)
        if r['addr'] in NAMES:
            node['name'] = NAMES[r['addr']]
        out.append(node)
    return dict(
        source='Super Metroid (Japan, USA) room headers (bank $8F) and door tables (bank $83), '
               'decoded by scripts/extract_world.py. Layout facts only; no ROM data, art or collision.',
        area='Crateria', block=16, screen_blocks=16, nodes=out)


def dump(doc):
    """One door per line: compact, and a changed door shows up as a one-line diff."""
    lines = ['{']
    for key in ('source', 'area', 'block', 'screen_blocks'):
        lines.append(f' {json.dumps(key)}: {json.dumps(doc[key])},')
    lines.append(' "nodes": [')
    nodes = []
    for n in doc['nodes']:
        head = {k: v for k, v in n.items() if k != 'doors'}
        text = '  {' + ', '.join(f'{json.dumps(k)}: {json.dumps(v)}' for k, v in head.items()) + ', "doors": ['
        rows = ['   ' + json.dumps(d) for d in n['doors']]
        nodes.append(text + '\n' + ',\n'.join(rows) + '\n  ]}')
    lines.append(',\n'.join(nodes))
    lines += [' ]', '}']
    return '\n'.join(lines) + '\n'


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    ap.add_argument('--check', action='store_true')
    ap.add_argument('--root', type=Path, default=ROOT)
    args = ap.parse_args(argv)
    try:
        text = dump(build(Rom(load_rom(args.rom))))
    except (WorldError, OSError) as error:
        print(f'extract_world: {error}', file=sys.stderr)
        return 1
    target = args.root / 'rooms/world.json'
    current = target.read_bytes().decode('utf-8').replace('\r\n', '\n') if target.exists() else None
    if args.check:
        if current != text:
            print('extract_world: rooms/world.json is out of date', file=sys.stderr)
            return 1
        print('PASS rooms/world.json matches the ROM')
        return 0
    if current != text:
        with open(target, 'w', encoding='utf-8', newline='\n') as f:
            f.write(text)
    print(f'extract_world: {len(json.loads(text)["nodes"])} Crateria rooms -> {target.relative_to(args.root)}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
