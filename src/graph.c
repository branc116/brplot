#include "plotter.h"

#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"

#ifndef RELEASE
static void refresh_shaders_if_dirty(graph_values_t* gv);
#endif

void emscripten_run_script(const char* script);
static void update_resolution(graph_values_t* gv);
static void draw_left_panel(graph_values_t* gv);
static void draw_grid_values(graph_values_t* gv);
static void graph_update_context(graph_values_t* gv);
static void graph_draw_grid(Shader shader, Rectangle screen_rect);

void graph_init(graph_values_t* gv, float width, float height) {
  *gv = (graph_values_t){
#ifdef RELEASE
    .gridShader = LoadShaderFromMemory(NULL, SHADER_GRID_FS),
    .linesShader = LoadShaderFromMemory(SHADER_LINE_VS, SHADER_LINE_FS),
    .quadShader = LoadShaderFromMemory(SHADER_QUAD_VS, SHADER_QUAD_FS),
#else
    .gridShader = LoadShader(NULL, "src/desktop/shaders/grid.fs"),
    .linesShader = LoadShader("src/desktop/shaders/line.vs", "src/desktop/shaders/line.fs"),
    .quadShader = LoadShader("src/desktop/shaders/quad.vs", "src/desktop/shaders/quad.fs"),
#endif
    .uvOffset = { 0., 0. },
    .uvZoom = { 1., 1. },
    .uvScreen = { width, height },
    .uvDelta = { 0., 0. },
    .groups = {0},
    .graph_rect = { GRAPH_LEFT_PAD, 50, width - GRAPH_LEFT_PAD - 60, height - 110 },
    .lines_mesh = NULL,
    .quads_mesh = NULL,
    .follow = false,
    .shaders_dirty = false,
    .commands = {0}
  };
  for (int i = 0; i < 3; ++i) {
    gv->uResolution[i] = GetShaderLocation(gv->shaders[i], "resolution");
    gv->uZoom[i] = GetShaderLocation(gv->shaders[i], "zoom");
    gv->uOffset[i] = GetShaderLocation(gv->shaders[i], "offset");
    gv->uScreen[i] = GetShaderLocation(gv->shaders[i], "screen");
  }
  gv->lines_mesh = smol_mesh_malloc(PTOM_COUNT, gv->linesShader);
  gv->quads_mesh = smol_mesh_malloc(PTOM_COUNT, gv->quadShader);
  q_init(&gv->commands);
  help_load_default_font();

  context.font_scale = 1.8f;
  context.mouse_inside_graph = false;
	context.recoil = 0.85f;
  memset(context.buff, 0, sizeof(context.buff));
}

void graph_free(graph_values_t* gv) {
  for (size_t i = 0; i < sizeof(gv->shaders) / sizeof(Shader); ++i) {
    UnloadShader(gv->shaders[i]);
  }
  smol_mesh_free(gv->lines_mesh);
  smol_mesh_free(gv->quads_mesh);
  for (size_t i = 0; i < gv->groups.len; ++i) {
    points_groups_deinit(&gv->groups);
  }
  free(gv->commands.commands);
}
static void update_shader_values(graph_values_t* gv) {
  for (int i = 0; i < 3; ++i) {
    SetShaderValue(gv->shaders[i], gv->uResolution[i], &gv->graph_rect, SHADER_UNIFORM_VEC4);
    SetShaderValue(gv->shaders[i], gv->uZoom[i], &gv->uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uOffset[i], &gv->uvOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uScreen[i], &gv->uvScreen, SHADER_UNIFORM_VEC2);
  }
  // Todo: don't assign this every frame, no need for it. Assign it only when shaders are recompiled.
  gv->lines_mesh->active_shader = gv->linesShader;
  gv->quads_mesh->active_shader = gv->quadShader;
}

static void update_variables(graph_values_t* gv) {
#ifndef RELEASE
  refresh_shaders_if_dirty(gv);
#endif
  graph_update_context(gv);
  if (gv->follow) {
    Rectangle sr = context.graph_rect;
    Vector2 middle = { sr.x + sr.width/2, sr.y - sr.height/2 };
    for (size_t i = 0; i < gv->groups.len; ++i) {
      points_group_t* pg = &gv->groups.arr[i];
      size_t gl = pg->len;
      if (!pg->is_selected || gl == 0) continue;
      gv->uvDelta.x += ((middle.x - pg->points[gl - 1].x))/1000;
      gv->uvDelta.y += ((middle.y - pg->points[gl - 1].y))/1000;
    }
    gv->uvOffset.x -= gv->uvDelta.x;
    gv->uvOffset.y -= gv->uvDelta.y;
    gv->uvDelta.x *= context.recoil;
    gv->uvDelta.y *= context.recoil;
  } else {
    gv->uvDelta = (Vector2){ 0, 0 };
  }

  if (context.mouse_inside_graph) {
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      Vector2 delt = GetMouseDelta();
      gv->uvOffset.x -= gv->uvZoom.x*delt.x/gv->graph_rect.width;
      gv->uvOffset.y += gv->uvZoom.y*delt.y/gv->graph_rect.height;
    }

    float mw = -GetMouseWheelMove();
    float mw_scale = (1 + mw/10);
    if (IsKeyDown(KEY_X)) {
      gv->uvZoom.x *= mw_scale;
    } else if (IsKeyDown(KEY_Y)) {
      gv->uvZoom.y *= mw_scale;      
    } else {
      gv->uvZoom.x *= mw_scale;
      gv->uvZoom.y *= mw_scale;
      float normalized_delta_x = ((float)context.mouse_screen_pos.x - (gv->graph_rect.x + gv->graph_rect.width / 2)) / gv->graph_rect.width;
      float normalized_delta_y = -((float)context.mouse_screen_pos.y - (gv->graph_rect.y + gv->graph_rect.height / 2)) / gv->graph_rect.height;

      float graph_mouse_x = (context.mouse_graph_pos.x - (gv->graph_rect.x + gv->graph_rect.width / 2)) / gv->graph_rect.width;
      float graph_mouse_y = (context.mouse_graph_pos.y - (gv->graph_rect.y + gv->graph_rect.height/ 2)) / gv->graph_rect.height;
      if(mw_scale != 1) {
        // gv->uvOffset.x = (normalized_delta_x - graph_mouse_x) * gv->uvZoom.x;
        // gv->uvOffset.y = (normalized_delta_y - graph_mouse_y) * gv->uvZoom.y;
      }
    }
    if (IsKeyPressed(KEY_R)) {
      if (!IsKeyDown(KEY_LEFT_CONTROL)) gv->uvZoom.x = gv->uvZoom.y = 1;
      if (!IsKeyDown(KEY_LEFT_SHIFT)) gv->uvOffset.x = gv->uvOffset.y = 0;
    }
    if (IsKeyPressed(KEY_C)) {
      if (IsKeyDown(KEY_LEFT_SHIFT)) points_groups_deinit(&gv->groups);
      else                           points_groups_empty(&gv->groups);
    }
    if (IsKeyPressed(KEY_H)) {
      if (IsKeyDown(KEY_LEFT_SHIFT)) {
        for (size_t i = 0; i < gv->groups.len; ++i)
          gv->groups.arr[i].is_selected = !gv->groups.arr[i].is_selected;
      } else {
        for (size_t i = 0; i < gv->groups.len; ++i)
          gv->groups.arr[i].is_selected = false;
      }
    }
    if (IsKeyPressed(KEY_T)) {
      points_groups_add_test_points(&gv->groups);
    }
    if (IsKeyPressed(KEY_F)) {
      gv->follow = !gv->follow;
    }
    if (IsKeyPressed(KEY_D)) {
      context.debug_bounds = !context.debug_bounds;
    }
    if (IsKeyDown(KEY_X) && IsKeyDown(KEY_LEFT_SHIFT)) gv->uvZoom.x *= 1.1f;
    if (IsKeyDown(KEY_Y) && IsKeyDown(KEY_LEFT_SHIFT)) gv->uvZoom.y *= 1.1f;
    if (IsKeyDown(KEY_X) && IsKeyDown(KEY_LEFT_CONTROL)) gv->uvZoom.x *= .9f;
    if (IsKeyDown(KEY_Y) && IsKeyDown(KEY_LEFT_CONTROL)) gv->uvZoom.y *= .9f;
    if (IsKeyDown(KEY_J)) context.recoil -= 0.001f;
    if (IsKeyDown(KEY_K)) context.recoil += 0.001f;
    if (IsKeyPressed(KEY_S)) {
      graph_screenshot(gv, "test.png");
    }
    if (gv->jump_around) {
      gv->graph_rect.x += 100.f * (float)sin(GetTime());
      gv->graph_rect.y += 77.f * (float)cos(GetTime());
      gv->graph_rect.width += 130.f * (float)sin(GetTime());
      gv->graph_rect.height += 177.f * (float)cos(GetTime());
    }
  }
  update_shader_values(gv);
  while (1) {
    q_command comm = q_pop(&gv->commands);
    switch (comm.type) {
      case q_command_none: goto end;
      case q_command_push_point_y: points_group_push_y(&gv->groups, comm.push_point_y.y, comm.push_point_y.group);
                                   break;
      case q_command_push_point_xy: points_group_push_xy(&gv->groups, comm.push_point_xy.x, comm.push_point_xy.y, comm.push_point_xy.group);
                                    break;
      case q_command_pop: break; //TODO
      case q_command_clear: points_group_clear(&gv->groups, comm.clear.group);
                            break;
      case q_command_clear_all: points_groups_deinit(&gv->groups);
                                break;
      default: assert(false);
    }
  }
end: return;
}

void graph_draw(graph_values_t* gv) {
  update_resolution(gv);
  update_variables(gv);
  help_draw_fps(0, 0);
  draw_left_panel(gv);
  draw_grid_values(gv);
  graph_draw_grid(gv->gridShader, gv->graph_rect);
  points_groups_draw(&gv->groups, gv->lines_mesh, gv->quads_mesh, context.graph_rect);
}

static void graph_draw_grid(Shader shader, Rectangle screen_rect) {
  BeginScissorMode(screen_rect.x, screen_rect.y, screen_rect.width, screen_rect.height);
    BeginShaderMode(shader);
      DrawRectangleRec(screen_rect, RED);
    EndShaderMode();
  EndScissorMode();
}

void graph_screenshot(graph_values_t* gv, char const * path) {
  float left_pad = 80.f;
  float bottom_pad = 80.f;
  Vector2 is = {1280, 720};
  RenderTexture2D target = LoadRenderTexture((int)is.x, (int)is.y); // TODO: make this values user defined.
  //Rectangle old_gr = gv->graph_rect;
  gv->graph_rect = (Rectangle){left_pad, 0.f, is.x - left_pad, is.y - bottom_pad};
  //Vector2 old_sc = gv->uvScreen;
  gv->uvScreen = (Vector2){is.x, is.y};
  update_shader_values(gv);
  BeginTextureMode(target);
    graph_draw_grid(gv->gridShader, gv->graph_rect);
    points_groups_draw(&gv->groups, gv->lines_mesh, gv->quads_mesh, context.graph_rect);
    draw_grid_values(gv);
  EndTextureMode();
  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  ExportImage(img, path);
  UnloadImage(img);
  UnloadRenderTexture(target);
  //gv->uvScreen = old_sc;
  //gv->graph_rect = old_gr;

#if defined(PLATFORM_WEB)
  // Download file from MEMFS (emscripten memory filesystem)
  // saveFileFromMEMFSToDisk() function is defined in raylib/src/shell.html
  emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", GetFileName(path), GetFileName(path)));
#endif
}

static void graph_update_context(graph_values_t* gv) {
  Vector2 mp = context.mouse_screen_pos = GetMousePosition();
  Vector2 mp_in_graph = { mp.x - gv->graph_rect.x, mp.y - gv->graph_rect.y };
  context.mouse_graph_pos = (Vector2) {
    -(gv->graph_rect.width  - 2.f*mp_in_graph.x)/gv->graph_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x,
     (gv->graph_rect.height - 2.f *mp_in_graph.y)/gv->graph_rect.height*gv->uvZoom.y/2.f + gv->uvOffset.y };
  context.mouse_inside_graph = CheckCollisionPointRec(context.mouse_screen_pos, gv->graph_rect);
  context.graph_rect = (Rectangle){-gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x,
    gv->uvZoom.y/2.f + gv->uvOffset.y,
    gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x,
    gv->uvZoom.y};
}

#ifndef RELEASE
static void refresh_shaders_if_dirty(graph_values_t* gv) {
  if (gv->shaders_dirty) {
    gv->shaders_dirty = false;
    Shader new_line = LoadShader("./src/desktop/shaders/line.vs", "./src/desktop/shaders/line.fs");
    if (new_line.locs != NULL) {
      UnloadShader(gv->linesShader);
      gv->linesShader = new_line;
    }
    Shader new_grid = LoadShader(NULL, "./src/desktop/shaders/grid.fs");
    if (new_grid.locs != NULL) {
      UnloadShader(gv->gridShader);
      gv->gridShader = new_grid;
    }
    Shader new_quad = LoadShader("./src/desktop/shaders/quad.vs", "./src/desktop/shaders/quad.fs");
    if (new_quad.locs != NULL) {
      UnloadShader(gv->quadShader);
      gv->quadShader = new_quad;
    }
    for (int i = 0; i < 3; ++i) {
      gv->uResolution[i] = GetShaderLocation(gv->shaders[i], "resolution");
      gv->uZoom[i] = GetShaderLocation(gv->shaders[i], "zoom");
      gv->uOffset[i] = GetShaderLocation(gv->shaders[i], "offset");
      gv->uScreen[i] = GetShaderLocation(gv->shaders[i], "screen");
    }
  }
}
#endif

static void draw_grid_values(graph_values_t* gv) {
  Rectangle r = context.graph_rect;
  float font_size = 15.f * context.font_scale;
  float exp = floorf(log10f(r.height / 2.f));
  float base = powf(10.f, exp);
  float start = floorf(r.y / base) * base;
  char fmt[16];
  if (exp >= 0) strcpy(fmt, "%f");
  else sprintf(fmt, "%%.%df", -(int)exp);

  for (float c = start; c > r.y - r.height; c -= base) {
    sprintf(context.buff, fmt, c);
    help_trim_zeros(context.buff);
    Vector2 sz = help_measure_text(context.buff, font_size);
    float y = gv->graph_rect.y + (gv->graph_rect.height / r.height) * (r.y - c);
    y -= sz.y / 2.f;
    help_draw_text(context.buff, (Vector2){ .x = gv->graph_rect.x - sz.x - 2.f, .y = y }, font_size, RAYWHITE);
  }

  base = powf(10.f, floorf(log10f(r.width / 2.f)));
  start = ceilf(r.x / base) * base;
  float x_last_max = -INFINITY;
  for (float c = start; c < r.x + r.width; c += base) {
    sprintf(context.buff, fmt, c);
    help_trim_zeros(context.buff);
    Vector2 sz = help_measure_text(context.buff, font_size);
    float x = gv->graph_rect.x + (gv->graph_rect.width / r.width) * (c - r.x);
    x -= sz.x / 2.f;
    if (x - 5.f < x_last_max) continue; // Don't print if it will overlap with the previous text. 5.f is padding.
    x_last_max = x + sz.x;
    help_draw_text(context.buff, (Vector2){ .x = x, .y = gv->graph_rect.y + gv->graph_rect.height }, font_size, RAYWHITE);
  }
}

static float sp = 0.f;
static void draw_left_panel(graph_values_t* gv) {
  ui_stack_buttons_init((Vector2){.x = 30.f, .y = 25.f}, NULL, context.font_scale * 15);
  ui_stack_buttons_add(&gv->follow, "Follow");
  ui_stack_buttons_add(NULL, "graphMousePos: (%.2f, %.2f)", context.mouse_graph_pos.x, context.mouse_graph_pos.y);
  ui_stack_buttons_add(NULL, "screenMousePos: (%.1f, %.1f)", context.mouse_screen_pos.x, context.mouse_screen_pos.y);
  ui_stack_buttons_add(NULL, "offset: (%.2f, %.2f)", gv->uvOffset.x, gv->uvOffset.y);
  ui_stack_buttons_add(NULL, "zoom: (%.2f, %.2f)", gv->uvZoom.x, gv->uvZoom.y);
  float screen_rect_x = (context.mouse_screen_pos.x - (gv->graph_rect.x + gv->graph_rect.width/2))/gv->graph_rect.width * gv->uvZoom.x;
  float screen_rect_y = -(context.mouse_screen_pos.y - (gv->graph_rect.y + gv->graph_rect.height/2))/gv->graph_rect.height * gv->uvZoom.y;
  ui_stack_buttons_add(NULL, "rectPos: (%.2f, %.2f)", screen_rect_x, screen_rect_y);
  float graph_rect_x = (context.mouse_graph_pos.x - (gv->graph_rect.x + gv->graph_rect.width / 2)) / gv->graph_rect.width;
  float graph_rect_y = (context.mouse_graph_pos.y - (gv->graph_rect.y + gv->graph_rect.height/ 2)) / gv->graph_rect.height;
  ui_stack_buttons_add(NULL, "graphRectPos: (%.2f, %.2f)", graph_rect_x, graph_rect_y);
  if (context.debug_bounds) {
    ui_stack_buttons_add(&context.debug_bounds, "Debug view");
    ui_stack_buttons_add(&gv->jump_around, "Jump Around");
    ui_stack_buttons_add(NULL, "Line draw calls: %d", gv->lines_mesh->draw_calls);
    ui_stack_buttons_add(NULL, "Points drawn: %d", gv->lines_mesh->points_drawn);
    ui_stack_buttons_add(NULL, "Recoil: %f", context.recoil);
    if (2 == ui_stack_buttons_add(NULL, "Add test points")) {
      points_groups_add_test_points(&gv->groups);
    }
  }
  Vector2 new_pos = ui_stack_buttons_end();
  new_pos.y += 50;
  ui_stack_buttons_init(new_pos, &sp, context.font_scale * 15);
  for(size_t j = 0; j < gv->groups.len; ++j) {
    int res = 0;
    if (context.debug_bounds) {
      res = ui_stack_buttons_add(&gv->groups.arr[j].is_selected, "Line #%d: %d; %u/%u/%u",
        gv->groups.arr[j].group_id, gv->groups.arr[j].len,
        gv->groups.arr[j].resampling->intervals_count, gv->groups.arr[j].resampling->raw_count, gv->groups.arr[j].resampling->resampling_count);
    } else {
      res = ui_stack_buttons_add(&gv->groups.arr[j].is_selected, "Line #%d: %d", gv->groups.arr[j].group_id, gv->groups.arr[j].len);
    }
    if (res > 0) {
      if (IsKeyPressed(KEY_C)) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) points_group_clear(&gv->groups, gv->groups.arr[j].group_id);
        else                              points_group_empty(&gv->groups.arr[j]);
      }
    }
  }
  ui_stack_buttons_end();
  gv->lines_mesh->draw_calls = 0;
  gv->lines_mesh->points_drawn = 0;
}

static void update_resolution(graph_values_t* gv) {
  gv->uvScreen.x = (float)GetScreenWidth();
  gv->uvScreen.y = (float)GetScreenHeight();
  float w = gv->uvScreen.x - (float)GRAPH_LEFT_PAD - 20.f, h = gv->uvScreen.y - 50.f;
  gv->graph_rect.x = (float)GRAPH_LEFT_PAD;
  gv->graph_rect.y = 25.f;
  gv->graph_rect.width = w;
  gv->graph_rect.height = h;
}

