#!/usr/bin/env python3
"""Generate a packed binary archive from tile images stored in output/rm-black-images.

Format:
- 2048 bytes bitmask (128 x 128 tiles)
- For each present tile, a 32-bit offset and 32-bit size little-endian entry
- Tile payload data appended immediately after the table

Tile naming convention:
    tile_r<row>_c<col>.jpg
Example:
    tile_r001_c038.jpg => bit index = 128 * 1 + 38
"""
from __future__ import annotations

import argparse
import re
import struct
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent

GRID_ROWS = 98
GRID_COLS = 112
BITMASK_BYTES = ((GRID_ROWS * GRID_COLS) + 7) // 8

TILE_RE = re.compile(r"^tile_r(\d+)_c(\d+)\.(?:jpe?g|png|bmp|gif|webp)$", re.IGNORECASE)


def resolve_default_path(*parts: str) -> Path:
    cwd_candidate = Path.cwd().joinpath(*parts)
    root_candidate = PROJECT_ROOT.joinpath(*parts)
    if cwd_candidate.exists() or not root_candidate.exists():
        return cwd_candidate
    return root_candidate


def sanitize_identifier(name: str) -> str:
    cleaned = re.sub(r"[^0-9A-Za-z_]+", "_", name).strip("_")
    if not cleaned:
        cleaned = "tile_archive"
    if cleaned[0].isdigit():
        cleaned = f"archive_{cleaned}"
    return cleaned


def parse_tile_name(path: Path):
    match = TILE_RE.match(path.name)
    if match is None:
        return None
    row = int(match.group(1))
    col = int(match.group(2))
    return row, col


def build_bitmask(present_tiles):
    bitmask = bytearray(BITMASK_BYTES)
    for row, col in present_tiles:
        index = row * GRID_COLS + col
        if 0 <= index < GRID_ROWS * GRID_COLS:
            byte_index = index // 8
            bit_index = index % 8
            bitmask[byte_index] |= 1 << bit_index
    return bytes(bitmask)


def collect_tiles(input_dir: Path):
    tiles = []
    for path in sorted(input_dir.iterdir()):
        if not path.is_file():
            continue
        parsed = parse_tile_name(path)
        if parsed is None:
            continue
        row, col = parsed
        if row >= GRID_ROWS or col >= GRID_COLS:
            continue
        tiles.append((row, col, path))
    return sorted(tiles, key=lambda item: (item[0] * 128 + item[1], item[2].name))


def generate_archive_data(input_dir: Path):
    tile_entries = collect_tiles(input_dir)
    if not tile_entries:
        raise FileNotFoundError(f"No valid tile images found in {input_dir}")

    present_tiles = [(row, col) for row, col, _ in tile_entries]
    bitmask = build_bitmask(present_tiles)

    entries = []
    current_offset = BITMASK_BYTES + (len(tile_entries) * 8)
    for row, col, path in tile_entries:
        data = path.read_bytes()
        entries.append({
            "row": row,
            "col": col,
            "offset": current_offset,
            "size": len(data),
            "data": data,
        })
        current_offset += len(data)

    output = bytearray(BITMASK_BYTES + len(entries) * 8 + sum(len(item["data"]) for item in entries))
    output[0:BITMASK_BYTES] = bitmask

    cursor = BITMASK_BYTES
    for entry in entries:
        struct.pack_into("<II", output, cursor, entry["offset"], entry["size"])
        cursor += 8

    cursor = BITMASK_BYTES + len(entries) * 8
    for entry in entries:
        output[cursor:cursor + len(entry["data"])] = entry["data"]
        cursor += len(entry["data"])

    return entries, bytes(output)


def build_header_text(prefix: str, header_path: Path) -> str:
    guard = sanitize_identifier(f"asset_{header_path.stem}_h").upper()
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <stdint.h>",
        "",
        "#define GRID_ROWS 98u",
        "#define GRID_COLS 112u",
        "",
        f"extern const uint8_t {prefix}_bitmask[GRID_ROWS][GRID_COLS];",
        f"extern const uint32_t {prefix}_offsets[GRID_ROWS][GRID_COLS];",
        f"extern const uint32_t {prefix}_sizes[GRID_ROWS][GRID_COLS];",
        "",
        "#endif",
        "",
    ]
    return "\n".join(lines)


def build_source_text(prefix: str, header_name: str, entries: list[dict]) -> str:
    offset_matrix = [[0 for _ in range(GRID_COLS)] for _ in range(GRID_ROWS)]
    size_matrix = [[0 for _ in range(GRID_COLS)] for _ in range(GRID_ROWS)]
    bitmask_matrix = [[0 for _ in range(GRID_COLS)] for _ in range(GRID_ROWS)]

    for entry in entries:
        row = entry["row"]
        col = entry["col"]
        offset_matrix[row][col] = entry["offset"]
        size_matrix[row][col] = entry["size"]
        bitmask_matrix[row][col] = 1

    bitmask_rows = []
    for row in range(GRID_ROWS):
        row_values = ", ".join(str(bitmask_matrix[row][col]) for col in range(GRID_COLS))
        bitmask_rows.append(f"    {{ {row_values} }}")

    offset_rows = []
    for row in range(GRID_ROWS):
        row_values = ", ".join(str(offset_matrix[row][col]) for col in range(GRID_COLS))
        offset_rows.append(f"    {{ {row_values} }}")

    size_rows = []
    for row in range(GRID_ROWS):
        row_values = ", ".join(str(size_matrix[row][col]) for col in range(GRID_COLS))
        size_rows.append(f"    {{ {row_values} }}")

    lines = [
        f'#include "{header_name}"',
        "",
        f"const uint8_t {prefix}_bitmask[GRID_ROWS][GRID_COLS] = {{",
        *bitmask_rows,
        "};",
        "",
        f"const uint32_t {prefix}_offsets[GRID_ROWS][GRID_COLS] = {{",
        *offset_rows,
        "};",
        "",
        f"const uint32_t {prefix}_sizes[GRID_ROWS][GRID_COLS] = {{",
        *size_rows,
        "};",
        "",
    ]
    return "\n".join(lines)


def generate_archive(input_dir: Path, output_path: Path, header_path: Path | None = None, source_path: Path | None = None):
    entries, archive_bytes = generate_archive_data(input_dir)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(archive_bytes)

    header_name = header_path.name if header_path is not None else output_path.with_suffix(".h").name
    source_name = source_path.name if source_path is not None else output_path.with_suffix(".c").name

    if header_path is None:
        header_path = output_path.with_suffix(".h")
    if source_path is None:
        source_path = output_path.with_suffix(".c")

    prefix = sanitize_identifier(output_path.stem)
    header_path.parent.mkdir(parents=True, exist_ok=True)
    source_path.parent.mkdir(parents=True, exist_ok=True)

    header_path.write_text(build_header_text(prefix, header_path), encoding="utf-8")
    source_path.write_text(build_source_text(prefix, header_name, entries), encoding="utf-8")
    return len(entries)


def main():
    parser = argparse.ArgumentParser(description="Generate a packed tile binary from filtered tile images")
    parser.add_argument("--input", "-i", default=str(resolve_default_path("output", "rm-black-images")), help="Folder containing tile images")
    parser.add_argument("--output", "-o", default=str(resolve_default_path("output", "data.bin")), help="Output archive path (.bin)")
    parser.add_argument("--header", "-H", dest="header", default=str(resolve_default_path("output", "data.h")), help="Header output path (.h)")
    parser.add_argument("--source", "-C", dest="source", default=str(resolve_default_path("output", "data.c")), help="C source output path (.c)")
    args = parser.parse_args()

    input_dir = Path(args.input)
    output_path = Path(args.output)
    header_path = Path(args.header) if args.header else None
    source_path = Path(args.source) if args.source else None

    if not input_dir.exists() or not input_dir.is_dir():
        print(f"Input folder not found: {input_dir}")
        return 0

    try:
        header_path = header_path or output_path.with_suffix(".h")
        source_path = source_path or output_path.with_suffix(".c")
        count = generate_archive(input_dir, output_path, header_path, source_path)
        print(f"Created {output_path} with {count} tiles")
        print(f"Created {header_path}")
        print(f"Created {source_path}")
    except FileNotFoundError as exc:
        print(str(exc))
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
