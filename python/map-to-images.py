#!/usr/bin/env python3
"""Convert an input image into many 320x240 JPEG tiles.

Usage:
  python main.py --input input.png --output output

Options saved for JPEG: quality=70, progressive, optimize, subsampling=2 (4:2:0)
Smoothing is applied as a mild Gaussian blur before saving (0..1).
"""
from __future__ import annotations

import argparse
import math
import os
from pathlib import Path

from PIL import Image, ImageFilter


Image.MAX_IMAGE_PIXELS = 300_000_000

PROJECT_ROOT = Path(__file__).resolve().parent.parent


def resolve_default_path(*parts: str) -> Path:
    cwd_candidate = Path.cwd().joinpath(*parts)
    root_candidate = PROJECT_ROOT.joinpath(*parts)
    if cwd_candidate.exists() or not root_candidate.exists():
        return cwd_candidate
    return root_candidate


def tile_image(
    img: Image.Image,
    tile_w: int,
    tile_h: int,
    out_dir: Path,
    image_index: int = 0,
    quality: int = 70,
    smoothing: float = 0.2,
    progressive: bool = False,
    optimize: bool = True,
    subsampling: int = 2,
):
    img = img.convert("RGB")
    w, h = img.size
    cols = math.ceil(w / tile_w)
    rows = math.ceil(h / tile_h)
    out_dir.mkdir(parents=True, exist_ok=True)
    count = 0

    for row in range(rows):
        for col in range(cols):
            left = col * tile_w
            upper = row * tile_h
            right = min(left + tile_w, w)
            lower = min(upper + tile_h, h)

            crop = img.crop((left, upper, right, lower))

            # If the cropped tile is smaller than tile size, paste onto black canvas
            if crop.size != (tile_w, tile_h):
                canvas = Image.new("RGB", (tile_w, tile_h), (0, 0, 0))
                canvas.paste(crop, (0, 0))
                tile = canvas
            else:
                tile = crop

            # Apply mild smoothing as Gaussian blur (user-level 0..1 mapped to sigma)
            if smoothing and smoothing > 0:
                sigma = float(smoothing) * 2.0
                if sigma > 0:
                    tile = tile.filter(ImageFilter.GaussianBlur(radius=sigma))

            filename = f"tile_r{row:03d}_c{col:03d}.jpg"
            out_path = out_dir / filename
            tile.save(
                out_path,
                format="JPEG",
                quality=quality,
                progressive=progressive,
                optimize=optimize,
                subsampling=subsampling,
            )
            count += 1

    return count


def main():
    p = argparse.ArgumentParser(description="Slice an image into 320x240 JPEG tiles")
    p.add_argument("--input", "-i", default=str(resolve_default_path("input")), help="Input image path")
    p.add_argument("--output", "-o", default=str(resolve_default_path("output", "map-to-images")), help="Output directory")
    p.add_argument("--tile-width", type=int, default=320, help="Tile width (default 320)")
    p.add_argument("--tile-height", type=int, default=240, help="Tile height (default 240)")
    p.add_argument("--quality", type=int, default=70, help="JPEG quality 1-95 (default 70)")
    p.add_argument("--smoothing", type=float, default=0.2, help="Smoothing 0..1 (default 0.2)")
    p.add_argument("--progressive", dest="progressive", action="store_true", default=False, help="Generate progressive JPEG (unsupported by TJpgDec)")
    p.add_argument("--no-optimize", dest="optimize", action="store_false", help="Disable optimize flag")
    args = p.parse_args()

    inp = Path(args.input)
    out_dir = Path(args.output)

    if not inp.exists():
        print(f"Input folder not found: {inp}")
        print("Create this folder and put your images inside it, or pass --input <folder>.")
        return 0

    files = []
    if inp.is_dir():
        exts = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tiff", ".webp"}
        for f in sorted(inp.iterdir()):
            if f.is_file() and f.suffix.lower() in exts:
                files.append(f)
    elif inp.is_file():
        files = [inp]
    else:
        print(f"Input path is neither a file nor a directory: {inp}")
        return 0

    if not files:
        print(f"No supported image files found in: {inp}")
        return 0

    total_tiles = 0
    for idx, fpath in enumerate(files, start=0):
        try:
            img = Image.open(fpath)
        except Exception as e:
            print(f"Skipping {fpath}: cannot open ({e})")
            continue

        saved = tile_image(
            img,
            args.tile_width,
            args.tile_height,
            out_dir,
            image_index=idx,
            quality=args.quality,
            smoothing=args.smoothing,
            progressive=args.progressive,
            optimize=args.optimize,
            subsampling=2,
        )
        total_tiles += saved
        print(f"[{idx}/{len(files)}] {fpath.name}: saved {saved} tiles")

    print(f"Saved {total_tiles} tiles to {out_dir}")


if __name__ == "__main__":
    raise SystemExit(main())
