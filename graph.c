#include "plotter.h"

#ifdef RELEASE

#ifdef PLATFORM_DESKTOP
#include "shaders.h"
#elif PLATFORM_WEB
#include "shaders_web.h"
#else
#error "Shaders for this platform arn't defined"
#endif

#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"

static void refresh_shaders_if_dirty(graph_values_t* gv);
static void update_resolution(graph_values_t* gv);
static int DrawButton(bool* is_pressed, float x, float y, float font_size, char* buff, const char* str, ...);
static void DrawLeftPanel(graph_values_t* gv, char *buff, float font_scale);
static Rectangle graph_get_rectangle(graph_values_t* gv);
static void graph_update_mouse_position(graph_values_t* gv);

Vector2 graph_mouse_position;

void graph_init(graph_values_t* gv, float width, float height) {
#ifdef RELEASE
  gv->gridShader = LoadShaderFromMemory(NULL, SHADER_GRID_FS);
  gv->linesShader = LoadShaderFromMemory(SHADER_LINE_VS, SHADER_LINE_FS);
#else
  gv->gridShader = LoadShader(NULL, "shaders/grid.fs");
  gv->linesShader = LoadShader("shaders/line.vs", "shaders/line.fs");
#endif
  Color cs[] = { RED, GREEN, BLUE, LIGHTGRAY, PINK, GOLD, VIOLET, DARKPURPLE };
  for (int i = 0; i < 8; ++i) {
    gv->group_colors[i] = cs[i];
  }
  gv->groups_len = 0;
  gv->graph_rect = (Rectangle){ GRAPH_LEFT_PAD, 50, width - GRAPH_LEFT_PAD - 60, height - 110 };
  gv->uvOffset = (Vector2){ 0., 0. };
  gv->uvZoom = (Vector2){ 1., 1. };
  gv->uvScreen = (Vector2){ width, height };
  gv->uvDelta = (Vector2){ 0., 0. };
  for (int i = 0; i < 2; ++i) {
    gv->uResolution[i] = GetShaderLocation(gv->shaders[i], "resolution");
    gv->uZoom[i] = GetShaderLocation(gv->shaders[i], "zoom");
    gv->uOffset[i] = GetShaderLocation(gv->shaders[i], "offset");
    gv->uScreen[i] = GetShaderLocation(gv->shaders[i], "screen");
  }
  gv->uColor = GetShaderLocation(gv->linesShader, "color");
  memset(gv->groups, 0, sizeof(gv->groups));
  smol_mesh_init_temp();
  q_init(&gv->commands);
}

static float signf(float x) {
  return x > 0.f ?  1.f :
         x < 0.f ? -1.f : 0.f;
}

void graph_draw(graph_values_t* gv) {
  char buff[128];
  float font_scale = 1.4;
  update_resolution(gv);
  refresh_shaders_if_dirty(gv);
  Vector2 mp = GetMousePosition();
  graph_update_mouse_position(gv);
  bool is_inside = CheckCollisionPointRec(mp, gv->graph_rect);
  if (gv->follow) {
    Rectangle sr = graph_get_rectangle(gv);
    Vector2 middle = { sr.x + sr.width/2, sr.y - sr.height/2 };
    for (int i = 0; i < gv->groups_len; ++i) {
      points_group_t* pg = &gv->groups[i];
      int gl = pg->len;
      if (!pg->is_selected) continue;
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
      points_group_clear_all(gv->groups, &gv->groups_len);
    }
    if (IsKeyPressed(KEY_T)) {
      points_group_add_test_points(gv->groups, &gv->groups_len, GROUP_CAP);
    }
    if (IsKeyPressed(KEY_F)) {
      gv->follow = !gv->follow;
    }
    if (IsKeyPressed(KEY_D)) {
      gv->debug = (gv->debug + 1) % 5;
    }
  }
  for (int i = 0; i < 2; ++i) {
    SetShaderValue(gv->shaders[i], gv->uResolution[i], &gv->graph_rect, SHADER_UNIFORM_VEC4);
    SetShaderValue(gv->shaders[i], gv->uZoom[i], &gv->uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uOffset[i], &gv->uvOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uScreen[i], &gv->uvScreen, SHADER_UNIFORM_VEC2);
  }
  DrawFPS(0, 0);
  DrawLeftPanel(gv, buff, font_scale);
  BeginShaderMode(gv->gridShader);
    DrawRectangleRec(gv->graph_rect, RED);
  EndShaderMode();
  points_groups_draw(gv->groups, gv->groups_len, gv->linesShader, gv->uColor, gv->group_colors, graph_get_rectangle(gv), gv->debug);
  if (is_inside) {
    float pad = 5.;
    float fs = 10 * font_scale;
    Vector2 s = { 100, fs + 2 * pad};
    sprintf(buff, "(%.1e, %.1e)", graph_mouse_position.x, graph_mouse_position.y);
    s.x = MeasureText(buff, fs) + 2 * pad;
    DrawRectangleV(mp, s, RAYWHITE);
    DrawText(buff, mp.x + pad, mp.y + pad, fs, BLACK);
  }
  while (1) {
    q_command comm = q_pop(&gv->commands);
    switch (comm.type) {
      case q_command_none: goto end;
      case q_command_push_point_y: points_group_push_y(gv->groups, &gv->groups_len, GROUP_CAP, comm.push_point_y.y, comm.push_point_y.group);
                                   break;
      case q_command_push_point_xy: points_group_push_xy(gv->groups, &gv->groups_len, GROUP_CAP, comm.push_point_xy.x, comm.push_point_xy.y, comm.push_point_xy.group);
                                    break;
      case q_command_pop: break; //TODO
      case q_command_clear: points_group_clear(gv->groups, &gv->groups_len, comm.clear.group);
                            break;
      case q_command_clear_all: points_group_clear_all(gv->groups, &gv->groups_len);
                                break;
    }
  }
end: return;
}

static void graph_update_mouse_position(graph_values_t* gv) {
  Vector2 mp = GetMousePosition();
  Vector2 mp_in_graph = { mp.x - gv->graph_rect.x, mp.y - gv->graph_rect.y };
  graph_mouse_position = (Vector2) { 
    -(gv->graph_rect.width  - 2.*mp_in_graph.x)/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x,
     (gv->graph_rect.height - 2.*mp_in_graph.y)/gv->graph_rect.height*gv->uvZoom.y/2. + gv->uvOffset.y };
}

static void refresh_shaders_if_dirty(graph_values_t* gv) {
  if (gv->shaders_dirty) {
    gv->shaders_dirty = false;
    Shader new_line = LoadShader("./shaders/line.vs", "./shaders/line.fs");
    if (new_line.locs != NULL) {
      UnloadShader(gv->linesShader);
      gv->linesShader = new_line;
    }
    Shader new_grid = LoadShader(NULL, "./shaders/grid.fs");
    if (new_grid.locs != NULL) {
      UnloadShader(gv->gridShader);
      gv->gridShader = new_grid;
    }
    for (int i = 0; i < 2; ++i) {
      gv->uResolution[i] = GetShaderLocation(gv->shaders[i], "resolution");
      gv->uZoom[i] = GetShaderLocation(gv->shaders[i], "zoom");
      gv->uOffset[i] = GetShaderLocation(gv->shaders[i], "offset");
      gv->uScreen[i] = GetShaderLocation(gv->shaders[i], "screen");
    }
    gv->uColor = GetShaderLocation(gv->linesShader, "color");
  }
}

static int DrawButton(bool* is_pressed, float x, float y, float font_size, char* buff, const char* str, ...) {
  Vector2 mp = GetMousePosition();
  int c = 0;
  va_list args;
  va_start(args, str);
  vsprintf(buff, str, args);
  va_end(args);
  float pad = 5.;
  Vector2 size = { MeasureText(buff, font_size) +  2 * pad, font_size + 2 * pad };
  Rectangle box = { x, y, size.x, size.y };
  bool is_in = CheckCollisionPointRec(mp, box);
  if (is_in) {
    bool is_p = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    c = is_p ? 2 : 1;
    if (is_p && is_pressed) {
      *is_pressed = !*is_pressed;
    }
  }
  if (is_pressed && *is_pressed) {
    DrawRectangleRec(box, BLUE);
  } else if (is_in) {
    DrawRectangleRec(box, RED);
  }
  DrawText(buff, x + pad, y + pad, font_size, WHITE);
  return c;
}

static Rectangle graph_get_rectangle(graph_values_t* gv) {
  return (Rectangle){-gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x, gv->uvZoom.y/2 + gv->uvOffset.y,
    gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x, gv->uvZoom.y};
}

#define StackPannel(max_height, x_offset, y_offset, y_item_offset, item_height)

static void DrawLeftPanel(graph_values_t* gv, char *buff, float font_scale) {
  Rectangle r = graph_get_rectangle(gv);
  DrawButton(NULL, gv->graph_rect.x - 30, gv->graph_rect.y - 30, font_scale * 10,
      buff, "(%f, %f)", r.x, r.y);
  DrawButton(NULL, gv->graph_rect.x + gv->graph_rect.width - 120, gv->graph_rect.y - 30, font_scale * 10,
      buff, "(%f, %f)", r.x + r.width, r.y);

  DrawButton(NULL, gv->graph_rect.x - 30, gv->graph_rect.y + 20 + gv->graph_rect.height, font_scale * 10,
      buff, "(%f, %f)", r.x, r.y - r.height);
  DrawButton(NULL, gv->graph_rect.x + gv->graph_rect.width - 120, gv->graph_rect.y + 20 + gv->graph_rect.height, font_scale * 10,
      buff, "(%f, %f)", r.x + r.width, r.y - r.height);

  int i = 0;
  DrawButton(&gv->follow, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "Follow");
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "offset: (%f, %f)", gv->uvOffset.x, gv->uvOffset.y);
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "zoom: (%f, %f)", gv->uvZoom.x, gv->uvZoom.y);
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "Line groups: %d/%d", gv->groups_len, GROUP_CAP);
  for(int j = 0; j < gv->groups_len; ++j) {
    int p = DrawButton(&gv->groups[j].is_selected, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "Group #%d: %d/%d; %d", gv->groups[j].group_id, gv->groups[j].len, gv->groups[j].cap, gv->groups[j].qt_expands);
    if (p > 0 && IsKeyPressed(KEY_P)) {
      quad_tree_print_dot(&gv->groups[j].qt);
    }
    if (p > 0 && IsKeyPressed(KEY_B)) {
      quad_tree_t qt = {0};
      quad_tree_balance(&qt, &gv->groups[j].qt, gv->groups[j].points);
      //quad_tree_free(&gv->groups[j].qt); TODO: implement
      memcpy(&gv->groups[j].qt, &qt, sizeof(quad_tree_t));
    }
  }
}

static void update_resolution(graph_values_t* gv) {
  gv->uvScreen.x = GetScreenWidth();
  gv->uvScreen.y = GetScreenHeight();
  int w = gv->uvScreen.x - GRAPH_LEFT_PAD - 60, h = gv->uvScreen.y - 120;
  gv->graph_rect.x = GRAPH_LEFT_PAD;
  gv->graph_rect.y = 60;
  gv->graph_rect.width = w;
  gv->graph_rect.height = h;
}

