#include "libs/eadk.h"
#include <stdio.h>
#include "fps.h"
#include "jpg.h"
#include "game.h"

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "HollowKnight";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

map_point_t camera = {5000, 5000};
map_point_t player = {0, 0};

#define TILE_WIDTH 320
#define TILE_HEIGHT 240
#define TILE_COLS 56
#define TILE_ROWS 49

static inline bool tile_in_bounds(int16_t col, int16_t row) {
    return col >= 0 && col < TILE_COLS && row >= 0 && row < TILE_ROWS;
}

static inline bool tile_visible_on_screen(int16_t screen_x, int16_t screen_y) {
    return (screen_x < EADK_SCREEN_WIDTH && (screen_x + TILE_WIDTH) > 0 &&
            screen_y < EADK_SCREEN_HEIGHT && (screen_y + TILE_HEIGHT) > 0);
}

static void draw_visible_tiles(int16_t tile_col, int16_t tile_row, int16_t pixel_offset_x, int16_t pixel_offset_y) {
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int16_t current_col = tile_col + dx;
            int16_t current_row = tile_row + dy;

            int16_t screen_x = dx * TILE_WIDTH - pixel_offset_x;
            int16_t screen_y = dy * TILE_HEIGHT - pixel_offset_y;

            if (tile_visible_on_screen(screen_x, screen_y) && tile_in_bounds(current_col, current_row)) {
                jpg_draw_tile(current_row, current_col, (map_point_t){screen_x, screen_y});
            }
        }
    }
}

int main(void) {
    eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_black);

    fps_manager_t* fps = fps_manager_create(TARGET_FPS);
    
    uint64_t frame_count = 0;
    while (1) {
        frame_count++;

        fps_stats_t stats;
        int dx = 0;
        int dy = 0;
        fps_manager_start_frame(fps);

        /*------------------------------------------*/

        eadk_keyboard_state_t state = eadk_keyboard_scan();
        if (eadk_keyboard_key_down(state, eadk_key_home)) break;

        if (eadk_keyboard_key_down(state, eadk_key_left)) dx -= 1;
        if (eadk_keyboard_key_down(state, eadk_key_right)) dx += 1;
        if (eadk_keyboard_key_down(state, eadk_key_up)) dy -= 1;
        if (eadk_keyboard_key_down(state, eadk_key_down)) dy += 1;

        camera.x += dx;
        camera.y += dy;

        // Clamp camera to map bounds
        if (camera.x < 0) camera.x = 0;
        if (camera.y < 0) camera.y = 0;
        if (camera.x > MAP_WIDTH - 320) camera.x = MAP_WIDTH - 320;
        if (camera.y > MAP_HEIGHT - 240) camera.y = MAP_HEIGHT - 240;

        // Calculate tile indices and pixel offset
        int16_t tile_col = camera.x / 320;
        int16_t tile_row = camera.y / 240;
        int16_t pixel_offset_x = camera.x % 320;
        int16_t pixel_offset_y = camera.y % 240;
        
        // if (frame_count % 4 == 0) {
        //     //eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_black);
        //     draw_visible_tiles(tile_col, tile_row, pixel_offset_x, pixel_offset_y);
        // }
        draw_visible_tiles(tile_col, tile_row, pixel_offset_x, pixel_offset_y);

        {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Camera: (%d, %d)", camera.x, camera.y);
            eadk_display_draw_string(buffer, (eadk_point_t){0, 220}, true, eadk_color_white, eadk_color_black);
        }

        /*------------------------------------------*/

        fps_manager_end_frame(fps, &stats);
        //fps_manager_cap_frame(fps, &stats);
        fps_display_stats(&stats, (eadk_point_t){0, 0});
    }
    fps_manager_destroy(fps);

    return 0;
}