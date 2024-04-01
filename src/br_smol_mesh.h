#pragma once
#include "br_shaders.h"
#include "br_data.h"

#ifdef __cplusplus
extern "C" {
#endif

//void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, Vector2 mid_point, Vector2 tangent, Color color);
//void smol_mesh_gen_quad_simple(smol_mesh_t* mesh, Rectangle rect, Color color);
void smol_mesh_3d_gen_line_strip(br_shader_line_3d_t* shader, Vector3 const* ps, size_t len, Color color);
void smol_mesh_3d_gen_line_strip2(br_shader_line_3d_t* shader, Vector2 const* ps, size_t len, Color color);
void smol_mesh_gen_point(br_shader_line_t* mesh, Vector2 point, Color color);
void smol_mesh_gen_point1(br_shader_line_t* mesh, Vector2 point, Vector2 size, Color color);
void smol_mesh_gen_line(br_shader_line_t* mesh, Vector2 p1, Vector2 p2, Color color);
void smol_mesh_gen_line_strip(br_shader_line_t* mesh, Vector2 const * points, size_t len, Color color);
void smol_mesh_gen_line_strip_stride(br_shader_line_t* mesh, Vector2 const * points, ssize_t len, Color color, int stride);
void smol_mesh_gen_bb(br_shader_line_t* mesh, bb_t bb, Color color);
void smol_mesh_grid_draw(br_plot_t* plot);

void smol_mesh_3d_gen_line(br_shader_line_3d_t* shader, Vector3 p1, Vector3 p2, Color color);

#ifdef __cplusplus
}
#endif
