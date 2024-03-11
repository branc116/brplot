#include "br_plot.h"
#include "src/br_shaders.h"
#include "raymath.h"
#include <math.h>
#include <assert.h>

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

void smol_mesh_3d_gen_line(br_shader_line_3d_t* shader, Vector3 p1, Vector3 p2, Color color) {
  Vector3 const cv = {color.r/255.f, color.g/255.f, color.b/255.f};
  //Vector3 mid   = Vector3Scale(Vector3Add(p1, p2), 0.5f);
  Vector3 diff  = Vector3Normalize(Vector3Subtract(p2, p1));
  Vector3 norm = { diff.z, -diff.x, diff.y }; 
  norm = Vector3Perpendicular(diff);
  float dist1 = 0.1f * Vector3Distance(shader->uvs.eye_uv, p1);
  float dist2 = 0.1f * Vector3Distance(shader->uvs.eye_uv, p2);
  int n = (int)(12.f/dist1) + 4;
  for (int k = 0; k <= n; ++k) {
    Vector3 next = Vector3Normalize(Vector3RotateByAxisAngle(norm, diff, (float)PI * 2 / (float)n));
    int i = shader->len;
    shader->len += 2;
    if (shader->len > shader->cap) {
      shader->len -= 2;
      br_shader_line_3d_draw(shader);
      shader->len = 2;
      i = 0;
    } // [0 0 0] [0 0 0] [0 0 0]
    Vector3* vecs = (Vector3*) &shader->vertexPosition_vbo[i*9];
    Vector3* colors = (Vector3*) &shader->vertexColor_vbo[i*9];
    Vector3* normals = (Vector3*) &shader->vertexNormal_vbo[i*9];
    normals[0] = norm;
    normals[1] = next;
    normals[2] = norm;

    normals[3] = next;
    normals[4] = norm;
    normals[5] = next;

    vecs[0] = Vector3Add(p1, Vector3Scale(normals[0], 0.1f*dist1));
    vecs[1] = Vector3Add(p1, Vector3Scale(normals[1], 0.1f*dist1));
    vecs[2] = Vector3Add(p2, Vector3Scale(normals[2], 0.1f*dist2));

    vecs[3] = Vector3Add(p2, Vector3Scale(normals[3], 0.1f*dist2));
    vecs[4] = Vector3Add(p2, Vector3Scale(normals[4], 0.1f*dist2));
    vecs[5] = Vector3Add(p1, Vector3Scale(normals[5], 0.1f*dist1));

    for (int j = 0; j < 6; ++j) colors[j] = cv;
    norm = next;
  }
}

void smol_mesh_gen_line_strip_stride(br_shader_line_t* shader, Vector2 const * points, ssize_t len, Color const color, int stride) {
  ssize_t v = 0;
  for (; v < (len - stride); v += stride) {
    smol_mesh_gen_line(shader, points[v], points[v + stride], color);
  }
  if (v != len - 1) smol_mesh_gen_line(shader, points[v], points[len - 1], color);
}

void smol_mesh_gen_quad_3d_simple(br_shader_grid_3d_t* shader, Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, Color color) {
  int c = shader->len;
  shader->len += 2;
  if (c >= shader->cap) {
    br_shader_grid_3d_draw(shader);
    shader->len = 2;
    c = 0;
  }
  c*=18;
  Vector3 vc = {color.r/255.f, color.g/255.f, color.b/255.f};
  for (int i = 0; i < 18; i += 3) {
    shader->vertexColor_vbo[c+i+0] = vc.x;
    shader->vertexColor_vbo[c+i+1] = vc.y;
    shader->vertexColor_vbo[c+i+2] = vc.z;
  }
  Vector3* points = (Vector3*)&shader->vertexPosition_vbo[c];
  points[0] = p1;
  points[1] = p2;
  points[2] = p3;

  points[3] = p3;
  points[4] = p4;
  points[5] = p1;
}

// TODO: This should be split to _gen and _draw
void smol_mesh_grid_draw(br_plot_instance_t* plot) {
  // TODO 2D/3D
  int h = (int)plot->graph_screen_rect.height;
  rlViewport((int)plot->graph_screen_rect.x, (int)plot->resolution.y - h - (int)plot->graph_screen_rect.y, (int)plot->graph_screen_rect.width, h);
  switch (plot->kind) {
    case br_plot_instance_kind_2d: {
      br_shader_grid_t* grid = plot->dd.grid_shader;
      assert(grid->len == 0);
      Vector2* p = (Vector2*)grid->vertexPosition_vbo;
      p[0] = (Vector2) { .x = -1, .y = -1 };
      p[1] = (Vector2) { .x = 1,  .y = -1 };
      p[2] = (Vector2) { .x = 1,  .y = 1 };
      p[3] = (Vector2) { .x = -1, .y = -1 };
      p[4] = (Vector2) { .x = 1,  .y = 1 };
      p[5] = (Vector2) { .x = -1, .y = 1 };
      grid->len = 2;
      br_shader_grid_draw(grid);
      grid->len = 0;
    } break;
    case br_plot_instance_kind_3d: {
      float sz = 10000.f;
      rlDisableBackfaceCulling();
      //smol_mesh_gen_quad_simple(gv->graph_mesh_3d, r3, Color {0, 0, 1, 0});
      smol_mesh_gen_quad_3d_simple(plot->ddd.grid_shader, (Vector3){ -sz, 0, -sz }, (Vector3){ sz, 0, -sz }, (Vector3){ sz, 0, sz }, (Vector3){ -sz, 0, sz }, (Color){0, 1, 0, 0});
      smol_mesh_gen_quad_3d_simple(plot->ddd.grid_shader, (Vector3){ -sz, -sz, 0 }, (Vector3){ sz, -sz, 0 }, (Vector3){ sz, sz, 0 }, (Vector3){ -sz, sz, 0 }, (Color){0, 0, 1, 0});
      br_shader_grid_3d_draw(plot->ddd.grid_shader);
      plot->ddd.grid_shader->len = 0;
      rlEnableBackfaceCulling();
    } break;
    default: assert(0);
  }
  rlViewport(0, 0, (int)plot->resolution.x, (int)plot->resolution.y);
}

