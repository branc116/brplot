#include "br_plotter.h"
#include "src/br_data.h"
#include "src/br_pp.h"
#include "src/br_q.h"
#include "src/br_resampling2.h"
#include "br_permastate.h"
#include "br_da.h"
#include "tracy/TracyC.h"

void br_gui_init_specifics_gui(br_plotter_t* plotter);
static void* main_gui(void* plotter) {
  br_plotter_t* br = (br_plotter_t*)plotter;
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow((int)br->width, (int)br->height, "brplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  br->shaders = br_shaders_malloc();
  help_load_default_font();
  br_gui_init_specifics_gui(br);
  while(!WindowShouldClose() && !br->should_close) {
    TracyCFrameMark;
    br_plotter_draw(br);
    br_dagens_handle(&br->groups, &br->dagens, &br->plots, GetTime() + 0.010);
    TracyCFrameMarkStart("plotter_frame_end");
    br_plotter_frame_end(br);
    TracyCFrameMarkEnd("plotter_frame_end");
  }
  CloseWindow();
  br->should_close = true;

  return 0;
}

#if !defined(LIB)
#include "br_plot.h"
#include "br_pp.h"

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

int main(void) {
#if !defined(RELEASE)
  SetTraceLogLevel(LOG_ALL);
#else
  SetTraceLogLevel(LOG_ERROR);
#endif
  br_plotter_t* br = br_plotter_malloc();
  if (NULL == br) {
    LOGE("Failed to malloc br plotter, exiting...\n");
    exit(1);
  }
  br->height = HEIGHT;
  br->width = WIDTH;
  br_plotter_init(br, true);
#if BR_HAS_SHADER_RELOAD
  start_refreshing_shaders(br);
#endif
  read_input_start(br);
  SetExitKey(KEY_NULL);
  main_gui(br);

  // CLEAN UP
  read_input_stop();
  br_permastate_save(br);
  br_plotter_free(br);
  BR_FREE(br);
  return 0;
}
#endif

#if defined(LIB)
#include "br_plotter.h"
#include "br_threads.h"
#include "include/brplot.h"
#include <unistd.h>

#define VERSION 1

typedef union {
  br_plotter_ctor_t plotter;
  br_plot_ctor_t plot;
  br_data_ctor_t data;
} br_common_ctor;

br_common_ctor ctors[128/8];
int ctors_len = 0;

br_plotter_ctor_t* br_plotter_default_ctor(void) {
  // TODO: Only run this once...
  br_data_construct();
  br_resampling2_construct();
  ctors[ctors_len].plotter = (br_plotter_ctor_t) {
    .version = VERSION,
    .ctor = {
      .width = 800, .height = 600,
      .kind = br_plotter_ui_kind_minimal,
      .use_permastate = false
    }
  };
  return &ctors[ctors_len++].plotter;
}

br_plotter_t* br_plotter_new(br_plotter_ctor_t const* ctor) {
  br_plotter_t* plotter = br_plotter_malloc();
  br_plotter_init(plotter, ctor->ctor.use_permastate);
  br_thread_start(&main_gui, plotter);
  return plotter;
}
// Platform specific
void br_plotter_wait(br_plotter_t const* plotter) {
  while(false == plotter->should_close) sleep(1);
}

br_plot_ctor_t* br_plot_default_ctor(void) {
  ctors[ctors_len].plot = (br_plot_ctor_t) {
    .version = VERSION,
    .ctor.kind = br_plot_kind_2d
  };
  return &ctors[ctors_len++].plot;
}

br_plot_id br_plot_new(br_plotter_t* plotter, br_plot_ctor_t const* ctor) {
  switch (ctor->ctor.kind) {
    case br_plot_kind_2d: return br_plotter_add_plot_2d(plotter); break;
    case br_plot_kind_3d: return br_plotter_add_plot_3d(plotter); break;
  }
  BR_ASSERT(0);
}

void br_plot_free(br_plotter_t* plotter, br_plot_id plot) {
  // TODO: This will invalidate all other inexies so, better dont remote if from the list
  plotter->plots.arr[plot].is_deleted = true;
}

void br_plot_show_data(br_plotter_t* plotter, br_plot_id plot, br_data_id data) {
  br_plot_t* p = &plotter->plots.arr[plot];
  bool contains = false;
  br_da_contains_t(int, p->groups_to_show, data, contains);
  if (contains) return;
  else br_da_push_t(int, p->groups_to_show, data);
}

br_data_ctor_t* br_data_default_ctor(void) {
  ctors[ctors_len].data = (br_data_ctor_t) {
    .version = VERSION,
    .ctor.kind = br_data_kind_2d
  };
  return &ctors[ctors_len++].data;
}

br_data_id br_data_new(br_plotter_t* plotter, br_data_ctor_t const* ctor) {
  br_data_id id = br_datas_get_new_id(&plotter->groups);
  br_datas_create(&plotter->groups, id, ctor->ctor.kind);
  return id;
}

int br_data_add_v1(br_plotter_t* plotter, br_data_id data, float x) {
  q_push(plotter->commands, (q_command){ .type = q_command_push_point_y, .push_point_y = { .y = x, .group = data } } );
  return 1;
}

int br_data_add_v1n(br_plotter_t* plotter, br_data_id data, float const* x, int n) {
  int i = 0;
  for (i = 0; i < n; ++i) br_data_add_v1(plotter, data, x[i]);
  return i;
}

int br_data_add_v1ns(br_plotter_t* plotter, br_data_id data, float const* x, int n, int stride, int offset) {
  int i = 0, ret = 0;
  n -= n % stride;
  for (i = offset; i < n; i += stride, ++ret) br_data_add_v1(plotter, data, x[i]);
  return ret;
}

int br_data_add_v2(br_plotter_t* plotter, br_data_id data, float x, float y) {
  q_push(plotter->commands, (q_command){ .type = q_command_push_point_xy, .push_point_xy = { .x = x, .y = y, .group = data } } );
  return 1;
}

int br_data_add_v2n(br_plotter_t* plotter, br_data_id data, float const* v, int n) {
  int ret = 0;
  for (int i = 0; i < n; i += 2, ++ret) br_data_add_v2(plotter, data, v[i], v[i + 1]);
  return ret;
}

int br_data_add_v2ns(br_plotter_t* plotter, br_data_id data, float const* v, int n, int stride, int offset_x, int offset_y) {
  int ret = 0;
  n -= n % stride;
  for (int i = 0; i < n; i += stride, ++ret) br_data_add_v2(plotter, data, v[i + offset_x], v[i + offset_y]);
  return ret;
}

int br_data_add_v2nd(br_plotter_t* plotter, br_data_id data, float const* xs, float const* ys, int n) {
  for (int i = 0; i < n; ++i) br_data_add_v2(plotter, data, xs[i], ys[i]);
  return n;
}

int br_data_add_v2nds(br_plotter_t* plotter, br_data_id data, float const* xs, float const* ys, int n, int stride, int offset_x, int offset_y) {
  int ret = 0;
  n -= n % stride;
  for (int i = 0; i < n; i += stride, ++ret) br_data_add_v2(plotter, data, xs[i + offset_x], ys[i + offset_y]);
  return ret;
}

static br_plotter_t* g_brplot_br_plotter = NULL;

br_data_id br_simp_plot_v1n(br_data_id data_id, const float *points, int n) {
  if (NULL == g_brplot_br_plotter) {
    g_brplot_br_plotter = br_plotter_new(br_plotter_default_ctor());
    br_plot_new(g_brplot_br_plotter, br_plot_default_ctor());
  }
  if (data_id <= 0) {
    data_id = br_data_new(g_brplot_br_plotter, br_data_default_ctor());
    for (int i = 0; i < g_brplot_br_plotter->plots.len; ++i) {
      br_da_push_t(int, g_brplot_br_plotter->plots.arr->groups_to_show, data_id);
    }
  }
  br_data_add_v1n(g_brplot_br_plotter, data_id, points, n);
  return data_id;
}

void br_simp_wait(void) {
  br_plotter_wait(g_brplot_br_plotter);
}

#endif

