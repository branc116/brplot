#include "src/br_smol_mesh.h"

void br_smol_mesh_gen_point(br_smol_mesh_line_t args, br_vec2_t point);
void br_smol_mesh_gen_point1(br_smol_mesh_line_t args, br_vec2_t point, br_vec2_t size);
void br_smol_mesh_gen_line(br_smol_mesh_line_t* args, br_vec2_t p1, br_vec2_t p2) { (void)args; (void)p1; (void)p2; }
void br_smol_mesh_gen_line_strip(br_smol_mesh_line_t args, br_vec2_t const * points, size_t len);
void br_smol_mesh_gen_line_strip2(br_smol_mesh_line_t args, float const* xs, float const* ys, size_t len);
void br_smol_mesh_gen_line_strip_stride(br_smol_mesh_line_t args, br_vec2_t const * points, ssize_t len, int stride);

void br_smol_mesh_3d_gen_line(br_smol_mesh_line_3d_t args, br_vec3_t p1, br_vec3_t p2) { (void)args; (void)p1; (void)p2; }
void br_smol_mesh_3d_gen_line_strip(br_smol_mesh_line_3d_t args, br_vec3_t const* ps, size_t len);
void br_smol_mesh_3d_gen_line_strip1(br_smol_mesh_line_3d_t args, float const* xs, float const* ys, float const* zs, size_t len) { (void)args; (void)xs; (void)ys; (void)zs; (void)len; }
void br_smol_mesh_3d_gen_line_strip2(br_smol_mesh_line_3d_t args, br_vec2_t const* ps, size_t len) { (void)args; (void)ps; (void)len; }
void br_smol_mesh_3d_gen_line_strip3(br_smol_mesh_line_3d_t args, float const* xs, float const* ys, size_t len) { (void)args; (void)xs; (void)ys; (void)len; }

void br_smol_mesh_grid_draw(br_plot_t* plot, br_shaders_t* shaders) { (void)plot; (void)shaders; }

void br_shader_line_draw(br_shader_line_t* shader) { (void)shader; }
void br_shader_line_3d_draw(br_shader_line_3d_t* shader) { (void)shader; }
