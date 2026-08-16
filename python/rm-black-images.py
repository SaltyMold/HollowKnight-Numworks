#!/usr/bin/env python3
"""Remove images that are >90% black and copy the rest to an output folder.

Usage:
    python rm-black-images.py --input output/map-to-images --output output/rm-black-images

Behavior:
- If an image contains > `--black-ratio` fraction of pixels whose luminance <= `--black-threshold`,
  it is considered black and will be deleted from the input folder.
- Non-black images are copied to the output folder.
"""
from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

from PIL import Image


PROJECT_ROOT = Path(__file__).resolve().parent.parent


def resolve_default_path(*parts: str) -> Path:
    cwd_candidate = Path.cwd().joinpath(*parts)
    root_candidate = PROJECT_ROOT.joinpath(*parts)
    if cwd_candidate.exists() or not root_candidate.exists():
        return cwd_candidate
    return root_candidate


def is_mostly_black(img: Image.Image, threshold: int = 16, ratio: float = 0.99) -> bool:
    # convert to grayscale
    g = img.convert("L")
    # optionally downscale large images for faster processing
    max_size = (800, 800)
    if g.width > max_size[0] or g.height > max_size[1]:
        g = g.copy()
        g.thumbnail(max_size, Image.Resampling.BILINEAR)

    pixels = list(g.getdata())
    if not pixels:
        return True
    black = sum(1 for p in pixels if p <= threshold)
    return (black / len(pixels)) >= ratio


def process_folder(inp: Path, out_dir: Path, threshold: int, ratio: float, delete_black: bool = False):
    out_dir.mkdir(parents=True, exist_ok=True)
    exts = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tiff", ".webp"}

    files = [p for p in sorted(inp.iterdir()) if p.is_file() and p.suffix.lower() in exts]
    total = 0
    skipped_black = 0
    copied = 0

    for p in files:
        try:
            img = Image.open(p)
        except Exception as e:
            print(f"Skipping {p.name}: cannot open ({e})")
            continue

        total += 1
        try:
            black = is_mostly_black(img, threshold=threshold, ratio=ratio)
        except Exception as e:
            print(f"Error processing {p.name}: {e}")
            continue

        if black:
            skipped_black += 1
            print(f"Skipped black image: {p.name}")
            continue

        dst = out_dir / p.name
        try:
            shutil.copy2(p, dst)
            copied += 1
            print(f"Copied valid image: {p.name}")
        except Exception as e:
            print(f"Failed to copy {p.name} -> {dst}: {e}")

    print(f"Processed {total} images: copied={copied}, skipped_black={skipped_black}")


def main(argv=None):
    p = argparse.ArgumentParser(description="Remove images that are mostly black")
    p.add_argument("--input", "-i", default=str(resolve_default_path("output", "map-to-images")), help="Input folder")
    p.add_argument("--output", "-o", default=str(resolve_default_path("output", "rm-black-images")), help="Output folder for kept images")
    p.add_argument("--black-threshold", type=int, default=16, help="Pixel value <= threshold considered black (0-255)")
    p.add_argument("--black-ratio", type=float, default=0.99, help="Fraction of pixels that must be black to consider the image black")
    p.add_argument("--delete", dest="delete", action="store_true", help="Deprecated: kept for compatibility; black files are never deleted anymore")
    args = p.parse_args(argv)

    inp = Path(args.input)
    out_dir = Path(args.output)

    if not inp.exists() or not inp.is_dir():
        print(f"Input folder not found: {inp}")
        print("Nothing to process. Generate the tiles first, then rerun this script.")
        return 0

    # avoid copying into same folder being scanned
    try:
        if out_dir.resolve().is_relative_to(inp.resolve()):
            p.error("Output folder must not be inside input folder")
    except Exception:
        pass

    process_folder(inp, out_dir, threshold=args.black_threshold, ratio=args.black_ratio, delete_black=False)
    return 0


if __name__ == "__main__":
    main()
