# Tile archive generation

This folder contains scripts that convert a large source image into a set of tile images, then filter out mostly-black tiles, and finally pack the valid tiles into a single binary archive.

The map is split according to the real canvas size: 17 920 / 160 = 112 columns and 11 760 / 120 = 98 rows. The runtime therefore uses a 112 x 98 grid instead of a fixed 128 x 128 grid.

The final packaging step now generates three files:

- a `.bin` archive containing the bitmask, metadata and image payloads
- a `.h` header exposing the generated matrices
- a `.c` source file containing the bitmask, offsets and sizes as 112 x 98 matrices

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

The row and column numbers are used to compute the position in the 112 x 98 grid.

## 2. Remove mostly-black tiles

Use the script `rm-black-images.py` to copy only tiles whose black-pixel ratio is below the threshold.

Example:

```bash
python rm-black-images.py --input output/map-to-images --output output/rm-black-images
```

Only valid tiles are copied to the output folder.

## 3. Pack the filtered tiles into a binary archive

Use `generate-bin.py` to generate the archive and the companion C files.

Example:

```bash
python generate-bin.py --input output/rm-black-images --output output/output.bin
```

This creates:

```text
output/data.bin
output/data.h
output/data.c
```

You can also explicitly set the output names:

```bash
python generate-bin.py \
  --input output/rm-black-images \
  --output output/data.bin \
  --header output/data.h \
  --source output/data.c
```

## Generated C files

The generated `.c` contains matrices such as:

```c
const uint8_t data_bitmask[GRID_ROWS][GRID_COLS] = {
    { 1, 0, 0, ... },
    { 0, 0, 0, 1, ... },
    ...
};

const uint32_t data_offsets[GRID_ROWS][GRID_COLS] = {
    { 0, 0, 0, ... },
    { 0, 0, 0, 2065, ... },
    ...
};

const uint32_t data_sizes[GRID_ROWS][GRID_COLS] = {
    { 0, 0, 0, ... },
    { 0, 0, 0, 2, ... },
    ...
};
```

The matching header declares those matrices and exposes the 112 x 98 grid dimensions.

## Binary format

The generated archive follows this structure:

1. Bitmask: ceil((112 * 98) / 8) bytes, i.e. a compact bitmask for the 112 x 98 grid
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
98 * 1 + 38 = 136
```

That bit is set to `1` in the compact bitmask. If a tile is absent, its bit remains `0`.

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
[2048-byte bitmask][tile0 offset][tile0 size][tile1 offset][tile1 size]...[tile data 0][tile data 1]...
```

The actual ordering of metadata entries follows the tile positions in row/column order.

## Notes

- The bitmask always uses the actual map grid: 112 x 98.
- Only files matching the valid tile naming rule are included.
- The archive is designed to be compact and easy to read from a game runtime or loader.
- The generated `.c` exposes the bitmask, offsets and sizes as 112 x 98 matrices for direct access in the game code.


<!--
python map-to-images.py --quality 50 --smoothing 0.2 --tile-width 160 --tile-height 120
python rm-black-images.py





-->

