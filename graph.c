#include "plotter.h"
#ifdef RELEASE
#include "shaders.h"
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
  for (int i = 0; i < 2; ++i) {
    gv->uResolution[i] = GetShaderLocation(gv->shaders[i], "resolution");
    gv->uZoom[i] = GetShaderLocation(gv->shaders[i], "zoom");
    gv->uOffset[i] = GetShaderLocation(gv->shaders[i], "offset");
    gv->uScreen[i] = GetShaderLocation(gv->shaders[i], "screen");
  }
  gv->uColor = GetShaderLocation(gv->linesShader, "color");
  memset(gv->groups, 0, sizeof(gv->groups));
  smol_mesh_init_temp();
}

void graph_draw(graph_values_t* gv) {
  char buff[128];
  float font_scale = 1.4;
  update_resolution(gv);
  refresh_shaders_if_dirty(gv);
  Vector2 mp = GetMousePosition();
  bool is_inside = CheckCollisionPointRec(mp, gv->graph_rect);

  if (is_inside) {
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      Vector2 delt = GetMouseDelta();
      gv->uvOffset.x -= gv->uvZoom.x*delt.x/gv->graph_rect.height;
      gv->uvOffset.y += gv->uvZoom.y*delt.y/gv->graph_rect.height;
    }

    float mw = GetMouseWheelMove();
    if (IsKeyDown(KEY_X)) {
      gv->uvZoom.x *= (1 + mw/10);
    } else if (IsKeyDown(KEY_Y)) {
      gv->uvZoom.y *= (1 + mw/10);
    } else {
      gv->uvZoom.x *= (1 + mw/10);
      gv->uvZoom.y *= (1 + mw/10);
    }

    if (IsKeyPressed(KEY_R)) {
      gv->uvZoom.x = gv->uvZoom.y = 1;
      gv->uvOffset.x = gv->uvOffset.y = 0;
    }
    if (IsKeyPressed(KEY_C)) {
      for (int i = 0; i < gv->groups_len; ++i) {
        points_group_t* g = &gv->groups[i];
        for (int j = 0; j < g->smol_meshes_len; ++j) {
          smol_mesh_unload(&g->meshes[j]);
        }
        memset(gv->groups, 0, sizeof(gv->groups));
      }
      gv->groups_len = 0;
      gv->groups_need_freeing = true;
    }
    if (IsKeyPressed(KEY_T)) {
      for (int i = 0; i < 100; ++i) {
        points_group_add_test_points(gv->groups, &gv->groups_len, GROUP_CAP);
      }
    }
  }
  DrawLeftPanel(gv, buff, font_scale);
  for (int i = 0; i < 2; ++i) {
    SetShaderValue(gv->shaders[i], gv->uResolution[i], &gv->graph_rect, SHADER_UNIFORM_VEC4);
    SetShaderValue(gv->shaders[i], gv->uZoom[i], &gv->uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uOffset[i], &gv->uvOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uScreen[i], &gv->uvScreen, SHADER_UNIFORM_VEC2);
  }
  DrawFPS(0, 0);
  BeginShaderMode(gv->gridShader);
    DrawRectangleRec(gv->graph_rect, RED);
  EndShaderMode();
  points_groups_draw(gv->groups, gv->groups_len, gv->linesShader, gv->uColor, gv->group_colors, graph_get_rectangle(gv));
  if (is_inside) {
    float pad = 5.;
    float fs = 10 * font_scale;
    Vector2 s = { 100, fs + 2 * pad};
    Vector2 mp_in_graph = { mp.x - gv->graph_rect.x, mp.y - gv->graph_rect.y };
    sprintf(buff, "(%.1e, %.1e)", -(gv->graph_rect.width  - 2.*mp_in_graph.x)/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x,
                                   (gv->graph_rect.height - 2.*mp_in_graph.y)/gv->graph_rect.height*gv->uvZoom.y/2. + gv->uvOffset.y);
    s.x = MeasureText(buff, fs) + 2 * pad;
    DrawRectangleV(mp, s, RAYWHITE);
    DrawText(buff, mp.x + pad, mp.y + pad, fs, BLACK);
  }
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
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "offset: (%f, %f)", gv->uvOffset.x, gv->uvOffset.y);
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "zoom: (%f, %f)", gv->uvZoom.x, gv->uvZoom.y);
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "Line groups: %d/%d", gv->groups_len, GROUP_CAP);
  for(int j = 0; j < gv->groups_len; ++j) {
    DrawButton(&gv->groups[j].is_selected, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "Group #%d: %d/%d", gv->groups[j].group_id, gv->groups[j].len, gv->groups[j].cap);
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

