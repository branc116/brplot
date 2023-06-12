#include "graph.h"
#include "stdio.h"
#include "stdlib.h"
#include <raylib.h>

pp_ret push_point(graph_values_t* gv, Vector2 v, int group) {
  pp_ret ret;
  if (gv->groups_len == 0) {
    point_group_t g = { 0, POINTS_CAP, 1, group };
    gv->groups[0] = g;
    gv->points[0] = v;
    gv->groups_len++;
    ret.v = &gv->points[0];
    ret.group = &gv->groups[0];
    return ret;
  }
  int max_size = 0;
  point_group_t* max_group = &gv->groups[0];
  for (int i = 0; i < gv->groups_len; ++i) {
    if (gv->groups[i].group_id == group) {
      if (gv->groups[i].len + 1 > gv->groups[i].cap) {
        fprintf(stderr, "Trying to add point to a group thats full");
        exit(-1);
      }
      int index = gv->groups[i].start + gv->groups[i].len++;
      gv->points[index] = v;
      ret.v = &gv->points[index];
      ret.group = &gv->groups[i];
      return ret;
    }
    int size = gv->groups[i].cap;
    if (size > max_size) {
      max_group = &gv->groups[i];
      max_size = size;
    }
  }
  int l = max_group->cap;
  int ns2 = max_group->start + (l - (l / 2));
  point_group_t ng = { ns2, l / 2, 1, group };
  gv->groups[gv->groups_len++] = ng;
  max_group->cap = l - (l / 2);
  gv->points[ns2] = v;
  ret.v = &gv->points[ns2];
  ret.group = &gv->groups[gv->groups_len - 1];
  return ret;
}

void test_points(graph_values_t* gv) {
  for(int i = 0; i < 1025; ++i) {
    int group = i % 8;
    int x = i / 8;
    float y = (float)i / 10;
    Vector2 p = {x, y * y * (1 + group) };
    push_point(gv, p, group);
  }
}

void update_graph_values(graph_values_t* gv) {
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
  }
  int w = GetScreenWidth() - GRAPH_LEFT_PAD - 60, h = GetScreenHeight() - 120;
  gv->graph_rect.x = GRAPH_LEFT_PAD;
  gv->graph_rect.y = 60;
  gv->graph_rect.width = w;
  gv->graph_rect.height = h;
  gv->uvScreen.x = GetScreenWidth();
  gv->uvScreen.y = GetScreenHeight();

  if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
    Vector2 delt = GetMouseDelta();
    gv->uvOffset.x -= gv->uvZoom.x*delt.x/h;
    gv->uvOffset.y += gv->uvZoom.y*delt.y/h;
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
  for (int i = 0; i < 2; ++i) {
    SetShaderValue(gv->shaders[i], gv->uResolution[i], &gv->graph_rect, SHADER_UNIFORM_VEC4);
    SetShaderValue(gv->shaders[i], gv->uZoom[i], &gv->uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uOffset[i], &gv->uvOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uScreen[i], &gv->uvScreen, SHADER_UNIFORM_VEC2);
  }
}

void DrawGraph(graph_values_t* gv) {
  char buff[128];
  int i = 0;
  sprintf(buff, "(%f, %f)", gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2. - gv->uvOffset.x, gv->uvZoom.y/2 + gv->uvOffset.y);
  DrawText(buff, gv->graph_rect.x - 30, gv->graph_rect.y - 30, 10, WHITE);

  sprintf(buff, "offset: (%f, %f)", gv->uvOffset.x, gv->uvOffset.y);
  DrawText(buff, 30, gv->graph_rect.y + 30*(i++), 15, WHITE);

  sprintf(buff, "zoom: (%f, %f)", gv->uvZoom.x, gv->uvZoom.y);
  DrawText(buff, 30, gv->graph_rect.y + 30*(i++), 15, WHITE);

  DrawText("dl(10,20)", gv->graph_rect.x - 30, gv->graph_rect.y + 30 + gv->graph_rect.height, 10, WHITE);
  BeginShaderMode(gv->gridShader);
    DrawRectangleRec(gv->graph_rect, RED);
  EndShaderMode();
  BeginShaderMode(gv->linesShader);
    for (int i = 0; i < gv->groups_len; ++i) {
        DrawLineStrip(&gv->points[gv->groups[i].start], gv->groups[i].len, gv->group_colors[i]);
    }
  EndShaderMode();
}


