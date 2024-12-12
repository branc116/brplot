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

#include <math.h>
#include <string.h>

#if BR_HAS_HOTRELOAD
#  include <pthread.h>
#endif

#include "tracy/TracyC.h"

context_t context;

BR_API br_plotter_t* br_plotter_malloc(void) {
  br_resampling2_construct();
  br_data_construct();
  return BR_MALLOC(sizeof(br_plotter_t));
}

BR_API void br_plotter_init(br_plotter_t* br) {
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
}

BR_API void br_plotter_resize(br_plotter_t* br, float width, float height) {
  (void)br;
  brtl_window_set_size((int)width, (int)height);
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
  int x = 400;
  br_plot_t plot = {
    .groups_to_show = { 0 },
    .graph_screen_rect = BR_EXTENTI( x, 50, br->win.size.width - x - 60, br->win.size.height - 110 ),
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .is_visible = true,
    .kind = br_plot_kind_2d,
    .dd =  {
      .zoom = BR_VEC2(1.f, 1.f),
      .offset = BR_VEC2(0.f, 0.f),
      .delta = BR_VEC2(0, 0),
    }
  };
  br_plot_create_texture(&plot);
  br_da_push_t(int, (br->plots), plot);
  br->any_2d = true;
  return br->plots.len - 1;
}

int br_plotter_add_plot_3d(br_plotter_t* br) {
  br_plot_t plot = {
    .groups_to_show = { 0 },
    .graph_screen_rect = BR_EXTENTI(500, 50, br->win.size.width - 500 - 60, br->win.size.height - 110),
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
  br_da_push_t(int, (br->plots), plot);
  br->any_3d = true;
  return br->plots.len - 1;
}

BR_API void br_plotter_free(br_plotter_t* br) {
  br_text_renderer_free(br->text);
  br_shaders_free(br->shaders);
  br_datas_deinit(&br->groups);
  q_free(br->commands);
  for (int i = 0; i < br->plots.len; ++i) {
    BR_FREE(br->plots.arr[i].groups_to_show.arr);
  }
  BR_FREE(br->plots.arr);
  br_dagens_free(&br->dagens);
}

void br_plotter_update_variables(br_plotter_t* br) {
#if BR_HAS_SHADER_RELOAD
  if (br->shaders_dirty) {
    br_shaders_refresh(br->shaders);
    br->shaders_dirty = false;
  }
#endif
  br->shaders.font->uvs.resolution_uv = BR_VEC2((float)br->win.size.width, (float)br->win.size.height);
  br->shaders.font->uvs.sub_pix_aa_map_uv =  BR_VEC3(-1, 0, 1);
  br->shaders.font->uvs.sub_pix_aa_scale_uv = 0.2f;
  br_plotter_update_context(br, brtl_mouse_get_pos());
  handle_all_commands(br, br->commands);
}

BR_API void br_plotter_frame_end(br_plotter_t* br) {
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

void br_plotter_update_context(br_plotter_t* br, br_vec2_t mouse_pos) {
// TODO 2D/3D
  for (int i = 0; i < br->plots.len; ++i) br_plot_update_context(&br->plots.arr[i], mouse_pos);
}

void draw_grid_numbers(br_text_renderer_t* tr, br_plot_t* plot) {
  // TODO 2D/3D
  //assert(plot->kind == br_plot_kind_2d);
  if(plot->kind != br_plot_kind_2d) return;

  TracyCFrameMarkStart("draw_grid_numbers");
  br_extent_t r = plot->dd.graph_rect;
  br_size_t sz = BR_SIZEI_TOF(plot->graph_screen_rect.size);
  int font_size = (int)(18.f * context.font_scale);
  char fmt[16];
  char* scrach = br_scrach_get(128);

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
        sprintf(scrach, fmt, cur);
        help_trim_zeros(scrach);
        float y = sz.height / r.height * (r.y - cur);
        if (y > sz.height) break;
        br_text_renderer_push2(tr, 2.f, y, font_size, BR_WHITE, br_strv_from_c_str(scrach), br_text_renderer_ancor_left_mid);
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
      //float x_last_max = -INFINITY;
      float i = 0;
      while (i < 50.f) {
        float cur = start + base * i;
        i += 1.f;
        sprintf(scrach, fmt, cur);
        help_trim_zeros(scrach);
        //Vector2 sz = help_measure_text(scrach, font_size);
        float x = (sz.width / r.width) * (cur - r.x);
        //x -= sz.x / 2.f;
        //if (x - 5.f < x_last_max) continue; // Don't print if it will overlap with the previous text. 5.f is padding.
        //x_last_max = x + sz.x;
        if (x > sz.width) break;
        br_text_renderer_push2(tr, x, sz.height, font_size, BR_WHITE, br_strv_from_c_str(scrach), br_text_renderer_ancor_mid_up);
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

