#include "raylib.h"
#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udp.h"
#include "graph.h"

#include "unistd.h"
#include "pthread.h"
#include <sys/inotify.h>
#include <sys/select.h>

#define WIDTH 800
#define HEIGHT 600
int main_gui(graph_values_t* gv) {
  add_point_callback(gv, "0.0;0", 10);
  add_point_callback(gv, "1.0;0", 10);
  add_point_callback(gv, "3.0;0", 10);
  add_point_callback(gv, "2.0;0", 10);
  add_point_callback(gv, "2.0;0", 10);
  add_point_callback(gv, "-0.0;1", 20);
  add_point_callback(gv, "-1.0;1", 20);
  add_point_callback(gv, "-3.0;1", 20);
  add_point_callback(gv, "-2.0;1", 20);
  add_point_callback(gv, "-2.0;1", 20);
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
void* watch_shader_change(void* gv) {
  int fd = inotify_init();
  uint32_t buff[128];
  inotify_add_watch(fd, "shaders", IN_MODIFY | IN_CLOSE_WRITE);
  graph_values_t* gvt = gv;
  while(true) {
    read(fd, buff, sizeof(buff));
    gvt->shaders_dirty = true;
  }
  return NULL;
}


int main(void) {
  pthread_t thread1, thread2;
  pthread_attr_t attrs1, attrs2;
  InitWindow(WIDTH, HEIGHT, "Ray plot");
  SetTargetFPS(165);
  SetWindowState(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  init_graph_values(gv, WIDTH, HEIGHT);
  {
    pthread_attr_init(&attrs1);
    if (pthread_create(&thread1, &attrs1, udp_main, &gv)) {
      fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
    }
  }
  {
    pthread_attr_init(&attrs2);
    if (pthread_create(&thread2, &attrs2, watch_shader_change, &gv)) {
      fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
    }
  }
  main_gui(&gv);
} 
