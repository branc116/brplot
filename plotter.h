#pragma once
#include "raylib.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//Maximum number of points that can be shown by this plotter.
//If you try to show more, this program will crash....
#ifdef PLATFORM_WEB
#define POINTS_CAP (1 * 1024 * 1024)
#else
#define POINTS_CAP (128 * 1024 * 1024)
#endif
#define SMOL_MESHES_CAP 1024

// This is the size of one group of points that will be drawn with one draw call.
// TODO: make this dynamic.
#define PTOM_COUNT (1<<15)

#define GROUP_CAP 32

//TODO: Do something with this...
#define GRAPH_LEFT_PAD 400

typedef struct {
  unsigned int vaoId;     // OpenGL Vertex Array Object id
  union {
    unsigned int vboId[2];
    struct {
      unsigned int vboIdVertex, vboIdNormal;
    };
  };
  float* verticies;
  float* normals;
  // Max Number of points and only number of points.
  int length;
  // length * (2 triengle per line) * (3 verticies per triangle)
  int vertex_count;
  // length * (2 triengle per line)
  int triangle_count;
} smol_mesh_t;

typedef struct {
  int cap, len;
  int group_id;
  bool is_selected, is_sorted;
  Vector2* points;
  int smol_meshes_len;
  smol_mesh_t meshes[SMOL_MESHES_CAP];
} points_group_t;

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

  points_group_t groups[GROUP_CAP];
  int groups_len;
  Color group_colors[GROUP_CAP];

  bool shaders_dirty;
} graph_values_t;

void smol_mesh_init(smol_mesh_t* mesh);
void smol_mesh_init_temp(void);
smol_mesh_t* smol_mesh_get_temp(void);
bool smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2* points, int len, int offset);
void smol_mesh_upload(smol_mesh_t* mesh, bool dynamic);
void smol_mesh_draw_line_strip(smol_mesh_t* mesh, Shader shader);
void smol_mesh_update(smol_mesh_t* mesh);
void smol_mesh_unload(smol_mesh_t* mesh);

points_group_t* points_group_get(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, Vector2* all_points, int group);
void points_group_push_point(points_group_t* g, Vector2 v);
void points_groups_draw(points_group_t* pg_array, int pg_len, Shader shader, int color_uniform, Color* colors, Rectangle rect);
void points_group_add_test_points(points_group_t* pg_array, int* pg_len, int pg_array_cap, Vector2* all_points);

void graph_init(graph_values_t* gv, float width, float height);
void graph_draw(graph_values_t* gv);

// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(graph_values_t* gv);

void read_input_main(graph_values_t* ptr);
void add_point_callback(graph_values_t* gv, float value, int group);

#ifdef __cplusplus
}
#endif

