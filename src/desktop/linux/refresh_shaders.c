#include "src/br_plot.h"
#include "src/br_plotter.h"

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "unistd.h"
#include "pthread.h"
#include "sys/inotify.h"
#include "sys/select.h"

static void* watch_shader_change(void* gv) {
  int fd = inotify_init();
  uint32_t buff[128];
  inotify_add_watch(fd, "src/desktop/shaders", IN_MODIFY | IN_CLOSE_WRITE);
  br_plotter_t* gvt = gv;
  while(!gvt->should_close) {
    read(fd, buff, sizeof(buff));
    printf("DIRTY SHADERS\n");
    gvt->shaders_dirty = true;
  }
  fprintf(stderr, "Stopping refresh shader thread\n");
  return NULL;
}

void start_refreshing_shaders(br_plotter_t* gv) {
  pthread_t thread2;
  pthread_attr_t attrs2;
  pthread_attr_init(&attrs2);
  if (pthread_create(&thread2, &attrs2, watch_shader_change, gv)) {
    fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
  }
}

