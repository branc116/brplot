#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_q.h"
#include "src/br_permastate.h"
#include "src/br_tl.h"
#include "src/br_gl.h"
#include "src/br_icons.h"
#include "src/br_theme.h"
#include "src/br_ui.h"

void br_gui_init_specifics_gui(br_plotter_t* plotter);
static void* main_gui(void* plotter) {
  bool use_permastate = true;
  br_plotter_t* br = (br_plotter_t*)plotter;
  br_plotter_init_specifics_platform(br, 1280, 720);
  brui_resizable_init();
  if (use_permastate) br->loaded = br_permastate_load(br);
  if (false == br->loaded) {
    br_datas_deinit(&br->groups);
    br->plots.len = 0;
    br_plotter_add_plot_2d(br);
  } else {
    for (int i = 0; i < br->plots.len; ++i) {
      br_plot_t* p = &br->plots.arr[i];
      br->plots.arr[i].texture_id = brgl_create_framebuffer(p->cur_extent.width, p->cur_extent.height);
    }
  }
  br->menu_extent_handle = brui_resizable_new(BR_EXTENTI(10, 40, 160, brtl_viewport().height/2), 0); 
  br_icons_init(br->shaders.icon);
  if (false == br->loaded) {
    br_theme_dark();
    br_theme_reset_ui();
  }
  bruir_resizable_refresh(0);
  while(br->should_close == false) {
    TracyCFrameMark;
    br_plotter_draw(br);
    br_dagens_handle(&br->groups, &br->dagens, &br->plots, brtl_time() + 0.010);
    TracyCFrameMarkStart("plotter_frame_end");
    br_plotter_frame_end(br);
    TracyCFrameMarkEnd("plotter_frame_end");
  }
  // CLEAN UP
  read_input_stop();
  br_permastate_save(br);
  br_plotter_free(br);
  brtl_window_close();
  BR_FREE(br);
  return 0;
}

#if !defined(LIB)
#include "br_plot.h"
#include "br_pp.h"

int main(void) {
  br_plotter_t* br = br_plotter_malloc();
  if (NULL == br) {
    LOGE("Failed to malloc br plotter, exiting...\n");
    exit(1);
  }
  br_plotter_init(br);
#if BR_HAS_SHADER_RELOAD
  start_refreshing_shaders(br);
#endif
  read_input_start(br);
  //SetExitKey(KEY_NULL);
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

#if defined(LIB)
#  include "lib.c"
#endif

