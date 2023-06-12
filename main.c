#include "raylib.h"
#include "stddef.h"
#include "stdio.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udp.h"
#include "graph.h"

#include "pthread.h"

#define WIDTH 800
#define HEIGHT 600
int main_gui(graph_values_t* gv) {
  add_point_callback(gv, "0.0;0", 10);
  add_point_callback(gv, "1.0;0", 10);
  add_point_callback(gv, "3.0;0", 10);
  add_point_callback(gv, "2.0;0", 10);
  add_point_callback(gv, "2.0;0", 10);
  while(!WindowShouldClose()) {
    update_graph_values(gv);
    BeginDrawing();
      ClearBackground(BLACK);
      DrawGraph(gv);
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
  pp_ret r = push_point(gv, p, group);
  r.v->x = r.group->len - 1;
}


int main(void) {
  InitWindow(WIDTH, HEIGHT, "Ray plot");
  SetTargetFPS(30);
  SetWindowState(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  init_graph_values(gv, WIDTH, HEIGHT);

  pthread_t thread;
  pthread_attr_t attrs;

  pthread_attr_init(&attrs);
  if (pthread_create(&thread, &attrs, udp_main, &gv)) {
    fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
  }
  main_gui(&gv);
} 
