#pragma once

#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

// This is the size of one group of points that will be drawn with one draw call.
// TODO: make this dynamic.
#define PTOM_COUNT (1<<15)

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


void smol_mesh_init(smol_mesh_t* mesh);
void smol_mesh_init_temp();
smol_mesh_t* smol_mesh_get_temp();
bool smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2* points, int len, int offset);
void smol_mesh_upload(smol_mesh_t* mesh, bool dynamic);
void smol_mesh_draw_line_strip(smol_mesh_t* mesh, Shader shader);
void smol_mesh_update(smol_mesh_t* mesh);
void smol_mesh_unload(smol_mesh_t* mesh);

#ifdef __cplusplus
}
#endif

