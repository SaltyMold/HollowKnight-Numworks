#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "libs/eadk.h"

typedef struct {
    uint16_t row;
    uint16_t col;
    uint32_t offset;
    uint32_t size;
} jpg_tile_t;

uint16_t jpg_archive_tile_count(void);
bool jpg_archive_has_tile(uint16_t row, uint16_t col);
bool jpg_archive_get_tile(uint16_t row, uint16_t col, jpg_tile_t *tile);
bool jpg_draw_tile(uint16_t row, uint16_t col, eadk_point_t origin);
void jpg_draw_archive(eadk_point_t origin);
