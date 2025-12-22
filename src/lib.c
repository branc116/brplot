#include "src/br_pp.h"
#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_plotter.h"
#include "src/br_gui.h"
#include "src/br_resampling.h"
#include "src/br_threads.h"
#include "src/br_q.h"
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

static void br_main_iter(br_plotter_t* br) {
  br_plotter_one_iter(br);
}

static BR_THREAD_RET_TYPE main_loop(void* plotterv) {
  br_plotter_t* plotter = plotterv;
  br_plotter_init(plotter);
  while (plotter->uiw.pl.should_close == false) {
    br_main_iter(plotter);
  }
  br_plotter_deinit(plotter);
  plotter->exited = true;
  return (BR_THREAD_RET_TYPE)0;
}


#define br_ctors_cap_mask 0xef
static br_common_ctor br_ctors[128];
static int br_ctors_len = 0;

br_plotter_ctor_t* br_plotter_default_ctor(void) {
  // TODO: Only run this once...
  int id = br_ctors_len;
  br_ctors[id].plotter = (br_plotter_ctor_t) {
    .version = BR_VERSION,
    .ctor = {
      .width = 800, .height = 600,
      .kind = br_plotter_ui_kind_minimal,
      .use_permastate = false,
      .use_stdin = false
    }
  };
  br_ctors_len = ((br_ctors_len + 1) & br_ctors_cap_mask);
  return &br_ctors[id].plotter;
}

br_plotter_t* br_plotter_new(br_plotter_ctor_t const* ctor) {
  br_plotter_t* plotter = br_plotter_malloc();
  if (ctor->ctor.use_stdin) {
    br_read_input_start(plotter);
  }
  br_thread_start(&main_loop, plotter);

  return plotter;
}

// Platform specific
void br_plotter_wait(br_plotter_t const* plotter) {
  while (false == plotter->uiw.pl.should_close) {
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
  int id = br_ctors_len;
  br_ctors[id].plot = (br_plot_ctor_t) {
    .version = BR_VERSION,
    .ctor = {
      .kind = br_plot_kind_2d,
      .width = 1.f,
      .height = 1.f,
      .rearange_others = true
    }
  };
  br_ctors_len = ((br_ctors_len + 1) & br_ctors_cap_mask);
  return &br_ctors[id].plot;
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
  br_da_contains_feeld_t(int, p->data_info, group_id, data, contains);
  if (contains) return;
  else br_da_push_t(int, p->data_info, br_plot_data(data));
}

br_data_ctor_t* br_data_default_ctor(void) {
  int id = br_ctors_len;
  br_ctors[id].data = (br_data_ctor_t) {
    .version = BR_VERSION,
    .ctor.kind = br_data_kind_2d
  };
  br_ctors_len = ((br_ctors_len + 1) & br_ctors_cap_mask);
  return &br_ctors[id].data;
}

br_data_id br_data_new(br_plotter_t* plotter, br_data_ctor_t const* ctor) {
  br_data_id id = br_datas_get_new_id(&plotter->groups);
  q_push(plotter->commands, (q_command){ .type = q_command_new_data, .new_data = { .data_id = id, .kind = ctor->ctor.kind } } );
  return id;
}

int br_data_add_v1(br_plotter_t* plotter, double x, br_data_id data) {
  q_push(plotter->commands, (q_command){ .type = q_command_push_point_y, .push_point_y = { .y = x, .group = data } } );
  return 1;
}

int br_data_add_v1n(br_plotter_t* plotter, double const* x, int n, br_data_id data) {
  int i = 0;
  for (i = 0; i < n; ++i) br_data_add_v1(plotter, x[i], data);
  return i;
}

int br_data_add_v1ns(br_plotter_t* plotter, double const* x, int n, int stride, int offset, br_data_id data) {
  int i = 0, ret = 0;
  n -= n % stride;
  for (i = offset; i < n; i += stride, ++ret) br_data_add_v1(plotter, x[i], data);
  return ret;
}

int br_data_add_v2(br_plotter_t* plotter, double x, double y, br_data_id data) {
  q_push(plotter->commands, (q_command){ .type = q_command_push_point_xy, .push_point_xy = { .x = x, .y = y, .group = data } } );
  return 1;
}

int br_data_add_v3(br_plotter_t* plotter, double x, double y, double z, br_data_id data) {
  q_push(plotter->commands, (q_command){ .type = q_command_push_point_xyz, .push_point_xyz = { .x = x, .y = y, .z = z, .group = data } } );
  return 1;
}

int br_data_add_v2n(br_plotter_t* plotter, double const* v, int n, br_data_id data) {
  int ret = 0;
  for (int i = 0; i < n; i += 2, ++ret) br_data_add_v2(plotter, v[i], v[i + 1], data);
  return ret;
}

int br_data_add_v2ns(br_plotter_t* plotter, double const* v, int n, int stride, int offset_x, int offset_y, br_data_id data) {
  int ret = 0;
  n -= n % stride;
  for (int i = 0; i < n; i += stride, ++ret) br_data_add_v2(plotter, v[i + offset_x], v[i + offset_y], data);
  return ret;
}

int br_data_add_v2nd(br_plotter_t* plotter, double const* xs, double const* ys, int n, br_data_id data) {
  for (int i = 0; i < n; ++i) br_data_add_v2(plotter, xs[i], ys[i], data);
  return n;
}

int br_data_add_v2nds(br_plotter_t* plotter, double const* xs, double const* ys, int n, int stride, int offset_x, int offset_y, br_data_id data) {
  int ret = 0;
  n -= n % stride;
  for (int i = 0; i < n; i += stride, ++ret) br_data_add_v2(plotter, xs[i + offset_x], ys[i + offset_y], data);
  return ret;
}

void br_empty(br_plotter_t* plotter, br_data_id data_id) {
  q_push(plotter->commands, (q_command){ .type = q_command_empty, .clear = { .group = data_id } } );
}

#define BR_PLOTTERS_CAP 4
static BR_THREAD_LOCAL int g_brplot_br_plotter_id = 0;

static BR_THREAD_LOCAL br_plotter_t* g_brplot_br_plotter[BR_PLOTTERS_CAP] = { 0 };
#define TOP_PLOTTER g_brplot_br_plotter[g_brplot_br_plotter_id]

static void brp_simp_create_plotter_if_no_exist(void) {
  if (NULL == TOP_PLOTTER || true == TOP_PLOTTER->uiw.pl.should_close) {
    g_brplot_br_plotter_id = (g_brplot_br_plotter_id + 1) % BR_PLOTTERS_CAP;
    if (TOP_PLOTTER != NULL) {
      //if (TOP_PLOTTER->exited) br_plotter_free(TOP_PLOTTER);
    }
    LOGI("Create %d", g_brplot_br_plotter_id);
    TOP_PLOTTER = br_plotter_new(br_plotter_default_ctor());
  }
}

br_data_id brp_1(double x, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v1(TOP_PLOTTER, x, data_id);
  return data_id;
}

br_data_id brp_f1(float x, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v1(TOP_PLOTTER, (double)x, data_id);
  return data_id;
}

br_data_id brp_2(double x, double y, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v2(TOP_PLOTTER, x, y, data_id);
  return data_id;
}

br_data_id brp_f2(float x, float y, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v2(TOP_PLOTTER, (double)x, (double)y, data_id);
  return data_id;
}

br_data_id brp_3(double x, double y, double z, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v3(TOP_PLOTTER, x, y, z, data_id);
  return data_id;
}

br_data_id brp_f3(float x, float y, float z, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_data_add_v3(TOP_PLOTTER, (double)x, (double)y, (double)z, data_id);
  return data_id;
}

void brp_wait(void) {
  brp_simp_create_plotter_if_no_exist();
  br_plotter_wait(TOP_PLOTTER);
}

void brp_flush(void) {
  brp_simp_create_plotter_if_no_exist();
  q_push(TOP_PLOTTER->commands, (q_command){ .type = q_command_flush} );
}

void brp_label(const char* label, br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  br_str_t str = { 0 };
  br_str_push_c_str(&str, label);
  q_push(TOP_PLOTTER->commands, (q_command){ .type = q_command_set_name, .set_quoted_str = { .group = data_id, .str = str } } );
}

void brp_empty(br_data_id data_id) {
  brp_simp_create_plotter_if_no_exist();
  q_push(TOP_PLOTTER->commands, (q_command){ .type = q_command_empty, .clear = { .group = data_id } } );
}

void brp_focus_all(void) {
  brp_simp_create_plotter_if_no_exist();
  q_push(TOP_PLOTTER->commands, (q_command){ .type = q_command_focus} );
}

#undef TOP_PLOTTER

#if defined(BR_PYTHON_BULLSHIT)
BR_EXPORT void* PyInit_brplot(void) { return NULL; }
#endif

#if defined(__EMSCRIPTEN__)
#  define BR_WASM_BULLSHIT 1
#endif

#if defined(BR_WASM_BULLSHIT)
BR_EXPORT void br_wasm_init(br_plotter_t* br) {
  br_plotter_init(br);
}

BR_EXPORT void br_wasm_loop(br_plotter_t* br) {
  br_main_iter(br);
}

// void glfwSetWindowSize(GLFWwindow* window, int width, int height);
BR_EXPORT void br_wasm_resize(br_plotter_t* br, int width, int height) {
  brpl_window_size_set(&br->uiw.pl, width, height);
}

BR_EXPORT void br_wasm_touch_event(br_plotter_t* br, int kind, float x, float y, int id) {
  brpl_additional_event_touch(&br->uiw.pl, kind, x, y, id);
}
#endif
