#pragma once
#include "raylib.h"
#include <stdbool.h>
#include "smol_mesh.h"
#include "points_group.h"

#ifdef __cplusplus
extern "C" {
#endif

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
  int uColor; // Only for linesShader

  Rectangle graph_rect;
  Vector2 uvZoom;
  Vector2 uvOffset;
  Vector2 uvScreen;

Vector2 points[POINTS_CAP];

#define GROUP_CAP 32
  points_group_t groups[GROUP_CAP];
  int groups_len;
  Color group_colors[GROUP_CAP];

  bool shaders_dirty;
} graph_values_t;

#define GRAPH_LEFT_PAD 400
#define init_graph_values(name, w, h) \
  name.gridShader = LoadShader(NULL, "shaders/grid.fs"); \
  name.linesShader = LoadShader("shaders/line.vs", "shaders/line.fs"); \
  Color cs[] = { RED, GREEN, BLUE, LIGHTGRAY, PINK, GOLD, VIOLET, DARKPURPLE }; \
  for (int i = 0; i < 8; ++i) { \
    name.group_colors[i] = cs[i]; \
  } \
  name.groups_len = 0; \
  name.graph_rect = (Rectangle){ GRAPH_LEFT_PAD, 50, w - GRAPH_LEFT_PAD - 60, h - 110 }; \
  name.uvOffset = (Vector2){ 0., 0. }; \
  name.uvZoom = (Vector2){ 1., 1. }; \
  name.uvScreen = (Vector2){ w, h }; \
  do { \
    for (int i = 0; i < 2; ++i) { \
      name.uResolution[i] = GetShaderLocation(name.shaders[i], "resolution"); \
      name.uZoom[i] = GetShaderLocation(name.shaders[i], "zoom"); \
      name.uOffset[i] = GetShaderLocation(name.shaders[i], "offset"); \
      name.uScreen[i] = GetShaderLocation(name.shaders[i], "screen"); \
    } \
    name.uColor = GetShaderLocation(name.linesShader, "color"); \
    memset(name.points, 0, sizeof(name.points)); \
    memset(name.groups, 0, sizeof(name.groups)); \
  } while(0)

void init_graph(graph_values_t* gv);
void DrawGraph(graph_values_t* gv);

#ifdef __cplusplus
}
#endif
