#pragma once
#include "src/br_shaders.h"
#include "src/br_data.h"
#include "src/br_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_mesh_line_t {
  br_vec2d_t zoom, offset, screen_size;
  float line_thickness;
  br_vec2_t prev[2];
} br_mesh_line_t;

typedef struct br_mesh_line_3d_t {
  float line_thickness;
  br_mat_t mvp;
} br_mesh_line_3d_t;

//void br_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, br_vec2_t mid_point, br_vec2_t tangent, br_color_t color);
//void br_mesh_gen_quad_simple(smol_mesh_t* mesh, Rectangle rect, br_color_t color);

void br_mesh_gen_bb(br_mesh_line_t args, br_bb_t bb);
void br_mesh_gen_point(br_mesh_line_t args, br_vec2_t point);
void br_mesh_gen_point1(br_mesh_line_t args, br_vec2_t point, br_vec2_t size);
void br_mesh_gen_line(br_mesh_line_t* args, br_vec2_t p1, br_vec2_t p2);
void br_mesh_gen_line_strip(br_mesh_line_t args, br_vec2_t const * points, size_t len);
void br_mesh_gen_line_strip2(br_mesh_line_t args, float const* xs, float const* ys, size_t len);
void br_mesh_gen_line_strip_stride(br_mesh_line_t args, br_vec2_t const * points, ssize_t len, int stride);

void br_mesh_3d_gen_line(br_mesh_line_3d_t args, br_vec3_t p1, br_vec3_t p2);
void br_mesh_3d_gen_line_strip(br_mesh_line_3d_t args, br_vec3_t const* ps, size_t len);
void br_mesh_3d_gen_line_strip1(br_mesh_line_3d_t args, float const* xs, float const* ys, float const* zs, size_t len);
void br_mesh_3d_gen_line_strip2(br_mesh_line_3d_t args, br_vec2_t const* ps, size_t len);
void br_mesh_3d_gen_line_strip3(br_mesh_line_3d_t args, float const* xs, float const* ys, size_t len);

void br_mesh_grid_draw(br_plot_t* plot, br_shaders_t* shaders);


#ifdef __cplusplus
}
#endif
