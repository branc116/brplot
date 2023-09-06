#include "plotter.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"
#include "default_font.h"

static void refresh_shaders_if_dirty(graph_values_t* gv);
static void update_resolution(graph_values_t* gv);
static void draw_left_panel(graph_values_t* gv, char *buff, float font_scale);
static void draw_grid_values(graph_values_t* gv, char *buff, float font_scale);
static Rectangle graph_get_rectangle(graph_values_t* gv);
static void graph_update_mouse_position(graph_values_t* gv);

Vector2 graph_mouse_position;

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
    .commands = {0},
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
  help_load_default_sdf_font();
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

extern int cur_font_size;

void graph_draw(graph_values_t* gv) {
  char buff[128];
  float font_scale = 1.8f;
  update_resolution(gv);
  refresh_shaders_if_dirty(gv);
  Vector2 mp = GetMousePosition();
  graph_update_mouse_position(gv);
  bool is_inside = CheckCollisionPointRec(mp, gv->graph_rect);
  if (gv->follow) {
    Rectangle sr = graph_get_rectangle(gv);
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
    gv->uvDelta.x *= 0.99f;
    gv->uvDelta.y *= 0.99f;
  } else {
    gv->uvDelta = (Vector2){ 0, 0 };
  }

  if (is_inside) {
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      Vector2 delt = GetMouseDelta();
      gv->uvOffset.x -= gv->uvZoom.x*delt.x/gv->graph_rect.height;
      gv->uvOffset.y += gv->uvZoom.y*delt.y/gv->graph_rect.height;
    }

    float mw = GetMouseWheelMove();
    float mw_scale = (1 + mw/10);
    if (IsKeyDown(KEY_X)) {
      gv->uvZoom.x *= mw_scale;
    } else if (IsKeyDown(KEY_Y)) {
      gv->uvZoom.y *= mw_scale;
    } else {
      gv->uvZoom.x *= mw_scale;
      gv->uvZoom.y *= mw_scale;
    }

    if (IsKeyPressed(KEY_R)) {
      gv->uvZoom.x = gv->uvZoom.y = 1;
      gv->uvOffset.x = gv->uvOffset.y = 0;
    }
    if (IsKeyPressed(KEY_C)) {
      points_groups_deinit(&gv->groups);
    }
    if (IsKeyPressed(KEY_T)) {
      points_group_add_test_points(&gv->groups);
    }
    if (IsKeyPressed(KEY_F)) {
      gv->follow = !gv->follow;
    }
    if (IsKeyPressed(KEY_W)) { 
      cur_font_size++;
      help_load_default_sdf_font();
    }
    if (IsKeyPressed(KEY_S)) { 
      cur_font_size--;
      help_load_default_sdf_font();
    }
  }
  for (int i = 0; i < 3; ++i) {
    SetShaderValue(gv->shaders[i], gv->uResolution[i], &gv->graph_rect, SHADER_UNIFORM_VEC4);
    SetShaderValue(gv->shaders[i], gv->uZoom[i], &gv->uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uOffset[i], &gv->uvOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uScreen[i], &gv->uvScreen, SHADER_UNIFORM_VEC2);
  }
  DrawFPS(0, 0);
  draw_left_panel(gv, buff, font_scale);
  draw_grid_values(gv, buff, font_scale);
  BeginShaderMode(gv->gridShader);
    DrawRectangleRec(gv->graph_rect, RED);
  EndShaderMode();
  // Todo: don't assign this every frame, no need for it. Assign it only when shaders are recompiled.
  gv->lines_mesh->active_shader = gv->linesShader;
  gv->quads_mesh->active_shader = gv->quadShader;
  points_groups_draw(&gv->groups, gv->lines_mesh, gv->quads_mesh, graph_get_rectangle(gv));
  if (is_inside) {
    float pad = 5.f;
    float fs = (10.f * font_scale);
    Vector2 s = { 100.f, fs + 2 * pad};
    sprintf(buff, "(%.1e, %.1e)", graph_mouse_position.x, graph_mouse_position.y);
    s.x = help_measure_text(buff, fs).y + 2.f * (float)pad;
    DrawRectangleV(mp, s, RAYWHITE);
    help_draw_text(buff, (Vector2){mp.x + pad, mp.y + pad}, fs, BLACK);
  }
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

static void graph_update_mouse_position(graph_values_t* gv) {
  Vector2 mp = GetMousePosition();
  Vector2 mp_in_graph = { mp.x - gv->graph_rect.x, mp.y - gv->graph_rect.y };
  graph_mouse_position = (Vector2) {
    -(gv->graph_rect.width  - 2.f*mp_in_graph.x)/gv->graph_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x,
     (gv->graph_rect.height - 2.f *mp_in_graph.y)/gv->graph_rect.height*gv->uvZoom.y/2.f + gv->uvOffset.y };
}

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

static Rectangle graph_get_rectangle(graph_values_t* gv) {
  return (Rectangle){-gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x, gv->uvZoom.y/2.f + gv->uvOffset.y,
    gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x, gv->uvZoom.y};
}

static void draw_grid_values(graph_values_t* gv, char *buff, float font_scale) {
  Rectangle r = graph_get_rectangle(gv);
  float font_size = 15.f * font_scale;
  float exp = floorf(log10f(r.height / 2.f));
  float base = powf(10.f, exp);
  float start = floorf(r.y / base) * base;
  static char fmt_buf[32];
  if (exp >= 0) strcpy(fmt_buf, "%f");
  else sprintf(fmt_buf, "%%.%df", -(int)exp);

  for (float c = start; c > r.y - r.height; c -= base) {
    sprintf(buff, fmt_buf, c);
    help_trim_zeros(buff);
    Vector2 sz = MeasureTextEx(default_font, buff, font_size, 1.f);
    float y = gv->graph_rect.y + (gv->graph_rect.height / r.height) * (r.y - c);
    y -= sz.y / 2.f;
    help_draw_text(buff, (Vector2){ .x = gv->graph_rect.x - sz.x - 2.f, .y = y }, font_size, RAYWHITE);
  }

  base = powf(10.f, floorf(log10f(r.width / 2.f)));
  start = ceilf(r.x / base) * base;
  float x_last_max = -INFINITY;
  for (float c = start; c < r.x + r.width; c += base) {
    sprintf(buff, fmt_buf, c);
    help_trim_zeros(buff);
    Vector2 sz = MeasureTextEx(default_font, buff, font_size, 1.f);
    float x = gv->graph_rect.x + (gv->graph_rect.width / r.width) * (c - r.x);
    x -= sz.x / 2.f;
    if (x - 5.f < x_last_max) continue; // Don't print if it will overlap with the previous text. 5.f is padding.
    x_last_max = x + sz.x;
    help_draw_text(buff, (Vector2){ .x = x, .y = gv->graph_rect.y + gv->graph_rect.height }, font_size, RAYWHITE);
  }
}

static float sp = 0.f;
static void draw_left_panel(graph_values_t* gv, char *buff, float font_scale) {
  ui_stack_buttons_init((Vector2){.x = 30.f, .y = 25.f}, NULL, font_scale * 15, buff);
  ui_stack_buttons_add(&gv->follow, "Follow");
  ui_stack_buttons_add(NULL, "Line draw calls: %d", gv->lines_mesh->draw_calls);
  ui_stack_buttons_add(NULL, "Points drawn: %d", gv->lines_mesh->points_drawn);
  ui_stack_buttons_add(NULL, "Font size: %d", cur_font_size);
  Vector2 new_pos = ui_stack_buttons_end();
  new_pos.y += 50;
  ui_stack_buttons_init(new_pos, &sp, font_scale * 15, buff);
  for(size_t j = 0; j < gv->groups.len; ++j) {
    ui_stack_buttons_add(&gv->groups.arr[j].is_selected, "Group #%d: %d/%d; %ul/%ul/%ul", gv->groups.arr[j].group_id, gv->groups.arr[j].len, gv->groups.arr[j].cap, gv->groups.arr[j].resampling->intervals_count, gv->groups.arr[j].resampling->raw_count, gv->groups.arr[j].resampling->resampling_count);
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

