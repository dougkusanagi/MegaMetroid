"""Room exporter (etapa 6.2): committed sources match rooms/*.tmj, and edits regenerate correctly."""
from pathlib import Path
import contextlib
import io
import json
import re
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import export_rooms as ex  # noqa: E402

# 1. The generated files in the repository are exactly what the maps produce.
assert ex.main(['--check']) == 0  # also run by hand: python scripts/export_rooms.py --check

rooms = ex.load_all()
assert [r['id'] for r in rooms] == ['crateria_1', 'crateria_2']
assert (rooms[0]['cols'], rooms[0]['rows'], rooms[1]['cols'], rooms[1]['rows']) == (288, 156, 160, 160)
assert rooms[0]['scroll'] == 512 and rooms[1]['scroll'] == 0
assert len(rooms[0]['doors']) == 4 and len(rooms[0]['zones']) == 2


def scratch():
    """A copy of the exporter inputs with only the images the maps reference."""
    tmp = Path(tempfile.mkdtemp(prefix='check_rooms_'))
    shutil.copytree(ROOT / 'rooms', tmp / 'rooms')
    for r in rooms:
        for key in ('fg_image', 'bg_image'):
            dest = tmp / 'res/images' / r['images'][key]
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(ROOT / 'res/images' / r['images'][key], dest)
    return tmp


def load(tmp, rid):
    return json.loads((tmp / f'rooms/{rid}.tmj').read_text())


def save(tmp, rid, doc):
    (tmp / f'rooms/{rid}.tmj').write_text(ex.dump_tmj(doc), newline='\n')


def layer(doc, name):
    return next(l for l in doc['layers'] if l['name'] == name)


def rejects(tmp, text):
    try:
        ex.generate(tmp)
    except ex.RoomError as error:
        assert text in str(error), f'{error!r} does not mention {text!r}'
        return
    raise AssertionError(f'accepted invalid map ({text})')


def cells(source):
    return [[int(n) for n in re.findall(r'\d+', row)]
            for row in re.findall(r'^  \{([0-9, ]+)\},$', source, re.M)]


tmp = scratch()
try:
    base = ex.generate(tmp)
    assert base == ex.generate(ROOT)

    # 2. Painting one tile in the editor changes exactly that cell of the C array.
    doc = load(tmp, 'crateria_2')
    data = layer(doc, 'collision')['data']
    index = 5 * 160 + 7
    data[index] = 3 if data[index] != 3 else 1
    save(tmp, 'crateria_2', doc)
    edited = ex.generate(tmp)
    before, after = cells(base['src/map_crateria_2.c']), cells(edited['src/map_crateria_2.c'])
    changed = [(y, x) for y in range(160) for x in range(160) if before[y][x] != after[y][x]]
    assert changed == [(5, 7)] and after[5][7] == data[index] - 1
    assert {k for k in edited if edited[k] != base[k]} == {'src/map_crateria_2.c'}

    # 3. The route of a door comes from the world graph: Landing Site east door <-> Crateria 2 west door.
    c1 = base['src/map_crateria_1.c']
    assert '{{2272, 352}, ROOM_CRATERIA_2, {24, 112}, ROOM_DOOR_BLUE, ROOM_DOOR_FACING_LEFT}' in c1
    assert c1.count('ROOM_NO_CONNECTION') == 3  # the three doors whose far rooms are not built
    assert '{{0, 96}, ROOM_CRATERIA_1, {2240, 368}, ROOM_DOOR_BLUE, ROOM_DOOR_FACING_RIGHT}' in base['src/map_crateria_2.c']

    # 4. A door without a link, an enemy and a camera zone appear in the tables.
    doc = load(tmp, 'crateria_2')
    next_id = doc['nextobjectid']
    layer(doc, 'doors')['objects'].append({
        'id': next_id, 'name': '', 'type': 'door', 'x': 64, 'y': 128, 'width': 32, 'height': 64, 'rotation': 0, 'visible': True,
        'properties': [{'name': 'color', 'type': 'string', 'value': 'red'},
                       {'name': 'facing', 'type': 'string', 'value': 'up'}]})
    layer(doc, 'enemies')['objects'].append({
        'id': next_id + 1, 'name': '', 'type': 'enemy', 'x': 200, 'y': 300, 'width': 0, 'height': 0, 'rotation': 0,
        'visible': True, 'point': True, 'properties': [{'name': 'kind', 'type': 'int', 'value': 2}]})
    layer(doc, 'camera')['objects'].append({
        'id': next_id + 2, 'name': '', 'type': 'camera_zone', 'x': 0, 'y': 0, 'width': 256, 'height': 128, 'rotation': 0,
        'visible': True, 'properties': [{'name': f'bounds_{s}', 'type': 'int', 'value': v}
                                        for s, v in zip(('left', 'top', 'right', 'bottom'), (0, 0, 512, 256))]})
    save(tmp, 'crateria_2', doc)
    source = ex.generate(tmp)['src/map_crateria_2.c']
    assert '{{64, 128}, ROOM_NO_CONNECTION, {0, 0}, ROOM_DOOR_RED, ROOM_DOOR_FACING_UP}' in source
    assert '{2, {200, 300}}' in source
    assert '{{0, 0, 256, 128}, {0, 0, 512, 256}}' in source
    assert '.door_count = sizeof(crateria_2_doors)' in source and '.enemy_spawn_count = sizeof(crateria_2_enemies)' in source

    # 5. Tiled may rewrite the data array on one line; the content is what counts.
    compact = json.loads(json.dumps(doc))
    (tmp / 'rooms/crateria_2.tmj').write_text(json.dumps(compact), newline='\n')
    assert ex.generate(tmp)['src/map_crateria_2.c'] == source
    save(tmp, 'crateria_2', load(tmp, 'crateria_2'))
    assert ex.generate(tmp)['src/map_crateria_2.c'] == source

    # 6. Invalid maps stop the export instead of producing a broken room.
    good = load(tmp, 'crateria_2')

    def broken(mutate, text):
        doc = json.loads(json.dumps(good))
        mutate(doc)
        save(tmp, 'crateria_2', doc)
        rejects(tmp, text)

    broken(lambda d: layer(d, 'collision')['data'].__setitem__(0, 0), 'empty or not a collision tile')
    broken(lambda d: layer(d, 'collision')['data'].__setitem__(0, 9), 'empty or not a collision tile')
    broken(lambda d: layer(d, 'collision')['data'].pop(), 'does not cover the whole map')
    broken(lambda d: layer(d, 'collision').update(encoding='base64'), 'CSV')
    broken(lambda d: layer(d, 'spawn')['objects'][0].update(x=99999), 'spawn outside')
    broken(lambda d: layer(d, 'spawn')['objects'].clear(), 'exactly one point')
    broken(lambda d: layer(d, 'doors')['objects'][0]['properties'][2].update(value=7), 'above 0')
    broken(lambda d: layer(d, 'doors')['objects'][0]['properties'][1].update(value='left'), 'is on the left wall')
    broken(lambda d: layer(d, 'doors')['objects'][0].update(y=320), 'belongs at block (0, 6)')
    broken(lambda d: layer(d, 'doors')['objects'].clear(), 'have no door in the map')
    broken(lambda d: layer(d, 'doors')['objects'].append(json.loads(json.dumps(layer(d, 'doors')['objects'][0]))),
           'share one link')
    broken(lambda d: [p.update(value='ZZZZ') for p in d['properties'] if p['name'] == 'world_node'], 'is not a room in rooms/world.json')
    twins = [dict(id='a', node='91F8', doors=[]), dict(id='b', node='91F8', doors=[])]
    try:
        ex.resolve_routes(twins, ex.world_graph.load())
        raise AssertionError('two rooms accepted for one world_node')
    except ex.RoomError as error:
        assert 'is also used by a' in str(error)
    broken(lambda d: layer(d, 'doors')['objects'][0]['properties'][0].update(value='pink'), 'door color')
    broken(lambda d: layer(d, 'doors')['objects'][0].update(x=1270), 'outside the map')
    broken(lambda d: layer(d, 'enemies')['objects'][0]['properties'][0].update(value=70000), 'above 65535')
    broken(lambda d: layer(d, 'camera')['objects'][0].update(width=0), 'trigger is empty')
    broken(lambda d: d.update(properties=[p for p in d['properties'] if p['name'] != 'bg_image']), 'bg_image')
    broken(lambda d: d.update(width=159), 'does not cover the whole map')
    broken(lambda d: d['tilesets'][0].update(firstgid=2), 'collision.tsj')
    broken(lambda d: d.update(layers=[l for l in d['layers'] if l['name'] != 'doors']), 'missing objectgroup "doors"')

    save(tmp, 'crateria_2', good)
    (tmp / 'rooms/world.json').write_text('{"nodes": []}', newline=chr(10))
    rejects(tmp, 'world.json')
    shutil.copy(ROOT / 'rooms/world.json', tmp / 'rooms/world.json')
    shutil.copy(tmp / 'rooms/crateria_2.tmj', tmp / 'rooms/extra_room.tmj')
    rejects(tmp, 'not listed')
    (tmp / 'rooms/extra_room.tmj').unlink()

    # 7. --check notices a hand edit of a generated file (and the write mode repairs it).
    with contextlib.redirect_stdout(io.StringIO()):
        ex.main(['--root', str(tmp)])
    target = tmp / 'src/map_crateria_1.c'
    target.write_text(target.read_text().replace('2304', '2305'), newline='\n')
    with contextlib.redirect_stderr(io.StringIO()), contextlib.redirect_stdout(io.StringIO()):
        assert ex.main(['--root', str(tmp), '--check']) == 1
    with contextlib.redirect_stdout(io.StringIO()):
        ex.main(['--root', str(tmp)])
        assert ex.main(['--root', str(tmp), '--check']) == 0
finally:
    shutil.rmtree(tmp, ignore_errors=True)

print('PASS room exporter: generated files up to date, edits regenerate, invalid maps rejected')
