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
    // Realloc full points groups
    for (int i = 0; i < gv->groups_len; ++i) {
      points_group_t* pg = &gv->groups[i];
      if (pg->len == pg->cap) {
        points_group_realloc(pg, pg->len * 2);
      }
    }
  }
  CloseWindow();
  return 0;
}

void add_point_callback(graph_values_t* gv, float value, int group) {
  Vector2 p = {0, value};
  points_group_t *g = points_group_get(gv->groups, &gv->groups_len, GROUP_CAP, group);
  p.x = g->len;
  points_group_push_point(g, p);

  // Free points in groups that were removed.
  // Only render thread can remove groups, so this is safe.
  if (gv->groups_need_freeing) {
    for (int i = gv->groups_len; i < GROUP_CAP; ++i) {
      Vector2* ps = gv->groups[i].points;
      if (ps != NULL) free(ps);
    }
  }
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
  graph_init(gv, WIDTH, HEIGHT);
#ifndef RELEASE
  start_refreshing_shaders(gv);
#endif
  read_input_main(gv);
  main_gui(gv);
} 

