#pragma once
#include "raylib.h"
#include "smol_mesh.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int cap, len;
  int group_id;
  bool is_selected;
  Vector2* points;
  int smol_meshes_len;
  smol_mesh_t meshes[SMOL_MESHES_CAP];
} points_group_t;

points_group_t* points_group_get(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, Vector2* all_points, int group);
void points_group_push_point(points_group_t* g, Vector2 v);
void points_groups_draw(points_group_t* pg_array, int pg_len, Shader shader, int color_uniform, Color* colors);
void points_group_add_test_points(points_group_t* pg_array, int* pg_len, int pg_array_cap, Vector2* all_points);

#ifdef __cplusplus
}
#endif

