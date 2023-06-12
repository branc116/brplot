#pragma once
#include "raylib.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int start, cap, len;
  int group_id;
} point_group_t;

typedef struct {
  union {
    Shader shaders[2];
    struct {
      Shader gridShader, linesShader;
    };
  };
  int uResolution[2];
  int uZoom[2];
  int uOffset[2];
  int uScreen[2];

  Rectangle graph_rect;
  Vector2 uvZoom;
  Vector2 uvOffset;
  Vector2 uvScreen;

#define POINTS_CAP 1024
  Vector2 points[POINTS_CAP];

#define GROUP_CAP 8
  point_group_t groups[GROUP_CAP];
  int groups_len;
  Color group_colors[GROUP_CAP];

  bool shaders_dirty;
} graph_values_t;


typedef struct {Vector2* v; point_group_t* group;} pp_ret;

#define GRAPH_LEFT_PAD 400
#define init_graph_values(name, w, h) graph_values_t name = { \
  .gridShader = LoadShader(NULL, "shaders/grid.fs"), \
  .linesShader = LoadShader("shaders/line.vs", "shaders/line.fs"), \
  .group_colors = { RED, GREEN, BLUE, LIGHTGRAY, PINK, GOLD, VIOLET, DARKPURPLE }, \
  .groups_len = 0, \
  .graph_rect = { GRAPH_LEFT_PAD, 50, w - GRAPH_LEFT_PAD - 60, h - 110 }, \
  .uvOffset = { 0., 0. }, \
  .uvZoom = { 1., 1. }, \
  .uvScreen = { w, h } \
}; do { \
    for (int i = 0; i < 2; ++i) { \
      gv.uResolution[i] = GetShaderLocation(name.shaders[i], "resolution"); \
      gv.uZoom[i] = GetShaderLocation(name.shaders[i], "zoom"); \
      gv.uOffset[i] = GetShaderLocation(name.shaders[i], "offset"); \
      gv.uScreen[i] = GetShaderLocation(name.shaders[i], "screen"); \
    } \
    memset(gv.points, 0, sizeof(gv.points)); \
    memset(gv.groups, 0, sizeof(gv.groups)); \
  } while(0)

pp_ret push_point(graph_values_t* gv, Vector2 v, int group);
void test_points(graph_values_t* gv);
void update_graph_values(graph_values_t* gv);
void DrawGraph(graph_values_t* gv);

#ifdef __cplusplus
}
#endif
