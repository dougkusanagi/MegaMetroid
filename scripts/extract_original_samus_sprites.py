#!/usr/bin/env python3
"""Extract Samus poses, palettes and effects from the local reference sheet.

The PNGs keep the sheet's original SNES colors. Every pose uses the same
indexed palette (the sheet's "Power Suit" swatch, in palette order), so a suit
or color cycle is a palette swap: load another PALETTE, keep the same tiles.
rescomp converts the colors to the Mega Drive format at build time.
"""

from __future__ import annotations

import argparse
import csv
import re
import shutil
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = (
    ROOT
    / "res"
    / "images"
    / "original-references"
    / "SNES - Super Metroid - Playable Characters - Samus Aran.png"
)
DEFAULT_OUTPUT = ROOT / "res" / "images" / "samus-original-sgdk"
LABELS = ROOT / "scripts" / "data" / "samus_pose_labels.csv"

CELL_BACKGROUND = (192, 128, 246)
SHEET_BACKGROUND = (230, 164, 244)
CELL_SIZE = 80
TILE_SIZE = 8
# Every pose fits in this window of the 80x80 cell, so all frames share one
# size and one pivot: frame pixel (x, y) is cell pixel (x + 8, y + 8).
FRAME_BOX = (8, 8, 72, 72)
FRAME_SIZE = FRAME_BOX[2] - FRAME_BOX[0]
# Cells below this line, in the last column, demonstrate palettes; the
# palettes themselves are exported from the swatches.
PALETTE_DEMO_TOP = 2600
SWATCH_BLOCK = 7
SWATCH_COLORS = 15

# (name, x, y, rows, row pitch) of the swatch rows: 15 colors, indices 1..15.
PALETTES = [
    ("power_suit", 4048, 2928, 1, 9),
    ("power_suit_yellow_cycle", 4048, 2944, 3, 9),
    ("power_suit_green_cycle", 4048, 2976, 3, 9),
    ("power_suit_blue_cycle", 4048, 3008, 3, 9),
    ("power_suit_white_cycle", 4048, 3040, 3, 9),
    ("power_suit_long_white_cycle", 4048, 3072, 7, 9),
    ("gravity_suit", 4608, 2928, 1, 9),
    ("gravity_suit_yellow_cycle", 4608, 2944, 3, 9),
    ("gravity_suit_green_cycle", 4608, 2976, 3, 9),
    ("gravity_suit_blue_cycle", 4608, 3008, 3, 9),
    ("gravity_suit_white_cycle", 4608, 3040, 3, 9),
    ("gravity_suit_long_white_cycle", 4608, 3072, 7, 9),
    ("varia_suit", 4048, 3568, 1, 9),
    ("varia_suit_yellow_cycle", 4048, 3584, 3, 9),
    ("varia_suit_green_cycle", 4048, 3616, 3, 9),
    ("varia_suit_blue_cycle", 4048, 3648, 3, 9),
    ("varia_suit_white_cycle", 4048, 3680, 3, 9),
    ("varia_suit_long_white_cycle", 4048, 3712, 7, 9),
    ("hyper_beam", 4608, 4184, 10, 9),
    ("flashback", 4736, 3392, 1, 9),
    ("death_tiles_suit", 5336, 2641, 1, 9),
    ("death_tiles_suitless", 5464, 2641, 1, 9),
    ("death_intro_frames_1_2", 5352, 2772, 1, 16),
    ("death_intro_frames_3_5", 5352, 2788, 1, 16),
    ("death_intro_frame_4", 5352, 2804, 1, 16),
    ("death_fading", 5448, 2953, 9, 9),
    ("crystal_flash_parts", 5744, 3073, 6, 9),
    ("death_unused_original", 5744, 3193, 1, 9),
    ("crystal_flash_reference_gray", 5488, 3273, 1, 9),
    ("crystal_flash_reference_suit", 5616, 3273, 1, 9),
    ("crystal_flash_palette_animation", 5775, 3400, 6, 9),
    ("missile_fins", 5200, 3689, 1, 9),
]
BASE_PALETTE = "power_suit"

# Unboxed art: (name, box, palette groups to try first). Components inside each box
# become numbered frames, in reading order.
EFFECT_REGIONS = [
    ("death_tiles_ingame", (5460, 2650, 5745, 2714), []),
    ("death_intro", (5195, 2760, 5345, 2822),
     ["death_intro_frames_1_2", "death_intro_frames_3_5", "death_intro_frame_4"]),
    ("death_suitless", (5195, 2960, 5425, 3030), ["death_tiles_suitless"]),
    ("crystal_flash_silhouette", (5195, 3065, 5297, 3258), ["crystal_flash_parts", "crystal_flash_palette_animation"]),
    ("crystal_flash_bubble", (5300, 3065, 5735, 3245), ["crystal_flash_parts"]),
    ("death_unused_original", (5738, 3195, 5835, 3255), ["death_unused_original"]),
    ("missile_fins_tiles", (5195, 3512, 5235, 3662), ["missile_fins"]),
    ("missile_fins_examples", (5425, 3515, 5735, 3800), ["power_suit"]),
]
# Exported whole, not split: tilesets, and rows whose frames overlap (the
# shattering suit particles, and the death sequence that draws them over the
# suitless body with another palette).
EFFECT_BLOCKS = [
    ("death_tiles_tileset", (5198, 2655, 5330, 2737)),
    ("death_tiles_loose", (5334, 2654, 5428, 2732)),
    ("death_sequence", (5195, 2840, 5595, 2940)),
    ("death_shattering_suit", (5600, 2840, 5905, 2940)),
]
EFFECT_TRANSPARENT = {CELL_BACKGROUND, SHEET_BACKGROUND, (0, 0, 0)}
EFFECT_MERGE_DISTANCE = 3
EFFECT_MIN_PIXELS = 4


def slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")


class Components:
    """Run-based connected-component labeling for exact pixel predicates."""

    def __init__(self) -> None:
        self.parent: list[int] = []
        self.rank: list[int] = []
        self.bounds: list[list[int]] = []

    def add(self, x0: int, y: int, x1: int) -> int:
        label = len(self.parent)
        self.parent.append(label)
        self.rank.append(0)
        self.bounds.append([x0, y, x1, y + 1, x1 - x0])
        return label

    def find(self, label: int) -> int:
        root = label
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[label] != label:
            next_label = self.parent[label]
            self.parent[label] = root
            label = next_label
        return root

    def union(self, left: int, right: int) -> int:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return left_root

        if self.rank[left_root] < self.rank[right_root]:
            left_root, right_root = right_root, left_root
        self.parent[right_root] = left_root
        if self.rank[left_root] == self.rank[right_root]:
            self.rank[left_root] += 1

        left_bounds = self.bounds[left_root]
        right_bounds = self.bounds[right_root]
        left_bounds[0] = min(left_bounds[0], right_bounds[0])
        left_bounds[1] = min(left_bounds[1], right_bounds[1])
        left_bounds[2] = max(left_bounds[2], right_bounds[2])
        left_bounds[3] = max(left_bounds[3], right_bounds[3])
        left_bounds[4] += right_bounds[4]
        return left_root


def roots_with_bounds(components: Components) -> list[tuple[int, int, int, int, int]]:
    found = []
    for label in range(len(components.parent)):
        root = components.find(label)
        if label != root:
            continue
        found.append(tuple(components.bounds[root]))
    return found


def join_runs(
    components: Components,
    label: int,
    x0: int,
    x1: int,
    previous: list[tuple[int, int, int]],
    diagonal: bool,
) -> None:
    for prev_x0, prev_x1, prev_label in previous:
        if (prev_x1 < x0 if diagonal else prev_x1 <= x0):
            continue
        if (prev_x0 > x1 if diagonal else prev_x0 >= x1):
            break
        overlaps = (
            prev_x1 >= x0 and prev_x0 <= x1
            if diagonal
            else prev_x1 > x0 and prev_x0 < x1
        )
        if overlaps:
            components.union(label, prev_label)


def find_cells(image: Image.Image) -> list[tuple[int, int, int, int]]:
    """Find the 80x80 interiors by labeling the sheet's exact cell fill color."""

    pixels = image.load()
    width, height = image.size
    components = Components()
    previous: list[tuple[int, int, int]] = []

    for y in range(height):
        current: list[tuple[int, int, int]] = []
        x = 0
        while x < width:
            while x < width and pixels[x, y] != CELL_BACKGROUND:
                x += 1
            x0 = x
            while x < width and pixels[x, y] == CELL_BACKGROUND:
                x += 1
            if x0 == x:
                continue

            label = components.add(x0, y, x)
            join_runs(components, label, x0, x, previous, diagonal=False)
            current.append((x0, x, label))
        previous = current

    cells = []
    for x0, y0, x1, y1, _area in roots_with_bounds(components):
        if x1 - x0 == CELL_SIZE and y1 - y0 == CELL_SIZE:
            cells.append((x0, y0, x1, y1))
    return sorted(cells, key=lambda box: (box[1], box[0]))


def read_palettes(image: Image.Image) -> dict[str, list[list[tuple[int, int, int]]]]:
    pixels = image.load()
    palettes = {}
    for name, x, y, rows, pitch in PALETTES:
        entries = []
        for row in range(rows):
            center_y = y + row * pitch + SWATCH_BLOCK // 2
            colors = [
                pixels[x + index * SWATCH_BLOCK + SWATCH_BLOCK // 2, center_y]
                for index in range(SWATCH_COLORS)
            ]
            if SHEET_BACKGROUND in colors:
                raise RuntimeError(f"Amostra de paleta {name} fora do lugar em ({x}, {center_y})")
            entries.append([CELL_BACKGROUND] + colors)
        palettes[name] = entries
    return palettes


def indexed(rgb: Image.Image, palette: list[tuple[int, int, int]], transparent: set) -> Image.Image:
    lookup = {}
    for index, color in enumerate(palette):
        lookup.setdefault(color, index)
    data = []
    for color in rgb.getdata():
        if color in transparent:
            data.append(0)
        elif color in lookup and lookup[color] != 0:
            data.append(lookup[color])
        else:
            raise KeyError(color)
    out = Image.new("P", rgb.size, 0)
    out.putdata(data)
    flat = [channel for color in palette for channel in color]
    out.putpalette(flat + [0] * (768 - len(flat)))
    out.info["transparency"] = 0
    return out


def own_palette(colors: list[tuple[int, int, int]]) -> list[tuple[int, int, int]]:
    # Darkest first, so files from the same region read alike.
    return [CELL_BACKGROUND] + sorted(colors, key=lambda c: (sum(c), c))


def components(image: Image.Image, box, transparent, distance):
    """Bounding boxes of opaque blobs, merging blobs closer than `distance`."""

    crop = image.crop(box)
    width, height = crop.size
    pixels = crop.load()
    opaque = [[pixels[x, y] not in transparent for x in range(width)] for y in range(height)]
    label = [[-1] * width for _ in range(height)]
    boxes = []
    for y in range(height):
        for x in range(width):
            if not opaque[y][x] or label[y][x] >= 0:
                continue
            stack = [(x, y)]
            label[y][x] = len(boxes)
            bx0, by0, bx1, by1, count = x, y, x + 1, y + 1, 0
            while stack:
                cx, cy = stack.pop()
                count += 1
                bx0, by0 = min(bx0, cx), min(by0, cy)
                bx1, by1 = max(bx1, cx + 1), max(by1, cy + 1)
                for nx in range(cx - 1, cx + 2):
                    for ny in range(cy - 1, cy + 2):
                        if 0 <= nx < width and 0 <= ny < height and opaque[ny][nx] and label[ny][nx] < 0:
                            label[ny][nx] = len(boxes)
                            stack.append((nx, ny))
            boxes.append([bx0, by0, bx1, by1, count])

    merged = True
    while merged:
        merged = False
        for i in range(len(boxes)):
            for j in range(i + 1, len(boxes)):
                a, b = boxes[i], boxes[j]
                if (
                    a[0] - distance < b[2] and b[0] - distance < a[2]
                    and a[1] - distance < b[3] and b[1] - distance < a[3]
                ):
                    boxes[i] = [min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3]), a[4] + b[4]]
                    del boxes[j]
                    merged = True
                    break
            if merged:
                break

    found = []
    for x0, y0, x1, y1, count in boxes:
        # Separator lines between death frames are 1-2 px wide and very tall.
        if count < EFFECT_MIN_PIXELS or (x1 - x0 <= 2 and y1 - y0 > 40):
            continue
        found.append((box[0] + x0, box[1] + y0, box[0] + x1, box[1] + y1))
    # Reading order: rows first (centers within half a sprite), then columns.
    found.sort(key=lambda b: (b[1] + b[3]) // 2)
    rows = []
    for item in found:
        center = (item[1] + item[3]) / 2
        if rows and abs(center - rows[-1][0]) < 12:
            rows[-1][1].append(item)
        else:
            rows.append([center, [item]])
    return [b for _center, row in rows for b in sorted(row)]


def pad8(value: int) -> int:
    return -(-value // TILE_SIZE) * TILE_SIZE


def clean(output: Path) -> None:
    for name in ("poses", "palettes", "effects", "frames"):
        if (output / name).is_dir():
            shutil.rmtree(output / name)
    for name in ("manifest.csv", "sprites.res", "sprites.s", "sprites.h"):
        (output / name).unlink(missing_ok=True)


def group_poses(image, cells, labels):
    """Map each label stack (bars with no cells between them) to its cells."""

    columns = sorted({label["label_x"] for label in labels})
    by_column: dict[int, list[dict]] = {}
    for label in labels:
        by_column.setdefault(label["label_x"], []).append(label)
    groups: dict[tuple[int, int], list] = {}
    for cell in cells:
        x0, y0 = cell[0], cell[1]
        column = max(c for c in columns if c <= x0 + 2)
        above = [l for l in by_column[column] if l["label_y"] < y0]
        if not above or (column == columns[-1] and y0 > PALETTE_DEMO_TOP):
            continue
        groups.setdefault((column, above[-1]["label_y"]), []).append(cell)

    poses = []
    for column, stack_labels in by_column.items():
        pending = []
        for label in stack_labels:
            pending.append(label)
            cells_here = groups.get((column, label["label_y"]))
            if cells_here:
                poses.append((pending, cells_here))
                pending = []
        if pending:
            raise RuntimeError(f"Rótulos sem quadros: {[l['name'] for l in pending]}")
    return poses


def extract(source: Path, output: Path) -> dict[str, int]:
    if not source.is_file():
        raise FileNotFoundError(f"Imagem de referência não encontrada: {source}")
    image = Image.open(source).convert("RGB")
    with LABELS.open(encoding="utf-8") as handle:
        labels = [
            {**row, "label_x": int(row["label_x"]), "label_y": int(row["label_y"])}
            for row in csv.DictReader(handle)
        ]

    clean(output)
    for name in ("poses", "palettes", "effects"):
        (output / name).mkdir(parents=True, exist_ok=True)

    palettes = read_palettes(image)
    base = palettes[BASE_PALETTE][0]
    stats = {"poses": 0, "frames": 0, "palettes": 0, "effects": 0}

    with (output / "manifest_palettes.csv").open("w", newline="", encoding="utf-8") as manifest, \
            (output / "samus_palettes.res").open("w", encoding="utf-8", newline="\n") as res:
        writer = csv.writer(manifest, lineterminator="\n")
        writer.writerow(["resource", "file", "group", "row", "source_x", "source_y"] + [f"c{i}" for i in range(16)])
        res.write("# Generated by scripts/extract_original_samus_sprites.py\n")
        res.write("# One palette per swatch row. Index i plays the same role in every row.\n")
        for name, x, y, _rows, pitch in PALETTES:
            for row, colors in enumerate(palettes[name]):
                resource = f"pal_samus_{name}" + (f"_{row}" if len(palettes[name]) > 1 else "")
                file = f"palettes/{resource}.png"
                strip = Image.new("P", (16, 1), 0)
                strip.putdata(list(range(16)))
                flat = [channel for color in colors for channel in color]
                strip.putpalette(flat + [0] * (768 - len(flat)))
                strip.save(output / file, optimize=True)
                writer.writerow([resource, file, name, row, x, y + row * pitch] + ["#%02x%02x%02x" % c for c in colors])
                res.write(f'PALETTE {resource} "{file}"\n')
                stats["palettes"] += 1

    cells = find_cells(image)
    poses = group_poses(image, cells, labels)
    with (output / "manifest_poses.csv").open("w", newline="", encoding="utf-8") as manifest, \
            (output / "samus_poses.res").open("w", encoding="utf-8", newline="\n") as res:
        writer = csv.writer(manifest, lineterminator="\n")
        writer.writerow(["resource", "file", "pose", "name", "aliases", "diamond", "frames", "cells"])
        res.write("# Generated by scripts/extract_original_samus_sprites.py\n")
        res.write(f"# {FRAME_SIZE}x{FRAME_SIZE} frames with a shared pivot; palette = {BASE_PALETTE}.\n")
        res.write("# Copy the poses a sprite needs into the game's .res (or build a sheet from them).\n")
        used_names = set()
        for stack, pose_cells in poses:
            first = stack[0]
            tag = first["pose"].lower() if first["pose"] else "xx"
            resource = f"samus_{tag}_{slug(first['name'])}"
            while resource in used_names:
                resource += "_b"
            used_names.add(resource)
            file = f"poses/{resource}.png"
            strip = Image.new("RGB", (FRAME_SIZE * len(pose_cells), FRAME_SIZE), CELL_BACKGROUND)
            for index, (x0, y0, _x1, _y1) in enumerate(pose_cells):
                frame = image.crop((x0 + FRAME_BOX[0], y0 + FRAME_BOX[1], x0 + FRAME_BOX[2], y0 + FRAME_BOX[3]))
                strip.paste(frame, (index * FRAME_SIZE, 0))
            indexed(strip, base, {CELL_BACKGROUND}).save(output / file, optimize=True)
            writer.writerow([
                resource, file, first["pose"], first["name"],
                "; ".join(f"{l['pose'] or '--'}: {l['name']}" for l in stack[1:]),
                first["diamond"], len(pose_cells),
                " ".join(f"{x0},{y0}" for x0, y0, _x1, _y1 in pose_cells),
            ])
            tiles = FRAME_SIZE // TILE_SIZE
            res.write(f'SPRITE {resource} "{file}" {tiles} {tiles} FAST 0 NONE\n')
            stats["poses"] += 1
            stats["frames"] += len(pose_cells)

    known = [(f"{name}_{row}" if len(rows) > 1 else name, colors)
             for name, rows in palettes.items() for row, colors in enumerate(rows)]
    with (output / "manifest_effects.csv").open("w", newline="", encoding="utf-8") as manifest, \
            (output / "samus_effects.res").open("w", encoding="utf-8", newline="\n") as res:
        writer = csv.writer(manifest, lineterminator="\n")
        writer.writerow(["resource", "file", "region", "source_x", "source_y", "width", "height",
                         "colors", "palette", "sgdk_ready"])
        res.write("# Generated by scripts/extract_original_samus_sprites.py\n")
        res.write("# Single effect frames. Blocks and multi-palette art are references (manifest_effects.csv).\n")
        items = [(name, box, True, []) for name, box in EFFECT_BLOCKS]
        for name, region, hints in EFFECT_REGIONS:
            for index, box in enumerate(components(image, region, EFFECT_TRANSPARENT, EFFECT_MERGE_DISTANCE)):
                items.append((f"{name}_{index:02d}", box, False, hints))
        for name, box, whole, hints in items:
            crop = image.crop(box)
            colors = sorted({c for c in crop.getdata() if c not in EFFECT_TRANSPARENT})
            candidates = sorted(known, key=lambda item: not any(item[0].startswith(h) for h in hints))
            palette_name = next((n for n, pal in candidates if set(colors) <= set(pal[1:])), "")
            palette = dict(known)[palette_name] if palette_name else own_palette(colors)
            width, height = pad8(crop.width), pad8(crop.height)
            canvas = Image.new("RGB", (width, height), CELL_BACKGROUND)
            canvas.paste(crop, (0, 0))
            resource = f"samus_fx_{name}"
            file = f"effects/{resource}.png"
            ready = len(colors) <= SWATCH_COLORS and not whole
            if len(colors) <= SWATCH_COLORS:
                indexed(canvas, palette, EFFECT_TRANSPARENT).save(output / file, optimize=True)
                if ready:
                    tiles = f"{width // TILE_SIZE} {height // TILE_SIZE}"
                    res.write(f'SPRITE {resource} "{file}" {tiles} FAST 0 NONE\n')
            else:
                # Mixes palettes (several sprites drawn together): keep as RGB reference.
                canvas.save(output / file, optimize=True)
            writer.writerow([resource, file, name, box[0], box[1], crop.width, crop.height,
                             len(colors), palette_name or "own", "yes" if ready else "no"])
            stats["effects"] += 1
    return stats


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_SOURCE, help="PNG da prancha original")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="Pasta de saída")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    stats = extract(args.input, args.output)
    print(f"Poses: {stats['poses']} ({stats['frames']} quadros)")
    print(f"Paletas: {stats['palettes']}")
    print(f"Efeitos: {stats['effects']}")
    print(f"Saída: {args.output.resolve()}")


if __name__ == "__main__":
    main()
