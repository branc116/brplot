#include "br_plot.h"

#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define WIDTH 1280
#define HEIGHT 720

int main_gui(br_plot_t* gv) {
  while(!WindowShouldClose() && !gv->should_close) {
    graph_draw(gv);
    graph_frame_end(gv);
  }
  return 0;
}

int main(void) {
#ifdef RELEASE
  SetTraceLogLevel(LOG_ERROR);
#endif
  SetWindowState(FLAG_MSAA_4X_HINT);
  InitWindow(WIDTH, HEIGHT, "brplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  br_plot_t* gv = BR_MALLOC(sizeof(br_plot_t));
  graph_init(gv, WIDTH, HEIGHT);
#ifndef RELEASE
  start_refreshing_shaders(gv);
#endif
  read_input_main(gv);
  SetExitKey(KEY_NULL);
  main_gui(gv);

  // Clean up
  read_input_stop();
  graph_free(gv);
  BR_FREE(gv);
  CloseWindow();
  return 0;
} 

