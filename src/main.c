#ifndef LIB
#include "br_plot.h"
#include "br_plotter.h"
#include "br_permastate.h"
#include "br_pp.h"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) ||  defined(_WIN32) || defined(__CYGWIN__)
#  include "desktop/platform.c"
#elif defined(__EMSCRIPTEN__)
#  include "web/platform.c"
#else
#  error "Unsupported Platform"
#endif

#include "tracy/TracyC.h"

#include "raylib.h"
#if defined(__linux__) && !defined(RELEASE)
const char* __asan_default_options(void) {
  return "verbosity=0:"
    "sleep_before_dying=120:"
    "print_scariness=true:"
    "allocator_may_return_null=1:"
    "soft_rss_limit_mb=512222"
    ;
}
#endif

#define WIDTH 1280
#define HEIGHT 720

int main_gui(br_plotter_t* gv) {
  while(!WindowShouldClose() && !gv->should_close) {
    TracyCFrameMark;
    br_plotter_draw(gv);
    br_dagens_handle(gv, GetTime() + 0.010);
    TracyCFrameMarkStart("plotter_frame_end");
    br_plotter_frame_end(gv);
    TracyCFrameMarkEnd("plotter_frame_end");
  }
  return 0;
}

int main(void) {
#if !defined(RELEASE)
  SetTraceLogLevel(LOG_ERROR);
#endif
  br_plotter_t* gv = br_plotter_malloc();
  br_plotter_init(gv, WIDTH, HEIGHT);
#if BR_HAS_SHADER_RELOAD
  start_refreshing_shaders(gv);
#endif
  read_input_start(gv);
  SetExitKey(KEY_NULL);
  main_gui(gv);

  // Clean up
  read_input_stop();
  br_permastate_save(gv);
  br_plotter_free(gv);
  BR_FREE(gv);
  CloseWindow();
  return 0;
}
#endif
