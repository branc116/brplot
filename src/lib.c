#include "br_da.h"
#include "br_plotter.h"
#include "br_gui_internal.h"
#include "br_resampling2.h"
#include "br_threads.h"
#include "br_q.h"
#include "br_smol_mesh.h"
#include "include/brplot.h"

#include <unistd.h>
#include <math.h>

#define VERSION 1

typedef union {
  br_plotter_ctor_t plotter;
  br_plot_ctor_t plot;
  br_data_ctor_t data;
} br_common_ctor;

static void even_split(br_plots_t plots) {
  int visible_c = 0;
  for (int i = 0; i < plots.len; ++i) visible_c += plots.arr[i].is_visible ? 1 : 0;
  if (visible_c == 0) return;
  int col_c =  (int)(0.5f + sqrtf((float)visible_c));
  int row_c = visible_c / col_c;
  if (col_c * row_c < visible_c) row_c++;
  float w = (float)GetScreenWidth();
  float h = (float)GetScreenHeight();
  float padding_left = 50.f,
        padding_right = 10.f,
        padding_top = 10.f,
        padding_bot = 50.f;
  for (int i = 0; i < plots.len; ++i) {
    plots.arr[i].graph_screen_rect = (Rectangle) {
      padding_left + (float)(i % col_c) * w/(float)col_c,
      padding_top + (float)(i / row_c) * h/(float)row_c,
      w / (float)col_c - padding_left - padding_right,
      h / (float)row_c - padding_top - padding_bot
    };
  }
}

static void* main_loop(void* plotterv) {
  br_plotter_t* plotter = plotterv;
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(plotter->width, plotter->height, "brplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  plotter->shaders = br_shaders_malloc();
  help_load_default_font();
  while (WindowShouldClose() == false) {
    BeginDrawing();
    ClearBackground(BLACK);
    int new_w = GetScreenWidth(), new_h = GetScreenHeight();
    if (plotter->width == 0 || plotter->height == 0) {
      plotter->width = new_w;
      plotter->height = new_h;
    } else if (plotter->width != new_w || plotter->height != new_h) {
      float wd = (float)new_w / (float)plotter->width;
      float hd = (float)new_h / (float)plotter->height;
      for (int i = 0; i < plotter->plots.len; ++i) {
        Rectangle old = plotter->plots.arr[i].graph_screen_rect;
        plotter->plots.arr[i].resolution = (Vector2) { (float)new_w, (float)new_h };
        plotter->plots.arr[i].graph_screen_rect = (Rectangle) {
          old.x * wd,
          old.y * hd,
          old.width * wd,
          old.height * hd
        };
      }
    }
    even_split(plotter->plots);
    br_plotter_update_variables(plotter);
    for (int i = 0; i < plotter->plots.len; ++i) {
      br_plot_t* plot = &plotter->plots.arr[i];
      if (plot->is_visible) {
        br_plot_update_variables(plotter, plot, plotter->groups, GetMousePosition());
        br_plot_update_shader_values(plot, &plotter->shaders);
        draw_grid_numbers(plot);
        smol_mesh_grid_draw(plot, &plotter->shaders);
        br_plot_draw(plot, plotter->groups, &plotter->shaders);
      }
    }
    EndDrawing();
  }
  CloseWindow();
  plotter->should_close = true;
  return NULL;
}

static br_common_ctor ctors[128/8];
static int ctors_len = 0;

br_plotter_ctor_t* br_plotter_default_ctor(void) {
  // TODO: Only run this once...
  br_data_construct();
  br_resampling2_construct();
  ctors[ctors_len].plotter = (br_plotter_ctor_t) {
    .version = VERSION,
    .ctor = {
      .width = 800, .height = 600,
      .kind = br_plotter_ui_kind_minimal,
      .use_permastate = false,
      .use_stdin = false
    }
  };
  return &ctors[ctors_len++].plotter;
}

br_plotter_t* br_plotter_new(br_plotter_ctor_t const* ctor) {
  br_plotter_t* plotter = br_plotter_malloc();
  br_plotter_init(plotter, ctor->ctor.use_permastate);
  plotter->width = ctor->ctor.width;
  plotter->height = ctor->ctor.height;
  if (ctor->ctor.use_stdin) {
    read_input_start(plotter);
  }
  br_thread_start(&main_loop, plotter);

  return plotter;
}
// Platform specific
void br_plotter_wait(br_plotter_t const* plotter) {
  while(false == plotter->should_close) sleep(1);
}

br_plot_ctor_t* br_plot_default_ctor(void) {
  ctors[ctors_len].plot = (br_plot_ctor_t) {
    .version = VERSION,
    .ctor = {
      .kind = br_plot_kind_2d,
      .width = 1.f,
      .height = 1.f,
      .rearange_others = true
    }
  };
  return &ctors[ctors_len++].plot;
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

