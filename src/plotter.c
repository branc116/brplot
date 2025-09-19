#include "src/br_pp.h"
#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_gui.h"
#include "src/br_da.h"
#include "src/br_resampling.h"
#include "src/br_q.h"
#include "src/br_permastate.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"
#include "src/br_theme.h"
#include "src/br_ui.h"
#include "src/br_icons.h"
#include "src/br_gl.h"
#include "src/br_free_list.h"
#include "src/br_memory.h"

#include <math.h>
#include <string.h>

#if BR_HAS_HOTRELOAD
#  include <pthread.h>
#endif
static bool br_plot_update_variables_2d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos);
static bool br_plot_update_variables_3d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos);

br_plotter_t* br_plotter_malloc(void) {
  br_resampling_construct();
  br_data_construct();
  br_plotter_t* br = BR_MALLOC(sizeof(br_plotter_t));
  *br = (br_plotter_t){
    .groups = {0},
    .plots = {0},
    .shaders = {0},
    .dagens = {0},
    .text = NULL,
    .commands = NULL,
    .pressed_chars = {0},
#if BR_HAS_SHADER_RELOAD
    .shaders_dirty = false,
#endif
    .should_close = false,
    .ui = {
      .fm_state = {
        .file_selected = -1,
        .path_id = -1,
      },
      .dark_theme = true,
    },
  };
#if BR_HAS_HOTRELOAD
  br->hot_state = (br_hotreload_state_t) { .handl = NULL, .plotter = br, .func_loop_ui = NULL, .func_loop = NULL, .func_init = NULL, .is_init_called = false };
#endif
  br->commands = q_malloc();
  if (NULL == br->commands) {
    LOGE("Failed to malloc command queue. Exiting...\n");
    exit(1);
  }
  return br;
}

static br_plot_t br_plot_2d(void) {
  br_sizei_t ws = brtl_window_size();
  int padding = 4;
  br_plot_t plot = {
    .data_info = { 0 },
    .cur_extent = BR_EXTENTI( padding, padding, ws.width - padding*2, ws.height - padding*2 ),
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .kind = br_plot_kind_2d,
    .dd =  {
#if defined(__EMSCRIPTEN__)
      .zoom = BR_VEC2D(10.f, 10.f),
#else
      .zoom = BR_VEC2D(1.f, 1.f),
#endif
      .offset = BR_VEC2D(0.f, 0.f),
      .grid_line_thickness =  brtl_theme()->ui.default_grid_line_thickenss,
      .grid_major_line_thickness = 2.f,
      .line_thickness = 0.05f
    }
  };
  return plot;
}

void br_plotter_init(br_plotter_t* br) {
  br_plotter_init_specifics_platform(br, 1280, 720);
  br_theme_reset_ui();
  br->loaded_status = br_permastate_load(br);
  if (br->loaded_status != br_permastate_status_ok) {
    br_datas_deinit(&br->groups);
    br->plots.len = 0;
    if (br_permastate_status_ui_loaded == br->loaded_status) {
      br_plot_t plot = br_plot_2d();
      br_plot_create_texture(&plot);
      bool found_resizable = false, found_menu_ex = false, found_legend_ex = false;
      brfl_foreach(i, br->resizables) {
        if (br_da_get(br->resizables, i).tag != 100) continue;
        found_resizable = true;
        plot.extent_handle = i;
        break;
      }
      if (false == found_resizable) {
        plot.extent_handle = brui_resizable_new2(BR_EXTENTI_TOF(plot.cur_extent), 0, (brui_resizable_t) { .current = { .title_enabled = true, .tag = 100 } });
      } else {
        brfl_foreach(i, br->resizables) {
          if (br_da_get(br->resizables, i).parent != plot.extent_handle) continue;
          int tag = br_da_get(br->resizables, i).tag;
          if (tag == 101) {
            found_menu_ex = true;
            plot.menu_extent_handle = i;
          }
          if (tag == 102) {
            found_legend_ex = true;
            plot.legend_extent_handle = i;
          }
        }
      }
      if (false == found_menu_ex) {
        plot.menu_extent_handle = brui_resizable_new2(BR_EXTENT(0, 0, 300, (float)plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .current.tag = 101, .target = { .hidden_factor = 1.f } });
      }
      if (false == found_legend_ex) {
        plot.legend_extent_handle = brui_resizable_new2(BR_EXTENT((float)plot.cur_extent.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { .current.tag = 102 });
      }
      br_da_push_t(int, (br->plots), plot);
    } else {
      brui_resizable_init();
      br_plotter_add_plot_2d(br);
    }
  } else {
    for (int i = 0; i < br->plots.len; ++i) {
      br_plot_t* p = &br->plots.arr[i];
      br->plots.arr[i].texture_id = brgl_create_framebuffer(p->cur_extent.width, p->cur_extent.height);
    }
  }
  br_icons_init(br->shaders.icon);
  if (br->loaded_status < br_permastate_status_ui_loaded) {
    brtl_bruirs()->menu_extent_handle = brui_resizable_new(BR_EXTENT(10, 40, 160, (float)brtl_viewport().height/2.f), 0); 
    br_theme_dark();
    br_theme_reset_ui();
  } else {
    br_text_renderer_load_font(br->text, brsp_try_get(*brtl_brsp(), br->ui.font_path_id));
  }
  memset(&br->ui.fm_state.cur_dir, 0, sizeof(br->ui.fm_state.cur_dir));
  bruir_resizable_refresh(0);
}

void br_plotter_deinit(br_plotter_t* br) {
  read_input_stop();
  br_permastate_save(br);
  br_icons_deinit();
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_deinit(br_da_getp(br->plots, i));
  }
  br_datas_deinit(&br->groups);
  brui_resizable_deinit();
  br_plotter_deinit_specifics_platform(br);
  BR_FREE(br->plots.arr);
  br_dagens_free(&br->dagens);
  brsp_free(brtl_brsp());
  LOGI("Plotter deinited");
}

void br_plotter_free(br_plotter_t* br) {
  for (size_t i = 0; i < br->ui.fm_state.cur_dir.len; ++i) {
    br_str_free(br->ui.fm_state.cur_dir.arr[i].name);
  }
  br_str_free(br->ui.fm_state.cur_dir.cur_dir);
  br_str_free(br->ui.fm_state.cur_dir.last_good_dir);
  br_da_free(br->ui.fm_state.cur_dir);
  q_free(br->commands);
  br_da_free(br->pressed_chars);
  br_scrach_finish();
  brui_finish();
  //BR_FREE(br);
  LOGI("Plotter freed");
}

void br_plotter_resize(br_plotter_t* br, float width, float height) {
  (void)br;
  brtl_window_size_set((int)width, (int)height);
}

br_datas_t* br_plotter_get_br_datas(br_plotter_t* br) {
  return &br->groups;
}

void br_plotter_switch_2d(br_plotter_t* br) {
  for (int i = 0; i < br->plots.len; ++i) {
    if (br->plots.arr[i].kind != br_plot_kind_2d) continue;
    return;
  }
  br_plotter_add_plot_2d(br);
}

void br_plotter_switch_3d(br_plotter_t* br) {
  for (int i = 0; i < br->plots.len; ++i) {
    if (br->plots.arr[i].kind != br_plot_kind_3d) continue;
    return;
  }
  br_plotter_add_plot_3d(br);
}

void br_plots_update_variables(br_plotter_t* br, br_plot_t* plot, br_datas_t groups, br_vec2_t mouse_pos) {
  for (int i = 0; i < plot->data_info.len; ++i) {
    plot->data_info.arr[i].thickness_multiplyer = br_float_lerp(plot->data_info.arr[i].thickness_multiplyer, plot->data_info.arr[i].thickness_multiplyer_target, brtl_frame_time()*5);
  }
  bool apply_kb = true;
  switch (plot->kind) {
    case br_plot_kind_2d: {
      apply_kb = br_plot_update_variables_2d(plot, groups, mouse_pos);
    } break;
    case br_plot_kind_3d: {
      apply_kb = br_plot_update_variables_3d(plot, groups, mouse_pos);
    } break;
    default: BR_UNREACHABLE("Plot kind %d is not handled", plot->kind);
  }
  apply_kb &= brui_action()->kind == brui_action_none;
  if (apply_kb) br_keybinding_handle_keys(br, plot);
}

static bool br_plot_update_variables_2d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos) {
  BR_ASSERT(plot->kind == br_plot_kind_2d);
  if (plot->follow) {
    br_vec2d_t maxy = BR_VEC2D(-DBL_MAX, -DBL_MAX);
    br_vec2d_t mini = BR_VEC2D(DBL_MAX, DBL_MAX);
    int n = 0;
    for (int i = 0; i < plot->data_info.len; ++i) {
      br_plot_data_t di = plot->data_info.arr[i];
      if (false == BR_PLOT_DATA_IS_VISIBLE(di)) continue;
      br_data_t* pg = br_data_get1(groups, di.group_id);
      if (pg->len == 0) continue;

      for (size_t j = 0; j < 50 && j < pg->len; ++j) {
        double cx = ((double)pg->dd.xs[pg->len - 1 - j] + pg->dd.rebase_x);
        double cy = ((double)pg->dd.ys[pg->len - 1 - j] + pg->dd.rebase_y);
        if (cx > maxy.x) maxy.x = cx;
        if (cx < mini.x) mini.x = cx;
        if (cy > maxy.y) maxy.y = cy;
        if (cy < mini.y) mini.y = cy;
        ++n;
      }
    }
    if (n > 3) {
      br_vec2d_t wanted_zoom = br_vec2d_scale(br_vec2d_sub(maxy, mini), 2.8);
      if (maxy.y == mini.y) wanted_zoom.y = (double)plot->dd.zoom.y;
      if (maxy.x == mini.x) wanted_zoom.x = (double)plot->dd.zoom.x;
      br_vec2d_t wanted_offset = br_vec2d_add(br_vec2d_scale(maxy, 0.5), br_vec2d_scale(mini, 0.5));
      plot->dd.zoom = br_vec2d_lerp(plot->dd.zoom, wanted_zoom, 1.f*brtl_frame_time());
      plot->dd.offset = br_vec2d_lerp(plot->dd.offset, wanted_offset, 0.05f);
    }
  }
  if (plot->mouse_inside_graph) {
    // TODO: Move this to br_keybindings.c
    // Stuff related to zoom
    {
      float mw = -brtl_mouse_scroll().y;
      br_vec2d_t old = plot->dd.mouse_pos;
      bool any = false;
      if (false == br_float_near_zero(mw)) {
        float mw_scale = (1 + mw/10);
        if (brtl_key_down('x')) {
          plot->dd.zoom.x *= mw_scale;
        } else if (brtl_key_down('y')) {
          plot->dd.zoom.y *= mw_scale;
        } else {
          plot->dd.zoom.x *= mw_scale;
          plot->dd.zoom.y *= mw_scale;
        }
        any = true;
      }
      if (brtl_key_down('x') && brtl_key_shift()) any = true, plot->dd.zoom.x *= 1.1f;
      if (brtl_key_down('y') && brtl_key_shift()) any = true, plot->dd.zoom.y *= 1.1f;
      if (brtl_key_down('x') && brtl_key_ctrl())  any = true, plot->dd.zoom.x *= .9f;
      if (brtl_key_down('y') && brtl_key_ctrl())  any = true, plot->dd.zoom.y *= .9f;
      if (any) {
        br_plot_update_context(plot, mouse_pos);
        br_vec2d_t now = plot->dd.mouse_pos;
        plot->dd.offset.x -= now.x - old.x;
        plot->dd.offset.y -= now.y - old.y;
        brui_action_stop();
      }
    }
    if (false && plot->jump_around) {
      double t = brtl_time();
      plot->cur_extent.x += (int)(100.f * (float)sin(t));
      plot->cur_extent.y += (int)(77.f * (float)cos(t));
      plot->cur_extent.width += (int)(130.f * (float)sin(t));
      plot->cur_extent.height += (int)(177.f * (float)cos(t));
    }
    if (brtl_mouser_down()) {
      br_vec2_t delt = brtl_mouse_delta();
      float height = (float)plot->cur_extent.height;
      plot->dd.offset.x -= plot->dd.zoom.x*delt.x/height;
      plot->dd.offset.y += plot->dd.zoom.y*delt.y/height;
      brui_action_stop();
      return false;
    } else return true;
  }
  return false;
}

static bool br_plot_update_variables_3d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos) {
  (void)groups; (void)mouse_pos;
  BR_ASSERT(plot->kind == br_plot_kind_3d);
  if (!plot->mouse_inside_graph) return false;
  if (brtl_mouser_down()) {
    float speed = 10.f;
    if (brtl_key_ctrl()) speed *= 0.1f;
    if (brtl_key_shift()) speed *= 10.f;
    br_vec3_t zeroed = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
    br_vec3_t zero_dir = br_vec3_normalize(zeroed);
    br_vec3_t right = br_vec3_normalize(br_vec3_cross(plot->ddd.up, zero_dir));
    br_vec3_t delta = {0};
    if (brtl_key_down('w')) delta = br_vec3_scale(zero_dir, -brtl_frame_time()*speed);
    if (brtl_key_down('s')) delta = br_vec3_add(delta, br_vec3_scale(zero_dir, brtl_frame_time()*speed));
    if (brtl_key_down('a')) delta = br_vec3_add(delta, br_vec3_scale(right, -brtl_frame_time()*speed));
    if (brtl_key_down('d')) delta = br_vec3_add(delta, br_vec3_scale(right, brtl_frame_time()*speed));
    if (brtl_key_down('q')) delta = br_vec3_add(delta, br_vec3_scale(plot->ddd.up, -brtl_frame_time()*speed));
    if (brtl_key_down('e')) delta = br_vec3_add(delta, br_vec3_scale(plot->ddd.up, brtl_frame_time()*speed));

    plot->ddd.target = br_vec3_add(plot->ddd.target, delta);
    plot->ddd.eye = br_vec3_add(plot->ddd.eye, delta);

    br_vec2_t m = brtl_mouse_delta();
    br_vec2_t md = br_vec2_scale(BR_VEC2(m.x, m.y), -0.003f);
    br_vec3_t rotated_up = br_vec3_rot(zeroed, plot->ddd.up, md.x);
    br_vec3_t rotated_right = br_vec3_rot(rotated_up, right, md.y);
    if (fabsf(br_vec3_dot(rotated_right, plot->ddd.up)) > 0.94f) plot->ddd.eye = br_vec3_add(rotated_up,    plot->ddd.target);
    else                                                         plot->ddd.eye = br_vec3_add(rotated_right, plot->ddd.target);
    plot->ddd.eye = br_vec3_add(rotated_right, plot->ddd.target);
    return false;
  }
  {
    float mw = brtl_mouse_scroll().y;
    float mw_scale = (1 + mw/10);
    br_vec3_t zeroed = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
    float len = br_vec3_len(zeroed);
    len *= mw_scale;
    plot->ddd.eye = br_vec3_add(br_vec3_scale(br_vec3_normalize(zeroed), len), plot->ddd.target);
  }
  return true;
}



int br_plotter_add_plot_2d(br_plotter_t* br) {
  br_plot_t plot = br_plot_2d();
  br_plot_create_texture(&plot);
  plot.extent_handle = brui_resizable_new2(BR_EXTENTI_TOF(plot.cur_extent), 0, (brui_resizable_t) { .tag = 100, .title_enabled = true });
#if defined(__EMSCRIPTEN__)
  brui_resizable_maximize(plot.extent_handle, true);
#endif
  plot.menu_extent_handle = brui_resizable_new2(BR_EXTENT(0, 0, 300, (float)plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .current.tag = 101, .target.hidden_factor = 1.f });
  plot.legend_extent_handle = brui_resizable_new2(BR_EXTENT((float)plot.cur_extent.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { .current.tag = 102 });
  br_da_push_t(int, (br->plots), plot);
  return br->plots.len - 1;
}

int br_plotter_add_plot_3d(br_plotter_t* br) {
  int padding = 4;
  br_plot_t plot = {
    .data_info = { 0 },
    .cur_extent = BR_EXTENTI(padding, padding, br->win.size.width - 2 * padding, br->win.size.height - 2 * padding),
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
  plot.extent_handle = brui_resizable_new(BR_EXTENT(500, 50, (float)br->win.size.width - 500 - 60, (float)br->win.size.height - 110), 0);
  plot.menu_extent_handle = brui_resizable_new2(BR_EXTENT(0, 0, 300, (float)plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .target.hidden_factor = 1.f });
  plot.legend_extent_handle = brui_resizable_new2(BR_EXTENT((float)plot.cur_extent.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { 0 });
  br_da_push_t(int, (br->plots), plot);
  return br->plots.len - 1;
}

void br_plotter_remove_plot(br_plotter_t* br, int plot_index) {
  // TODO: Implement free list with plots...
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

void br_plotter_data_remove(br_plotter_t* br, int group_id) {
  br_datas_t* pg = &br->groups;
  br_plots_t plots = br->plots;
  br_data_remove(pg, group_id);
  br_plots_remove_group(plots, group_id);
  br_da_remove_feeld(br->dagens, group_id, group_id);
}

void br_plotter_frame_end(br_plotter_t* br) {
  for (size_t i = 0; i < br->groups.len; ++i) {
    if (br->groups.arr[i].is_new == false) continue;
    bool any_added = false;
    for (int j = 0; j < br->plots.len; ++j) {
      if (br->plots.arr[j].kind == br_plot_kind_2d && br->groups.arr[i].kind == br_data_kind_2d) {
        br_da_push_t(int, br->plots.arr[j].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
        any_added = true;
      } else if (br->plots.arr[j].kind == br_plot_kind_3d) {
        br_da_push_t(int, br->plots.arr[j].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
        any_added = true;
      }
    }
    if (false == any_added) {
      switch (br->groups.arr[i].kind) {
        case br_data_kind_2d: {
          int id = br_plotter_add_plot_2d(br);
          br_da_push_t(int, br->plots.arr[id].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
        } break;
        case br_data_kind_3d: {
          int id = br_plotter_add_plot_3d(br);
          br_da_push_t(int, br->plots.arr[id].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
        } break;
        default: BR_UNREACHABLE("Kind: %d", br->groups.arr[i].kind);
      }
    }
    br->groups.arr[i].is_new = false;
  }
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
  br_extentd_t r = plot->dd.graph_rect;
  br_extentd_t gr = BR_EXTENTI_TOD(plot->cur_extent);
  br_vec2d_t sz = BR_VEC2D(gr.width, gr.height);
  int font_size = BR_THEME.ui.font_size;
  char* scrach = br_scrach_get(128);
  br_extentd_t vp = BR_EXTENTI_TOD(brtl_viewport());
  br_bbd_t limit = BR_EXTENTD_TOBB(vp);
  br_bb_t limitf = BR_BBD_TOF(limit);

  if (r.height > 0.f) {
    double exp = floor(log10(r.height / 2.f));
    if (false == isnan(exp)) {
      double base = pow(10.0, exp);
      double start = floor(r.y / base) * base;

      double i = 0.f;
      double x = -r.x * sz.x / r.width;
      br_text_renderer_ancor_t ancor = br_text_renderer_ancor_mid_mid;
      const float padding = 10.f;
      if (x < padding) x = padding, ancor = br_text_renderer_ancor_left_mid;
      else if (x > sz.x - padding) x = sz.x - padding, ancor = br_text_renderer_ancor_right_mid;
      while (i < 50.f) {
        double cur = start - base * i;
        i += 1.f;
        int n = sprintf(scrach, "%.*f", exp < 0 ? -(int)exp : 1, cur);
        br_strv_t s = BR_STRV(scrach, (uint32_t)n);
        s = br_strv_trim_zeros(s);
        double y = sz.y / r.height * (r.y - cur);
        if (y > sz.y) break;
        br_text_renderer_push2(tr, BR_VEC3((float)x, (float)y, 0.9f), font_size, BR_THEME.colors.grid_nums, BR_COLOR(0,0,0,0), s, limitf, ancor);
      }
    }
  }

  if (r.width > 0.0) {
    double exp = floor(log10(r.width / 2.0));
    if (false == isnan(exp)) {
      double base = pow(10.0, exp);
      if (false == isfinite(base)) goto end;
      double start = ceil(r.x / base) * base;
      double i = 0;
      double y = r.y * sz.y / r.height;
      const float padding = 10.f;
      br_text_renderer_ancor_t ancor = br_text_renderer_ancor_mid_mid;
      if (y < padding) y = padding, ancor = br_text_renderer_ancor_mid_up;
      else if (y > sz.y - padding) y = sz.y - padding, ancor = br_text_renderer_ancor_mid_down;
      while (i < 50.0) {
        double cur = start + base * i;
        i += 1.0;
        int n = sprintf(scrach, "%.*f", exp < 0 ? -(int)exp : 1, cur);
        br_strv_t s = BR_STRV(scrach, (uint32_t)n);
        s = br_strv_trim_zeros(s);
        double x = (sz.x / r.width) * (cur - r.x);
        if (x > sz.x) break;
        br_text_renderer_push2(tr, BR_VEC3((float)x, (float)y, 0.9f), font_size, BR_THEME.colors.grid_nums, BR_COLOR(0,0,0,0), s, limitf, ancor);
      }
    }
  }
  end: TracyCFrameMarkEnd("draw_grid_numbers");
  br_scrach_free();
}

void br_plotter_datas_deinit(br_plotter_t* br) {
  br_datas_deinit(&br->groups);
  for (int i = 0; i < br->plots.len; ++i) {
    br->plots.arr[i].data_info.len = 0;
  }
  br_dagens_free(&br->dagens);
}

void br_on_fatal_error(void) {
  br_str_t config_file = { 0 };
  br_str_t config_file_old = { 0 };
  br_fs_get_config_dir(&config_file);
  br_fs_cd(&config_file, BR_STRL("plotter.br"));
  br_str_copy2(&config_file_old, config_file);
  br_str_push_strv(&config_file_old, BR_STRL(".old."));
  br_str_push_float(&config_file_old, (float)brtl_time());
  br_str_push_zero(&config_file);
  br_str_push_zero(&config_file_old);

  br_fs_move(config_file.str, config_file_old.str);

  br_str_free(config_file);
  br_str_free(config_file_old);
}
