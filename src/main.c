#include "plotter.h"

#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#define WIDTH 1280
#define HEIGHT 720

extern context_t context;

int main_gui(graph_values_t* gv) {
  while(!WindowShouldClose()) {
    BeginDrawing();
      ClearBackground(BLACK);
      if (gv->state == plotter_state_default) graph_draw(gv);
      else if (gv->state == plotter_state_saving_file) file_saver_draw(gv->fs);
      graph_frame_end(gv);
    EndDrawing();
  }
  return 0;
}

int main(void) {
#ifdef RELEASE
  SetTraceLogLevel(LOG_ERROR);
#endif
  SetWindowState(FLAG_MSAA_4X_HINT);
  InitWindow(WIDTH, HEIGHT, "rlplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  graph_values_t* gv = BR_MALLOC(sizeof(graph_values_t));
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

