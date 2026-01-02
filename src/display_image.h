#ifndef DISPLAY_IMAGE_H
#define DISPLAY_IMAGE_H

#include "libs/eadk/eadkpp.h"
#include "libs/lz4/lz4.h"

namespace DisplayImage {

static inline void drawCompressedImage(EADK::Rect rect, const uint8_t *compressedData, uint32_t compressedSize, uint32_t width, uint32_t height)
{
	int decompressedSize = (int)(width * height * sizeof(eadk_color_t));
	uint8_t *buffer = (uint8_t *)malloc((size_t)decompressedSize);
	if (!buffer)
		return;
	int ret = LZ4_decompress_safe((const char *)compressedData, (char *)buffer, (int)compressedSize, decompressedSize);
	if (ret <= 0) {
		free(buffer);
		return;
	}
	eadk_display_push_rect(rect, reinterpret_cast<const eadk_color_t *>(buffer));
	free(buffer);
}

template<typename ImageT>
static inline void drawCompressedImage(EADK::Point origin)
{
	EADK::Rect rect(origin.x(), origin.y(), (int)ImageT::k_width, (int)ImageT::k_height);
	drawCompressedImage(rect, ImageT::compressedPixelData, ImageT::k_compressedPixelSize, ImageT::k_width, ImageT::k_height);
}

static inline void drawCompressedImage(EADK::Point origin, const uint8_t *compressedData, uint32_t compressedSize, uint32_t width, uint32_t height)
{
	EADK::Rect rect(origin.x(), origin.y(), (int)width, (int)height);
	drawCompressedImage(rect, compressedData, compressedSize, width, height);
}

#define DRAW_IMAGE_AT(point, image_ns) \
	DisplayImage::drawCompressedImage(point, image_ns::compressedPixelData, image_ns::k_compressedPixelSize, image_ns::k_width, image_ns::k_height)

}

#endif