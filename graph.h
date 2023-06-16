#pragma once
#include "raylib.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned int vaoId;     // OpenGL Vertex Array Object id
  unsigned int *vboId;    // OpenGL Vertex Buffer Objects id (default vertex data)
  float* verticies;
  float* tex_cords;
  float* normals;
  // Max Number of points and only number of points.
  int length;
  // length * (2 triengle per line) * (3 verticies per triangle)
  int vertex_count;
  // length * (2 triengle per line)
  int triangle_count;
} smol_mesh_t;

#define SMOL_MESHES_CAP 1024
typedef struct {
  int cap, len;
  int group_id;
  bool is_selected;
  Vector2* points;
  int smol_meshes_len;
  smol_mesh_t meshes[SMOL_MESHES_CAP];
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

#define POINTS_CAP (64 * 1024 * 1024)
  Vector2 points[POINTS_CAP];

#define GROUP_CAP 32
  point_group_t groups[GROUP_CAP];
  int groups_len;
  Color group_colors[GROUP_CAP];

  bool shaders_dirty;
} graph_values_t;


typedef struct {Vector2* v; point_group_t* group;} pp_ret;

#define GRAPH_LEFT_PAD 400
#define init_graph_values(name, w, h) \
  name.gridShader = LoadShader(NULL, "shaders/grid.fs"); \
  name.linesShader = LoadShader("shaders/line.vs", "shaders/line.fs"); \
  Color cs[] = { RED, GREEN, BLUE, LIGHTGRAY, PINK, GOLD, VIOLET, DARKPURPLE }; \
  for (int i = 0; i < 8; ++i) { \
    name.group_colors[i] = cs[i]; \
  } \
  name.groups_len = 0; \
  Rectangle r = { GRAPH_LEFT_PAD, 50, w - GRAPH_LEFT_PAD - 60, h - 110 }; \
  name.graph_rect = r; \
  Vector2 o = { 0., 0. }; \
  name.uvOffset = o; \
  Vector2 z = { 1., 1. }; \
  name.uvZoom = z; \
  Vector2 sc = { w, h }; \
  name.uvScreen = sc; \
  do { \
    for (int i = 0; i < 2; ++i) { \
      gv.uResolution[i] = GetShaderLocation(name.shaders[i], "resolution"); \
      gv.uZoom[i] = GetShaderLocation(name.shaders[i], "zoom"); \
      gv.uOffset[i] = GetShaderLocation(name.shaders[i], "offset"); \
      gv.uScreen[i] = GetShaderLocation(name.shaders[i], "screen"); \
    } \
    memset(gv.points, 0, sizeof(gv.points)); \
    memset(gv.groups, 0, sizeof(gv.groups)); \
  } while(0)


point_group_t* push_point_group(graph_values_t* gv, int group);
void push_point(point_group_t* gv, Vector2 v);
void init_graph(graph_values_t* gv);
void DrawGraph(graph_values_t* gv);

#ifdef __cplusplus
}
#endif
