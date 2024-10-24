#include "br_plot.h"
#include "br_plotter.h"
#include "br_gui_internal.h"
#include "br_da.h"
#include "br_resampling2.h"
#include "br_q.h"
#include "br_permastate.h"

#include <math.h>
#include <string.h>

#if BR_HAS_HOTRELOAD
#  include <pthread.h>
#endif

#include "raylib.h"
#include "rlgl.h"
#include "src/br_pp.h"
#include "tracy/TracyC.h"

context_t context;

BR_API br_plotter_t* br_plotter_malloc(void) {
  br_resampling2_construct();
  br_data_construct();
  return BR_MALLOC(sizeof(br_plotter_t));
}

BR_API void br_plotter_init(br_plotter_t* br, bool use_permaste) {
  *br = (br_plotter_t){
    .groups = {0},
    .plots = {0},
    .shaders = {0},
    .dagens = {0},
    .commands = NULL,
#if BR_HAS_SHADER_RELOAD
    .shaders_dirty = false,
#endif
    .file_saver_inited = false,
    .should_close = false,
    .switch_to_active = false,
    .active_plot_index = 0
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
  context.font_scale = 1.8f;
  memset(context.buff, 0, sizeof(context.buff));
  if (use_permaste) br->loaded = br_permastate_load(br);
  if (false == br->loaded) {
    br_datas_deinit(&br->groups);
    br->plots.len = 0;
  }
  br_gui_init_specifics_platform(br);
}

BR_API void br_plotter_resize(br_plotter_t* br, float width, float height) {
  (void)br;
  SetWindowSize((int)width, (int)height);
}

BR_API br_datas_t* br_plotter_get_br_datas(br_plotter_t* br) {
  return &br->groups;
}

BR_API void br_plotter_switch_2d(br_plotter_t* br) {
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

BR_API void br_plotter_switch_3d(br_plotter_t* br) {
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
  br_plot_t plot = {
    .groups_to_show = { 0 },
    .graph_screen_rect = { GRAPH_LEFT_PAD, 50, (float)br->width - GRAPH_LEFT_PAD - 60, (float)br->height - 110 },
    .resolution = { (float)br->width, (float)br->height },
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .is_visible = true,
    .kind = br_plot_kind_2d,
    .dd =  {
      .zoom = { 1.f, 1.f },
      .offset = { 0.f, 0.f },
      .delta = { 0 },
    }
  };
  br_da_push_t(int, (br->plots), plot);
  br->any_2d = true;
  return br->plots.len - 1;
}

int br_plotter_add_plot_3d(br_plotter_t* br) {
  br_plot_t plot = {
    .groups_to_show = { 0 },
    .graph_screen_rect = { GRAPH_LEFT_PAD, 50, (float)br->width - GRAPH_LEFT_PAD - 60, (float)br->height - 110 },
    .resolution = { (float)br->width, (float)br->height },
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .kind = br_plot_kind_3d,
    .ddd =  {
      .eye = { 0, 0, 100 },
      .target = { 0, 0, 0},
      .up = { 0, 1, 0},
      .fov_y = 1,
      .near_plane = 0.001f,
      .far_plane = 10e7f,
    }
  };
  br_da_push_t(int, (br->plots), plot);
  br->any_3d = true;
  return br->plots.len - 1;
}

BR_API void br_plotter_free(br_plotter_t* gv) {
  br_shaders_free(gv->shaders);
  br_datas_deinit(&gv->groups);
  q_free(gv->commands);
  for (int i = 0; i < gv->plots.len; ++i) {
    BR_FREE(gv->plots.arr[i].groups_to_show.arr);
  }
  BR_FREE(gv->plots.arr);
  br_dagens_free(&gv->dagens);
}

void br_plotter_update_variables(br_plotter_t* br) {
#if BR_HAS_SHADER_RELOAD
  if (br->shaders_dirty) {
    br_shaders_refresh(br->shaders);
    br->shaders_dirty = false;
  }
#endif
  Vector2 mouse_pos = GetMousePosition();
  br_plotter_update_context(br, mouse_pos);
  handle_all_commands(br, br->commands);
}

BR_API void br_plotter_frame_end(br_plotter_t* gv) {
  (void)gv;
  for (size_t i = 0; i < gv->groups.len; ++i) {
    if (gv->groups.arr[i].is_new == false) continue;
    for (int j = 0; j < gv->plots.len; ++j) {
      if (gv->plots.arr[j].kind == br_plot_kind_2d && gv->groups.arr[i].kind == br_data_kind_3d) continue;
      br_da_push_t(int, gv->plots.arr[j].groups_to_show, gv->groups.arr[i].group_id);
    }
    gv->groups.arr[i].is_new = false;
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
  for (size_t i = 0; i < gv->groups.len; ++i) {
    assert(0);
  }
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

void br_plotter_update_context(br_plotter_t* br, Vector2 mouse_pos) {
// TODO 2D/3D
  for (int i = 0; i < br->plots.len; ++i) br_plot_update_context(&br->plots.arr[i], mouse_pos);
}

void draw_grid_numbers(br_plot_t* plot) {
  // TODO 2D/3D
  //assert(plot->kind == br_plot_kind_2d);
  if(plot->kind != br_plot_kind_2d) return;

  TracyCFrameMarkStart("draw_grid_numbers");
  Rectangle r = plot->dd.graph_rect;
  Rectangle graph_screen_rect = plot->graph_screen_rect;
  float font_size = 15.f * context.font_scale;
  char fmt[16];

  if (r.height > 0.f) {
    float exp = floorf(log10f(r.height / 2.f));
    if (false == isnan(exp)) {
      float base = powf(10.f, exp);
      float start = floorf(r.y / base) * base;
      if (exp >= 0) strcpy(fmt, "%f");
      else sprintf(fmt, "%%.%df", -(int)exp);

      float i = 0.f;
      while (i < 50.f) {
        float cur = start - base * i;
        i += 1.f;
        sprintf(context.buff, fmt, cur);
        help_trim_zeros(context.buff);
        Vector2 sz = help_measure_text(context.buff, font_size);
        float y = graph_screen_rect.y + (graph_screen_rect.height / r.height) * (r.y - cur);
        y -= sz.y / 2.f;
        if (y > graph_screen_rect.y + graph_screen_rect.height) break;
        help_draw_text(context.buff, (Vector2){ .x = graph_screen_rect.x - sz.x - 2.f, .y = y }, font_size, RAYWHITE);
      }
    }
  }

  if (r.width > 0.f) {
    float exp = floorf(log10f(r.width / 2.f));
    if (false == isnan(exp)) {
      float base = powf(10.f, exp);
      if (isnan(base) || isinf(base)) goto end;
      float start = ceilf(r.x / base) * base;
      if (exp >= 0) strcpy(fmt, "%f");
      else sprintf(fmt, "%%.%df", -(int)exp);
      float x_last_max = -INFINITY;
      float i = 0;
      while (i < 50.f) {
        float cur = start + base * i;
        i += 1.f;
        sprintf(context.buff, fmt, cur);
        help_trim_zeros(context.buff);
        Vector2 sz = help_measure_text(context.buff, font_size);
        float x = graph_screen_rect.x + (graph_screen_rect.width / r.width) * (cur - r.x);
        x -= sz.x / 2.f;
        if (x - 5.f < x_last_max) continue; // Don't print if it will overlap with the previous text. 5.f is padding.
        if (x > graph_screen_rect.x + graph_screen_rect.width) break;
        x_last_max = x + sz.x;
        help_draw_text(context.buff, (Vector2){ .x = x, .y = graph_screen_rect.y + graph_screen_rect.height }, font_size, RAYWHITE);
      }
    }
  }
  end: TracyCFrameMarkEnd("draw_grid_numbers");
}

