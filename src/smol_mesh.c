#include "src/br_smol_mesh.h"
#include "src/br_plot.h"
#include "src/br_gl.h"
#include "src/br_math.h"

#include "tracy/TracyC.h"

#include <assert.h>

void smol_mesh_gen_bb(br_shader_line_t* shader, bb_t bb, br_color_t color) {
  float xmi = bb.xmin, ymi = bb.ymin , xma = bb.xmax, yma = bb.ymax;
  br_vec2_t v[5] = {
    BR_VEC2(xmi, ymi),
    BR_VEC2(xma, ymi),
    BR_VEC2(xma, yma),
    BR_VEC2(xmi, yma),
    BR_VEC2(xmi, ymi),
  };

  smol_mesh_gen_line_strip(shader, v, 5, color);
}

void smol_mesh_gen_point(br_shader_line_t* shader, br_vec2_t point, br_color_t color) {
  br_vec2_t size = BR_VEC2(shader->uvs.zoom_uv.x * .01f, shader->uvs.zoom_uv.y * .01f);
  smol_mesh_gen_bb(shader, (bb_t){
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  }, color);
}

void smol_mesh_gen_point1(br_shader_line_t* shader, br_vec2_t point, br_vec2_t size, br_color_t color) {
  smol_mesh_gen_bb(shader, (bb_t){
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  }, color);
}

void smol_mesh_gen_line(br_shader_line_t* shader, br_vec2_t startPos, br_vec2_t endPos, br_color_t const color) {
  br_vec3_t const cv = BR_VEC3(color.r/255.f, color.g/255.f, color.b/255.f);
  int c = shader->len;
  if (c + 1 >= shader->cap) {
    br_shader_line_draw(shader);
    c = 0;
    shader->len = 2;
  } else {
    shader->len += 2;
  }
  c *= 3;
  br_vec2_t delta = BR_VEC2(endPos.x - startPos.x, endPos.y - startPos.y);

  br_vec2_t strip[2] = {
    BR_VEC2(startPos.x, startPos.y),
    BR_VEC2(endPos.x, endPos.y),
  };
  //First triangle
  shader->vertexX_vbo[c+0] = strip[0].x; shader->vertexY_vbo[c+0] = strip[0].y;
  shader->vertexX_vbo[c+1] = strip[1].x; shader->vertexY_vbo[c+1] = strip[1].y;
  shader->vertexX_vbo[c+2] = strip[0].x; shader->vertexY_vbo[c+2] = strip[0].y;
  shader->vertexX_vbo[c+3] = strip[0].x; shader->vertexY_vbo[c+3] = strip[0].y;
  shader->vertexX_vbo[c+4] = strip[1].x; shader->vertexY_vbo[c+4] = strip[1].y;
  shader->vertexX_vbo[c+5] = strip[1].x; shader->vertexY_vbo[c+5] = strip[1].y;

  br_vec2_t* normals = (br_vec2_t*)&shader->delta_vbo[2*c];
  normals[0] = BR_VEC2(delta.x, delta.y);
  normals[1] = BR_VEC2(delta.x, delta.y);
  normals[2] = BR_VEC2(delta.x, delta.y);
  normals[3] = BR_VEC2(delta.x, delta.y);
  normals[4] = BR_VEC2(delta.x, delta.y);
  normals[5] = BR_VEC2(delta.x, delta.y);

  br_vec3_t* colors = (br_vec3_t*)&shader->vertexColor_vbo[3*c];
  colors[0] = BR_VEC3(cv.x, cv.y, 2.f+cv.z);
  colors[1] = BR_VEC3(cv.x, cv.y, 2.f+cv.z);
  colors[2] = BR_VEC3(cv.x, cv.y, cv.z);
  colors[3] = BR_VEC3(cv.x, cv.y, cv.z);
  colors[4] = BR_VEC3(cv.x, cv.y, 2.f+cv.z);
  colors[5] = BR_VEC3(cv.x, cv.y, cv.z);

  if (context.debug_bounds) {
    context.debug_bounds = false;
    smol_mesh_gen_point(shader, startPos, BR_WHITE);
    smol_mesh_gen_point(shader, endPos, BR_WHITE);
    context.debug_bounds = true;
  }
}

void smol_mesh_gen_line_strip(br_shader_line_t* shader, br_vec2_t const * points, size_t len, br_color_t color) {
  TracyCZoneN(gen_line_strip_ctx, "GenLineStrip", true);
  TracyCZoneValue(gen_line_strip_ctx, len);
  for (size_t v = 0; v < (len - 1); ++v) smol_mesh_gen_line(shader, points[v], points[v + 1], color);
  TracyCZoneEnd(gen_line_strip_ctx);
}

void smol_mesh_gen_line_strip2(br_shader_line_t* shader, float const* xs, float const* ys, size_t len, br_color_t color) {
  TracyCZoneN(gen_line_strip_ctx, "GenLineStrip2", true);
  TracyCZoneValue(gen_line_strip_ctx, len);
  for (size_t v = 0; v < (len - 1); ++v)
    smol_mesh_gen_line(shader, BR_VEC2(xs[v], ys[v]), BR_VEC2(xs[v+1], ys[v+1]), color);
  TracyCZoneEnd(gen_line_strip_ctx);
}

void smol_mesh_3d_gen_line_simple(br_shader_line_3d_simple_t* shader, br_vec3_t p1, br_vec3_t p2, br_color_t color, br_vec3_t eye) {
  int i = shader->len;
  shader->len += 2;
  if (shader->len > shader->cap) {
    shader->len -= 2;
    br_shader_line_3d_simple_draw(shader);
    shader->len = 2;
    i = 0;
  }
  br_vec3_t* vecs = (br_vec3_t*) &shader->vertexPosition_vbo[i*18];
  br_vec3_t* colors = (br_vec3_t*) &shader->vertexColor_vbo[i*18];
  br_vec3_t* normals = (br_vec3_t*) &shader->vertexNormal_vbo[i*18];
  br_vec3_t const cv = BR_VEC3(color.r/255.f, color.g/255.f, color.b/255.f);
  //br_vec3_t mid = br_vec3_scale(br_vec3_add(p1, p2), 0.5f);
  br_vec3_t diff = br_vec3_normalize(br_vec3_sub(p2, p1));
  float dist1 = 0.01f * br_vec3_dist(eye, p1);
  float dist2 = 0.01f * br_vec3_dist(eye, p2);
  dist1 = dist2 = (dist1 + dist2) * .5f;
  br_vec3_t right1 = br_vec3_cross(br_vec3_normalize(br_vec3_sub(eye, p1)), diff);
  br_vec3_t right2 = br_vec3_cross(br_vec3_normalize(br_vec3_sub(eye, p2)), diff);
  normals[0] = br_vec3_scale(right1, -dist1);
  normals[1] = br_vec3_scale(right1, dist1);
  normals[2] = br_vec3_scale(right2, dist2);

  normals[3] = br_vec3_scale(right2, dist2);
  normals[4] = br_vec3_scale(right2, -dist2);
  normals[5] = br_vec3_scale(right1, -dist1);

  vecs[0] = br_vec3_add(p1, normals[0]);
  vecs[1] = br_vec3_add(p1, normals[1]);
  vecs[2] = br_vec3_add(p2, normals[2]);

  vecs[3] = br_vec3_add(p2, normals[3]);
  vecs[4] = br_vec3_add(p2, normals[4]);
  vecs[5] = br_vec3_add(p1, normals[5]);

  for (int j = 0; j < 6; ++j) colors[j] = cv;
}

void smol_mesh_3d_gen_line_strip(br_shader_line_3d_t* shader, br_vec3_t const* ps, size_t len, br_color_t color) {
  for (size_t i = 0; i < len - 1; ++i) smol_mesh_3d_gen_line(shader, ps[i], ps[i + 1], color);
}
void smol_mesh_3d_gen_line_strip1(br_shader_line_3d_t* shader, float const* xs, float const* ys, float const* zs, size_t len, br_color_t color) {
  for (size_t i = 0; i < len - 1; ++i) smol_mesh_3d_gen_line(shader, BR_VEC3(xs[i], ys[i], zs[i]), BR_VEC3(xs[i + 1], ys[i + 1], zs[i + 1]), color);

}

void smol_mesh_3d_gen_line_strip2(br_shader_line_3d_t* shader, br_vec2_t const* ps, size_t len, br_color_t color) {
  for (size_t i = 0; i < len - 1; ++i) smol_mesh_3d_gen_line(shader,
      BR_VEC3(ps[i].x, ps[i].y, 0),
      BR_VEC3(ps[i+1].x, ps[i+1].y, 0), color);
}

void smol_mesh_3d_gen_line_strip3(br_shader_line_3d_t* shader, float const* xs, float const* ys, size_t len, br_color_t color) {
  for (size_t i = 0; i < len - 1; ++i) smol_mesh_3d_gen_line(shader,
      BR_VEC3(xs[i], ys[i], 0),
      BR_VEC3(xs[i+1], ys[i+1], 0), color);
}

float line_3d_size = 0.02f;

void smol_mesh_3d_gen_line(br_shader_line_3d_t* shader, br_vec3_t p1, br_vec3_t p2, br_color_t color) {
  br_vec3_t const cv = BR_VEC3(color.r/255.f, color.g/255.f, color.b/255.f);
  br_vec3_t diff  = br_vec3_normalize(br_vec3_sub(p2, p1));
  br_vec3_t norm = br_vec3_perpendicular(diff);
  float dist1 = 0.1f * br_vec3_dist(shader->uvs.eye_uv, p1);
  float dist2 = 0.1f * br_vec3_dist(shader->uvs.eye_uv, p2);
  int n = (int)(6.f/fminf(dist1, dist2)) + 6;
  for (int k = 0; k <= n; ++k) {
    br_vec3_t next = br_vec3_normalize(br_vec3_rot(norm, diff, BR_PI * 2 / (float)n));
    int i = shader->len;
    shader->len += 2;
    if (shader->len > shader->cap) {
      shader->len -= 2;
      br_shader_line_3d_draw(shader);
      shader->len = 2;
      i = 0;
    } // [0 0 0] [0 0 0] [0 0 0]
    br_vec3_t* vecs = (br_vec3_t*) &shader->vertexPosition_vbo[i*9];
    br_vec3_t* colors = (br_vec3_t*) &shader->vertexColor_vbo[i*9];
    br_vec3_t* normals = (br_vec3_t*) &shader->vertexNormal_vbo[i*9];
    normals[0] = norm;
    normals[1] = next;
    normals[2] = norm;

    normals[3] = next;
    normals[4] = norm;
    normals[5] = next;

    vecs[0] = br_vec3_add(p1, br_vec3_scale(normals[0], line_3d_size*dist1));
    vecs[1] = br_vec3_add(p1, br_vec3_scale(normals[1], line_3d_size*dist1));
    vecs[2] = br_vec3_add(p2, br_vec3_scale(normals[2], line_3d_size*dist2));

    vecs[3] = br_vec3_add(p2, br_vec3_scale(normals[3], line_3d_size*dist2));
    vecs[4] = br_vec3_add(p2, br_vec3_scale(normals[4], line_3d_size*dist2));
    vecs[5] = br_vec3_add(p1, br_vec3_scale(normals[5], line_3d_size*dist1));

    for (int j = 0; j < 6; ++j) colors[j] = cv;
    norm = next;
  }
}

void smol_mesh_gen_line_strip_stride(br_shader_line_t* shader, br_vec2_t const * points, ssize_t len, br_color_t const color, int stride) {
  ssize_t v = 0;
  for (; v < (len - stride); v += stride) {
    smol_mesh_gen_line(shader, points[v], points[v + stride], color);
  }
  if (v != len - 1) smol_mesh_gen_line(shader, points[v], points[len - 1], color);
}

void smol_mesh_gen_quad_3d_simple(br_shader_grid_3d_t* shader, br_vec3_t p1, br_vec3_t p2, br_vec3_t p3, br_vec3_t p4, br_color_t color) {
  int c = shader->len;
  shader->len += 2;
  if (c >= shader->cap) {
    br_shader_grid_3d_draw(shader);
    shader->len = 2;
    c = 0;
  }
  c*=9;
  br_vec3_t vc = BR_VEC3(color.r, color.g, color.b);
  for (int i = 0; i < 18; i += 3) {
    shader->vertexColor_vbo[c+i+0] = vc.x;
    shader->vertexColor_vbo[c+i+1] = vc.y;
    shader->vertexColor_vbo[c+i+2] = vc.z;
  }
  br_vec3_t* points = (br_vec3_t*)&(shader->vertexPosition_vbo[c]);
  points[0] = p1;
  points[1] = p2;
  points[2] = p3;

  points[3] = p3;
  points[4] = p4;
  points[5] = p1;
}

// TODO: This should be split to _gen and _draw
void smol_mesh_grid_draw(br_plot_t* plot, br_shaders_t* shaders) {
  // TODO 2D/3D
  int h = plot->graph_screen_rect.height;
  br_extenti_t ex = plot->graph_screen_rect;
  brgl_viewport(ex.x, plot->resolution.height - h - ex.y, ex.width, h);
  switch (plot->kind) {
    case br_plot_kind_2d: {
      TracyCFrameMarkStart("grid_draw_2d");
      assert(shaders->grid->len == 0);
      br_vec2_t* p = (br_vec2_t*)shaders->grid->vertexPosition_vbo;
      p[0] = (br_vec2_t) { .x = -1, .y = -1 };
      p[1] = (br_vec2_t) { .x = 1,  .y = -1 };
      p[2] = (br_vec2_t) { .x = 1,  .y = 1 };
      p[3] = (br_vec2_t) { .x = -1, .y = -1 };
      p[4] = (br_vec2_t) { .x = 1,  .y = 1 };
      p[5] = (br_vec2_t) { .x = -1, .y = 1 };
      shaders->grid->len = 2;
      br_shader_grid_draw(shaders->grid);
      shaders->grid->len = 0;
      TracyCFrameMarkEnd("grid_draw_2d");
    } break;
    case br_plot_kind_3d: {
      TracyCFrameMarkStart("grid_draw_3d");
      float sz = 10000.f;
      //smol_mesh_gen_quad_simple(gv->graph_mesh_3d, r3, br_color_t {0, 0, 1, 0});
      smol_mesh_gen_quad_3d_simple(shaders->grid_3d, BR_VEC3(-sz, 0, -sz), BR_VEC3(sz, 0, -sz), BR_VEC3(sz, 0, sz), BR_VEC3(-sz, 0, sz), (br_color_t){0, 1, 0, 0});
      smol_mesh_gen_quad_3d_simple(shaders->grid_3d, BR_VEC3(-sz, -sz, 0), BR_VEC3(sz, -sz, 0), BR_VEC3(sz, sz, 0), BR_VEC3(-sz, sz, 0), (br_color_t){0, 0, 1, 0});
      br_shader_grid_3d_draw(shaders->grid_3d);
      shaders->grid_3d->len = 0;
      TracyCFrameMarkEnd("grid_draw_3d");
    } break;
    default: assert(0);
  }
  brgl_viewport(0, 0, plot->resolution.width, plot->resolution.height);
}

