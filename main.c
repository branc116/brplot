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
  graph_init(gv, WIDTH, HEIGHT);
  while(!WindowShouldClose()) {
    BeginDrawing();
      ClearBackground(BLACK);
      graph_draw(gv);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}

void add_point_callback(void* gv, char* buffer, size_t s) {
  (void)s;
  Vector2 p = {0, 0};
  int group = 0;
  sscanf(buffer, "%f;%d", &p.y, &group);
  graph_values_t* gvt = gv;
  points_group_t *g = points_group_get(gvt->groups, &gvt->groups_len, GROUP_CAP, gvt->points, group);
  p.x = g->len;
  points_group_push_point(g, p);
}

int main(void) {
  InitWindow(WIDTH, HEIGHT, "Ray plot");
  graph_values_t* gv = malloc(sizeof(graph_values_t));
#ifdef PLATFORM_WEB
  SetTargetFPS(60);
#else
  SetTargetFPS(165);
#endif
  SetWindowState(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  start_refreshing_shaders(gv);
  udp_main(gv);
  main_gui(gv);
} 

