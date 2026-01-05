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

Keyboard::State prev_state;
Keyboard::State state;

int xKnight = 160;
int yKnight = 120;
int xCamera = 0;
int yCamera = 0;

bool inBounds(int x, int y, int width, int height) {
	return (x >= 0 && x < EADK_SCREEN_WIDTH || x > UINT16_MAX - width + 1 && x <= UINT16_MAX) &&
	       (y >= 0 && y < EADK_SCREEN_HEIGHT || y > UINT16_MAX - height + 1 && y <= UINT16_MAX);
}

static inline void computeCrop(const EADK::Point &origin, int imgW, int imgH,
							   uint32_t &cropLeft, uint32_t &cropTop,
							   uint32_t &cropRight, uint32_t &cropBottom)
{
	// origin.x() and origin.y() overflow to UINT16_MAX after going below 0

	if (origin.x() > UINT16_MAX - imgW + 1) {
		cropLeft = UINT16_MAX - origin.x() + 1;
	}
	else {
		cropLeft = 0;
	}

	if (origin.y() > UINT16_MAX - imgH + 1) {
		cropTop = UINT16_MAX - origin.y() + 1;
	}
	else {
		cropTop = 0;
	}
	
	
	if (origin.x() < EADK_SCREEN_WIDTH && origin.x() + imgW > EADK_SCREEN_WIDTH) {
		cropRight = (uint32_t)(origin.x() + imgW - EADK_SCREEN_WIDTH);
	} else {
		cropRight = 0;
	}

	if (origin.y() < EADK_SCREEN_HEIGHT && origin.y() + imgH > EADK_SCREEN_HEIGHT) {
		cropBottom = (uint32_t)(origin.y() + imgH - EADK_SCREEN_HEIGHT);
	} else {
		cropBottom = 0;
	}
}

void display(bool bg = false) {
	if (bg) {
		//eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_red); // bg
	}

	// if in screen bounds, draw floor
	Point floor_start_point(100 - xCamera, 100 - yCamera);
	//if ((floor_start_point.y() >= 0 && floor_start_point.y() < EADK_SCREEN_HEIGHT || floor_start_point.y() > UINT16_MAX - 10 + 1 && floor_start_point.y() <= UINT16_MAX) && // y
	//	(floor_start_point.x() >= 0 && floor_start_point.x() < EADK_SCREEN_WIDTH || floor_start_point.x() > UINT16_MAX - 80 + 1 && floor_start_point.x() <= UINT16_MAX))   // x

	uint32_t cropLeft, cropTop, cropRight, cropBottom;

	if (inBounds(floor_start_point.x(), floor_start_point.y(), 80, 10)) {
		eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_green);

		const int imgW = 80;
		const int imgH = 10;

		computeCrop(floor_start_point, imgW, imgH, cropLeft, cropTop, cropRight, cropBottom);

		DRAW_IMAGE_CROPPED(floor_start_point, Image::FloorStone80x10, cropLeft, cropTop, cropRight, cropBottom);
	}
	else {
		eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_red);
	}

	// /*
	char buf[64];
	snprintf(buf, sizeof(buf), "xKnight: %d", xKnight);
	eadk_display_draw_string(buf, (eadk_point_t){0, 0}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "yKnight: %d", yKnight);
	eadk_display_draw_string(buf, (eadk_point_t){0, 12}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "xCamera: %d", xCamera);
	eadk_display_draw_string(buf, (eadk_point_t){0, 24}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "yCamera: %d", yCamera);
	eadk_display_draw_string(buf, (eadk_point_t){0, 36}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "floor_start_point.x(): %d", floor_start_point.x());
	eadk_display_draw_string(buf, (eadk_point_t){0, 48}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "floor_start_point.y(): %d", floor_start_point.y());
	eadk_display_draw_string(buf, (eadk_point_t){0, 60}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "cropLeft: %d", (int)cropLeft);
	eadk_display_draw_string(buf, (eadk_point_t){0, 72}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "cropTop: %d", (int)cropTop);
	eadk_display_draw_string(buf, (eadk_point_t){0, 84}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "cropRight: %d", (int)cropRight);
	eadk_display_draw_string(buf, (eadk_point_t){0, 96}, false, eadk_color_black, eadk_color_white);
	snprintf(buf, sizeof(buf), "cropBottom: %d", (int)cropBottom);
	eadk_display_draw_string(buf, (eadk_point_t){0, 108}, false, eadk_color_black, eadk_color_white);
	// */

	
	DRAW_IMAGE(Point(xKnight - xCamera, yKnight - yCamera), Image::KnightAFK14x30, false);
}

int main(void) {

	display();

	int target_fps = 30;
	int frame_count = 0;

	int dx = 0;
	int dy = 0;
		
	while (1) {
		uint64_t start_frame_ts_ms = eadk_timing_millis();

		// ----------------------------

		dx = 0;
		dy = 0;

		state = Keyboard::scan();
		if (state.keyDown(Keyboard::Key::Back)) break;
		// crash
		if (state.keyDown(Keyboard::Key::EXE) && state.keyDown(Keyboard::Key::Zero)) {
			volatile int *ptr = (int *)0xFFFFFFFF;
        	*ptr = 0;
		}
			
		
		if (state.keyDown(Keyboard::Key::Left))  dx -= 1;
		if (state.keyDown(Keyboard::Key::Right)) dx += 1;

		if (state.keyDown(Keyboard::Key::Up))    dy -= 1;
		if (state.keyDown(Keyboard::Key::Down))  dy += 1;

		if (dx != 0 || dy != 0) {
			xKnight += dx;
			yKnight += dy;
			xCamera += dx;
			yCamera += dy;
			display();
		}

		// ----------------------------

		frame_count++;

        uint64_t end_frame_ts_ms = eadk_timing_millis();
		int frame_duration_ms = end_frame_ts_ms - start_frame_ts_ms;
		int sleep_ms = (1000 / target_fps) > frame_duration_ms ? (1000 / target_fps) - frame_duration_ms : 0;

		/*
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
		*/

		eadk_timing_msleep(sleep_ms);
	}

	return 0;
}