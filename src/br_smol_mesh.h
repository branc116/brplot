#pragma once
#include "src/br_shaders.h"
#include "src/br_data.h"
#include "src/br_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_smol_mesh_line_t {
  br_vec2_t zoom, offset, screen_size;
} br_smol_mesh_line_t;

//void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, br_vec2_t mid_point, br_vec2_t tangent, br_color_t color);
//void smol_mesh_gen_quad_simple(smol_mesh_t* mesh, Rectangle rect, br_color_t color);

void smol_mesh_gen_bb(br_smol_mesh_line_t args, bb_t bb);
void smol_mesh_gen_point(br_smol_mesh_line_t args, br_vec2_t point);
void smol_mesh_gen_point1(br_smol_mesh_line_t args, br_vec2_t point, br_vec2_t size);
void smol_mesh_gen_line(br_smol_mesh_line_t args, br_vec2_t p1, br_vec2_t p2);
void smol_mesh_gen_line_strip(br_smol_mesh_line_t args, br_vec2_t const * points, size_t len);
void smol_mesh_gen_line_strip2(br_smol_mesh_line_t args, float const* xs, float const* ys, size_t len);
void smol_mesh_gen_line_strip_stride(br_smol_mesh_line_t args, br_vec2_t const * points, ssize_t len, int stride);

void smol_mesh_3d_gen_line(br_shader_line_3d_t* shader, br_vec3_t p1, br_vec3_t p2, br_color_t color);
void smol_mesh_3d_gen_line_strip(br_shader_line_3d_t* shader, br_vec3_t const* ps, size_t len, br_color_t color);
void smol_mesh_3d_gen_line_strip1(br_shader_line_3d_t* shader, float const* xs, float const* ys, float const* zs, size_t len, br_color_t color);
void smol_mesh_3d_gen_line_strip2(br_shader_line_3d_t* shader, br_vec2_t const* ps, size_t len, br_color_t color);
void smol_mesh_3d_gen_line_strip3(br_shader_line_3d_t* shader, float const* xs, float const* ys, size_t len, br_color_t color);

void smol_mesh_grid_draw(br_plot_t* plot, br_shaders_t* shaders);


#ifdef __cplusplus
}
#endif
