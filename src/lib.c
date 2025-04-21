#include "src/br_pp.h"
#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_resampling2.h"
#include "src/br_threads.h"
#include "src/br_q.h"
#include "src/br_tl.h"
#include "include/brplot.h"


#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#  include <unistd.h>
#endif

#define BR_VERSION ((BR_MAJOR_VERSION << 16) | (BR_MINOR_VERSION << 8) | (BR_PATCH_VERSION))

typedef union {
  br_plotter_ctor_t plotter;
  br_plot_ctor_t plot;
  br_data_ctor_t data;
} br_common_ctor;

static BR_THREAD_RET_TYPE main_loop(void* plotterv) {
  br_plotter_t* plotter = plotterv;
  br_plotter_init(plotter);
  while (plotter->should_close == false) {
    br_plotter_draw(plotter);
    br_dagens_handle(&plotter->groups, &plotter->dagens, &plotter->plots, brtl_time() + 0.010);
    br_plotter_frame_end(plotter);
  }
  br_plotter_free(plotter);
  return (BR_THREAD_RET_TYPE)0;
}

static br_common_ctor br_ctors[128/8];
static int br_ctors_len = 0;

br_plotter_ctor_t* br_plotter_default_ctor(void) {
  // TODO: Only run this once...
  br_data_construct();
  br_resampling2_construct();
  br_ctors[br_ctors_len].plotter = (br_plotter_ctor_t) {
    .version = BR_VERSION,
    .ctor = {
      .width = 800, .height = 600,
      .kind = br_plotter_ui_kind_minimal,
      .use_permastate = false,
      .use_stdin = false
    }
  };
  return &br_ctors[br_ctors_len++].plotter;
}

br_plotter_t* br_plotter_new(br_plotter_ctor_t const* ctor) {
  br_plotter_t* plotter = br_plotter_malloc();
  if (ctor->ctor.use_stdin) {
    read_input_start(plotter);
  }
  br_thread_start(&main_loop, plotter);

  return plotter;
}

// Platform specific
void br_plotter_wait(br_plotter_t const* plotter) {
  while (false == plotter->should_close) {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__CYGWIN__)
      sleep(1);
#elif defined(_WIN32)
      Sleep(1000);
#elif defined(__EMSCRIPTEN__)
      // Web don't sleep
#else
#  error "Sleep not defined on this platform.."
#endif
  }
}

br_plot_ctor_t* br_plot_default_ctor(void) {
  br_ctors[br_ctors_len].plot = (br_plot_ctor_t) {
    .version = BR_VERSION,
    .ctor = {
      .kind = br_plot_kind_2d,
      .width = 1.f,
      .height = 1.f,
      .rearange_others = true
    }
  };
  return &br_ctors[br_ctors_len++].plot;
}

br_plot_id br_plot_new(br_plotter_t* plotter, br_plot_ctor_t const* ctor) {
  int ret = -1;
  switch (ctor->ctor.kind) {
    case br_plot_kind_2d: ret = br_plotter_add_plot_2d(plotter); break;
    case br_plot_kind_3d: ret = br_plotter_add_plot_3d(plotter); break;
  }
  return ret;
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
  br_ctors[br_ctors_len].data = (br_data_ctor_t) {
    .version = BR_VERSION,
    .ctor.kind = br_data_kind_2d
  };
  return &br_ctors[br_ctors_len++].data;
}

br_data_id br_data_new(br_plotter_t* plotter, br_data_ctor_t const* ctor) {
  br_data_id id = br_datas_get_new_id(&plotter->groups);
  q_push(plotter->commands, (q_command){ .type = q_command_new_data, .new_data = { .data_id = id, .kind = ctor->ctor.kind } } );
  return id;
}

int br_data_add_v1(br_plotter_t* plotter, float x, br_data_id data) {
  q_push(plotter->commands, (q_command){ .type = q_command_push_point_y, .push_point_y = { .y = x, .group = data } } );
  return 1;
}

int br_data_add_v1n(br_plotter_t* plotter, float const* x, int n, br_data_id data) {
  int i = 0;
  for (i = 0; i < n; ++i) br_data_add_v1(plotter, x[i], data);
  return i;
}

int br_data_add_v1ns(br_plotter_t* plotter, float const* x, int n, int stride, int offset, br_data_id data) {
  int i = 0, ret = 0;
  n -= n % stride;
  for (i = offset; i < n; i += stride, ++ret) br_data_add_v1(plotter, x[i], data);
  return ret;
}

int br_data_add_v2(br_plotter_t* plotter, float x, float y, br_data_id data) {
  q_push(plotter->commands, (q_command){ .type = q_command_push_point_xy, .push_point_xy = { .x = x, .y = y, .group = data } } );
  return 1;
}

int br_data_add_v2n(br_plotter_t* plotter, float const* v, int n, br_data_id data) {
  int ret = 0;
  for (int i = 0; i < n; i += 2, ++ret) br_data_add_v2(plotter, v[i], v[i + 1], data);
  return ret;
}

int br_data_add_v2ns(br_plotter_t* plotter, float const* v, int n, int stride, int offset_x, int offset_y, br_data_id data) {
  int ret = 0;
  n -= n % stride;
  for (int i = 0; i < n; i += stride, ++ret) br_data_add_v2(plotter, v[i + offset_x], v[i + offset_y], data);
  return ret;
}

int br_data_add_v2nd(br_plotter_t* plotter, float const* xs, float const* ys, int n, br_data_id data) {
  for (int i = 0; i < n; ++i) br_data_add_v2(plotter, xs[i], ys[i], data);
  return n;
}

int br_data_add_v2nds(br_plotter_t* plotter, float const* xs, float const* ys, int n, int stride, int offset_x, int offset_y, br_data_id data) {
  int ret = 0;
  n -= n % stride;
  for (int i = 0; i < n; i += stride, ++ret) br_data_add_v2(plotter, xs[i + offset_x], ys[i + offset_y], data);
  return ret;
}

static br_plotter_t* g_brplot_br_plotter = NULL;

static void brp_simp_create_plotter_if_no_exist(void) {
  if (NULL == g_brplot_br_plotter) {
    g_brplot_br_plotter = br_plotter_new(br_plotter_default_ctor());
  }
}

br_data_id brp_1(float x, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v1(g_brplot_br_plotter, x, data_id);
  return data_id;
}

br_data_id brp_1n(const float *points, int n, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v1n(g_brplot_br_plotter, points, n, data_id);
  return data_id;
}

br_data_id brp_2(float x, float y, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v2(g_brplot_br_plotter, x, y, data_id);
  return data_id;
}

void brp_wait(void) {
  br_plotter_wait(g_brplot_br_plotter);
}

void brp_flush(void) {
  q_push(g_brplot_br_plotter->commands, (q_command){ .type = q_command_flush} );
}

void brp_empty(br_data_id data_id) {
  q_push(g_brplot_br_plotter->commands, (q_command){ .type = q_command_empty, .clear = { .group = data_id } } );
}

void brp_focus_all(void) {
  q_push(g_brplot_br_plotter->commands, (q_command){ .type = q_command_focus} );
}

#if defined(BR_PYTHON_BULLSHIT)
BR_EXPORT void* PyInit_brplot(void) { return NULL; }
#endif
