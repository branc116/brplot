#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_plot.h"
#include "src/br_math.h"
#include "src/br_tl.h"
#include "src/br_theme.h"

#include <assert.h>

void smol_mesh_gen_bb(br_smol_mesh_line_t args, bb_t bb) {
  float xmi = bb.xmin, ymi = bb.ymin , xma = bb.xmax, yma = bb.ymax;
  br_vec2_t v[5] = {
    BR_VEC2(xmi, ymi),
    BR_VEC2(xma, ymi),
    BR_VEC2(xma, yma),
    BR_VEC2(xmi, yma),
    BR_VEC2(xmi, ymi),
  };

  smol_mesh_gen_line_strip(args, v, 5);
}

void smol_mesh_gen_point(br_smol_mesh_line_t args, br_vec2_t point) {
  br_vec2_t size = BR_VEC2(args.zoom.x * .01f, args.zoom.y * .01f);
  smol_mesh_gen_bb(args, (bb_t) {
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  });
}

void smol_mesh_gen_point1(br_smol_mesh_line_t args, br_vec2_t point, br_vec2_t size) {
  smol_mesh_gen_bb(args, (bb_t) {
      .xmin = point.x - size.x / 2,
      .ymin = point.y - size.y / 2,
      .xmax = point.x + size.x / 2,
      .ymax = point.y + size.y / 2,
  });
}

void smol_mesh_gen_line(br_smol_mesh_line_t args, br_vec2_t startPos, br_vec2_t endPos) {
  br_vec2_t const delta = br_vec2_sub(endPos, startPos);

  br_vec2_t strip[2] = {
    BR_VEC2(startPos.x, startPos.y),
    BR_VEC2(endPos.x, endPos.y),
  };

  float thick = 0.05f;
  br_vec2_t zoom = args.zoom;
  br_vec2_t screen = args.screen_size;
  br_vec2_t offset = args.offset;

  br_vec2_t normal = br_vec2_normalize(br_vec2_mul(BR_VEC2(delta.y, -delta.x), zoom));
  br_vec2_t dif = br_vec2_scale(normal, thick);
  br_vec2_t dif0 = br_vec2_max(br_vec2_scale(zoom, 0.1f), br_vec2_scale(strip[0], 7e-5f));
  br_vec2_t dif1 = br_vec2_max(br_vec2_scale(zoom, 0.1f), br_vec2_scale(strip[1], 7e-5f));
  br_vec2_t poss[4] = {
    br_vec2_add(strip[0], br_vec2_mul(dif, dif0)),
    br_vec2_add(strip[0], br_vec2_mul(br_vec2_scale(dif, -1.f), dif0)),
    br_vec2_add(strip[1], br_vec2_mul(br_vec2_scale(dif, -1.f), dif1)),
    br_vec2_add(strip[1], br_vec2_mul(dif, dif1)),
  };
  br_vec2_t fact = br_vec2_div(screen, BR_VEC2(screen.y, screen.y));
  fact = br_vec2_mul(fact, zoom);
  for (int i = 0; i < 4; ++i) {
    poss[i] = br_vec2_sub(poss[i], offset);
    poss[i] = br_vec2_div(poss[i], fact);
    poss[i] = br_vec2_scale(poss[i], 2.f);
  }

  br_shader_line_push_quad(brtl_shaders()->line, (br_shader_line_el_t[4]) {
      { .pos_delta = BR_VEC4(poss[0].x, poss[0].y, 0.98f,  1.f) },
      { .pos_delta = BR_VEC4(poss[1].x, poss[1].y, 0.98f, -1.f) },
      { .pos_delta = BR_VEC4(poss[2].x, poss[2].y, 0.98f, -1.f) },
      { .pos_delta = BR_VEC4(poss[3].x, poss[3].y, 0.98f,  1.f) },
  });

  if (br_context.debug_bounds) {
    br_context.debug_bounds = false;
    smol_mesh_gen_point(args, startPos);
    smol_mesh_gen_point(args, endPos);
    br_context.debug_bounds = true;
  }
}

void smol_mesh_gen_line_strip(br_smol_mesh_line_t args, br_vec2_t const * points, size_t len) {
  TracyCZoneN(gen_line_strip_ctx, "GenLineStrip", true);
  TracyCZoneValue(gen_line_strip_ctx, len);
  for (size_t v = 0; v < (len - 1); ++v) smol_mesh_gen_line(args, points[v], points[v + 1]);
  TracyCZoneEnd(gen_line_strip_ctx);
}

void smol_mesh_gen_line_strip2(br_smol_mesh_line_t args, float const* xs, float const* ys, size_t len) {
  TracyCZoneN(gen_line_strip_ctx, "GenLineStrip2", true);
  TracyCZoneValue(gen_line_strip_ctx, len);
  for (size_t v = 0; v < (len - 1); ++v)
    smol_mesh_gen_line(args, BR_VEC2(xs[v], ys[v]), BR_VEC2(xs[v+1], ys[v+1]));
  TracyCZoneEnd(gen_line_strip_ctx);
}


void smol_mesh_gen_line_strip_stride(br_smol_mesh_line_t args, br_vec2_t const * points, ssize_t len, int stride) {
  ssize_t v = 0;
  for (; v < (len - stride); v += stride) {
    smol_mesh_gen_line(args, points[v], points[v + stride]);
  }
  if (v != len - 1) smol_mesh_gen_line(args, points[v], points[len - 1]);
}

void smol_mesh_3d_gen_line_simple(br_shader_line_3d_simple_t* shader, br_vec3_t p1, br_vec3_t p2, br_color_t color, br_vec3_t eye) {
  // TODO: This actually looks quite good. Maybe use this in the future
  (void)color;
  br_vec3_t diff = br_vec3_sub(p2, p1);
  float dist1 = 0.01f * br_vec3_dist(eye, p1);
  float dist2 = 0.01f * br_vec3_dist(eye, p2);
  dist1 = dist2 = (dist1 + dist2) * .5f;
  br_vec3_t right1 = br_vec3_normalize(br_vec3_cross(br_vec3_sub(eye, p1), diff));
  br_vec3_t right2 = br_vec3_normalize(br_vec3_cross(br_vec3_sub(eye, p2), diff));
  br_vec3_t bl = br_vec3_scale(right1, -dist1);
  br_vec3_t br = br_vec3_scale(right1, dist1);
  br_vec3_t tl = br_vec3_scale(right2, -dist1);
  br_vec3_t tr = br_vec3_scale(right2, dist1);
  br_shader_line_3d_simple_push_quad(shader, (br_shader_line_3d_simple_el_t[4]) {
    {.vertexPosition = br_vec3_add(p1, bl), .vertexNormal = bl },
    {.vertexPosition = br_vec3_add(p1, br), .vertexNormal = br },
    {.vertexPosition = br_vec3_add(p2, tr), .vertexNormal = br },
    {.vertexPosition = br_vec3_add(p2, tl), .vertexNormal = tl },
  });
}

void smol_mesh_3d_gen_line(br_shader_line_3d_t* shader, br_vec3_t p1, br_vec3_t p2, br_color_t color) {
  (void)color;
  float const line_3d_size = 0.02f;
  br_vec3_t diff  = br_vec3_normalize(br_vec3_sub(p2, p1));
  br_vec3_t norm = br_vec3_perpendicular(diff);
  float dist1 = 0.1f * br_vec3_dist(shader->uvs.eye_uv, p1);
  float dist2 = 0.1f * br_vec3_dist(shader->uvs.eye_uv, p2);
  int n = (int)(6.f/fminf(dist1, dist2)) + 6;
  for (int k = 0; k <= n; ++k) {
    br_vec3_t next = br_vec3_normalize(br_vec3_rot(norm, diff, BR_PI * 2 / (float)n));
    br_shader_line_3d_push_quad(shader, (br_shader_line_3d_el_t[4]) {
        { .vertexPosition = br_vec3_add(p1, br_vec3_scale(norm, line_3d_size*dist1)), .vertexNormal = norm },
        { .vertexPosition = br_vec3_add(p1, br_vec3_scale(next, line_3d_size*dist1)), .vertexNormal = next },
        { .vertexPosition = br_vec3_add(p2, br_vec3_scale(norm, line_3d_size*dist2)), .vertexNormal = norm },
        { .vertexPosition = br_vec3_add(p2, br_vec3_scale(next, line_3d_size*dist2)), .vertexNormal = next }
    });
    norm = next;
  }
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


// TODO: This should be split to _gen and _draw
void smol_mesh_grid_draw(br_plot_t* plot, br_shaders_t* shaders) {
  // TODO 2D/3D
  switch (plot->kind) {
    case br_plot_kind_2d: {
      TracyCFrameMarkStart("grid_draw_2d");
      br_shader_grid_push_quad(shaders->grid, (br_shader_grid_el_t[4]) {
          { .vertexPosition = BR_VEC2(-1, 1) },
          { .vertexPosition = BR_VEC2(1, 1) },
          { .vertexPosition = BR_VEC2(1, -1) },
          { .vertexPosition = BR_VEC2(-1, -1) },
      });
      shaders->grid->uvs.bg_color_uv = BR_COLOR_TO4(BR_THEME.colors.plot_bg);
      shaders->grid->uvs.lines_color_uv = BR_COLOR_TO4(BR_THEME.colors.grid_lines);
      TracyCFrameMarkEnd("grid_draw_2d");
    } break;
    case br_plot_kind_3d: {
      TracyCFrameMarkStart("grid_draw_3d");
      float sz = 10000.f;
      br_shader_grid_3d_push_quad(shaders->grid_3d, (br_shader_grid_3d_el_t[4]) {
        { .vertexPosition = BR_VEC3(-sz, 0, -sz), .vertexColor = BR_VEC3(0, 1, 0), .z = BR_Z_TO_GL(1) },
        { .vertexPosition = BR_VEC3(sz, 0, -sz),  .vertexColor = BR_VEC3(0, 1, 0), .z = BR_Z_TO_GL(1) },
        { .vertexPosition = BR_VEC3(sz, 0, sz),   .vertexColor = BR_VEC3(0, 1, 0), .z = BR_Z_TO_GL(1) },
        { .vertexPosition = BR_VEC3(-sz, 0, sz),  .vertexColor = BR_VEC3(0, 1, 0), .z = BR_Z_TO_GL(1) },
      });
      br_shader_grid_3d_push_quad(shaders->grid_3d, (br_shader_grid_3d_el_t[4]) {
        { .vertexPosition = BR_VEC3(-sz, -sz, 0), .vertexColor = BR_VEC3(0, 0, 1), .z = BR_Z_TO_GL(5) },
        { .vertexPosition = BR_VEC3(sz, -sz, 0),  .vertexColor = BR_VEC3(0, 0, 1), .z = BR_Z_TO_GL(5) },
        { .vertexPosition = BR_VEC3(sz, sz, 0),   .vertexColor = BR_VEC3(0, 0, 1), .z = BR_Z_TO_GL(5) },
        { .vertexPosition = BR_VEC3(-sz, sz, 0),  .vertexColor = BR_VEC3(0, 0, 1), .z = BR_Z_TO_GL(5) },
      });
      TracyCFrameMarkEnd("grid_draw_3d");
    } break;
    default: assert(0);
  }
}

