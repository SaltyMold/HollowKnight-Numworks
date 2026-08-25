#ifndef ASSET_DATA_H
#define ASSET_DATA_H

#include <stdint.h>

#define GRID_ROWS 98u
#define GRID_COLS 112u

extern const uint8_t data_bitmask[GRID_ROWS][GRID_COLS];
extern const uint32_t data_offsets[GRID_ROWS][GRID_COLS];
extern const uint32_t data_sizes[GRID_ROWS][GRID_COLS];

#endif
