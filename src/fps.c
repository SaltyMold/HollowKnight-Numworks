#include "fps.h"
#include "libs/eadk.h"
#include "libs/macro.h"
#include <stdlib.h>

struct fps_manager {
  uint32_t target_fps;
  uint64_t start_ts_ms;
};

fps_manager_t* fps_manager_create(uint32_t target_fps) {
  fps_manager_t* m = (fps_manager_t*)malloc(sizeof(fps_manager_t));
  if (!m) return NULL;
  m->target_fps = target_fps;
  m->start_ts_ms = 0;
  return m;
}

void fps_manager_destroy(fps_manager_t* m) {
  if (!m) return;
  free(m);
}

void fps_manager_start_frame(fps_manager_t* m) {
  if (!m) return;
  m->start_ts_ms = eadk_timing_millis();
}

void fps_manager_end_frame(fps_manager_t* m, fps_stats_t* out) {
  if (!m || !out) return;
  out->start_ts_ms = m->start_ts_ms;
  out->end_ts_ms = eadk_timing_millis();
  out->frame_duration_ms = (int)(out->end_ts_ms - out->start_ts_ms);
  int target_frame_ms = m->target_fps ? (1000 / (int)m->target_fps) : 0;
  out->sleep_ms = target_frame_ms > out->frame_duration_ms ? target_frame_ms - out->frame_duration_ms : 0;
  out->fps_no_cap = out->frame_duration_ms ? (int)(1000 / out->frame_duration_ms) : 0;
  out->fps_capped = 0; // will be computed after sleeping
}

void fps_manager_cap_frame(fps_manager_t* m, fps_stats_t* out) {
  if (!m || !out) return;
  if (out->sleep_ms > 0) eadk_timing_msleep(out->sleep_ms);
  out->fps_capped = (out->frame_duration_ms + out->sleep_ms) ? (int)(1000 / (out->frame_duration_ms + out->sleep_ms)) : 0;
}

void fps_display_stats(const fps_stats_t* stats, eadk_point_t origin) {
  if (!stats) return;
  char buf[64];
  snprintf(buf, sizeof(buf), "start_frame_ts_ms: %d", (int)(stats->start_ts_ms));
  eadk_display_draw_string(buf, (eadk_point_t){origin.x, origin.y + 0}, false, eadk_color_black, eadk_color_white);

  snprintf(buf, sizeof(buf), "end_frame_ts_ms: %d", (int)(stats->end_ts_ms));
  eadk_display_draw_string(buf, (eadk_point_t){origin.x, origin.y + 12}, false, eadk_color_black, eadk_color_white);

  snprintf(buf, sizeof(buf), "frame_duration_ms: %d", (int)(stats->frame_duration_ms));
  eadk_display_draw_string(buf, (eadk_point_t){origin.x, origin.y + 24}, false, eadk_color_black, eadk_color_white);

  snprintf(buf, sizeof(buf), "target_fps: %d", (int)(stats->fps_capped ? (1000 / (stats->frame_duration_ms + stats->sleep_ms)) : 0));
  eadk_display_draw_string(buf, (eadk_point_t){origin.x, origin.y + 36}, false, eadk_color_black, eadk_color_white);

  snprintf(buf, sizeof(buf), "sleep_ms: %d", (int)(stats->sleep_ms));
  eadk_display_draw_string(buf, (eadk_point_t){origin.x, origin.y + 48}, false, eadk_color_black, eadk_color_white);

  snprintf(buf, sizeof(buf), "fps_no_cap: %d", stats->fps_no_cap);
  eadk_display_draw_string(buf, (eadk_point_t){origin.x, origin.y + 60}, false, eadk_color_black, eadk_color_white);

  snprintf(buf, sizeof(buf), "fps_capped: %d", stats->fps_capped);
  eadk_display_draw_string(buf, (eadk_point_t){origin.x, origin.y + 72}, false, eadk_color_black, eadk_color_white);
}
