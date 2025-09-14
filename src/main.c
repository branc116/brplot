#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_q.h"
#include "src/br_permastate.h"
#include "src/br_tl.h"
#include "src/br_memory.h"

#if !defined(BR_LIB)
#include "src/br_plot.h"
#include "src/br_pp.h"


static void* main_gui(void* plotter) {
  br_plotter_t* br = (br_plotter_t*)plotter;
  br_plotter_init(br);
  while(br->should_close == false) {
    br_memory_frame();
    TracyCFrameMark;
    br_plotter_draw(br);
    br_dagens_handle(&br->groups, &br->dagens, &br->plots, brtl_time() + 0.010);
    TracyCFrameMarkStart("plotter_frame_end");
    br_plotter_frame_end(br);
    TracyCFrameMarkEnd("plotter_frame_end");
  }
  // CLEAN UP
  br_plotter_deinit(br);
  br_plotter_free(br);
  br_memory_finish();
  return 0;
}


#if !defined(FUZZ)
int main(void) {
  br_plotter_t* br = br_plotter_malloc();
  if (NULL == br) {
    LOGE("Failed to malloc br plotter, exiting...\n");
    exit(1);
  }
#if BR_HAS_SHADER_RELOAD
  start_refreshing_shaders(br);
#endif
  read_input_start(br);
  main_gui(br);

  return 0;
}
#endif

#if defined(__linux__) && defined(BR_DEBUG)
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

#if defined(BR_LIB)
#  include "src/lib.c"
#endif
