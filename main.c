#include "plotter.h"

#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#ifdef PLATFORM_WEB
#define WIDTH (1280*1.6)
#define HEIGHT (720*1.6)
#else
#define WIDTH 1280
#define HEIGHT 720
#endif

int main_gui(graph_values_t* gv) {
  while(!WindowShouldClose()) {
    BeginDrawing();
      ClearBackground(BLACK);
      graph_draw(gv);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}

RLAPI void rlEnableDoubleBuffer(void);                  // Enable double buffer
                                                        //
int main(void) {
#ifdef RELEASE
  SetTraceLogLevel(LOG_ERROR);
#endif
  InitWindow(WIDTH, HEIGHT, "rlplot");
  graph_values_t* gv = malloc(sizeof(graph_values_t));
  SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  rlEnableDoubleBuffer();
  graph_init(gv, WIDTH, HEIGHT);
#ifndef RELEASE
  start_refreshing_shaders(gv);
#endif
  read_input_main(gv);
  main_gui(gv);
} 

