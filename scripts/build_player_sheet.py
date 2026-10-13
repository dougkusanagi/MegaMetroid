#!/usr/bin/env python3
"""Build the Samus sheet that the game loads, from the extracted poses.

Each row of the sheet is one animation and each 64x64 cell is one frame, read
from left to right in pose order. The left-facing drawing has its own row (the
game never flips Samus), so the left row of an action is the right row + 1.
Rows can be shorter than the widest one: rescomp drops the blank cells at the end.

A pose is given as 'NN' (the whole strip) or 'NN:i' / 'NN:i-j' (only those frames).
Most aiming strips start with a transition frame before the held one, and some strips
hold both facings (pose 16 is frames 2-3 of the 15 strip).

Writes res/images/player/samus_sheet.png and inc/player_anim.h (the PLAYER_ANIM_*
enum and player_muzzle[], the cannon tip in each row's first frame, in pixels from
the body position). Run from the project root: python scripts/build_player_sheet.py
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
POSES = ROOT / 'res/images/samus-original-sgdk/poses'
SHEET = ROOT / 'res/images/player/samus_sheet.png'
HEADER = ROOT / 'inc/player_anim.h'
CELL = 64
# The game draws a cell at (x - 12, y - 11), so the feet of a standing Samus land on row 58 of the
# cell, which is the last row of her hitbox (y + 47). Grounded poses are moved to that row.
GROUND_ROW = 58
# Actions whose feet touch the floor. The original art draws the crouch frames 11 px higher in
# their box than the standing ones (and the crouch-down and stand-up frames 3 px higher), so
# without this Samus floats while crouched.
# The ball and its transitions sit on the same row: the ball box keeps the standing feet, in the air too.
GROUNDED = {'STAND', 'WALK', 'LAND', 'SPIN_LAND', 'CROUCH_DOWN', 'CROUCH', 'STAND_UP',
            'MORPH', 'BALL', 'UNMORPH', 'AIM_UP', 'WALK_SHOOT', 'WALK_AIM_UP', 'WALK_AIM_DOWN',
            'CROUCH_AIM_UP'}
# Actions aligned frame by frame instead of as a whole. The extracted ball frames drift up and down by
# a pixel from the sheet grid; a rolling ball must not bounce.
ALIGN_EACH_FRAME = {'BALL'}
# Palette indices of the arm cannon's greens in the Power Suit palette (the visor is index 4).
CANNON_COLORS = {5, 7, 8, 13}
# A cell pixel minus this is a body pixel (the game draws a cell at x - 12, y - 11).
DRAW_OFFSET = (12, 11)
# (action, pose facing right, pose facing left[, aim]), in row order. To add an action, add its entry
# here and rebuild; the game then uses PLAYER_ANIM_<action>_R / _L. The aim (forward if left out)
# says where to look for the cannon tip: U up, DU up and forward, D down, DD down and forward.
ANIMATIONS = [
    ('STAND', '01', '02'),
    ('WALK', '09', '0A'),
    ('SPIN', '19', '1A'),
    ('TAKEOFF', '4B', '4C'),
    ('JUMP_UP', '4D', '4E'),
    ('FALL', '29', '2A'),
    ('LAND', 'A4', 'A5'),
    ('SPIN_LAND', 'A6', 'A7'),
    ('WALL_KICK', '83', '84'),
    ('CROUCH_DOWN', '35', '36'),
    ('CROUCH', '27', '28'),
    ('STAND_UP', '3B', '3C'),
    ('MORPH', '37', '38'),
    # 1D faces right; 1F is the left roll (the sheet has no separate 1E/41 drawings).
    ('BALL', '1D', '1F'),
    ('UNMORPH', '3D', '3E'),
    # 3.2.5, aiming and shooting. Aiming strips use their held frame only.
    ('AIM_UP', '03:1', '04:1', 'U'),
    ('WALK_SHOOT', '0B', '0C'),
    ('WALK_AIM_UP', '0F', '10', 'DU'),
    ('WALK_AIM_DOWN', '11', '12', 'DD'),
    ('CROUCH_AIM_UP', '85:1', '86:1', 'U'),
    ('JUMP_SHOOT', '13:0', '14:0'),
    ('JUMP_AIM_UP', '15:1', '15:3', 'U'),
    ('JUMP_AIM_DOWN', '17:1', '18:1', 'D'),
    ('JUMP_AIM_DIAG_UP', '69:0', '6A:0', 'DU'),
    ('JUMP_AIM_DIAG_DOWN', '6B:0', '6C:0', 'DD'),
    ('FALL_SHOOT', '67:0', '68:0'),
    ('FALL_AIM_UP', '2B:1', '2C:1', 'U'),
    ('FALL_AIM_DOWN', '2D:1', '2E:1', 'D'),
    ('FALL_AIM_DIAG_UP', '6D', '6E', 'DU'),
    ('FALL_AIM_DIAG_DOWN', '6F', '70', 'DD'),
]


def load_frames(spec: str) -> tuple[list[Image.Image], list[int]]:
    """Frames of a pose spec ('NN', 'NN:i' or 'NN:i-j') and the pose palette."""
    number, _, picked = spec.partition(':')
    pose = load_pose(number)
    frames = [pose.crop((x, 0, x + CELL, CELL)) for x in range(0, pose.width, CELL)]
    if picked:
        first, _, last = picked.partition('-')
        frames = frames[int(first):int(last or first) + 1]
        if not frames:
            raise SystemExit(f'pose {spec}: no such frames')
    return frames, pose.getpalette()


def muzzle(frame: Image.Image, aim: str, left: bool) -> tuple[int, int]:
    """Cannon tip of a frame, in body pixels: the cannon pixel farthest in the aiming direction.
    Only the cannon's grey-green is searched, so a foot or the helmet is never taken for it."""
    data = frame.tobytes()
    pixels = [(i % CELL, i // CELL) for i, p in enumerate(data) if p in CANNON_COLORS]
    # Poses without a visible cannon (the ball) never shoot: any pixel will do.
    pixels = pixels or [(i % CELL, i // CELL) for i, p in enumerate(data) if p]
    side = -1 if left else 1
    score = {
        'F': lambda p: p[0] * side,
        'U': lambda p: -p[1],
        'DU': lambda p: p[0] * side - p[1],
        'D': lambda p: p[1],
        'DD': lambda p: p[0] * side + p[1],
    }[aim]
    best = max(map(score, pixels))
    tip = [p for p in pixels if score(p) == best]
    x = round(sum(p[0] for p in tip) / len(tip))
    y = round(sum(p[1] for p in tip) / len(tip))
    return x - DRAW_OFFSET[0], y - DRAW_OFFSET[1]


def load_pose(number: str) -> Image.Image:
    matches = sorted(POSES.glob(f'samus_{number.lower()}_*.png'))
    if len(matches) != 1:
        raise SystemExit(f'pose {number}: {len(matches)} files found in {POSES}, expected 1')
    pose = Image.open(matches[0])
    if pose.mode != 'P' or pose.height != CELL or pose.width % CELL:
        raise SystemExit(f'{matches[0].name}: expected an indexed strip {CELL} px high')
    return pose


def lowest_row(frame: Image.Image) -> int | None:
    """Last row with a visible pixel (index 0 is transparent), or None for an empty frame."""
    box = frame.point(lambda p: 255 if p else 0).getbbox()
    return None if box is None else box[3] - 1


def visible_pixels(frame: Image.Image) -> int:
    data = frame.tobytes()
    return len(data) - data.count(0)


def align_feet(frames: list[Image.Image]) -> list[Image.Image]:
    """Move a whole pose so its lowest foot row is GROUND_ROW. Every frame moves by the same
    amount, so a walk cycle keeps its lifted feet above the planted one."""
    feet = max(row for row in map(lowest_row, frames) if row is not None)
    shift = GROUND_ROW - feet
    if not shift:
        return frames
    shifted = []
    for frame in frames:
        cell = Image.new('P', (CELL, CELL), 0)
        cell.paste(frame, (0, shift))
        # A shift that pushes pixels out of the cell would silently cut Samus; refuse it.
        if visible_pixels(cell) != visible_pixels(frame):
            raise SystemExit(f'aligning the feet by {shift} px would crop the pose')
        shifted.append(cell)
    return shifted


def main() -> None:
    rows = []
    muzzles = []
    palette = None
    for action, right, left, *aim in ANIMATIONS:
        for number, left_row in ((right, False), (left, True)):
            frames, pose_palette = load_frames(number)
            if palette is None:
                palette = pose_palette
            elif pose_palette[:48] != palette[:48]:
                raise SystemExit(f'pose {number} does not use the shared palette')
            if action in ALIGN_EACH_FRAME:
                frames = [align_feet([frame])[0] for frame in frames]
            elif action in GROUNDED:
                frames = align_feet(frames)
            rows.append((number, frames))
            muzzles.append(muzzle(frames[0], aim[0] if aim else 'F', left_row))

    widest = max(len(frames) for _, frames in rows)
    sheet = Image.new('P', (widest * CELL, len(rows) * CELL), 0)
    sheet.putpalette(palette)
    for row, (_, frames) in enumerate(rows):
        for col, frame in enumerate(frames):
            sheet.paste(frame, (col * CELL, row * CELL))
    SHEET.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(SHEET, transparency=0)

    # Read the file back: every cell must hold exactly the pixels of its pose frame.
    saved = Image.open(SHEET)
    for row, (number, frames) in enumerate(rows):
        for col, frame in enumerate(frames):
            cell = saved.crop((col * CELL, row * CELL, (col + 1) * CELL, (row + 1) * CELL))
            if cell.tobytes() != frame.tobytes():
                raise SystemExit(f'pose {number} frame {col} differs in {SHEET.name}')

    lines = [
        '/* Generated by scripts/build_player_sheet.py: do not edit by hand. */',
        '#ifndef MM_PLAYER_ANIM_H',
        '#define MM_PLAYER_ANIM_H',
        '',
        '/* One row of the Samus sheet per entry. Each action has a right row and the',
        '   left row right after it, so the facing is added to the right row. */',
        'enum {',
    ]
    for action, right, left, *_ in ANIMATIONS:
        lines.append(f'    PLAYER_ANIM_{action}_R, /* pose {right} */')
        lines.append(f'    PLAYER_ANIM_{action}_L, /* pose {left} */')
    lines += ['    PLAYER_ANIM_COUNT', '};', '',
              "/* Cannon tip in each row's first frame, {x, y} in pixels from the body position. */",
              'static const signed char player_muzzle[PLAYER_ANIM_COUNT][2] = {']
    for (number, _), (x, y) in zip(rows, muzzles):
        lines.append(f'    {{{x}, {y}}}, /* {number} */')
    lines += ['};', '', '#endif /* MM_PLAYER_ANIM_H */', '']
    HEADER.write_bytes('\n'.join(lines).encode('utf-8'))

    print(f'{SHEET.relative_to(ROOT)}: {len(rows)} rows, up to {widest} frames of {CELL}x{CELL}')
    print(f'{HEADER.relative_to(ROOT)}: {len(ANIMATIONS)} actions, {len(rows)} rows')


if __name__ == '__main__':
    main()
