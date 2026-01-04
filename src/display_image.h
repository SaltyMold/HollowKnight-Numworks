#ifndef DISPLAY_IMAGE_H
#define DISPLAY_IMAGE_H

#include "libs/eadk/eadkpp.h"
#include "libs/lz4/lz4.h"
#include <stdlib.h>
#include <stdint.h>

namespace DisplayImage {

static constexpr eadk_color_t COLOR_KEY_MAGENTA = (eadk_color_t)0xF81F;

static inline void drawCompressedImage(
    EADK::Rect rect,
    const uint8_t *compressedData,
    uint32_t compressedSize,
    uint32_t width,
    uint32_t height,
    bool invert = false)
{
    const size_t bufferSize = (size_t)width * (size_t)height * sizeof(eadk_color_t);

    eadk_color_t *buffer = (eadk_color_t *)malloc(bufferSize);
    if (!buffer) return;

    int ret = LZ4_decompress_safe(
        (const char *)compressedData,
        (char *)buffer,
        (int)compressedSize,
        (int)bufferSize);

    if (ret > 0) {
        if (!invert) {
            eadk_display_push_rect(rect, buffer);
        } else {
            eadk_color_t *flipped = (eadk_color_t *)malloc(bufferSize);
            if (flipped) {
                for (uint32_t y = 0; y < height; y++) {
                    for (uint32_t x = 0; x < width; x++) {
                        size_t src = (size_t)y * width + x;
                        size_t dst = (size_t)y * width + (width - 1 - x);
                        flipped[dst] = buffer[src];
                    }
                }
                eadk_display_push_rect(rect, flipped);
                free(flipped);
            } else {
                eadk_display_push_rect(rect, buffer);
            }
        }
    }

    free(buffer);
}

static inline void drawCompressedImage(
    EADK::Point origin,
    const uint8_t *compressedData,
    uint32_t compressedSize,
    uint32_t width,
    uint32_t height,
    bool invert = false)
{
    drawCompressedImage(
        EADK::Rect(origin.x(), origin.y(), (int)width, (int)height),
        compressedData,
        compressedSize,
        width,
        height,
        invert);
}

#define DRAW_IMAGE(point, image_ns, ...) \
    DisplayImage::drawCompressedImage( \
        point, \
        image_ns::compressedPixelData, \
        image_ns::k_compressedPixelSize, \
        image_ns::k_width, \
        image_ns::k_height, \
        ##__VA_ARGS__)

// -----------------------------

static inline void drawCompressedImageTransparent(
    EADK::Point origin,
    const uint8_t *compressedData,
    uint32_t compressedSize,
    uint32_t width,
    uint32_t height,
    bool invert = false)
{
    EADK::Rect rect(origin.x(), origin.y(), (int)width, (int)height);

    const size_t pixelCount = (size_t)width * (size_t)height;
    const size_t bufferSize = pixelCount * sizeof(eadk_color_t);

    eadk_color_t *img = (eadk_color_t *)malloc(bufferSize);
    if (!img) return;

    eadk_color_t *bg = (eadk_color_t *)malloc(bufferSize);
    if (!bg) {
        free(img);
        return;
    }

    int ret = LZ4_decompress_safe(
        (const char *)compressedData,
        (char *)img,
        (int)compressedSize,
        (int)bufferSize);

    if (ret <= 0) {
        free(img);
        free(bg);
        return;
    }

    eadk_display_pull_rect(rect, bg);

    for (size_t i = 0; i < pixelCount; i++) {
        if (img[i] == COLOR_KEY_MAGENTA) {
            img[i] = bg[i];
        }
    }

    if (!invert) {
        eadk_display_push_rect(rect, img);
    } else {
        eadk_color_t *flipped = (eadk_color_t *)malloc(bufferSize);
        if (flipped) {
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    size_t src = (size_t)y * width + x;
                    size_t dst = (size_t)y * width + (width - 1 - x);
                    flipped[dst] = img[src];
                }
            }
            eadk_display_push_rect(rect, flipped);
            free(flipped);
        } else {
            eadk_display_push_rect(rect, img);
        }
    }

    free(img);
    free(bg);
}

#define DRAW_IMAGE_TRANSPARENT(point, image_ns, ...) \
    DisplayImage::drawCompressedImageTransparent( \
        point, \
        image_ns::compressedPixelData, \
        image_ns::k_compressedPixelSize, \
        image_ns::k_width, \
        image_ns::k_height, \
        ##__VA_ARGS__)

} // DisplayImage

#endif
