#include "jpg.h"

#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "libs/TJpg_Decoder/tjpgd.h"

#define JPG_GRID_SIZE 64u
#define JPG_TILE_WIDTH 320u
#define JPG_TILE_HEIGHT 240u

typedef struct {
    const uint8_t *ptr;
    size_t remaining;
    eadk_point_t origin;
} jpg_decode_context_t;

static uint32_t jpg_u32le(const uint8_t *ptr) {
    return (uint32_t)ptr[0] |
           ((uint32_t)ptr[1] << 8) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[3] << 24);
}

static bool jpg_bit_is_set(uint16_t row, uint16_t col) {
    const uint8_t *archive = data_input_ptr();
    size_t archive_size = data_input_size();

    if (archive == NULL || archive_size < 512u) {
        return false;
    }

    if (row >= JPG_GRID_SIZE || col >= JPG_GRID_SIZE) {
        return false;
    }

    uint32_t index = ((uint32_t)row * (uint32_t)JPG_GRID_SIZE) + (uint32_t)col;
    uint32_t byte_index = index / 8u;
    uint8_t mask = (uint8_t)(1u << (index % 8u));
    return (archive[byte_index] & mask) != 0u;
}

uint16_t jpg_archive_tile_count(void) {
    uint16_t count = 0;
    for (uint16_t row = 0; row < JPG_GRID_SIZE; ++row) {
        for (uint16_t col = 0; col < JPG_GRID_SIZE; ++col) {
            if (jpg_bit_is_set(row, col)) {
                ++count;
            }
        }
    }
    return count;
}

bool jpg_archive_has_tile(uint16_t row, uint16_t col) {
    return jpg_bit_is_set(row, col);
}

bool jpg_archive_get_tile(uint16_t row, uint16_t col, jpg_tile_t *tile) {
    if (tile == NULL || row >= JPG_GRID_SIZE || col >= JPG_GRID_SIZE) {
        return false;
    }

    const uint8_t *archive = data_input_ptr();
    size_t archive_size = data_input_size();
    if (archive == NULL || archive_size < 512u) {
        return false;
    }

    uint32_t before = 0;
    for (uint16_t r = 0; r < JPG_GRID_SIZE; ++r) {
        for (uint16_t c = 0; c < JPG_GRID_SIZE; ++c) {
            if (!jpg_bit_is_set(r, c)) {
                continue;
            }
            if (r == row && c == col) {
                size_t metadata_offset = 512u + (size_t)before * 8u;
                if (archive_size < metadata_offset + 8u) {
                    return false;
                }
                tile->row = r;
                tile->col = c;
                tile->offset = jpg_u32le(archive + metadata_offset);
                tile->size = jpg_u32le(archive + metadata_offset + 4u);
                return true;
            }
            ++before;
        }
    }

    return false;
}

static size_t jpg_stream_reader(JDEC *jd, uint8_t *buff, size_t len) {
    jpg_decode_context_t *ctx = (jpg_decode_context_t *)jd->device;
    if (ctx == NULL) {
        return 0u;
    }

    size_t to_read = len < ctx->remaining ? len : ctx->remaining;
    if (buff != NULL) {
        memcpy(buff, ctx->ptr, to_read);
    }
    ctx->ptr += to_read;
    ctx->remaining -= to_read;
    return to_read;
}

static int jpg_screen_out(JDEC *jd, void *bitmap, JRECT *rect) {
    jpg_decode_context_t *ctx = (jpg_decode_context_t *)jd->device;
    if (ctx == NULL || rect == NULL || bitmap == NULL) {
        return 0;
    }

    uint16_t width = (uint16_t)(rect->right - rect->left + 1u);
    uint16_t height = (uint16_t)(rect->bottom - rect->top + 1u);
    if (width == 0u || height == 0u) {
        return 0;
    }

    size_t pixel_count = (size_t)width * (size_t)height;
    eadk_color_t *pixels = (eadk_color_t *)malloc(pixel_count * sizeof(eadk_color_t));
    if (pixels == NULL) {
        return 0;
    }

    uint16_t *src = (uint16_t *)bitmap;
    for (uint16_t y = 0; y < height; ++y) {
        memcpy(pixels + (size_t)y * width,
               src + (size_t)y * width,
               (size_t)width * sizeof(eadk_color_t));
    }

    eadk_rect_t target = {
        (uint16_t)(ctx->origin.x + rect->left),
        (uint16_t)(ctx->origin.y + rect->top),
        width,
        height
    };
    eadk_display_push_rect(target, pixels);
    free(pixels);
    return 1;
}

static bool jpg_draw_jpeg_bytes(const uint8_t *jpeg_data, size_t jpeg_size, eadk_point_t origin) {
    if (jpeg_data == NULL || jpeg_size == 0u) {
        return false;
    }

    JDEC jd = {0};
    uint8_t work[ TJPGD_WORKSPACE_SIZE ];
    jpg_decode_context_t ctx;
    ctx.ptr = jpeg_data;
    ctx.remaining = jpeg_size;
    ctx.origin = origin;
    jd.swap = 0;

    JRESULT result = jd_prepare(&jd, jpg_stream_reader, work, sizeof(work), &ctx);
    if (result != JDR_OK) {
        return false;
    }

    result = jd_decomp(&jd, jpg_screen_out, 0);
    return result == JDR_OK;
}

bool jpg_draw_tile(uint16_t row, uint16_t col, eadk_point_t origin) {
    if (!jpg_archive_has_tile(row, col)) {
        return false;
    }

    jpg_tile_t tile;
    if (!jpg_archive_get_tile(row, col, &tile)) {
        return false;
    }

    const uint8_t *archive = data_input_ptr();
    size_t archive_size = data_input_size();
    if (archive == NULL || archive_size < tile.offset + tile.size) {
        return false;
    }

    return jpg_draw_jpeg_bytes(archive + tile.offset, tile.size, origin);
}

void jpg_draw_archive(eadk_point_t origin) {
    for (uint16_t row = 0; row < JPG_GRID_SIZE; ++row) {
        for (uint16_t col = 0; col < JPG_GRID_SIZE; ++col) {
            if (!jpg_archive_has_tile(row, col)) {
                continue;
            }

            eadk_point_t pos = {
                (uint16_t)(origin.x + (uint32_t)col * JPG_TILE_WIDTH),
                (uint16_t)(origin.y + (uint32_t)row * JPG_TILE_HEIGHT)
            };

            if (pos.x >= EADK_SCREEN_WIDTH || pos.y >= EADK_SCREEN_HEIGHT) {
                continue;
            }

            jpg_draw_tile(row, col, pos);
        }
    }
}
