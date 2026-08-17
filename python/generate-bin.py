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

TILE_RE = re.compile(r"^tile_r(\d+)_c(\d+)\.(?:jpe?g|png|bmp|gif|webp)$", re.IGNORECASE)


def resolve_default_path(*parts: str) -> Path:
    cwd_candidate = Path.cwd().joinpath(*parts)
    root_candidate = PROJECT_ROOT.joinpath(*parts)
    if cwd_candidate.exists() or not root_candidate.exists():
        return cwd_candidate
    return root_candidate


def parse_tile_name(path: Path):
    match = TILE_RE.match(path.name)
    if match is None:
        return None
    row = int(match.group(1))
    col = int(match.group(2))
    return row, col


def build_bitmask(present_tiles):
    bitmask = bytearray(2048)
    for row, col in present_tiles:
        index = row * 128 + col
        if 0 <= index < 128 * 128:
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
        if row >= 128 or col >= 128:
            continue
        tiles.append((row, col, path))
    return sorted(tiles, key=lambda item: (item[0] * 128 + item[1], item[2].name))


def generate_archive(input_dir: Path, output_path: Path):
    tile_entries = collect_tiles(input_dir)
    if not tile_entries:
        raise FileNotFoundError(f"No valid tile images found in {input_dir}")

    present_tiles = [(row, col) for row, col, _ in tile_entries]
    bitmask = build_bitmask(present_tiles)

    entries = []
    current_offset = 2048 + (len(tile_entries) * 8)
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

    output = bytearray(2048 + len(entries) * 8 + sum(len(item["data"]) for item in entries))
    output[0:2048] = bitmask

    cursor = 2048
    for entry in entries:
        struct.pack_into("<II", output, cursor, entry["offset"], entry["size"])
        cursor += 8

    cursor = 2048 + len(entries) * 8
    for entry in entries:
        output[cursor:cursor + len(entry["data"])] = entry["data"]
        cursor += len(entry["data"])

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(output)
    return len(entries)


def main():
    parser = argparse.ArgumentParser(description="Generate a packed tile binary from filtered tile images")
    parser.add_argument("--input", "-i", default=str(resolve_default_path("output", "rm-black-images")), help="Folder containing tile images")
    parser.add_argument("--output", "-o", default=str(resolve_default_path("output", "output.bin")), help="Output archive path")
    args = parser.parse_args()

    input_dir = Path(args.input)
    output_path = Path(args.output)

    if not input_dir.exists() or not input_dir.is_dir():
        print(f"Input folder not found: {input_dir}")
        return 0

    try:
        count = generate_archive(input_dir, output_path)
        print(f"Created {output_path} with {count} tiles")
    except FileNotFoundError as exc:
        print(str(exc))
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
