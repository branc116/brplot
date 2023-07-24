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
#include "default_font.h"

static void refresh_shaders_if_dirty(graph_values_t* gv);
static void update_resolution(graph_values_t* gv);
static int DrawButton(bool* is_pressed, float x, float y, float font_size, char* buff, const char* str, ...);
static void DrawLeftPanel(graph_values_t* gv, char *buff, float font_scale);
static Rectangle graph_get_rectangle(graph_values_t* gv);
static void graph_update_mouse_position(graph_values_t* gv);

static Font default_font;

Vector2 graph_mouse_position;


void graph_init(graph_values_t* gv, float width, float height) {
#ifdef RELEASE
  gv->gridShader = LoadShaderFromMemory(NULL, SHADER_GRID_FS);
  gv->linesShader = LoadShaderFromMemory(SHADER_LINE_VS, SHADER_LINE_FS);
  gv->quadShader = LoadShaderFromMemory(SHADER_QUAD_VS, SHADER_QUAD_FS);
#else
  gv->gridShader = LoadShader(NULL, "shaders/grid.fs");
  gv->linesShader = LoadShader("shaders/line.vs", "shaders/line.fs");
  gv->quadShader = LoadShader("shaders/quad.vs", "shaders/quad.fs");
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
  for (int i = 0; i < 3; ++i) {
    gv->uResolution[i] = GetShaderLocation(gv->shaders[i], "resolution");
    gv->uZoom[i] = GetShaderLocation(gv->shaders[i], "zoom");
    gv->uOffset[i] = GetShaderLocation(gv->shaders[i], "offset");
    gv->uScreen[i] = GetShaderLocation(gv->shaders[i], "screen");
  }
  memset(gv->groups, 0, sizeof(gv->groups));
  q_init(&gv->commands);
  gv->lines_mesh = smol_mesh_malloc(PTOM_COUNT, gv->linesShader);
  gv->quads_mesh = smol_mesh_malloc(PTOM_COUNT, gv->quadShader);
  default_font = gv->font = LoadFontFromMemory(".ttf", default_font_data,
                        sizeof(default_font_data), 64, NULL,
                        default_font_glyphs);
}

static void help_draw_text(const char *text, Vector2 pos, float fontSize, Color color) {
  float defaultFontSize = 10.f;
  if (fontSize < defaultFontSize) fontSize = defaultFontSize;
  float spacing = fontSize/defaultFontSize;

  DrawTextEx(default_font, text, (Vector2){roundf(pos.x), roundf(pos.y)}, floorf(fontSize), 1.0f, color);
}

static float help_measure_text(const char* txt, float font_size) {
  Vector2 textSize = { 0.0f, 0.0f };

  float defaultFontSize = 10;   // Default Font chars height in pixel
  if (font_size < defaultFontSize) font_size = defaultFontSize;

  textSize = MeasureTextEx(default_font, txt, floorf(font_size), 1.0f);
 
  return textSize.x;
}

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
    for (size_t i = 0; i < gv->groups_len; ++i) {
      points_group_t* pg = &gv->groups[i];
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
  for (int i = 0; i < 3; ++i) {
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
  gv->lines_mesh->active_shader = gv->linesShader;
  gv->quads_mesh->active_shader = gv->quadShader;
  points_groups_draw(gv->groups, gv->groups_len, gv->lines_mesh, gv->quads_mesh, gv->group_colors, graph_get_rectangle(gv), gv->debug);
  if (is_inside) {
    float pad = 5.f;
    float fs = (10.f * font_scale);
    Vector2 s = { 100.f, fs + 2 * pad};
    sprintf(buff, "(%.1e, %.1e)", graph_mouse_position.x, graph_mouse_position.y);
    s.x = help_measure_text(buff, fs) + 2.f * (float)pad;
    DrawRectangleV(mp, s, RAYWHITE);
    help_draw_text(buff, (Vector2){mp.x + pad, mp.y + pad}, fs, BLACK);
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
    -(gv->graph_rect.width  - 2.f*mp_in_graph.x)/gv->graph_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x,
     (gv->graph_rect.height - 2.f *mp_in_graph.y)/gv->graph_rect.height*gv->uvZoom.y/2.f + gv->uvOffset.y };
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
    Shader new_quad = LoadShader("./shaders/quad.vs", "./shaders/quad.fs");
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

static int DrawButton(bool* is_pressed, float x, float y, float font_size, char* buff, const char* str, ...) {
  Vector2 mp = GetMousePosition();
  int c = 0;
  va_list args;
  va_start(args, str);
  vsprintf(buff, str, args);
  va_end(args);
  float pad = 5.f;
  Vector2 size = { help_measure_text(buff, font_size) + 2.f * pad, font_size + 2.f * pad };
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
  help_draw_text(buff, (Vector2){x + pad, y + pad}, font_size, WHITE);
  return c;
}

static Rectangle graph_get_rectangle(graph_values_t* gv) {
  return (Rectangle){-gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2.f + gv->uvOffset.x, gv->uvZoom.y/2.f + gv->uvOffset.y,
    gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x, gv->uvZoom.y};
}

#define StackPannel(max_height, x_offset, y_offset, y_item_offset, item_height)

static void DrawLeftPanel(graph_values_t* gv, char *buff, float font_scale) {
  Rectangle r = graph_get_rectangle(gv);
  DrawButton(NULL, gv->graph_rect.x - 30.f, gv->graph_rect.y - 30.f, font_scale * 10.f,
      buff, "(%f, %f)", r.x, r.y);
  DrawButton(NULL, gv->graph_rect.x + gv->graph_rect.width - 120.f, gv->graph_rect.y - 30.f, font_scale * 10.f,
      buff, "(%f, %f)", r.x + r.width, r.y);

  DrawButton(NULL, gv->graph_rect.x - 30.f, gv->graph_rect.y + 20.f + gv->graph_rect.height, font_scale * 10.f,
      buff, "(%f, %f)", r.x, r.y - r.height);
  DrawButton(NULL, gv->graph_rect.x + gv->graph_rect.width - 120.f, gv->graph_rect.y + 20.f + gv->graph_rect.height, font_scale * 10.f,
      buff, "(%f, %f)", r.x + r.width, r.y - r.height);

  int i = 0;
  DrawButton(&gv->follow, 30.f, gv->graph_rect.y + (float)(33*(i++)), font_scale * 15, buff, "Follow");
  DrawButton(NULL, 30.f, gv->graph_rect.y + (float)(33*(i++)), font_scale * 15, buff, "offset: (%f, %f)", gv->uvOffset.x, gv->uvOffset.y);
  DrawButton(NULL, 30.f, gv->graph_rect.y + (float)(33*(i++)), font_scale * 15, buff, "zoom: (%f, %f)", gv->uvZoom.x, gv->uvZoom.y);
  DrawButton(NULL, 30.f, gv->graph_rect.y + (float)(33*(i++)), font_scale * 15, buff, "Line groups: %d/%d", gv->groups_len, GROUP_CAP);
  DrawButton(NULL, 30.f, gv->graph_rect.y + (float)(33*(i++)), font_scale * 15, buff, "Line draw calls: %d", gv->lines_mesh->draw_calls);
  DrawButton(NULL, 30.f, gv->graph_rect.y + (float)(33*(i++)), font_scale * 15, buff, "Points drawn: %d", gv->lines_mesh->points_drawn);
  for(size_t j = 0; j < gv->groups_len; ++j) {
    int p = DrawButton(&gv->groups[j].is_selected, 30.f, gv->graph_rect.y + (float)(33 * (i++)), font_scale * 15.f, buff, "Group #%d: %d/%d; %d", gv->groups[j].group_id, gv->groups[j].len, gv->groups[j].cap, gv->groups[j].qt_expands);
    if (p > 0 && IsKeyPressed(KEY_P)) {
      quad_tree_print_dot(gv->groups[j].qt);
    }
  }
  gv->lines_mesh->draw_calls = 0;
  gv->lines_mesh->points_drawn = 0;
}

static void update_resolution(graph_values_t* gv) {
  gv->uvScreen.x = (float)GetScreenWidth();
  gv->uvScreen.y = (float)GetScreenHeight();
  float w = gv->uvScreen.x - (float)GRAPH_LEFT_PAD - 60.f, h = gv->uvScreen.y - 120.f;
  gv->graph_rect.x = (float)GRAPH_LEFT_PAD;
  gv->graph_rect.y = 60.f;
  gv->graph_rect.width = w;
  gv->graph_rect.height = h;
}

