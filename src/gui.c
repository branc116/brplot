#include "br_plot.h"
#include "br_gui_internal.h"
#include "br_help.h"

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "raylib.h"

#ifndef RELEASE
static void refresh_shaders_if_dirty(br_plot_t* gv);
#endif

context_t context;

void emscripten_run_script(const char* script);
static br_shader_t graph_load_shader(char const* vs, char const* fs);

BR_API br_plot_t* graph_malloc(void) {
  return BR_MALLOC(sizeof(br_plot_t));
}

BR_API void graph_init(br_plot_t* br, float width, float height) {
  InitWindow((int)width, (int)height, "brplot");
  SetWindowState(FLAG_VSYNC_HINT);
  *br = (br_plot_t){
    .gridShader    = graph_load_shader(NULL, SHADER_GRID_FS),
    .linesShader   = graph_load_shader(SHADER_LINE_VS, SHADER_LINE_FS),
    .quadShader    = graph_load_shader(SHADER_QUAD_VS, SHADER_QUAD_FS),
    .grid3dShader  = graph_load_shader(SHADER_GRID_3D_VS, SHADER_GRID_3D_FS),
    .lines3dShader = graph_load_shader(SHADER_LINE_3D_VS, SHADER_LINE_3D_FS),
#ifndef RELEASE
#ifdef IMGUI
#ifdef LINUX
    .hot_state = { .handl = NULL, .func_loop = NULL, .func_init = NULL, .is_init_called = false, .lock = { 0 } },
#endif
#endif
#endif
    .uvOffset = { 0., 0. },
    .uvZoom   = { 1., 1. },
    .uvScreen = { width, height },
    .uvDelta  = { 0., 0. },

    // 3D stuff
    .uvMvp           = {0},
    .uvMatView       = {0},
    .uvMatProjection = {0},
    .eye             = { 0.f, 0.f, -1.f},
    .target          = { 0.f, 0.f, 0.f},
    .up              = { 0.f, 1.f, 0.f },
    .fov_y           = PI / 2.f,
    .near_plane      = 0.1f,
    .far_plane       = 1000.f,

    .groups = {0},
    .groups_3d = {0},

    .graph_screen_rect = { GRAPH_LEFT_PAD, 50, width - GRAPH_LEFT_PAD - 60, height - 110 },

    .lines_mesh = NULL,
    .quads_mesh = NULL,
    .lines_mesh_3d = NULL,
    .graph_mesh_3d = NULL,

    .follow = false,
    .shaders_dirty = false,
    .commands = {0},
    .mouse_inside_graph = false,
    .recoil = 0.85f,
    .show_closest = false,
    .show_x_closest = false,
    .show_y_closest = false
  };
#ifdef IMGUI
#ifndef RELEASE
#ifdef LINUX
  pthread_mutexattr_t attrs;
  pthread_mutexattr_init(&attrs);
  pthread_mutex_init(&br->hot_state.lock, &attrs);
#endif
#endif
#endif
  br->lines_mesh = smol_mesh_malloc(PTOM_COUNT, br->linesShader.shader);
  br->quads_mesh = smol_mesh_malloc(PTOM_COUNT, br->quadShader.shader);
  br->lines_mesh_3d = smol_mesh_3d_malloc(PTOM_COUNT, br->lines3dShader.shader);
  br->graph_mesh_3d = smol_mesh_malloc(1, br->grid3dShader.shader);
  q_init(&br->commands);
  help_load_default_font();

  context.font_scale = 1.8f;
  memset(context.buff, 0, sizeof(context.buff));
  br_gui_init_specifics_gui(br);
  br_gui_init_specifics_platform(br);
}

BR_API void graph_resize(br_plot_t* br, float width, float height) {
  (void)br;
  SetWindowSize((int)width, (int)height);
}

BR_API points_groups_t* graph_get_points_groups(br_plot_t* br) {
  return &br->groups;
}

BR_API void graph_free(br_plot_t* gv) {
  for (size_t i = 0; i < sizeof(gv->shaders) / sizeof(br_shader_t); ++i) {
    UnloadShader(gv->shaders[i].shader);
  }
  smol_mesh_free(gv->lines_mesh);
  smol_mesh_free(gv->quads_mesh);
  smol_mesh_3d_free(gv->lines_mesh_3d);
  smol_mesh_free(gv->graph_mesh_3d);
  for (size_t i = 0; i < gv->groups.len; ++i) {
    points_groups_deinit(&gv->groups);
  }
  q_command c = q_pop(&gv->commands);
  while(c.type != q_command_none) {
    if (c.type == q_command_set_name) br_str_free(c.set_quoted_str.str);
    c = q_pop(&gv->commands);
  }
  BR_FREE(gv->commands.commands);
}

#include "rlgl.h"

void update_shader_values(br_plot_t* br) {
  for (size_t i = 0; i < sizeof(br->shaders) / sizeof(br_shader_t); ++i) {
    SetShaderValue(br->shaders[i].shader, br->shaders[i].uResolution, &br->graph_screen_rect, SHADER_UNIFORM_VEC4);
    SetShaderValue(br->shaders[i].shader, br->shaders[i].uZoom, &br->uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(br->shaders[i].shader, br->shaders[i].uOffset, &br->uvOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(br->shaders[i].shader, br->shaders[i].uScreen, &br->uvScreen, SHADER_UNIFORM_VEC2);
    SetShaderValue(br->shaders[i].shader, br->shaders[i].uTime, &br->uvTime, SHADER_UNIFORM_FLOAT);
    rlSetUniformMatrix(br->shaders[i].uMvp, br->uvMvp);
    rlSetUniformMatrix(br->shaders[i].uMatView, br->uvMatView);
    rlSetUniformMatrix(br->shaders[i].uMatProjection, br->uvMatProjection);
  }
  // TODO: don't assign this every frame, no need for it. Assign it only when shaders are recompiled.
  br->lines_mesh->active_shader = br->linesShader.shader;
  br->quads_mesh->active_shader = br->quadShader.shader;
  br->lines_mesh_3d->active_shader = br->lines3dShader.shader;
  br->graph_mesh_3d->active_shader = br->grid3dShader.shader;

  context.last_zoom_value = br->uvZoom;
}

void update_variables(br_plot_t* br) {
#ifndef RELEASE
  refresh_shaders_if_dirty(br);
#endif
  graph_update_context(br);
  br->uvTime += GetFrameTime();
  br->uvMatProjection = MatrixPerspective(br->fov_y, br->graph_rect.y / br->graph_rect.x, br->near_plane, br->far_plane);
  br->uvMatView = MatrixLookAt(br->eye, br->target, br->up);
  br->uvMvp = MatrixMultiply(br->uvMatView, br->uvMatProjection);
  if (br->follow) {
    Rectangle sr = br->graph_rect;
    Vector2 middle = { sr.x + sr.width/2, sr.y - sr.height/2 };
    for (size_t i = 0; i < br->groups.len; ++i) {
      points_group_t* pg = &br->groups.arr[i];
      size_t gl = pg->len;
      if (!pg->is_selected || gl == 0) continue;
      br->uvDelta.x += ((middle.x - pg->points[gl - 1].x))/1000.f;
      br->uvDelta.y += ((middle.y - pg->points[gl - 1].y))/1000.f;
    }
    br->uvOffset.x -= br->uvDelta.x;
    br->uvOffset.y -= br->uvDelta.y;
    br->uvDelta.x *= br->recoil;
    br->uvDelta.y *= br->recoil;
  } else {
    br->uvDelta = (Vector2){ 0.f, 0.f };
  }

  if (br->mouse_inside_graph) {
    // TODO: Move this to br_keybindings.c
    // Stuff related to zoom
    {
      float mw = -GetMouseWheelMove();
      Vector2 old = br->mouse_graph_pos;
      bool any = false;
      if (false == help_near_zero(mw)) {
        float mw_scale = (1 + mw/10);
        if (IsKeyDown(KEY_X)) {
          br->uvZoom.x *= mw_scale;
        } else if (IsKeyDown(KEY_Y)) {
          br->uvZoom.y *= mw_scale;
        } else {
          br->uvZoom.x *= mw_scale;
          br->uvZoom.y *= mw_scale;
        }
        any = true;
      }
      if (IsKeyDown(KEY_X) && IsKeyDown(KEY_LEFT_SHIFT)) any = true,   br->uvZoom.x *= 1.1f;
      if (IsKeyDown(KEY_Y) && IsKeyDown(KEY_LEFT_SHIFT)) any = true,   br->uvZoom.y *= 1.1f;
      if (IsKeyDown(KEY_X) && IsKeyDown(KEY_LEFT_CONTROL)) any = true, br->uvZoom.x *= .9f;
      if (IsKeyDown(KEY_Y) && IsKeyDown(KEY_LEFT_CONTROL)) any = true, br->uvZoom.y *= .9f;
      if (any) {
        graph_update_context(br);
        Vector2 now = br->mouse_graph_pos;
        br->uvOffset.x -= now.x - old.x;
        br->uvOffset.y -= now.y - old.y;
      }
    }
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      Vector2 delt = GetMouseDelta();
      //float speed = 1.f;
      if (IsKeyDown(KEY_W)) {
        //Vector3 diff = Vector3Subtract(br->eye, br->target);
      }
      br->uvOffset.x -= br->uvZoom.x*delt.x/br->graph_screen_rect.height;
      br->uvOffset.y += br->uvZoom.y*delt.y/br->graph_screen_rect.height;
    } else br_keybinding_handle_keys(br);
    if (br->jump_around) {
      br->graph_screen_rect.x += 100.f * (float)sin(GetTime());
      br->graph_screen_rect.y += 77.f * (float)cos(GetTime());
      br->graph_screen_rect.width += 130.f * (float)sin(GetTime());
      br->graph_screen_rect.height += 177.f * (float)cos(GetTime());
    }
  }
  update_shader_values(br);
  while (1) {
    q_command comm = q_pop(&br->commands);
    switch (comm.type) {
      case q_command_none:          goto end;
      case q_command_push_point_x:  points_group_push_x(&br->groups, comm.push_point_x.x, comm.push_point_y.group); break;
      case q_command_push_point_y:  points_group_push_y(&br->groups, comm.push_point_y.y, comm.push_point_y.group); break;
      case q_command_push_point_xy: points_group_push_xy(&br->groups, comm.push_point_xy.x, comm.push_point_xy.y, comm.push_point_xy.group); break;
      case q_command_pop:           break; //TODO
      case q_command_clear:         points_group_clear(&br->groups, comm.clear.group); break;
      case q_command_clear_all:     points_groups_deinit(&br->groups); break;
      case q_command_screenshot:    graph_screenshot(br, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_export:        graph_export(br, comm.path_arg.path);     free(comm.path_arg.path); break;
      case q_command_exportcsv:     graph_export_csv(br, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_hide:          points_group_get(&br->groups, comm.hide_show.group)->is_selected = false; break;
      case q_command_show:          points_group_get(&br->groups, comm.hide_show.group)->is_selected = true;  break;
      case q_command_set_name:      points_group_set_name(&br->groups, comm.set_quoted_str.group, comm.set_quoted_str.str);  break;
      case q_command_focus:         graph_focus_visible(br); break;
      default:                      BR_ASSERT(false);
    }
  }
  end: return;
}

BR_API void graph_frame_end(br_plot_t* gv) {
  gv->lines_mesh->draw_calls = 0;
  gv->lines_mesh->points_drawn = 0;
}

void graph_draw_grid(Shader shader, Rectangle screen_rect) {
  BeginScissorMode((int)screen_rect.x, (int)screen_rect.y, (int)screen_rect.width, (int)screen_rect.height);
    BeginShaderMode(shader);
      DrawRectangleRec(screen_rect, RED);
    EndShaderMode();
  EndScissorMode();
}

void graph_export(br_plot_t const* gv, char const * path) {
  FILE* file = fopen(path, "w");
  if (file == NULL) {
    fprintf(stderr, "Failed to open path: `%s`", path);
    return;
  }
  fprintf(file, "--zoomx %f\n", gv->uvZoom.x);
  fprintf(file, "--zoomy %f\n", gv->uvZoom.y);
  fprintf(file, "--offsetx %f\n", gv->uvOffset.x);
  fprintf(file, "--offsety %f\n", gv->uvOffset.y);
  for (size_t i = 0; i < gv->groups.len; ++i) {
    fprintf(file, gv->groups.arr[i].is_selected ? "--show %d\n" : "--hide %d\n", gv->groups.arr[i].group_id);
  }
  points_groups_export(&gv->groups, file);
  fclose(file);
}

void graph_export_csv(br_plot_t const* br, char const * path) {
  FILE* file = fopen(path, "w");
  // TODO: Show user an error message
  if (file == NULL) return;
  points_groups_export_csv(&br->groups, file);
  fclose(file);
}

void graph_update_context(br_plot_t* gv) {
  Vector2 mp = context.mouse_screen_pos = GetMousePosition();
  Vector2 mp_in_graph = { mp.x - gv->graph_screen_rect.x, mp.y - gv->graph_screen_rect.y };
  context.mouse_graph_pos =
  gv->mouse_graph_pos = (Vector2) {
    -(gv->graph_screen_rect.width  - 2.f*mp_in_graph.x)/gv->graph_screen_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x,
     (gv->graph_screen_rect.height - 2.f*mp_in_graph.y)/gv->graph_screen_rect.height*gv->uvZoom.y/2.f + gv->uvOffset.y};
  gv->mouse_inside_graph = CheckCollisionPointRec(context.mouse_screen_pos, gv->graph_screen_rect);
  gv->graph_rect = (Rectangle){-gv->graph_screen_rect.width/gv->graph_screen_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x,
    gv->uvZoom.y/2.f + gv->uvOffset.y,
    gv->graph_screen_rect.width/gv->graph_screen_rect.height*gv->uvZoom.x,
    gv->uvZoom.y};
}

#ifndef RELEASE
static void refresh_shaders_if_dirty(br_plot_t* gv) {
  if (gv->shaders_dirty) {
    gv->shaders_dirty = false;
    for (size_t i = 0; i < sizeof(gv->shaders) / sizeof(br_shader_t); ++i) {
      br_shader_t newS = graph_load_shader(gv->shaders[i].vs_file_name, gv->shaders[i].fs_file_name);
      if (newS.shader.locs != NULL) {
        UnloadShader(gv->shaders[i].shader);
        gv->shaders[i] = newS;
      }
    }
  }
}
#endif

void draw_grid_values(br_plot_t* gv) {
  Rectangle r = gv->graph_rect;
  float font_size = 15.f * context.font_scale;
  char fmt[16];

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
      float y = gv->graph_screen_rect.y + (gv->graph_screen_rect.height / r.height) * (r.y - cur);
      y -= sz.y / 2.f;
      help_draw_text(context.buff, (Vector2){ .x = gv->graph_screen_rect.x - sz.x - 2.f, .y = y }, font_size, RAYWHITE);
    }
  }

  exp =  floorf(log10f(r.width / 2.f));
  if (false == isnan(exp)) {
    float base = powf(10.f, exp);
    if (isnan(base) || isinf(base)) return;
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
      float x = gv->graph_screen_rect.x + (gv->graph_screen_rect.width / r.width) * (cur - r.x);
      x -= sz.x / 2.f;
      if (x - 5.f < x_last_max) continue; // Don't print if it will overlap with the previous text. 5.f is padding.
      x_last_max = x + sz.x;
      help_draw_text(context.buff, (Vector2){ .x = x, .y = gv->graph_screen_rect.y + gv->graph_screen_rect.height }, font_size, RAYWHITE);
    }
  }
}

#ifdef RELEASE
static br_shader_t graph_load_shader(char const* vs, char const* fs) {
  Shader s = LoadShaderFromMemory(vs, fs);
  return (br_shader_t) {
    .uResolution = GetShaderLocation(s, "resolution"),
    .uZoom = GetShaderLocation(s, "zoom"),
    .uOffset = GetShaderLocation(s, "offset"),
    .uScreen = GetShaderLocation(s, "screen"),
    .uTime = GetShaderLocation(s, "time"),
    .uMvp = GetShaderLocation(s, "m_mvp"),
    .uMatView = GetShaderLocation(s, "m_view"),
    .uMatProjection = GetShaderLocation(s, "m_projection"),
    .shader = s
  };
}
#else
static br_shader_t graph_load_shader(char const* vs, char const* fs) {
  Shader s = LoadShader(vs, fs);
  return (br_shader_t) {
    .uResolution = GetShaderLocation(s, "resolution"),
    .uZoom = GetShaderLocation(s, "zoom"),
    .uOffset = GetShaderLocation(s, "offset"),
    .uScreen = GetShaderLocation(s, "screen"),
    .uTime = GetShaderLocation(s, "time"),
    .uMvp = GetShaderLocation(s, "m_mvp"),
    .uMatView = GetShaderLocation(s, "m_view"),
    .uMatProjection = GetShaderLocation(s, "m_projection"),
    .shader = s,
    .vs_file_name = vs,
    .fs_file_name = fs
  };
}
#endif
