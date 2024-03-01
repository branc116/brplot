#include "br_plot.h"
#include <math.h>

void smol_mesh_gen_bb(br_shader_line_t* shader, bb_t bb, Color color) {
  float xmi = bb.xmin, ymi = bb.ymin , xma = bb.xmax, yma = bb.ymax;
  Vector2 v[5] = {
    {xmi, ymi},
    {xma, ymi},
    {xma, yma},
    {xmi, yma},
    {xmi, ymi},
  };

  smol_mesh_gen_line_strip(shader, v, 5, color);
}

void smol_mesh_gen_point(br_shader_line_t* shader, Vector2 point, Color color) {
  Vector2 size = { shader->uvs.zoom_uv.x * .01f, shader->uvs.zoom_uv.y * .01f };
  smol_mesh_gen_bb(shader, (bb_t){
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  }, color);
}

void smol_mesh_gen_point1(br_shader_line_t* shader, Vector2 point, Vector2 size, Color color) {
  smol_mesh_gen_bb(shader, (bb_t){
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  }, color);
}

void smol_mesh_gen_line(br_shader_line_t* shader, Vector2 startPos, Vector2 endPos, Color const color) {
  int const vn = 3*3;
  Vector3 const cv = {color.r/255.f, color.g/255.f, color.b/255.f};
  int c = shader->len;
  if (c + 1 >= shader->cap) {
    br_shader_line_draw(shader);
    c = 0;
    shader->len = 2;
  } else {
    shader->len += 2;
  }
  c *= vn;
  Vector2 delta = { endPos.x - startPos.x, endPos.y - startPos.y };
  float length = sqrtf(delta.x*delta.x + delta.y*delta.y);

  Vector2 strip[2] = {
    { startPos.x, startPos.y},
    { endPos.x, endPos.y},
  };
  //First triangle
  shader->vertexPosition_vbo[c+0] = strip[0].x;
  shader->vertexPosition_vbo[c+1] = strip[0].y;
  shader->vertexPosition_vbo[c+2] = -1;
  shader->vertexPosition_vbo[c+3] = strip[1].x;
  shader->vertexPosition_vbo[c+4] = strip[1].y;
  shader->vertexPosition_vbo[c+5] = -1;
  shader->vertexPosition_vbo[c+6] = strip[0].x;
  shader->vertexPosition_vbo[c+7] = strip[0].y;
  shader->vertexPosition_vbo[c+8] = 1;
  //Second triangle
  shader->vertexPosition_vbo[c+9]  = strip[0].x;
  shader->vertexPosition_vbo[c+10] = strip[0].y;
  shader->vertexPosition_vbo[c+11] = 1;
  shader->vertexPosition_vbo[c+12] = strip[1].x;
  shader->vertexPosition_vbo[c+13] = strip[1].y;
  shader->vertexPosition_vbo[c+14] = -1;
  shader->vertexPosition_vbo[c+15] = strip[1].x;
  shader->vertexPosition_vbo[c+16] = strip[1].y;
  shader->vertexPosition_vbo[c+17] = 1;

  for (ssize_t i = 0; i < vn*2; i += 3) {
    //Not a normal, this is dx, dy, length for first triangle
    shader->vertexNormal_vbo[c+i+0] = delta.x;
    shader->vertexNormal_vbo[c+i+1] = delta.y;
    shader->vertexNormal_vbo[c+i+2] = length;
  }

  for (ssize_t i = 0; i < vn*2; i += 3) {
    shader->vertexColor_vbo[c+i+0] = cv.x;
    shader->vertexColor_vbo[c+i+1] = cv.y;
    shader->vertexColor_vbo[c+i+2] = cv.z;
  }
  if (context.debug_bounds) {
    context.debug_bounds = false;
    smol_mesh_gen_point(shader, startPos, WHITE);
    smol_mesh_gen_point(shader, endPos, WHITE);
    context.debug_bounds = true;
  }
}

void smol_mesh_gen_line_strip(br_shader_line_t* shader, Vector2 const * points, size_t len, Color color) {
  for (size_t v = 0; v < (len - 1); ++v) smol_mesh_gen_line(shader, points[v], points[v + 1], color);
}

void smol_mesh_gen_line_strip_stride(br_shader_line_t* shader, Vector2 const * points, ssize_t len, Color const color, int stride) {
  ssize_t v = 0;
  for (; v < (len - stride); v += stride) {
    smol_mesh_gen_line(shader, points[v], points[v + stride], color);
  }
  if (v != len - 1) smol_mesh_gen_line(shader, points[v], points[len - 1], color);
}

