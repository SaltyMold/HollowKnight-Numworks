#ifndef DISPLAY_IMAGE_H
#define DISPLAY_IMAGE_H

#include "libs/eadk/eadkpp.h"
#include "libs/lz4/lz4.h"
#include <stdlib.h>
#include <stdint.h>

namespace DisplayImage {

static constexpr eadk_color_t COLOR_KEY_MAGENTA = (eadk_color_t)0xF81F;


// Transparent Magenta = Alpha
static inline void drawCompressedImage(
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

#define DRAW_IMAGE(point, image_ns, ...) \
    DisplayImage::drawCompressedImage( \
        point, \
        image_ns::compressedPixelData, \
        image_ns::k_compressedPixelSize, \
        image_ns::k_width, \
        image_ns::k_height, \
        ##__VA_ARGS__)

// -----------------------------

static inline void drawCroppedImage(
    EADK::Point origin,
    const uint8_t *compressedData,
    uint32_t compressedSize,
    uint32_t srcWidth,
    uint32_t srcHeight,
    uint32_t cropLeft,
    uint32_t cropTop,
    uint32_t cropRight,
    uint32_t cropBottom,
    bool invert = false)
{
    if (cropLeft + cropRight >= srcWidth || cropTop + cropBottom >= srcHeight) return;

    const uint32_t outW = srcWidth - cropLeft - cropRight;
    const uint32_t outH = srcHeight - cropTop - cropBottom;
    const size_t srcBufferSize = (size_t)srcWidth * (size_t)srcHeight * sizeof(eadk_color_t);
    const size_t outBufferSize = (size_t)outW * (size_t)outH * sizeof(eadk_color_t);

    eadk_color_t *src = (eadk_color_t *)malloc(srcBufferSize);
    if (!src) return;

    int ret = LZ4_decompress_safe((const char *)compressedData, (char *)src, (int)compressedSize, (int)srcBufferSize);
    if (ret <= 0) { free(src); return; }

    eadk_color_t *out = (eadk_color_t *)malloc(outBufferSize);
    if (!out) { free(src); return; }

    for (uint32_t y = 0; y < outH; y++) {
        for (uint32_t x = 0; x < outW; x++) {
            size_t sidx = (size_t)(y + cropTop) * srcWidth + (x + cropLeft);
            size_t didx = (size_t)y * outW + (invert ? (outW - 1 - x) : x);
            out[didx] = src[sidx];
        }
    }

    EADK::Rect dstRect(origin.x() + (int)cropLeft, origin.y() + (int)cropTop, (int)outW, (int)outH);

    eadk_color_t *bg = (eadk_color_t *)malloc(outBufferSize);
    if (!bg) {
        eadk_display_push_rect(dstRect, out);
        free(out);
        free(src);
        return;
    }
    eadk_display_pull_rect(dstRect, bg);
    const size_t pixelCount = (size_t)outW * (size_t)outH;
    for (size_t i = 0; i < pixelCount; i++) {
        if (out[i] == COLOR_KEY_MAGENTA) out[i] = bg[i];
    }
    eadk_display_push_rect(dstRect, out);
    free(bg);

    free(out);
    free(src);
}

#define DRAW_IMAGE_CROPPED(point, image_ns, l, t, r, b, ...) \
    DisplayImage::drawCroppedImage( \
        point, \
        image_ns::compressedPixelData, \
        image_ns::k_compressedPixelSize, \
        image_ns::k_width, \
        image_ns::k_height, \
        l, t, r, b, \
        ##__VA_ARGS__)

// -----------------------------


} // DisplayImage

#endif
