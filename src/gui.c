#include "br_plot.h"
#include "br_gui_internal.h"
#include "br_help.h"
#include "br_da.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "Tracy/tracy/TracyC.h"

context_t context;

void emscripten_run_script(const char* script);

BR_API br_plotter_t* br_plotter_malloc(void) {
  return BR_MALLOC(sizeof(br_plotter_t));
}

BR_API void br_plotter_init(br_plotter_t* br, float width, float height) {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow((int)width, (int)height, "brplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  *br = (br_plotter_t){
    .groups = {0},
    .groups_3d = {0},
    .plots = {0},
    .shaders = {0},
    .commands = {0},
    .shaders_dirty = false,
  };
#ifdef IMGUI
#ifndef RELEASE
#ifdef LINUX
  br->hot_state = (br_hotreload_state_t) { .handl = NULL, .func_loop = NULL, .func_init = NULL, .is_init_called = false, .lock = { 0 } };
  pthread_mutexattr_t attrs;
  pthread_mutexattr_init(&attrs);
  pthread_mutex_init(&br->hot_state.lock, &attrs);
#endif
#endif
#endif
  br->shaders = br_shaders_malloc();
  q_init(&br->commands);
  help_load_default_font();

  context.font_scale = 1.8f;
  memset(context.buff, 0, sizeof(context.buff));
  br_gui_init_specifics_gui(br);
  br_gui_init_specifics_platform(br);
}

BR_API void br_plotter_resize(br_plotter_t* br, float width, float height) {
  (void)br;
  SetWindowSize((int)width, (int)height);
}

BR_API points_groups_t* br_plotter_get_points_groups(br_plotter_t* br) {
  return &br->groups;
}

void br_plotter_add_plot_instance_2d(br_plotter_t* br) {
  br_plot_instance_t plot = {
    .groups_to_show = { 0 },
    .graph_screen_rect = { GRAPH_LEFT_PAD, 50, (float)GetScreenWidth() - GRAPH_LEFT_PAD - 60, (float)GetScreenHeight() - 110 },
    .resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() },
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .kind = br_plot_instance_kind_2d,
    .dd =  {
      .line_shader = br->shaders.line,
      .grid_shader = br->shaders.grid,
      .zoom = { 1.f, 1.f },
      .offset = { 0.f, 0.f },
      .delta = { 0 },
    }
  };
  br_da_push_t(int, (br->plots), plot);
}

void br_plotter_add_plot_instance_3d(br_plotter_t* br) {
  br_plot_instance_t plot = {
    .groups_to_show = { 0 },
    .graph_screen_rect = { GRAPH_LEFT_PAD, 50, (float)GetScreenWidth() - GRAPH_LEFT_PAD - 60, (float)GetScreenHeight() - 110 },
    .resolution = { (float)GetScreenWidth(), (float)GetScreenHeight() },
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .kind = br_plot_instance_kind_3d,
    .ddd =  {
      .grid_shader = br->shaders.grid_3d,
      .line_shader = br->shaders.line_3d,
      .line_simple_shader = br->shaders.line_3d_simple,
      .eye = { 0, 0, 100 },
      .target = { 0, 0, 0},
      .up = { 0, 1, 0},
      .fov_y = 1,
      .near_plane = 0.001f,
      .far_plane = 10e7f,
      .groups_3d_to_show = { 0 }
    }
  };
  br_da_push_t(int, (br->plots), plot);
}

BR_API void br_plotter_free(br_plotter_t* gv) {
  br_shaders_free(gv->shaders);
  for (size_t i = 0; i < gv->groups.len; ++i) {
    points_groups_deinit(&gv->groups);
  }
  q_command c = q_pop(&gv->commands);
  while(c.type != q_command_none) {
    if (c.type == q_command_set_name) br_str_free(c.set_quoted_str.str);
    c = q_pop(&gv->commands);
  }
  BR_FREE(gv->commands.commands);
  for (int i = 0; i < gv->plots.len; ++i) {
    BR_FREE(gv->plots.arr[i].groups_to_show.arr);
  }
  BR_FREE(gv->plots.arr);
}

void br_plotter_update_variables(br_plotter_t* br) {
#ifndef RELEASE
  if (br->shaders_dirty) {
    br_shaders_refresh(br->shaders);
    br->shaders_dirty = false;
  }
#endif
  Vector2 mouse_pos = GetMousePosition();
  br_plotter_update_context(br, mouse_pos);
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_instance_t* plot = &br->plots.arr[i];
    switch (plot->kind) {
      case br_plot_instance_kind_2d: {
        if (br_plot_instance_update_variables_2d(plot, br->groups, mouse_pos))
          br_keybinding_handle_keys(br, plot);
      } break;
      case br_plot_instance_kind_3d: {
        if (br_plot_instance_update_variables_3d(plot, br->groups, mouse_pos))
          br_keybinding_handle_keys(br, plot);
      } break;
      default: assert(0);
    }
  }

  while (1) {
    q_command comm = q_pop(&br->commands);
    switch (comm.type) {
      case q_command_none:          goto end;
      case q_command_push_point_x:  points_group_push_x(&br->groups, comm.push_point_x.x, comm.push_point_y.group); break;
      case q_command_push_point_y:  points_group_push_y(&br->groups, comm.push_point_y.y, comm.push_point_y.group); break;
      case q_command_push_point_xy: points_group_push_xy(&br->groups, comm.push_point_xy.x, comm.push_point_xy.y, comm.push_point_xy.group); break;
      case q_command_pop:           break; //TODO
      case q_command_clear:         points_group_clear(&br->groups, br->plots, comm.clear.group); break;
      case q_command_clear_all:     points_groups_deinit(&br->groups); break;
      case q_command_screenshot:    br_plot_instance_screenshot(&br->plots.arr[0], br->groups, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_export:        br_plotter_export(br, comm.path_arg.path);     free(comm.path_arg.path); break;
      case q_command_exportcsv:     br_plotter_export_csv(br, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_hide:          points_group_get(&br->groups, comm.hide_show.group)->is_selected = false; break;
      case q_command_show:          points_group_get(&br->groups, comm.hide_show.group)->is_selected = true;  break;
      case q_command_set_name:      points_group_set_name(&br->groups, comm.set_quoted_str.group, comm.set_quoted_str.str);  break;
      case q_command_focus:         br_plotter_focus_visible(&br->plots.arr[0], br->groups); break;
      default:                      BR_ASSERT(false);
    }
  }
  end: return;
}

BR_API void br_plotter_frame_end(br_plotter_t* gv) {
  (void)gv;
  for (size_t i = 0; i < gv->groups.len; ++i) {
    if (gv->groups.arr[i].is_new == false) continue;
    for (int j = 0; j < gv->plots.len; ++j) {
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
    fprintf(file, gv->groups.arr[i].is_selected ? "--show %d\n" : "--hide %d\n", gv->groups.arr[i].group_id);
  }
  points_groups_export(&gv->groups, file);
  fclose(file);
}

void br_plotter_export_csv(br_plotter_t const* br, char const * path) {
  FILE* file = fopen(path, "w");
  // TODO: Show user an error message
  if (file == NULL) return;
  points_groups_export_csv(&br->groups, file);
  fclose(file);
}

void br_plotter_update_context(br_plotter_t* br, Vector2 mouse_pos) {
// TODO 2D/3D
  for (int i = 0; i < br->plots.len; ++i) br_plot_instance_update_context(&br->plots.arr[i], mouse_pos);
}

void draw_grid_numbers(br_plot_instance_t* plot) {
  // TODO 2D/3D
  //assert(plot->kind == br_plot_instance_kind_2d);
  if(plot->kind != br_plot_instance_kind_2d) return;

  TracyCFrameMarkStart("draw_grid_numbers");
  Rectangle r = plot->dd.graph_rect;
  Rectangle graph_screen_rect = plot->graph_screen_rect;
  float font_size = 15.f * context.font_scale;
  char fmt[16];

  if (r.height > 1) {
    float exp = floorf(log10f(r.height / 2.f));
    if (false == isnan(exp)) {
      float base = powf(10.f, exp);
      float start = floorf(r.y / base) * base;
      if (exp >= 0) strcpy(fmt, "%f");
      else sprintf(fmt, "%%.%df", -(int)exp);

      float i = 0.f;
      while (base * i < r.height) {
        float cur = start - base * i;
        i += 1.f;
        sprintf(context.buff, fmt, cur);
        help_trim_zeros(context.buff);
        Vector2 sz = help_measure_text(context.buff, font_size);
        float y = graph_screen_rect.y + (graph_screen_rect.height / r.height) * (r.y - cur);
        y -= sz.y / 2.f;
        help_draw_text(context.buff, (Vector2){ .x = graph_screen_rect.x - sz.x - 2.f, .y = y }, font_size, RAYWHITE);
      }
    }
  }

  if (r.width > 1.f) {
    float exp = floorf(log10f(r.width / 2.f));
    if (false == isnan(exp)) {
      float base = powf(10.f, exp);
      if (isnan(base) || isinf(base)) goto end;
      float start = ceilf(r.x / base) * base;
      if (exp >= 0) strcpy(fmt, "%f");
      else sprintf(fmt, "%%.%df", -(int)exp);
      float x_last_max = -INFINITY;
      float i = 0;
      while (base * i < r.width) {
        float cur = start + base * i;
        i += 1.f;
        sprintf(context.buff, fmt, cur);
        help_trim_zeros(context.buff);
        Vector2 sz = help_measure_text(context.buff, font_size);
        float x = graph_screen_rect.x + (graph_screen_rect.width / r.width) * (cur - r.x);
        x -= sz.x / 2.f;
        if (x - 5.f < x_last_max) continue; // Don't print if it will overlap with the previous text. 5.f is padding.
        x_last_max = x + sz.x;
        help_draw_text(context.buff, (Vector2){ .x = x, .y = graph_screen_rect.y + graph_screen_rect.height }, font_size, RAYWHITE);
      }
    }
  }
  end: TracyCFrameMarkEnd("draw_grid_numbers");
}

