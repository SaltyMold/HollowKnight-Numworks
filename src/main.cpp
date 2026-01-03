// eadk
#include "libs/eadk/eadkpp.h"
#include "libs/eadk/eadk_vars.h"
// storage
#include "libs/storage/storage.h"
// lz4
#include "libs/lz4/lz4.h"
// color
#include "color.h"
// image 
#include "display_image.h"
// assets
#include "../output/assets/FloorStone80x10.h"
#include "../output/assets/KnightAFK14x30.h"

using namespace EADK;


int main(void) {
	int x = 100;
	int y = 240 - 50 - 30; 

	for (int i = 0; i < 240; i++) {
		for (int j = 0; j < 320; j++) {
			Display::pushRectUniform(Rect(j, i, 1, 1), EADK::random() % 0xFFFFFF);
		}
	}

	for (int i = 0; i < 4; i++) {
		DRAW_IMAGE_AT(Point(i * 80, 240 - 50), Image::FloorStone80x10);
	}

	// Prepare background buffer for the knight sprite to avoid trails.
	const int knightW = Image::KnightAFK14x30::k_width;
	const int knightH = Image::KnightAFK14x30::k_height;
	const size_t knightPixels = (size_t)knightW * (size_t)knightH;
	eadk_color_t *bgBuffer = (eadk_color_t *)malloc(knightPixels * sizeof(eadk_color_t));
	if (!bgBuffer) return 1;

	EADK::Rect knightRect(x, y, knightW, knightH);
	eadk_display_pull_rect(knightRect, bgBuffer);
    
	int target_fps = 30;
	Keyboard::State state;
	while (1) {
		uint64_t start_frame_ts_ms = eadk_timing_millis();

		// ----------------------------

		state = Keyboard::scan();
		if (state.keyDown(Keyboard::Key::Back)) break;
		
		if (state.keyDown(Keyboard::Key::Left)) {
			// restore previous background
			eadk_display_push_rect(knightRect, bgBuffer);
			x--;
			knightRect = EADK::Rect(x, y, knightW, knightH);
			eadk_display_pull_rect(knightRect, bgBuffer);
			DRAW_IMAGE_AT(Point(x, y), Image::KnightAFK14x30);
			Timing::msleep(2);
		}
		if (state.keyDown(Keyboard::Key::Right)) {
			// restore previous background
			eadk_display_push_rect(knightRect, bgBuffer);
			x++;
			knightRect = EADK::Rect(x, y, knightW, knightH);
			eadk_display_pull_rect(knightRect, bgBuffer);
			DRAW_IMAGE_AT(Point(x, y), Image::KnightAFK14x30);
			Timing::msleep(2);
		}
		if (state.keyDown(Keyboard::Key::Up)) {
			// restore previous background
			eadk_display_push_rect(knightRect, bgBuffer);
			y--;
			knightRect = EADK::Rect(x, y, knightW, knightH);
			eadk_display_pull_rect(knightRect, bgBuffer);
			DRAW_IMAGE_AT(Point(x, y), Image::KnightAFK14x30);
			Timing::msleep(2);
		}
		if (state.keyDown(Keyboard::Key::Down)) {
			// restore previous background
			eadk_display_push_rect(knightRect, bgBuffer);
			y++;
			knightRect = EADK::Rect(x, y, knightW, knightH);
			eadk_display_pull_rect(knightRect, bgBuffer);
			DRAW_IMAGE_AT(Point(x, y), Image::KnightAFK14x30);
			Timing::msleep(2);
		}

		// ----------------------------

        uint64_t end_frame_ts_ms = eadk_timing_millis();
		uint64_t frame_duration_ms = end_frame_ts_ms - start_frame_ts_ms;
		int sleep_ms = (1000 / target_fps) > frame_duration_ms ? (1000 / target_fps) - frame_duration_ms : 0;

		char buf[64];
		snprintf(buf, sizeof(buf), "start_frame_ts_ms: %d", (int)(start_frame_ts_ms));
		eadk_display_draw_string(buf, (eadk_point_t){0, 0}, false, eadk_color_black, eadk_color_white);
				
		snprintf(buf, sizeof(buf), "end_frame_ts_ms: %d", (int)(end_frame_ts_ms));
		eadk_display_draw_string(buf, (eadk_point_t){0, 12}, false, eadk_color_black, eadk_color_white);

		snprintf(buf, sizeof(buf), "frame_duration_ms: %d", (int)(frame_duration_ms));
		eadk_display_draw_string(buf, (eadk_point_t){0, 24}, false, eadk_color_black, eadk_color_white);

		snprintf(buf, sizeof(buf), "target_fps: %d", target_fps);
		eadk_display_draw_string(buf, (eadk_point_t){0, 36}, false, eadk_color_black, eadk_color_white);

		snprintf(buf, sizeof(buf), "sleep_ms: %d", (int)(sleep_ms));
		eadk_display_draw_string(buf, (eadk_point_t){0, 48}, false, eadk_color_black, eadk_color_white);

		int fps_no_cap = frame_duration_ms ? (int)(1000 / frame_duration_ms) : 0;
		int fps_capped = (frame_duration_ms + sleep_ms) ? (int)(1000 / (frame_duration_ms + sleep_ms)) : 0;
		snprintf(buf, sizeof(buf), "fps_no_cap: %d", fps_no_cap);
		eadk_display_draw_string(buf, (eadk_point_t){0, 60}, false, eadk_color_black, eadk_color_white);

		snprintf(buf, sizeof(buf), "fps_capped: %d", fps_capped);
		eadk_display_draw_string(buf, (eadk_point_t){0, 72}, false, eadk_color_black, eadk_color_white);
	}

	// Restore last background and free buffer
	eadk_display_push_rect(knightRect, bgBuffer);
	free(bgBuffer);

	return 0;
}