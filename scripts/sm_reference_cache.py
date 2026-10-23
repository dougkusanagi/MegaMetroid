"""Preserve Mesen RAM traces for reuse without starting the emulator.

Import: python scripts/sm_reference_cache.py import 'tests/out/*.csv'
Check:  python scripts/sm_reference_cache.py check
"""
import argparse
import csv
from collections import Counter, defaultdict
from datetime import datetime
import glob
import gzip
import hashlib
import io
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'tests/reference/super_metroid'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def import_traces(patterns, rom, state):
    if bool(rom) != bool(state):
        raise SystemExit('Supply both --rom and --state, or neither.')
    chosen = {}
    for pattern in patterns:
        for name in glob.glob(pattern):
            path = Path(name)
            with path.open(newline='') as stream:
                reader = csv.DictReader(stream)
                if not {'scenario', 'pose', 'xradius', 'yradius'} <= set(reader.fieldnames or []):
                    continue
                groups = defaultdict(list)
                for row in reader:
                    groups[row['scenario']].append(row)
                for scenario, rows in groups.items():
                    if scenario not in chosen or path.stat().st_mtime > chosen[scenario][0].stat().st_mtime:
                        chosen[scenario] = (path, reader.fieldnames, rows)
    if not chosen:
        raise SystemExit('No Mesen traces found.')
    DEST.mkdir(parents=True, exist_ok=True)
    manifest_path = DEST / 'manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8')) if manifest_path.exists() else {
        'schema': 1, 'emulator': 'Mesen 2.1.1', 'system': 'SNES NTSC',
        'method': 'WRAM sampled at endFrame; one integer/subpixel record per video frame',
        'limits': [
            'Observed scenarios only; not a complete extraction of the game logic.',
            'RAM values are exact integers; timestamps identify saved trace files, not a hardware clock.',
            'Pose and radii can disagree for a transition frame; preserve all samples, not just a modal size.',
            'Old captures may omit animation columns. Inputs and RAM field meanings are in sm_reference.lua.',
            'The recorded Git revision is the last committed script before the file timestamp; uncommitted capture edits may differ.',
        ], 'scenarios': {}
    }
    if rom and state:
        rom_path, state_path = Path(rom), Path(state)
        if rom_path.suffix.lower() == '.zip':
            with zipfile.ZipFile(rom_path) as archive:
                entry = next(n for n in archive.namelist() if n.lower().endswith(('.sfc', '.smc')))
                rom_data = archive.read(entry)
        else:
            rom_data = rom_path.read_bytes()
        manifest['reference'] = {'rom_sha256': digest(rom_data), 'state_sha256': digest(state_path.read_bytes()),
            'hashes_checked_at_import': datetime.now().astimezone().isoformat(timespec='seconds')}
    for scenario, (path, columns, rows) in sorted(chosen.items()):
        if not scenario.replace('_', '').isalnum():
            raise SystemExit('Unsafe scenario name: ' + scenario)
        captured = datetime.fromtimestamp(path.stat().st_mtime).astimezone().isoformat(timespec='seconds')
        revision = subprocess.check_output(['git', 'log', '-1', '--format=%H', '--before=' + captured,
            '--', 'scripts/sm_reference.lua'], cwd=ROOT, text=True).strip()
        stream = io.StringIO(newline='')
        writer = csv.DictWriter(stream, columns, lineterminator='\n')
        writer.writeheader(); writer.writerows(rows)
        data = stream.getvalue().encode()
        (DEST / (scenario + '.csv.gz')).write_bytes(gzip.compress(data, mtime=0))
        sizes = Counter((int(r['pose']), int(r['xradius']), int(r['yradius'])) for r in rows)
        manifest['scenarios'][scenario] = {
            'file': scenario + '.csv.gz', 'saved_at': captured, 'rows': len(rows),
            'csv_sha256': digest(data), 'last_committed_script_before_saved_at': revision,
            'source_file': path.name,
            'observed_radii': [{'pose': f'{pose:02X}', 'x': x, 'y': y, 'samples': count}
                              for (pose, x, y), count in sorted(sizes.items())]
        }
        sidecar = Path(str(path) + '.source.json')
        if sidecar.exists():
            manifest['scenarios'][scenario]['capture_reference'] = json.loads(sidecar.read_text(encoding='utf-8-sig'))
        if scenario == 'wall_open':
            manifest['scenarios'][scenario]['limits'] = [
                'After frame 339, position is moved to (629,720), vertical speed cleared; room terrain is unchanged.',
                'This avoids the ceiling that immediately blocks the older wall_jump capture.',
                'Right wall kick with no running bonus; not every equipment/input or mirrored scenario.',
            ]
    manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f'Archived {len(chosen)} scenarios (existing scenarios retained).')


def import_scene(prefix):
    """Preserve a visual sample and its origin; do not retain a ROM or save state."""
    from PIL import Image
    source = Path(prefix)
    origin = json.loads(Path(str(source) + '.source.json').read_text(encoding='utf-8-sig'))
    target = DEST / 'landing_site'
    target.mkdir(exist_ok=True)
    files = {}
    for suffix in ('', '.snesVideoRam', '.snesCgRam', '.snesSpriteRam', '.snesWorkRam', '.png'):
        path = Path(str(source) + suffix)
        data = path.read_bytes()
        name = ('state.txt' if not suffix else suffix[1:]) + '.gz'
        (target / name).write_bytes(gzip.compress(data, mtime=0))
        files[name] = digest(data)
    # This map sheet has a one-pixel border between rooms. Keep a lossless crop,
    # including its original door frames and the block tower, before conversion.
    sheet = ROOT / 'res/images/original-references/maps/SNES - Super Metroid - Maps - Crateria.png'
    Image.open(sheet).crop((4615, 1, 6919, 1249)).save(target / 'foreground.png')
    files['foreground.png'] = digest((target / 'foreground.png').read_bytes())
    metadata = {'scenario': 'landing_site_saved_game', 'emulator': 'Mesen 2.1.1 --testrunner',
        'captured_at': datetime.fromtimestamp(source.stat().st_mtime).astimezone().isoformat(timespec='seconds'),
        'source_map': str(sheet.relative_to(ROOT)), 'source_map_sha256': digest(sheet.read_bytes()),
        'crop_xywh': [4615, 1, 2304, 1248], 'files': files,
        'limits': ['One saved-game state, not the arrival cutscene or every lighting/weather phase.',
                   'Map sheet provides room geometry; sky blue is a transparency placeholder.',
                   'Current room retains its existing 1248-pixel height and collision grid.',
                   'Ship art origin (1057,1101) matches the screenshot within one pixel; no animation measurement.']}
    metadata['reference'] = origin
    (target / 'manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')


def import_sky(prefixes):
    """Archive sparse original scanline/video probes, keeping each capture's source hashes."""
    target = DEST / 'landing_sky'
    target.mkdir(exist_ok=True)
    files, captures = {}, []
    for prefix in prefixes:
        source = Path(prefix)
        origin = json.loads(Path(str(source)+'.source.json').read_text(encoding='utf-8-sig'))
        captures.append({'file': source.name, 'reference': origin,
            'captured_at': datetime.fromtimestamp(source.stat().st_mtime).astimezone().isoformat(timespec='seconds')})
        for path in [source, *sorted(source.parent.glob(source.name+'.*'))]:
            data = path.read_bytes()
            name = path.name + '.gz'
            (target/name).write_bytes(gzip.compress(data, mtime=0))
            files[name] = digest(data)
    metadata = {'scenario': 'landing_sky_scanlines', 'emulator': 'Mesen 2.1.1 --testrunner',
        'captures': captures, 'files': files, 'limits': [
            'Four saved-state probes only: ship, ground, high and upper; no rendering registers modified.',
            'High/upper positions are held artificially to let the camera settle; not normal traversal.',
            'One CPU-execution sample per scanline; not every HDMA write or subscanline timing.',
            'Sky wraps vertically at 512 pixels in these samples; horizontal HDMA/weather are not ported.',
        ]}
    (target/'manifest.json').write_text(json.dumps(metadata, indent=2)+'\n')


def import_door(shot_prefix, hold_prefix):
    """Archive the original's blue door opening: one screenshot per frame, two runs (watch, and walk in)."""
    target = DEST / 'door_open'
    target.mkdir(exist_ok=True)
    files, runs = {}, []
    for prefix in (shot_prefix, hold_prefix):
        source = Path(prefix)
        origin = json.loads(Path(str(source) + '.source.json').read_text(encoding='utf-8-sig'))
        runs.append({'file': source.name, 'reference': origin})
        for path in [source, *sorted(source.parent.glob(source.name + '.f*.png'))]:
            data = path.read_bytes()
            (target / (path.name + '.gz')).write_bytes(gzip.compress(data, mtime=0))
            files[path.name + '.gz'] = digest(data)
    metadata = {'scenario': 'blue_door_opening', 'emulator': 'Mesen 2.1.1 --testrunner', 'runs': runs, 'files': files,
        'limits': ['One door (Landing Site, lower west) shot with the uncharged Power Beam.',
                   'Screenshots repeat every two frames, so timings are good to two frames.',
                   'Run 2 keeps walking into the door; Samus stops at x=20 (no room change observed in 60 frames).']}
    (target / 'manifest.json').write_text(json.dumps(metadata, indent=2) + chr(10))


def check():
    manifest = json.loads((DEST / 'manifest.json').read_text(encoding='utf-8'))
    total = 0
    radii_by_pose = defaultdict(Counter)
    beam_vectors = {}
    missile_vectors = {}
    for name, item in manifest['scenarios'].items():
        data = gzip.decompress((DEST / item['file']).read_bytes())
        assert digest(data) == item['csv_sha256'], name + ': checksum mismatch'
        rows = list(csv.DictReader(io.StringIO(data.decode())))
        if name in ('beam_up', 'beam_up_right'):
            vectors = [r['shot_vectors'].split()[0].split(':') for r in rows if r.get('shot_vectors')]
            # First 16 live samples; later records may be wall impacts or off screen.
            direction = '0' if name == 'beam_up' else '1'
            assert len(vectors) >= 16 and all(len(v) == 5 and v[:2] == ['0', direction]
                and v[4] == '8000' for v in vectors[:16]), name
            beam_vectors[name] = [(abs(int(v[2])), abs(int(v[3]))) for v in vectors[:16]]
        if name in ('missile_right', 'missile_up_right', 'super_right'):
            vectors = [r['shot_vectors'].split()[0].split(':') for r in rows if r.get('shot_vectors')]
            direction, shot_type = ('1', '8100') if name == 'missile_up_right' else \
                ('2', '8200') if name == 'super_right' else ('2', '8100')
            # First 16 live samples: the shot has not reached the wall yet.
            assert len(vectors) >= 16 and all(len(v) == 5 and v[:2] == ['0', direction]
                and v[4] == shot_type for v in vectors[:16]), name
            missile_vectors[name] = [(int(v[2]), int(v[3])) for v in vectors[:16]]
            cooldowns = [int(r['cooldown']) for r in rows if r.get('shot_vectors')]
            start = 20 if name == 'super_right' else 10
            assert cooldowns[0] == start and cooldowns[:start + 1] == list(range(start, -1, -1)), name
        assert len(rows) == item['rows'] and all(r['scenario'] == name for r in rows), name
        assert [int(r['frame']) for r in rows] == list(range(1, len(rows) + 1)), name + ': missing frames'
        sizes = Counter((int(r['pose']), int(r['xradius']), int(r['yradius'])) for r in rows)
        assert sizes == Counter({(int(s['pose'], 16), s['x'], s['y']): s['samples'] for s in item['observed_radii']}), name
        for (pose, x, y), count in sizes.items():
            radii_by_pose[pose][x, y] += count
        total += len(rows)
    # Independent expected dimensions for the production C tests, from recorded RAM.
    # Modal sizes select steady states; the archive retains every transient sample.
    roles = [('stand', 0x01), ('jump', 0x4D), ('spin', 0x19), ('crouch', 0x27), ('ball', 0x1D), ('air_down', 0x17)]
    expected = []
    for role, pose in roles:
        (x, y), _ = radii_by_pose[pose].most_common(1)[0]
        expected.append(f'    {{{2*x}, {2*y}}}, /* {role}, pose {pose:02X}, modal observed radii */')
    out = ROOT / 'tests/out'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'sm_hitboxes.h').write_text('/* Generated from archived original RAM, not production constants. */\n'
        'static const int reference_hitboxes[][2] = {\n' + '\n'.join(expected) + '\n};\n')
    beam_header = '/* Exact Q8.8 projectile velocities sampled from original RAM. */\n'
    for name, vectors in sorted(beam_vectors.items()):
        beam_header += f'static const int reference_{name}[][2] = {{\n'
        beam_header += ''.join(f'    {{{x}, {y}}},\n' for x, y in vectors) + '};\n'
    (out / 'sm_beams.h').write_text(beam_header)
    missile_header = '/* Exact Q8.8 missile velocities sampled from original RAM, first 16 live frames. */\n'
    for name, vectors in sorted(missile_vectors.items()):
        missile_header += f'static const int reference_{name}[][2] = {{\n'
        missile_header += ''.join(f'    {{{x}, {y}}},\n' for x, y in vectors) + '};\n'
    missile_header += ('\n/* Cooldown timer at spawn: missiles 10, supers 20; counted down every tick. */\n'
                       '#define REFERENCE_MISSILE_COOLDOWN 10\n#define REFERENCE_SUPER_COOLDOWN 20\n')
    (out / 'sm_missiles.h').write_text(missile_header)
    # Independent confirmation of the wall-kick's sustained horizontal speed.
    if 'wall_open' in manifest['scenarios']:
        wall = list(csv.DictReader(io.StringIO(gzip.decompress((DEST/'wall_open.csv.gz').read_bytes()).decode())))
        kick = [r for r in wall if int(r['pose']) == 0x83 and int(r['frame']) >= 342]
        assert len(kick) >= 40 and min(int(r['y']) for r in kick) <= 621
        speeds = [int(r['vx_base'])*256 + int(r['vx_basesub'])//256 for r in kick]
        assert speeds[:4] == [0,192,352,352] and max(speeds) == 352
        (out/'sm_wall.h').write_text('/* Original wall_open, kick away from an unobstructed wall. */\n'
            f'#define REFERENCE_WALL_SPEED {max(speeds)}\n'
            'static const int reference_wall_speeds[] = {' + ', '.join(map(str, speeds)) + '};\n'
            '/* Frames 342..421: right kick, held right+jump; mirrored timing is a port assumption. */\n'
            'static const int reference_wall_frames[] = {' +
            ', '.join(r['anim_frame'] for r in kick) + '};\n')
    held = list(csv.DictReader(io.StringIO(gzip.decompress((DEST/'shoot_hold.csv.gz').read_bytes()).decode())))
    reloads = [int(r['frame']) for i,r in enumerate(held) if i and int(r['cooldown'])>int(held[i-1]['cooldown'])]
    assert reloads[:3] == [216,231,256], 'Original held Power Beam repeats (first gap 15, then 25)'
    print(f'PASS saved reference: {len(manifest["scenarios"])} scenarios, {total} RAM samples; no emulator needed')
    for scene in (DEST/'landing_site', DEST/'landing_sky', DEST/'door_open'):
        if scene.exists():
            metadata = json.loads((scene / 'manifest.json').read_text())
            for name, expected in metadata['files'].items():
                data = (scene / name).read_bytes()
                if name.endswith('.gz'): data = gzip.decompress(data)
                assert digest(data) == expected, 'scene checksum: ' + name
            print(f'PASS saved {scene.name} graphics and video memory; no emulator needed')
    sky = DEST/'landing_sky'
    if sky.exists():
        for name in ('sky_reference.csv.gz', 'sky_upper.csv.gz'):
            rows = csv.DictReader(io.StringIO(gzip.decompress((sky/name).read_bytes()).decode()))
            for row in rows:
                samples = [tuple(map(int,v.split(':'))) for v in row['bg2_lines'].split()]
                assert len(samples) >= 220
                assert all(scroll%512 == int(row['camera_y'])%512 for line,scroll,x in samples if line>0)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['import', 'import-scene', 'import-sky', 'import-door', 'check'])
    parser.add_argument('patterns', nargs='*')
    parser.add_argument('--rom')
    parser.add_argument('--state')
    args = parser.parse_args()
    if args.action == 'import':
        import_traces(args.patterns, args.rom, args.state)
    elif args.action == 'import-scene':
        import_scene(args.patterns[0])
    elif args.action == 'import-door':
        import_door(*args.patterns)
    elif args.action == 'import-sky':
        import_sky(args.patterns)
    check()
