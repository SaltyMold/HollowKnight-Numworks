# Tile archive generation

This folder contains scripts that convert a large source image into a set of tile images, then filter out mostly-black tiles, and finally pack the valid tiles into a single binary archive.

## 1. Map images to tiles

Use the script `map-to-images.py` to slice an input image or an input folder into JPEG tiles.

Example:

```bash
python map-to-images.py --input input --output output/map-to-images
```

The generated files follow this naming pattern:

```text
tile_r000_c000.jpg
tile_r000_c001.jpg
tile_r001_c038.jpg
```

The row and column numbers are used to compute the position in the 64x64 grid.

## 2. Remove mostly-black tiles

Use the script `rm-black-images.py` to copy only tiles whose black-pixel ratio is below the threshold.

Example:

```bash
python rm-black-images.py --input output/map-to-images --output output/rm-black-images
```

Only valid tiles are copied to the output folder.

## 3. Pack the filtered tiles into a binary archive

Use `generate-bin.py` to generate a single binary archive from the contents of `output/rm-black-images`.

Example:

```bash
python generate-bin.py --input output/rm-black-images --output output/output.bin
```

## Binary format

The generated archive follows this structure:

1. Bitmask: 512 bytes (64 x 64 entries)
2. For each present tile: 8 bytes of metadata
   - 4-byte little-endian offset
   - 4-byte little-endian size
3. Tile payload data appended after the metadata table

### Bitmask generation

The bitmask is computed from the tile filename.

For a tile named:

```text
tile_r001_c038.jpg
```

The bit index is:

```text
64 * 1 + 38 = 102
```

That bit is set to `1` in the 512-byte bitmask. If a tile is absent, its bit remains `0`.

### Offset and size

Each tile entry stores:

- `offset`: the absolute byte offset where the tile data starts in the archive
- `size`: the byte length of the tile payload

This is a flat binary file in which tile data is appended sequentially after the metadata table.

The offset of an image is the byte position where that image begins in the file, not the position relative to the previous tile.

### Example

If a tile is present at row 1, column 38, the bit corresponding to index `102` is set in the bitmask.

The archive layout is therefore:

```text
[512-byte bitmask][tile0 offset][tile0 size][tile1 offset][tile1 size]...[tile data 0][tile data 1]...
```

The actual ordering of metadata entries follows the tile positions in row/column order.

## Notes

- The bitmask always uses a fixed 64 x 64 grid.
- Only files matching the valid tile naming rule are included.
- The archive is designed to be compact and easy to read from a game runtime or loader.
