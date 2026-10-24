#!/usr/bin/env python3
"""Extract sprites and tiles from the other local reference sheets for SGDK.

Each sheet is cut into named regions (see SHEETS). A region becomes:

- "strip": a sprite animation. The blobs inside it, in reading order, become
  frames of one uniform size, centered (or bottom-aligned) in the frame.
- "rows": the region is split into rows of blobs, and each row is a strip.
- "block": the region is exported whole, as tiles (IMAGE).
- "frames": each blob is exported as its own IMAGE (art too big for sprites).
  With `cycle`, the frames differ only in color: one IMAGE plus one palette
  per frame.

Colors stay as in the original sheet. A strip shares one indexed palette
(index 0 transparent, other colors darkest first). When a row needs more than
15 colors, it is split into consecutive strips that fit one palette each.
"""

from __future__ import annotations

import argparse
import csv
import shutil
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
REFERENCES = ROOT / "res" / "images" / "original-references"
OUTPUT = ROOT / "res" / "images"
TILE_SIZE = 8
MAX_COLORS = 15
MAX_SPRITE = 128
DIRECTIONS = ["ul", "u", "ur", "l", "r", "dl", "d", "dr"]


def strip(name, box, merge=2, align="center", directions=False, ring=False):
    """`directions`: 8 blobs laid out around a center (ul, u, ur, l, r, dl, d, dr).
    `ring`: several frames per direction, grouped by angle around the box center."""
    return {"kind": "strip", "name": name, "box": box, "merge": merge, "align": align,
            "directions": directions, "ring": ring}


def frames(name, box, merge=2, cycle=False):
    return {"kind": "frames", "name": name, "box": box, "merge": merge, "cycle": cycle}


def rows(name, box, merge=2, align="bottom"):
    return {"kind": "rows", "name": name, "box": box, "merge": merge, "align": align}


def block(name, box):
    return {"kind": "block", "name": name, "box": box}


SHEETS = {
    "weapons": {
        "file": "miscellaneous/SNES - Super Metroid - Miscellaneous - Weapons.png",
        "transparent": [(0, 136, 255)],
        "regions": [
            strip("missile", (0, 0, 100, 80), directions=True),
            strip("super_missile", (100, 0, 200, 80), directions=True),
            strip("power_beam", (200, 0, 300, 80), directions=True),
            strip("bomb", (312, 0, 338, 80)),
            strip("bomb_b", (340, 0, 362, 80)),
            strip("smoke", (364, 0, 392, 80)),
            strip("beam_thin", (0, 90, 155, 262), merge=-1, ring=True),
            strip("beam_thick", (155, 85, 345, 270), merge=-1, ring=True),
            rows("impact", (0, 280, 445, 540), align="center"),
            rows("grapple", (0, 540, 445, 1168), merge=6, align="center"),
        ],
    },
    "objects": {
        "file": "miscellaneous/SNES - Super Metroid - Miscellaneous - Objects, Items & Doors.png",
        "transparent": [(0, 136, 255)],
        "regions": [
            block("door_cap_v_orange", (0, 0, 104, 66)),
            block("door_cap_v_green", (108, 0, 210, 66)),
            block("door_cap_v_pink", (218, 0, 320, 66)),
            block("door_cap_v_purple", (330, 0, 432, 66)),
            block("door_cap_v_blue", (0, 68, 104, 136)),
            block("door_cap_v_gray", (108, 68, 210, 136)),
            block("door_cap_h_blue", (0, 142, 70, 240)),
            block("door_cap_h_gray", (70, 142, 137, 240)),
            block("door_cap_h_orange", (137, 142, 206, 240)),
            block("door_cap_h_green", (0, 242, 70, 340)),
            block("door_cap_h_pink", (70, 242, 137, 340)),
            block("door_cap_h_purple", (137, 242, 206, 340)),
            rows("item", (208, 66, 438, 340), align="center"),
            rows("object", (0, 340, 438, 747)),
        ],
    },
    "doors": {
        "file": "miscellaneous/SNES - Super Metroid - Miscellaneous - Doors.png",
        "transparent": [(58, 124, 140)],
        # Captions, the ripper's avatar and the credits box.
        "exclude": [(315, 150, 697, 225), (620, 320, 697, 365), (0, 385, 697, 450)],
        "regions": [rows("door", (0, 0, 697, 385), merge=3)],
    },
    "starship": {
        "file": "miscellaneous/SNES - Super Metroid - Miscellaneous - Samus's Starship.png",
        "transparent": [(242, 29, 255)],
        "regions": [
            frames("lights", (0, 22, 218, 722), cycle=True),
            frames("hatch", (218, 22, 437, 900)),
            block("tiles_and_palettes", (20, 725, 205, 912)),
        ],
    },
    "status-bar": {
        "file": "miscellaneous/SNES - Super Metroid - Miscellaneous - Status Bar.png",
        # Black is the HUD background: it becomes index 0.
        "transparent": [(0, 0, 0)],
        "regions": [
            block("energy_text", (8, 23, 40, 31)),
            block("item_icons_off", (80, 61, 200, 78)),
            block("item_icons_on", (80, 90, 200, 108)),
            block("auto_reserve", (8, 109, 67, 134)),
            block("digits", (18, 141, 107, 150)),
            block("map_empty", (130, 171, 171, 195)),
            block("map_pink", (3, 198, 84, 264)),
            block("map_blue", (87, 198, 169, 264)),
            block("map_orange", (171, 198, 253, 264)),
        ],
    },
    "enemies": {
        "file": "enemies/SNES - Super Metroid - Enemies - Enemies.png",
        "transparent": [(255, 255, 255)],
        "regions": [
            rows("left", (0, 0, 274, 692)),
            rows("middle", (279, 0, 506, 692)),
            rows("right", (511, 0, 710, 692)),
        ],
    },
}


def pad8(value: int) -> int:
    return -(-value // TILE_SIZE) * TILE_SIZE


def blobs(image, box, transparent, merge, exclude):
    """Bounding boxes of opaque blobs in `box`, merging blobs closer than `merge` px."""

    crop = image.crop(box)
    width, height = crop.size
    pixels = crop.load()
    opaque = [[pixels[x, y] not in transparent for x in range(width)] for y in range(height)]
    for ex0, ey0, ex1, ey1 in exclude:
        for y in range(max(0, ey0 - box[1]), min(height, ey1 - box[1])):
            for x in range(max(0, ex0 - box[0]), min(width, ex1 - box[0])):
                opaque[y][x] = False
    seen = [[False] * width for _ in range(height)]
    found = []
    for y in range(height):
        for x in range(width):
            if not opaque[y][x] or seen[y][x]:
                continue
            seen[y][x] = True
            stack = [(x, y)]
            bx0, by0, bx1, by1 = x, y, x + 1, y + 1
            while stack:
                cx, cy = stack.pop()
                bx0, by0, bx1, by1 = min(bx0, cx), min(by0, cy), max(bx1, cx + 1), max(by1, cy + 1)
                for ny in range(max(0, cy - 1), min(height, cy + 2)):
                    for nx in range(max(0, cx - 1), min(width, cx + 2)):
                        if opaque[ny][nx] and not seen[ny][nx]:
                            seen[ny][nx] = True
                            stack.append((nx, ny))
            found.append([bx0, by0, bx1, by1])

    # merge < 0 only joins blobs whose boxes mostly overlap: parallel diagonal
    # lines have overlapping boxes but stay apart, pieces of one bar join.
    def close(a, b):
        if merge >= 0:
            return a[0] - merge < b[2] and b[0] - merge < a[2] and a[1] - merge < b[3] and b[1] - merge < a[3]
        w = min(a[2], b[2]) - max(a[0], b[0])
        h = min(a[3], b[3]) - max(a[1], b[1])
        smaller = min((a[2] - a[0]) * (a[3] - a[1]), (b[2] - b[0]) * (b[3] - b[1]))
        return w > 0 and h > 0 and w * h >= 0.5 * smaller

    merged = True
    while merged:
        merged = False
        for i in range(len(found)):
            for j in range(i + 1, len(found)):
                a, b = found[i], found[j]
                if close(a, b):
                    found[i] = [min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3])]
                    del found[j]
                    merged = True
                    break
            if merged:
                break
    return [(box[0] + a, box[1] + b, box[0] + c, box[1] + d) for a, b, c, d in found]


def split_rows(found):
    """Group blobs whose vertical spans overlap, then sort each row by x."""

    result = []
    for item in sorted(found, key=lambda b: b[1]):
        for row in result:
            top, bottom = row[0]
            overlap = min(bottom, item[3]) - max(top, item[1])
            if overlap > 0.5 * min(bottom - top, item[3] - item[1]):
                row[0] = (min(top, item[1]), max(bottom, item[3]))
                row[1].append(item)
                break
        else:
            result.append([(item[1], item[3]), [item]])
    result.sort(key=lambda row: row[0][0])
    return [sorted(row[1]) for row in result]


def reading_order(found):
    """Rows by center line (within 12 px), left to right inside each row."""

    ordered = sorted(found, key=lambda b: (b[1] + b[3]) / 2)
    lines = []
    for item in ordered:
        center = (item[1] + item[3]) / 2
        if lines and abs(center - lines[-1][0]) < 12:
            lines[-1][1].append(item)
        else:
            lines.append([center, [item]])
    return [b for _center, line in lines for b in sorted(line)]


def ring_order(found, box):
    """Group blobs by direction around the box center; keep reading order inside."""

    import math

    cx, cy = (box[0] + box[2]) / 2, (box[1] + box[3]) / 2
    sectors = {"r": 0, "dr": 1, "d": 2, "dl": 3, "l": 4, "ul": 5, "u": 6, "ur": 7}
    by_direction = {name: [] for name in DIRECTIONS}
    for item in found:
        angle = math.atan2((item[1] + item[3]) / 2 - cy, (item[0] + item[2]) / 2 - cx)
        sector = round(angle / (math.pi / 4)) % 8
        name = next(n for n, s in sectors.items() if s == sector)
        by_direction[name].append(item)
    ordered, names = [], []
    for name in DIRECTIONS:
        for index, item in enumerate(reading_order(by_direction[name])):
            ordered.append(item)
            names.append(f"{name}{index}")
    return ordered, names


def colors_of(image, box, transparent):
    return {c for c in image.crop(box).getdata() if c not in transparent}


def palette_chunks(image, frames, transparent):
    """Split consecutive frames into groups whose colors fit one palette."""

    chunks, current, colors = [], [], set()
    for frame in frames:
        frame_colors = colors_of(image, frame, transparent)
        if current and len(colors | frame_colors) > MAX_COLORS:
            chunks.append(current)
            current, colors = [], set()
        current.append(frame)
        colors |= frame_colors
    if current:
        chunks.append(current)
    return chunks


def save_indexed(canvas, background, transparent, path):
    colors = sorted({c for c in canvas.getdata() if c not in transparent}, key=lambda c: (sum(c), c))
    if len(colors) > MAX_COLORS:
        canvas.save(path, optimize=True)
        return len(colors), False
    lookup = {color: index + 1 for index, color in enumerate(colors)}
    out = Image.new("P", canvas.size, 0)
    out.putdata([lookup.get(c, 0) for c in canvas.getdata()])
    flat = [channel for color in [background] + colors for channel in color]
    out.putpalette(flat + [0] * (768 - len(flat)))
    out.info["transparency"] = 0
    out.save(path, optimize=True)
    return len(colors), True


def export_strip(image, sheet, name, frames, align, out_dir, writer, res, frame_names=None):
    background = sheet["transparent"][0]
    width = pad8(max(b[2] - b[0] for b in frames))
    height = pad8(max(b[3] - b[1] for b in frames))
    canvas = Image.new("RGB", (width * len(frames), height), background)
    for index, (x0, y0, x1, y1) in enumerate(frames):
        dx = index * width + (width - (x1 - x0)) // 2
        dy = height - (y1 - y0) if align == "bottom" else (height - (y1 - y0)) // 2
        canvas.paste(image.crop((x0, y0, x1, y1)), (dx, dy))
    file = f"{name}.png"
    count, ready = save_indexed(canvas, background, set(sheet["transparent"]), out_dir / file)
    # SGDK builds a frame from at most 16 hardware sprites of up to 32x32.
    ready = ready and width <= MAX_SPRITE and height <= MAX_SPRITE
    if ready:
        res.write(f'SPRITE {name} "{file}" {width // TILE_SIZE} {height // TILE_SIZE} FAST 0 NONE\n')
    writer.writerow([name, file, "strip", len(frames), width, height, count, "yes" if ready else "no",
                     " ".join(frame_names or []),
                     " ".join(f"{b[0]},{b[1]},{b[2] - b[0]}x{b[3] - b[1]}" for b in frames)])


def export_block(image, sheet, name, box, out_dir, writer, res):
    background = sheet["transparent"][0]
    width, height = pad8(box[2] - box[0]), pad8(box[3] - box[1])
    canvas = Image.new("RGB", (width, height), background)
    canvas.paste(image.crop(box), (0, 0))
    file = f"{name}.png"
    count, ready = save_indexed(canvas, background, set(sheet["transparent"]), out_dir / file)
    if ready:
        res.write(f'IMAGE {name} "{file}" BEST\n')
    writer.writerow([name, file, "block", 1, width, height, count, "yes" if ready else "no", "",
                     f"{box[0]},{box[1]},{box[2] - box[0]}x{box[3] - box[1]}"])


def export_frames(image, sheet, name, region, found, out_dir, writer, res):
    """Big art as one IMAGE per frame; with `cycle`, one IMAGE plus palettes."""

    background = sheet["transparent"][0]
    transparent = set(sheet["transparent"])
    found = reading_order(found)
    width = pad8(max(b[2] - b[0] for b in found))
    height = pad8(max(b[3] - b[1] for b in found))
    canvases = []
    for x0, y0, x1, y1 in found:
        canvas = Image.new("RGB", (width, height), background)
        canvas.paste(image.crop((x0, y0, x1, y1)), ((width - (x1 - x0)) // 2, height - (y1 - y0)))
        canvases.append(canvas)
    boxes = " ".join(f"{b[0]},{b[1]},{b[2] - b[0]}x{b[3] - b[1]}" for b in found)

    if region["cycle"]:
        # Index each pixel by its color in every frame: one shared tile set,
        # and palette k holds the colors of frame k.
        stacks = list(zip(*(canvas.getdata() for canvas in canvases)))
        opaque = sorted({s for s in stacks if s[0] not in transparent},
                        key=lambda s: (sum(map(sum, s)), s))
        if any((s[0] in transparent) != (c in transparent) for s in stacks for c in s):
            raise RuntimeError(f"{name}: quadros com formas diferentes; não é ciclo de paleta")
        ready = len(opaque) <= MAX_COLORS
        file = f"{name}.png"
        if ready:
            lookup = {s: i + 1 for i, s in enumerate(opaque)}
            out = Image.new("P", canvases[0].size, 0)
            out.putdata([lookup.get(s, 0) for s in stacks])
            flat = [ch for color in [background] + [s[0] for s in opaque] for ch in color]
            out.putpalette(flat + [0] * (768 - len(flat)))
            out.info["transparency"] = 0
            out.save(out_dir / file, optimize=True)
            res.write(f'IMAGE {name} "{file}" BEST\n')
            for index in range(len(canvases)):
                pal_name = f"{name}_pal_{index:02d}"
                swatch = Image.new("P", (16, 1), 0)
                swatch.putdata(list(range(16)))
                flat = [ch for color in [background] + [s[index] for s in opaque] for ch in color]
                swatch.putpalette(flat + [0] * (768 - len(flat)))
                swatch.save(out_dir / f"{pal_name}.png", optimize=True)
                res.write(f'PALETTE {pal_name} "{pal_name}.png"\n')
        else:
            canvases[0].save(out_dir / file, optimize=True)
        writer.writerow([name, file, "image+palettes", len(canvases), width, height, len(opaque),
                         "yes" if ready else "no", f"paletas {name}_pal_00..{len(canvases) - 1:02d}", boxes])
        return 1

    for index, canvas in enumerate(canvases):
        frame_name = f"{name}_{index:02d}"
        file = f"{frame_name}.png"
        count, ready = save_indexed(canvas, background, transparent, out_dir / file)
        if ready:
            res.write(f'IMAGE {frame_name} "{file}" BEST\n')
        writer.writerow([frame_name, file, "image", 1, width, height, count, "yes" if ready else "no", "",
                         boxes.split(" ")[index]])
    return len(canvases)


def extract_sheet(key, sheet, references, output):
    image = Image.open(references / sheet["file"]).convert("RGB")
    transparent = set(sheet["transparent"])
    out_dir = output / f"{key}-original-sgdk"
    if out_dir.is_dir():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True)
    prefix = key.replace("-", "_")
    total = 0
    with (out_dir / "manifest.csv").open("w", newline="", encoding="utf-8") as manifest, \
            (out_dir / f"{prefix}.res").open("w", encoding="utf-8", newline="\n") as res:
        writer = csv.writer(manifest, lineterminator="\n")
        writer.writerow(["resource", "file", "kind", "frames", "frame_width", "frame_height",
                         "colors", "sgdk_ready", "frame_names", "source_boxes"])
        res.write("# Generated by scripts/extract_sheet_sprites.py\n")
        res.write(f"# Source: original-references/{sheet['file']}\n")
        for region in sheet["regions"]:
            name = f"{prefix}_{region['name']}"
            if region["kind"] == "block":
                export_block(image, sheet, name, region["box"], out_dir, writer, res)
                total += 1
                continue
            found = blobs(image, region["box"], transparent, region["merge"], sheet.get("exclude", []))
            if region["kind"] == "frames":
                total += export_frames(image, sheet, name, region, found, out_dir, writer, res)
                continue
            if region.get("ring"):
                ordered, ring_names = ring_order(found, region["box"])
                export_strip(image, sheet, name, ordered, region["align"], out_dir, writer, res, ring_names)
                total += 1
                continue
            groups = [reading_order(found)] if region["kind"] == "strip" else split_rows(found)
            for row_index, frames in enumerate(groups):
                row_name = name if region["kind"] == "strip" else f"{name}_r{row_index:02d}"
                chunks = palette_chunks(image, frames, transparent)
                for chunk_index, chunk in enumerate(chunks):
                    chunk_name = row_name if len(chunks) == 1 else f"{row_name}_{chunk_index}"
                    names = DIRECTIONS if region.get("directions") and len(chunk) == 8 else None
                    export_strip(image, sheet, chunk_name, chunk, region["align"], out_dir, writer, res, names)
                    total += 1
    return total


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("sheets", nargs="*", help=f"Pranchas (padrão: todas): {', '.join(SHEETS)}")
    parser.add_argument("--references", type=Path, default=REFERENCES, help="Pasta das pranchas originais")
    parser.add_argument("--output", type=Path, default=OUTPUT, help="Pasta onde criar <prancha>-original-sgdk/")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    unknown = [key for key in args.sheets if key not in SHEETS]
    if unknown:
        raise SystemExit(f"Prancha desconhecida: {', '.join(unknown)}")
    for key in args.sheets or SHEETS:
        total = extract_sheet(key, SHEETS[key], args.references, args.output)
        print(f"{key}: {total} recursos")


if __name__ == "__main__":
    main()
