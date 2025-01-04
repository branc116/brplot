#pragma once
#include "src/br_shaders.h"
#include "src/br_data.h"
#include "src/br_math.h"

#ifdef __cplusplus
extern "C" {
#endif

//void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, br_vec2_t mid_point, br_vec2_t tangent, br_color_t color);
//void smol_mesh_gen_quad_simple(smol_mesh_t* mesh, Rectangle rect, br_color_t color);
void smol_mesh_3d_gen_line_strip(br_shader_line_3d_t* shader, br_vec3_t const* ps, size_t len, br_color_t color);
void smol_mesh_3d_gen_line_strip1(br_shader_line_3d_t* shader, float const* xs, float const* ys, float const* zs, size_t len, br_color_t color);
void smol_mesh_3d_gen_line_strip2(br_shader_line_3d_t* shader, br_vec2_t const* ps, size_t len, br_color_t color);
void smol_mesh_3d_gen_line_strip3(br_shader_line_3d_t* shader, float const* xs, float const* ys, size_t len, br_color_t color);
void smol_mesh_gen_point(br_shader_line_t* mesh, br_vec2_t point, br_color_t color);
void smol_mesh_gen_point1(br_shader_line_t* mesh, br_vec2_t point, br_vec2_t size, br_color_t color);
void smol_mesh_gen_line(br_shader_line_t* mesh, br_vec2_t p1, br_vec2_t p2, br_color_t color);
void smol_mesh_gen_line_strip(br_shader_line_t* mesh, br_vec2_t const * points, size_t len, br_color_t color);
void smol_mesh_gen_line_strip2(br_shader_line_t* shader, float const* xs, float const* ys, size_t len, br_color_t color);
void smol_mesh_gen_line_strip_stride(br_shader_line_t* mesh, br_vec2_t const * points, ssize_t len, br_color_t color, int stride);
void smol_mesh_gen_bb(br_shader_line_t* mesh, bb_t bb, br_color_t color);
void smol_mesh_grid_draw(br_plot_t* plot, br_shaders_t* shaders);

void smol_mesh_3d_gen_line(br_shader_line_3d_t* shader, br_vec3_t p1, br_vec3_t p2, br_color_t color);

#ifdef __cplusplus
}
#endif
