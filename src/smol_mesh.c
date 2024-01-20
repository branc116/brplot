#include "br_plot.h"

#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "raylib/src/rlgl.h"

static size_t smol_mesh_get_vb_size(smol_mesh_t* mesh);
static void smol_mesh_upload(smol_mesh_t* mesh, bool dynamic);

smol_mesh_t* smol_mesh_malloc(size_t capacity, Shader s) {
  smol_mesh_t* ret = BR_MALLOC(sizeof(smol_mesh_t));
  *ret = (smol_mesh_t){
    // Maybe join this 3 mallocs into 1 malloc
    .verticies = BR_MALLOC(3*capacity*3*2*sizeof(float)),
    .normals = BR_MALLOC(3*capacity*3*2*sizeof(float)),
    .colors = BR_MALLOC(3*capacity*3*2*sizeof(float)),
    .capacity = capacity,
    .cur_len = 0,
    .active_shader = s
  };
  smol_mesh_upload(ret, true);
  return ret;
}

void smol_mesh_free(smol_mesh_t* mesh) {
  rlUnloadVertexArray(mesh->vaoId);
  rlUnloadVertexBuffer(mesh->vboIdVertex);
  rlUnloadVertexBuffer(mesh->vboIdNormal);
  rlUnloadVertexBuffer(mesh->vboIdColor);
  BR_FREE(mesh->verticies);
  BR_FREE(mesh->normals);
  BR_FREE(mesh->colors);
  BR_FREE(mesh);
}

void smol_mesh_update(smol_mesh_t* mesh) {
  int number_of_floats = (int)smol_mesh_get_vb_size(mesh);
  rlUpdateVertexBuffer(mesh->vboIdVertex, mesh->verticies, number_of_floats, 0);
  rlUpdateVertexBuffer(mesh->vboIdNormal, mesh->normals, number_of_floats, 0);
  rlUpdateVertexBuffer(mesh->vboIdColor, mesh->colors, number_of_floats, 0);
}

void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, Vector2 center, Vector2 tangent, Color color) {
  (void)tangent;
  size_t c = mesh->cur_len++;
  if (c + 1 >= mesh->capacity) {
    smol_mesh_update(mesh);
    smol_mesh_draw(mesh);
    c = mesh->cur_len++;
  }
  c*=18;
  for (size_t i = 0; i < 18*2; i += 3) {
    mesh->normals[c+i+0] = center.x;
    mesh->normals[c+i+1] = center.y;
    mesh->normals[c+i+2] = 0;
  }
  Vector3 vc = {color.r/255.f, color.g/255.f, color.b/255.f};
  for (size_t i = 0; i < 18*2; i += 3) {
    mesh->colors[c+i+0] = vc.x;
    mesh->colors[c+i+1] = vc.y;
    mesh->colors[c+i+2] = vc.z;
  }
  mesh->verticies[c+0] = rect.x + rect.width;
  mesh->verticies[c+1] = rect.y;
  mesh->verticies[c+2] = 0.f;
  mesh->verticies[c+3] = center.x;
  mesh->verticies[c+4] = center.y;
  mesh->verticies[c+5] = 0.f;
  mesh->verticies[c+6] = rect.x;
  mesh->verticies[c+7] = rect.y;
  mesh->verticies[c+8] = 0.f;

  mesh->verticies[c+9] = rect.x + rect.width;
  mesh->verticies[c+10] = rect.y + rect.height;
  mesh->verticies[c+11] = 0.f;
  mesh->verticies[c+12] = center.x;
  mesh->verticies[c+13] = center.y;
  mesh->verticies[c+14] = 0.f;
  mesh->verticies[c+15] = rect.x + rect.width;
  mesh->verticies[c+16] = rect.y;
  mesh->verticies[c+17] = 0.f;

  c += 18;
  mesh->cur_len++;
  mesh->verticies[c+0] = rect.x;
  mesh->verticies[c+1] = rect.y + rect.height;
  mesh->verticies[c+2] = 0.f;
  mesh->verticies[c+3] = center.x;
  mesh->verticies[c+4] = center.y;
  mesh->verticies[c+5] = 0.f;
  mesh->verticies[c+6] = rect.x + rect.width;
  mesh->verticies[c+7] = rect.y + rect.height;
  mesh->verticies[c+8] = 0.f;

  mesh->verticies[c+9] = rect.x;
  mesh->verticies[c+10] = rect.y;
  mesh->verticies[c+11] = 0.f;
  mesh->verticies[c+12] = center.x;
  mesh->verticies[c+13] = center.y;
  mesh->verticies[c+14] = 0.f;
  mesh->verticies[c+15] = rect.x;
  mesh->verticies[c+16] = rect.y + rect.height;
  mesh->verticies[c+17] = 0.f;
}

void smol_mesh_gen_bb(smol_mesh_t* mesh, bb_t bb, Color color) {
  float xmi = bb.xmin, ymi = bb.ymin , xma = bb.xmax, yma = bb.ymax;
  Vector2 v[5] = {
    {xmi, ymi},
    {xma, ymi},
    {xma, yma},
    {xmi, yma},
    {xmi, ymi},
  };

  size_t old = mesh->points_drawn;
  smol_mesh_gen_line_strip(mesh, v, 5, color);
  mesh->points_drawn = old;
}

void smol_mesh_gen_point(smol_mesh_t* mesh, Vector2 point, Color color) {
  Vector2 size = { context.last_zoom_value.x * .01f, context.last_zoom_value.y * .01f };
  smol_mesh_gen_bb(mesh, (bb_t){
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  }, color);
}

void smol_mesh_gen_point1(smol_mesh_t* mesh, Vector2 point, Vector2 size, Color color) {
  smol_mesh_gen_bb(mesh, (bb_t){
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  }, color);
}

void smol_mesh_gen_line(smol_mesh_t* mesh, Vector2 startPos, Vector2 endPos, Color const color) {
  ssize_t const vn = 2*3*3;
  Vector3 const cv = {color.r/255.f, color.g/255.f, color.b/255.f};
  ++mesh->points_drawn;
  ssize_t c = (ssize_t)mesh->cur_len++;
  if (c >= (ssize_t)mesh->capacity) {
    smol_mesh_update(mesh);
    smol_mesh_draw(mesh);
    c = (ssize_t)mesh->cur_len++;
  }
  c *= vn;
  Vector2 delta = { endPos.x - startPos.x, endPos.y - startPos.y };
  float length = sqrtf(delta.x*delta.x + delta.y*delta.y);

  Vector2 strip[2] = {
    { startPos.x, startPos.y},
    { endPos.x, endPos.y},
  };
  //First triangle
  mesh->verticies[c+0] = strip[0].x;
  mesh->verticies[c+1] = strip[0].y;
  mesh->verticies[c+2] = -1;
  mesh->verticies[c+3] = strip[1].x;
  mesh->verticies[c+4] = strip[1].y;
  mesh->verticies[c+5] = -1;
  mesh->verticies[c+6] = strip[0].x;
  mesh->verticies[c+7] = strip[0].y;
  mesh->verticies[c+8] = 1;
  //Second triangle
  mesh->verticies[c+9]  = strip[0].x;
  mesh->verticies[c+10] = strip[0].y;
  mesh->verticies[c+11] = 1;
  mesh->verticies[c+12] = strip[1].x;
  mesh->verticies[c+13] = strip[1].y;
  mesh->verticies[c+14] = -1;
  mesh->verticies[c+15] = strip[1].x;
  mesh->verticies[c+16] = strip[1].y;
  mesh->verticies[c+17] = 1;

  for (ssize_t i = 0; i < vn; i += 3) {
    //Not a normal, this is dx, dy, length for first triangle
    mesh->normals[c+i+0] = delta.x;
    mesh->normals[c+i+1] = delta.y;
    mesh->normals[c+i+2] = length;
  }

  for (ssize_t i = 0; i < vn; i += 3) {
    mesh->colors[c+i+0] = cv.x;
    mesh->colors[c+i+1] = cv.y;
    mesh->colors[c+i+2] = cv.z;
  }
  if (context.debug_bounds) {
    context.debug_bounds = false;
    Vector2 size = { context.last_zoom_value.x * .01f, context.last_zoom_value.x * .01f };
    smol_mesh_gen_point1(mesh, startPos, size, WHITE);
    smol_mesh_gen_point1(mesh, endPos, size, WHITE);
    context.debug_bounds = true;
  }
}

void smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2 const * points, size_t len, Color color) {
  for (size_t v = 0; v < (len - 1); ++v) smol_mesh_gen_line(mesh, points[v], points[v + 1], color);
}

void smol_mesh_gen_line_strip_stride(smol_mesh_t* mesh, Vector2 const * points, ssize_t len, Color const color, int stride) {
  ssize_t v = 0;
  for (; v < (len - stride); v += stride) {
    smol_mesh_gen_line(mesh, points[v], points[v + stride], color);
  }
  if (v != len - 1) smol_mesh_gen_line(mesh, points[v], points[len - 1], color);
}

void smol_mesh_draw(smol_mesh_t* mesh) {
  // Bind shader program
  rlEnableShader(mesh->active_shader.id);

  rlEnableVertexArray(mesh->vaoId);
  // Bind mesh VBO data: vertex position (shader-location = 0)
  rlEnableVertexBuffer(mesh->vboIdVertex);
  rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_POSITION]);

  rlEnableVertexBuffer(mesh->vboIdNormal);
  rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_NORMAL]);

  rlEnableVertexBuffer(mesh->vboIdColor);
  rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_COLOR], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_COLOR]);

  // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
  rlDrawVertexArray(0, (int)mesh->cur_len*2*3);
  // Disable all possible vertex array objects (or VBOs)
  rlDisableVertexArray();
  rlDisableVertexBuffer();
  rlDisableVertexBufferElement();

  // Disable shader program
  rlDisableShader();
  ++mesh->draw_calls;
  mesh->cur_len = 0;
}

static size_t smol_mesh_get_vb_size(smol_mesh_t* mesh) {
  return mesh->capacity * 3 * 2 * 3 * sizeof(float);
}

static void smol_mesh_upload(smol_mesh_t* mesh, bool dynamic) {

    mesh->vaoId = 0;
    mesh->vboIdVertex = 0;
    mesh->vboIdNormal = 0;
    int cap = (int)smol_mesh_get_vb_size(mesh);

    mesh->vaoId = rlLoadVertexArray();
    rlEnableVertexArray(mesh->vaoId);

    mesh->vboIdVertex = rlLoadVertexBuffer(mesh->verticies, cap, dynamic);
    rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);

    mesh->vboIdNormal = rlLoadVertexBuffer(mesh->normals, cap, dynamic);
    rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);

    mesh->vboIdColor = rlLoadVertexBuffer(mesh->colors, cap, dynamic);
    rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_COLOR], 3, RL_FLOAT, 0, 0, 0);

    rlDisableVertexArray();
}

