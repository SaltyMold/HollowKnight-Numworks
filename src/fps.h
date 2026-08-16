#ifndef FPS_H
#define FPS_H

#include <stdint.h>
#include <stdbool.h>
#include "libs/eadk.h"

// Stats computed per frame
typedef struct {
  uint64_t start_ts_ms;
  uint64_t end_ts_ms;
  int frame_duration_ms;
  int sleep_ms;
  int fps_no_cap;
  int fps_capped;
} fps_stats_t;

typedef struct fps_manager fps_manager_t;

// Create/destroy manager
fps_manager_t* fps_manager_create(uint32_t target_fps);
void fps_manager_destroy(fps_manager_t* m);

// Mark start of frame
void fps_manager_start_frame(fps_manager_t* m);

// Compute stats for current frame (no sleep yet)
void fps_manager_end_frame(fps_manager_t* m, fps_stats_t* out);

// Perform sleep to cap the frame using values computed by `end_frame`
// and update `out->fps_capped` after sleeping.
void fps_manager_cap_frame(fps_manager_t* m, fps_stats_t* out);

// Display the stats on screen at the given origin point.
// Uses the EADK drawing helpers.
void fps_display_stats(const fps_stats_t* stats, eadk_point_t origin);

#endif
