#include "jpg.h"

#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "libs/TJpg_Decoder/tjpgd.h"

#define JPG_GRID_ROWS 98u
#define JPG_GRID_COLS 112u
#define JPG_TILE_WIDTH 160u
#define JPG_TILE_HEIGHT 120u

typedef struct {
    const uint8_t *ptr;
    size_t remaining;
    map_point_t origin;
    uint8_t *pixel_buffer;
    size_t pixel_buffer_size;
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
    size_t bitmask_size = ((size_t)JPG_GRID_ROWS * (size_t)JPG_GRID_COLS + 7u) / 8u;

    if (archive == NULL || archive_size < bitmask_size) {
        return false;
    }

    if (row >= JPG_GRID_ROWS || col >= JPG_GRID_COLS) {
        return false;
    }

    uint32_t index = ((uint32_t)row * (uint32_t)JPG_GRID_COLS) + (uint32_t)col;
    size_t byte_index = (size_t)index / 8u;
    if (byte_index >= bitmask_size) {
        return false;
    }
    uint8_t mask = (uint8_t)(1u << (index % 8u));
    return (archive[byte_index] & mask) != 0u;
}

uint16_t jpg_archive_tile_count(void) {
    uint16_t count = 0;
    for (uint16_t row = 0; row < JPG_GRID_ROWS; ++row) {
        for (uint16_t col = 0; col < JPG_GRID_COLS; ++col) {
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
    if (tile == NULL || row >= JPG_GRID_ROWS || col >= JPG_GRID_COLS) {
        return false;
    }

    const uint8_t *archive = data_input_ptr();
    size_t archive_size = data_input_size();
    size_t bitmask_size = ((size_t)JPG_GRID_ROWS * (size_t)JPG_GRID_COLS + 7u) / 8u;
    if (archive == NULL || archive_size < bitmask_size) {
        return false;
    }

    uint32_t before = 0;
    for (uint16_t r = 0; r < JPG_GRID_ROWS; ++r) {
        for (uint16_t c = 0; c < JPG_GRID_COLS; ++c) {
            if (!jpg_bit_is_set(r, c)) {
                continue;
            }
            if (r == row && c == col) {
                size_t metadata_offset = bitmask_size + (size_t)before * 8u;
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

    int32_t block_x = (int32_t)rect->left;
    int32_t block_y = (int32_t)rect->top;
    int32_t block_w = (int32_t)rect->right - block_x + 1;
    int32_t block_h = (int32_t)rect->bottom - block_y + 1;

    // Apply 2x zoom to the target position
    int32_t target_x = (int32_t)ctx->origin.x + block_x * 2;
    int32_t target_y = (int32_t)ctx->origin.y + block_y * 2;
    int32_t zoomed_w = block_w * 2;
    int32_t zoomed_h = block_h * 2;

    int32_t left = target_x < 0 ? 0 : target_x;
    int32_t top = target_y < 0 ? 0 : target_y;
    int32_t right = target_x + zoomed_w > EADK_SCREEN_WIDTH ? EADK_SCREEN_WIDTH : target_x + zoomed_w;
    int32_t bottom = target_y + zoomed_h > EADK_SCREEN_HEIGHT ? EADK_SCREEN_HEIGHT : target_y + zoomed_h;

    if (left >= EADK_SCREEN_WIDTH || top >= EADK_SCREEN_HEIGHT || right <= left || bottom <= top) {
        return 1;
    }

    uint16_t out_w = (uint16_t)(right - left);
    uint16_t out_h = (uint16_t)(bottom - top);
    if (out_w == 0u || out_h == 0u) {
        return 1;
    }

    // Calculate proper source offset based on clipped screen position
    int32_t src_left = (left - target_x) / 2;
    int32_t src_top = (top - target_y) / 2;

    uint16_t *src = (uint16_t *)bitmap;
    
    // Process in bands to save RAM (max 16px high = ~10KB for 320 width)
    const uint16_t band_h = 16u;
    uint16_t band_y = 0u;
    
    while (band_y < out_h) {
        uint16_t band_lines = out_h - band_y;
        if (band_lines > band_h) {
            band_lines = band_h;
        }
        
        size_t needed_bytes = (size_t)out_w * (size_t)band_lines * sizeof(eadk_color_t);
        if (needed_bytes > ctx->pixel_buffer_size) {
            return 0;
        }

        eadk_color_t *pixels = (eadk_color_t *)ctx->pixel_buffer;
        
        // Zoom 2x for this band
        uint16_t dst_y = 0u;
        for (int32_t src_y = src_top + (band_y / 2); src_y < block_h && dst_y < band_lines; ++src_y) {
            const uint16_t *row_start = src + (size_t)src_y * (size_t)block_w;
            
            // Check if we need one or two output rows from this source row
            int dup_count = (dst_y == 0u && (band_y & 1)) ? 1 : 2;
            
            for (int dup_y = 0; dup_y < dup_count && dst_y < band_lines; ++dup_y, ++dst_y) {
                // use memcpy for duplication within row
                uint16_t dst_x = 0u;
                for (int32_t src_x = src_left; src_x < block_w && dst_x < out_w; ++src_x) {
                    uint16_t pixel = row_start[src_x];
                    pixels[(size_t)dst_y * out_w + dst_x] = pixel;
                    pixels[(size_t)dst_y * out_w + dst_x + 1u] = pixel;
                    dst_x += 2u;
                }
            }
        }
        
        // Display this band
        eadk_rect_t band_rect = {(uint16_t)left, (uint16_t)(top + band_y), out_w, band_lines};
        eadk_display_push_rect(band_rect, pixels);
        
        band_y += band_lines;
    }
    
    return 1;
}

static bool jpg_draw_jpeg_bytes(const uint8_t *jpeg_data, size_t jpeg_size, map_point_t origin) {
    if (jpeg_data == NULL || jpeg_size == 0u) {
        return false;
    }

    static uint8_t pixel_buffer[10 * 1024];  // 10KB for band rendering (320*16*2)
    JDEC jd = {0};
    uint8_t work[ TJPGD_WORKSPACE_SIZE ];
    jpg_decode_context_t ctx;
    ctx.ptr = jpeg_data;
    ctx.remaining = jpeg_size;
    ctx.origin = origin;
    ctx.pixel_buffer = pixel_buffer;
    ctx.pixel_buffer_size = sizeof(pixel_buffer);
    jd.swap = 0;

    JRESULT result = jd_prepare(&jd, jpg_stream_reader, work, sizeof(work), &ctx);
    if (result != JDR_OK) {
        return false;
    }

    result = jd_decomp(&jd, jpg_screen_out, 0);
    return result == JDR_OK;
}

bool jpg_draw_tile(uint16_t row, uint16_t col, map_point_t origin) {
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

void jpg_draw_archive(map_point_t origin) {
    for (uint16_t row = 0; row < JPG_GRID_ROWS; ++row) {
        for (uint16_t col = 0; col < JPG_GRID_COLS; ++col) {
            if (!jpg_archive_has_tile(row, col)) {
                continue;
            }

            int32_t pos_x = (int32_t)origin.x + (int32_t)col * (int32_t)JPG_TILE_WIDTH;
            int32_t pos_y = (int32_t)origin.y + (int32_t)row * (int32_t)JPG_TILE_HEIGHT;

            // Skip tiles that are fully outside the screen
            if (pos_x + (int32_t)JPG_TILE_WIDTH <= 0 || pos_y + (int32_t)JPG_TILE_HEIGHT <= 0) {
                continue;
            }
            if (pos_x >= (int32_t)EADK_SCREEN_WIDTH || pos_y >= (int32_t)EADK_SCREEN_HEIGHT) {
                continue;
            }

            map_point_t pos = {(int16_t)pos_x, (int16_t)pos_y};
            jpg_draw_tile(row, col, pos);
        }
    }
}
