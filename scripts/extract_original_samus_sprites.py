#!/usr/bin/env python3
"""Extract boxed and loose Samus sprites from the local reference image.

Generated PNGs are indexed, use palette index 0 for transparency, and contain
at most 15 visible colors so they fit a Mega Drive 16-color sprite palette.
"""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path

from PIL import Image, ImageChops


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = (
    ROOT
    / "res"
    / "images"
    / "original-references"
    / "SNES - Super Metroid - Playable Characters - Samus Aran.png"
)
DEFAULT_OUTPUT = ROOT / "res" / "images" / "samus-original-sgdk"
CELL_BACKGROUND = (192, 128, 246)
SHEET_BACKGROUND = (230, 164, 244)
LABEL_BLUE = (30, 144, 255)
CELL_SIZE = 80
TILE_SIZE = 8
MAX_VISIBLE_COLORS = 15
LOOSE_MIN_WIDTH = 10
LOOSE_MIN_HEIGHT = 20
LOOSE_MIN_PIXELS = 60
LOOSE_MAX_DIMENSION = 80
TRANSPARENT_COLORS = {CELL_BACKGROUND, SHEET_BACKGROUND, LABEL_BLUE}


def flat_pixels(image: Image.Image) -> list[tuple[int, ...] | int]:
    get_flattened_data = getattr(image, "get_flattened_data", None)
    return list(get_flattened_data() if get_flattened_data else image.getdata())


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


def merge_intervals(intervals: list[tuple[int, int]]) -> list[tuple[int, int]]:
    merged: list[list[int]] = []
    for x0, x1 in sorted(intervals):
        if merged and x0 <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], x1)
        else:
            merged.append([x0, x1])
    return [(x0, x1) for x0, x1 in merged]


def find_loose_sprites(
    image: Image.Image, cells: list[tuple[int, int, int, int]]
) -> list[tuple[int, int, int, int]]:
    """Find larger unboxed art while ignoring captions, swatches, and box cells."""

    pixels = image.load()
    width, height = image.size
    excluded_by_row: list[list[tuple[int, int]]] = [[] for _ in range(height)]
    for x0, y0, x1, y1 in cells:
        left = max(0, x0 - 2)
        right = min(width, x1 + 2)
        for y in range(max(0, y0 - 2), min(height, y1 + 2)):
            excluded_by_row[y].append((left, right))
    excluded_by_row = [merge_intervals(row) for row in excluded_by_row]

    components = Components()
    previous: list[tuple[int, int, int]] = []
    for y in range(height):
        excluded = excluded_by_row[y]
        excluded_index = 0
        current: list[tuple[int, int, int]] = []
        x = 0

        while x < width:
            while excluded_index < len(excluded) and excluded[excluded_index][1] <= x:
                excluded_index += 1
            if excluded_index < len(excluded) and excluded[excluded_index][0] <= x:
                x = excluded[excluded_index][1]
                continue
            if pixels[x, y] in TRANSPARENT_COLORS:
                x += 1
                continue

            x0 = x
            while x < width:
                while excluded_index < len(excluded) and excluded[excluded_index][1] <= x:
                    excluded_index += 1
                if excluded_index < len(excluded) and excluded[excluded_index][0] <= x:
                    break
                if pixels[x, y] in TRANSPARENT_COLORS:
                    break
                x += 1

            label = components.add(x0, y, x)
            join_runs(components, label, x0, x, previous, diagonal=True)
            current.append((x0, x, label))

        previous = current

    sprites = []
    for x0, y0, x1, y1, area in roots_with_bounds(components):
        sprite_width = x1 - x0
        sprite_height = y1 - y0
        if (
            LOOSE_MIN_WIDTH <= sprite_width <= LOOSE_MAX_DIMENSION
            and LOOSE_MIN_HEIGHT <= sprite_height <= LOOSE_MAX_DIMENSION
            and area >= LOOSE_MIN_PIXELS
        ):
            sprites.append((x0, y0, x1, y1))
    return sorted(sprites, key=lambda box: (box[1], box[0]))


def indexed_pixels(rgba: Image.Image) -> tuple[Image.Image, int, bool, int]:
    """Return a P-mode PNG image, with palette index 0 reserved for alpha."""

    rgba_pixels = flat_pixels(rgba)
    opaque_colors = [pixel[:3] for pixel in rgba_pixels if pixel[3] != 0]
    unique_colors = list(dict.fromkeys(opaque_colors))
    color_count = len(unique_colors)
    quantized = color_count > MAX_VISIBLE_COLORS

    if quantized:
        samples = Image.new("RGB", (len(opaque_colors), 1))
        samples.putdata(opaque_colors)
        palette_image = samples.quantize(
            colors=MAX_VISIBLE_COLORS,
            method=Image.Quantize.MEDIANCUT,
            dither=Image.Dither.NONE,
        )
        sample_indices = flat_pixels(palette_image)
        palette = palette_image.getpalette() or []
        visible_palette = [
            tuple(palette[index : index + 3])
            for index in range(0, MAX_VISIBLE_COLORS * 3, 3)
        ]
        color_to_index: dict[tuple[int, int, int], int] = {}
        for color, palette_index in zip(opaque_colors, sample_indices):
            color_to_index.setdefault(color, palette_index + 1)
    else:
        visible_palette = unique_colors
        color_to_index = {color: index + 1 for index, color in enumerate(unique_colors)}

    output_indices = []
    for pixel in rgba_pixels:
        if pixel[3] == 0:
            output_indices.append(0)
        else:
            output_indices.append(color_to_index[pixel[:3]])

    indexed = Image.new("P", rgba.size, 0)
    indexed.putdata(output_indices)
    palette_data = [0, 0, 0]
    for color in visible_palette:
        palette_data.extend(color)
    palette_data.extend([0] * (768 - len(palette_data)))
    indexed.putpalette(palette_data)
    indexed.info["transparency"] = 0
    return indexed, color_count, quantized, sum(pixel[3] != 0 for pixel in rgba_pixels)


def padded_size(width: int, height: int) -> tuple[int, int]:
    return (
        math.ceil(width / TILE_SIZE) * TILE_SIZE,
        math.ceil(height / TILE_SIZE) * TILE_SIZE,
    )


def make_transparent_crop(
    image: Image.Image,
    box: tuple[int, int, int, int],
    transparent_colors: set[tuple[int, int, int]],
) -> tuple[Image.Image, tuple[int, int, int, int]] | None:
    crop = image.crop(box).convert("RGBA")
    pixels = flat_pixels(crop)
    crop.putdata(
        [
            (red, green, blue, 0 if (red, green, blue) in transparent_colors else 255)
            for red, green, blue, _alpha in pixels
        ]
    )
    content_box = crop.getchannel("A").getbbox()
    if content_box is None:
        return None
    return crop.crop(content_box), content_box


def extract(source: Path, output: Path) -> tuple[int, int, int]:
    if not source.is_file():
        raise FileNotFoundError(f"Imagem de referência não encontrada: {source}")

    image = Image.open(source).convert("RGB")
    cells = find_cells(image)
    if not cells:
        raise RuntimeError(
            "Nenhum quadro 80x80 encontrado; confira a cor de fundo e o formato da imagem."
        )
    loose_sprites = find_loose_sprites(image, cells)

    frames_dir = output / "frames"
    frames_dir.mkdir(parents=True, exist_ok=True)
    for pattern in ("samus_ref_*.png", "samus_loose_*.png"):
        for stale_frame in frames_dir.glob(pattern):
            stale_frame.unlink()
    manifest_path = output / "manifest.csv"
    resource_path = output / "sprites.res"
    manifest_fields = [
        "index",
        "resource",
        "file",
        "kind",
        "source_x",
        "source_y",
        "container_width",
        "container_height",
        "sprite_x",
        "sprite_y",
        "width",
        "height",
        "tiles_wide",
        "tiles_high",
        "source_colors",
        "quantized_to_15_colors",
        "opaque_pixels",
    ]

    extracted = 0
    quantized_count = 0
    loose_count = 0
    with manifest_path.open("w", newline="", encoding="utf-8") as manifest_file:
        writer = csv.DictWriter(manifest_file, fieldnames=manifest_fields)
        writer.writeheader()
        with resource_path.open("w", encoding="utf-8", newline="\n") as resource_file:
            resource_file.write("# Generated by scripts/extract_original_samus_sprites.py\n")
            resource_file.write("# Copy selected entries into the project's active .res file.\n")

            def export_sprite(
                name: str,
                kind: str,
                source_x: int,
                source_y: int,
                container_width: int,
                container_height: int,
                sprite_x: int,
                sprite_y: int,
                content: Image.Image,
            ) -> None:
                nonlocal extracted, quantized_count
                tile_width, tile_height = padded_size(*content.size)
                canvas = Image.new("RGBA", (tile_width, tile_height), (0, 0, 0, 0))
                canvas.alpha_composite(content)
                indexed, color_count, quantized, opaque_count = indexed_pixels(canvas)

                filename = f"{name}.png"
                relative_file = f"frames/{filename}"
                indexed.save(frames_dir / filename, format="PNG", optimize=True)

                writer.writerow(
                    {
                        "index": extracted + 1,
                        "resource": name,
                        "file": relative_file,
                        "kind": kind,
                        "source_x": source_x,
                        "source_y": source_y,
                        "container_width": container_width,
                        "container_height": container_height,
                        "sprite_x": sprite_x,
                        "sprite_y": sprite_y,
                        "width": content.width,
                        "height": content.height,
                        "tiles_wide": tile_width // TILE_SIZE,
                        "tiles_high": tile_height // TILE_SIZE,
                        "source_colors": color_count,
                        "quantized_to_15_colors": str(quantized).lower(),
                        "opaque_pixels": opaque_count,
                    }
                )
                resource_file.write(
                    f'SPRITE {name} "{relative_file}" '
                    f"{tile_width // TILE_SIZE} {tile_height // TILE_SIZE} FAST 1 NONE\n"
                )
                extracted += 1
                quantized_count += int(quantized)

            boxed_count = 0
            for x0, y0, x1, y1 in cells:
                content_result = make_transparent_crop(image, (x0, y0, x1, y1), {CELL_BACKGROUND})
                if content_result is None:
                    continue
                content, content_box = content_result
                boxed_count += 1
                export_sprite(
                    f"samus_ref_{boxed_count:04d}",
                    "boxed",
                    x0,
                    y0,
                    CELL_SIZE,
                    CELL_SIZE,
                    content_box[0],
                    content_box[1],
                    content,
                )

            for loose_index, (x0, y0, x1, y1) in enumerate(loose_sprites, start=1):
                content_result = make_transparent_crop(
                    image, (x0, y0, x1, y1), TRANSPARENT_COLORS
                )
                if content_result is None:
                    continue
                content, content_box = content_result
                export_sprite(
                    f"samus_loose_{loose_index:04d}",
                    "unboxed",
                    x0,
                    y0,
                    x1 - x0,
                    y1 - y0,
                    content_box[0],
                    content_box[1],
                    content,
                )
                loose_count += 1

    return extracted, quantized_count, loose_count


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_SOURCE, help="PNG da prancha original")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="Pasta de saída local")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    extracted, quantized_count, loose_count = extract(args.input, args.output)
    print(f"Sprites exportados: {extracted} ({extracted - loose_count} em quadros, {loose_count} avulsos)")
    print(f"Sprites quantizados para 15 cores visíveis: {quantized_count}")
    print(f"Saída: {args.output.resolve()}")


if __name__ == "__main__":
    main()
