#ifndef LIB
#include "br_plot.h"

#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Tracy/tracy/TracyC.h"

#include "raylib.h"
#ifdef LINUX
#include "sys/resource.h"
#include "errno.h"
#ifndef RELEASE
const char* __asan_default_options(void) {
  return "verbosity=0:"
    "sleep_before_dying=120:"
    "print_scariness=true:"
    "allocator_may_return_null=1:"
    "soft_rss_limit_mb=512222"
    ;
}
#endif
#endif

#define WIDTH 1280
#define HEIGHT 720

int main_gui(br_plotter_t* gv) {
  while(!WindowShouldClose() && !gv->should_close) {
    TracyCFrameMark;
    br_plotter_draw(gv);
    TracyCFrameMarkStart("plotter_frame_end");
    br_plotter_frame_end(gv);
    TracyCFrameMarkEnd("plotter_frame_end");
  }
  return 0;
}

int main(void) {
#ifdef RELEASE
#ifdef LINUX
  const unsigned long mem_size = 1L << 31L;
  struct rlimit rl = {mem_size, mem_size};
  int r = setrlimit(RLIMIT_AS, &rl);
  if (r != 0) fprintf(stderr, "setrlimit failed: %d:%s", errno, strerror(errno));
#endif
  SetTraceLogLevel(LOG_ERROR);
#endif
  br_plotter_t* gv = br_plotter_malloc();
  br_plotter_init(gv, WIDTH, HEIGHT);
#ifndef RELEASE
  start_refreshing_shaders(gv);
#endif
  read_input_start(gv);
  SetExitKey(KEY_NULL);
  main_gui(gv);

  // Clean up
  read_input_stop();
  fprintf(stdin, "\1\n");
  br_plotter_free(gv);
  BR_FREE(gv);
  CloseWindow();
  return 0;
}
#endif
