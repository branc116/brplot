#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_da.h"
#include "src/br_resampling2.h"
#include "src/br_q.h"
#include "src/br_permastate.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"
#include "src/br_pp.h"
#include "src/br_theme.h"
#include "src/br_ui.h"
#include "src/br_icons.h"
#include "src/br_gl.h"

#include <math.h>
#include <string.h>

#if BR_HAS_HOTRELOAD
#  include <pthread.h>
#endif

context_t context;

br_plotter_t* br_plotter_malloc(void) {
  br_resampling2_construct();
  br_data_construct();
  br_plotter_t* br = BR_MALLOC(sizeof(br_plotter_t));
  *br = (br_plotter_t){
    .groups = {0},
    .plots = {0},
    .shaders = {0},
    .dagens = {0},
    .text = NULL,
    .commands = NULL,
#if BR_HAS_SHADER_RELOAD
    .shaders_dirty = false,
#endif
    .should_close = false,
    .switch_to_active = false,
    .active_plot_index = 0,
    .ui = {
      .dark_theme = true,
      .file_saver_inited = false,
    },
  };
#if BR_HAS_HOTRELOAD
  br->hot_state = (br_hotreload_state_t) { .handl = NULL, .func_loop = NULL, .func_init = NULL, .is_init_called = false };
  pthread_mutexattr_t attrs;
  pthread_mutexattr_init(&attrs);
  pthread_mutex_init(&br->hot_state.lock, &attrs);
#endif
  br->commands = q_malloc();
  if (NULL == br->commands) {
    LOGE("Failed to malloc command queue. Exiting...\n");
    exit(1);
  }
  context.min_sampling = 0.001f;
  context.cull_min = 2.f;
  return br;
}

void br_plotter_init(br_plotter_t* br) {
  br_plotter_init_specifics_platform(br, 1280, 720);
  brui_resizable_init();
  br->loaded_status = br_permastate_load(br);
  if (br->loaded_status != br_permastate_status_ok) {
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
  if (br->loaded_status < br_permastate_status_ui_loaded) {
    br_theme_dark();
    br_theme_reset_ui();
  }
  bruir_resizable_refresh(0);
}

void br_plotter_free(br_plotter_t* br) {
  read_input_stop();
  br_permastate_save(br);
  br_icons_deinit();
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_deinit(br_da_getp(br->plots, i));
  }
  br_datas_deinit(&br->groups);
  brui_resizable_deinit();
  br_plotter_deinit_specifics_platform(br);
  q_free(br->commands);
  BR_FREE(br->plots.arr);
  br_dagens_free(&br->dagens);
  BR_FREE(br);
}


void br_plotter_resize(br_plotter_t* br, float width, float height) {
  (void)br;
  brtl_window_size_set((int)width, (int)height);
}

br_datas_t* br_plotter_get_br_datas(br_plotter_t* br) {
  return &br->groups;
}

void br_plotter_switch_2d(br_plotter_t* br) {
  if (false == br->any_2d) {
    br->active_plot_index = br_plotter_add_plot_2d(br);
    br->switch_to_active = true;
    return;
  }
  for (int i = 0; i < br->plots.len; ++i) {
    if (br->plots.arr[i].kind != br_plot_kind_2d) continue;
    br->active_plot_index = i;
    br->switch_to_active = true;
    return;
  }
  BR_ASSERT(0);
}

void br_plotter_switch_3d(br_plotter_t* br) {
  if (false == br->any_3d) {
    br->active_plot_index = br_plotter_add_plot_3d(br);
    br->switch_to_active = true;
    return;
  }
  for (int i = 0; i < br->plots.len; ++i) {
    if (br->plots.arr[i].kind != br_plot_kind_3d) continue;
    br->active_plot_index = i;
    br->switch_to_active = true;
    return;
  }
  BR_ASSERT(0);
}

int br_plotter_add_plot_2d(br_plotter_t* br) {
  int x = 400;
  br_plot_t plot = {
    .groups_to_show = { 0 },
    .cur_extent = BR_EXTENTI( x, 50, br->win.size.width - x - 60, br->win.size.height - 110 ),
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .kind = br_plot_kind_2d,
    .dd =  {
      .zoom = BR_VEC2(1.f, 1.f),
      .offset = BR_VEC2(0.f, 0.f),
      .delta = BR_VEC2(0, 0),
    }
  };
  br_plot_create_texture(&plot);
  plot.extent_handle = brui_resizable_new(BR_EXTENTI(x, 50, br->win.size.width - x - 60, br->win.size.height - 110), 0);
  plot.menu_extent_handle = brui_resizable_new2(BR_EXTENTI(0, 0, 300, plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .hidden = true });
  br_da_push_t(int, (br->plots), plot);
  br->any_2d = true;
  return br->plots.len - 1;
}

int br_plotter_add_plot_3d(br_plotter_t* br) {
  br_plot_t plot = {
    .groups_to_show = { 0 },
    .cur_extent = BR_EXTENTI(500, 50, br->win.size.width - 500 - 60, br->win.size.height - 110),
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .kind = br_plot_kind_3d,
    .ddd =  {
      .eye = BR_VEC3(0, 0, 100),
      .target = BR_VEC3(0, 0, 0),
      .up = BR_VEC3(0, 1, 0),
      .fov_y = 1,
      .near_plane = 0.001f,
      .far_plane = 10e7f,
    }
  };
  br_plot_create_texture(&plot);
  plot.extent_handle = brui_resizable_new(BR_EXTENTI(500, 50, br->win.size.width - 500 - 60, br->win.size.height - 110), 0);
  plot.menu_extent_handle = brui_resizable_new2(BR_EXTENTI(0, 0, 300, plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .hidden = true });
  br_da_push_t(int, (br->plots), plot);
  br->any_3d = true;
  return br->plots.len - 1;
}

void br_plotter_remove_plot(br_plotter_t* br, int plot_index) {
  br_plot_deinit(br_da_getp(br->plots, plot_index));
  // 0 1 2 3 | 4
  //   |
  // 0 2 3   | 3
  int count_to_move = (br->plots.len - plot_index - 1);
  if (count_to_move > 0) {
    size_t bytes_to_move = sizeof(br->plots.arr[0]) * (size_t)count_to_move;
    memmove(br_da_getp(br->plots, plot_index), br_da_getp(br->plots, plot_index + 1), bytes_to_move);
  }
  --br->plots.len;
}

void br_plotter_frame_end(br_plotter_t* br) {
  for (size_t i = 0; i < br->groups.len; ++i) {
    if (br->groups.arr[i].is_new == false) continue;
    for (int j = 0; j < br->plots.len; ++j) {
      if (br->plots.arr[j].kind == br_plot_kind_2d && br->groups.arr[i].kind == br_data_kind_3d) continue;
      br_da_push_t(int, br->plots.arr[j].groups_to_show, br->groups.arr[i].group_id);
    }
    br->groups.arr[i].is_new = false;
  }
  //gv->lines_mesh->draw_calls = 0;
  //gv->lines_mesh->points_drawn = 0;
}

void br_plotter_export(br_plotter_t const* gv, char const * path) {
  FILE* file = fopen(path, "w");
  if (file == NULL) {
    fprintf(stderr, "Failed to open path: `%s`", path);
    return;
  }
// TODO
#if 0
  fprintf(file, "--zoomx %f\n", gv->uvZoom.x);
  fprintf(file, "--zoomy %f\n", gv->uvZoom.y);
  fprintf(file, "--offsetx %f\n", gv->uvOffset.x);
  fprintf(file, "--offsety %f\n", gv->uvOffset.y);
#endif
  br_datas_export(&gv->groups, file);
  fclose(file);
}

void br_plotter_export_csv(br_plotter_t const* br, char const * path) {
  FILE* file = fopen(path, "w");
  // TODO: Show user an error message
  if (file == NULL) return;
  br_datas_export_csv(&br->groups, file);
  fclose(file);
}

void draw_grid_numbers(br_text_renderer_t* tr, br_plot_t* plot) {
  // TODO 2D/3D
  //assert(plot->kind == br_plot_kind_2d);
  if(plot->kind != br_plot_kind_2d) return;

  TracyCFrameMarkStart("draw_grid_numbers");
  br_extent_t r = plot->dd.graph_rect;
  br_extent_t gr = BR_EXTENTI_TOF(plot->cur_extent);
  br_size_t sz = gr.size;
  int font_size = BR_THEME.ui.font_size;
  char* scrach = br_scrach_get(128);
  br_extent_t vp = BR_EXTENTI_TOF(brtl_viewport());
  br_bb_t limit = BR_EXTENT_TOBB(vp);

  if (r.height > 0.f) {
    float exp = floorf(log10f(r.height / 2.f));
    if (false == isnan(exp)) {
      float base = powf(10.f, exp);
      float start = floorf(r.y / base) * base;

      float i = 0.f;
      float x = -r.x * sz.width / r.width;
      br_text_renderer_ancor_t ancor = br_text_renderer_ancor_mid_mid;
      const float padding = 10.f;
      if (x < padding) x = padding, ancor = br_text_renderer_ancor_left_mid;
      else if (x > sz.width - padding) x = sz.width - padding, ancor = br_text_renderer_ancor_right_mid;
      while (i < 50.f) {
        float cur = start - base * i;
        i += 1.f;
        int n = sprintf(scrach, "%.*f", exp < 0 ? -(int)exp : 1, cur);
        br_strv_t s = BR_STRV(scrach, (uint32_t)n);
        s = br_strv_trim_zeros(s);
        float y = sz.height / r.height * (r.y - cur);
        if (y > sz.height) break;
        br_text_renderer_push2(tr, BR_VEC3(x, y, 0.9f), font_size, BR_THEME.colors.grid_nums, s, limit, ancor);
      }
    }
  }

  if (r.width > 0.f) {
    float exp = floorf(log10f(r.width / 2.f));
    if (false == isnan(exp)) {
      float base = powf(10.f, exp);
      if (isnan(base) || isinf(base)) goto end;
      float start = ceilf(r.x / base) * base;
      float i = 0;
      float y = r.y * sz.height / r.height;
      const float padding = 10.f;
      br_text_renderer_ancor_t ancor = br_text_renderer_ancor_mid_mid;
      if (y < padding) y = padding, ancor = br_text_renderer_ancor_mid_up;
      else if (y > sz.height - padding) y = sz.height - padding, ancor = br_text_renderer_ancor_mid_down;
      while (i < 50.f) {
        float cur = start + base * i;
        i += 1.f;
        int n = sprintf(scrach, "%.*f", exp < 0 ? -(int)exp : 1, cur);
        br_strv_t s = BR_STRV(scrach, (uint32_t)n);
        s = br_strv_trim_zeros(s);
        float x = (sz.width / r.width) * (cur - r.x);
        if (x > sz.width) break;
        br_text_renderer_push2(tr, BR_VEC3(x, y, 0.9f), font_size, BR_THEME.colors.grid_nums, s, limit, ancor);
      }
    }
  }
  end: TracyCFrameMarkEnd("draw_grid_numbers");
  br_scrach_free();
}

void br_plotter_datas_deinit(br_plotter_t* br) {
  br_datas_deinit(&br->groups);
  for (int i = 0; i < br->plots.len; ++i) {
    br->plots.arr[i].groups_to_show.len = 0;
  }
  br_dagens_free(&br->dagens);
}

