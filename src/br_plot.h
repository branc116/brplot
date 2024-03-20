#pragma once
#include "raylib.h"
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include "br_shaders.h"
#include "br_pp.h"
#include "br_str.h"
#include "br_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  resampling_dir_null = 0ul,
  resampling_dir_left = 1ul,
  resampling_dir_right = 2ul,
  resampling_dir_up = 4ul,
  resampling_dir_down = 8ul
} resampling_dir;

typedef struct resampling2_t resampling2_t;
typedef struct br_hotreload_state_t br_hotreload_state_t;

typedef enum {
  br_plot_kind_2d,
  br_plot_kind_3d
} br_plot_kind_t;

typedef struct {
  br_shader_line_t* line_shader;
  br_shader_grid_t* grid_shader;
  // graph_rect is in the graph coordinates.
  //            That is if you zoom in and out, graph_rect will change.
  Rectangle graph_rect;
  float recoil;
  Vector2 mouse_pos;
  Vector2 zoom;
  Vector2 offset;
  Vector2 delta;
  bool show_closest, show_x_closest, show_y_closest;
} br_plot_2d_t;

typedef struct {
  br_shader_grid_3d_t* grid_shader;
  br_shader_line_3d_t* line_shader;
  br_shader_line_3d_simple_t* line_simple_shader;
  Vector3 eye, target, up;
  float fov_y, near_plane, far_plane;
  struct {
    int* arr;
    int len, cap; 
  } groups_3d_to_show;
} br_plot_3d_t;

typedef struct br_plot_t {
  struct {
    int* arr;
    int len, cap; 
  } groups_to_show;
  // graph_screen_rect is in the screen coordinates.
  //                   That is if you resize the whole plot, or move the plot around the screen this value will change.
  Rectangle graph_screen_rect;
  Vector2 resolution;
  bool follow;
  bool jump_around;
  bool mouse_inside_graph;

  br_plot_kind_t kind;
  union {
    br_plot_2d_t dd;
    br_plot_3d_t ddd;
  };
} br_plot_t;

typedef struct br_plots_t {
  br_plot_t* arr;
  int len, cap;
} br_plots_t;

typedef struct {
  float font_scale;
  bool debug_bounds;
  size_t alloc_size, alloc_count, alloc_total_size, alloc_total_count, alloc_max_size, alloc_max_count, free_of_unknown_memory;
  char buff[128];
  struct {
    float min_dist_sqr;
    float min_dist_loc;
    br_data_t* closest_to;
  } hover;
} context_t;

extern context_t context;

Vector2 br_graph_to_screen(Rectangle graph_rect, Rectangle screen_rect, Vector2 point);

//void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, Vector2 mid_point, Vector2 tangent, Color color);
//void smol_mesh_gen_quad_simple(smol_mesh_t* mesh, Rectangle rect, Color color);
void smol_mesh_gen_point(br_shader_line_t* mesh, Vector2 point, Color color);
void smol_mesh_gen_point1(br_shader_line_t* mesh, Vector2 point, Vector2 size, Color color);
void smol_mesh_gen_line(br_shader_line_t* mesh, Vector2 p1, Vector2 p2, Color color);
void smol_mesh_gen_line_strip(br_shader_line_t* mesh, Vector2 const * points, size_t len, Color color);
void smol_mesh_gen_line_strip_stride(br_shader_line_t* mesh, Vector2 const * points, ssize_t len, Color color, int stride);
void smol_mesh_gen_bb(br_shader_line_t* mesh, bb_t bb, Color color);
void smol_mesh_grid_draw(br_plot_t* plot);

void smol_mesh_3d_gen_line(br_shader_line_3d_t* shader, Vector3 p1, Vector3 p2, Color color);

resampling2_t* resampling2_malloc(void);
void resampling2_empty(resampling2_t* res);
void resampling2_free(resampling2_t* res);
void resampling2_draw(resampling2_t const* res, br_data_t const* pg, br_plot_t* rdi);
void resampling2_add_point(resampling2_t* res, br_data_t const* pg, uint32_t index);

typedef struct br_plotter_t br_plotter_t;

void br_plot_screenshot(br_plot_t* br, br_datas_t groups, char const* path);
void br_keybinding_handle_keys(br_plotter_t* br, br_plot_t* plot);

#ifndef RELEASE
// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(br_plotter_t* br);
#endif

void read_input_start(br_plotter_t* br);
void read_input_main_worker(br_plotter_t* br);
int  read_input_read_next(void);
void read_input_stop(void);

#ifndef IMGUI
int     ui_draw_button(bool* is_pressed, float x, float y, float font_size, const char* str, ...);
void    ui_stack_buttons_init(Vector2 pos, float* scroll_position, float font_size);
void    ui_stack_set_size(Vector2 v);
int     ui_stack_buttons_add(bool* is_pressed, const char* str, ...);
Vector2 ui_stack_buttons_end(void);
#endif

void    help_trim_zeros(char* buff);
void    help_draw_text(const char *text, Vector2 pos, float fontSize, Color color);
Vector2 help_measure_text(const char* txt, float font_size);
void    help_draw_fps(int posX, int posY);
void    help_load_default_font(void);
void    help_resampling_dir_to_str(char* buff, resampling_dir r);
min_distances_t min_distances_get(Vector2 const* points, size_t points_len, Vector2 to);
void min_distances_get1(min_distances_t* m, Vector2 const* points, size_t points_len, Vector2 to);

#ifdef IMGUI
#ifndef RELEASE
void br_hotreload_start(br_hotreload_state_t* s);
#endif
#endif

#ifdef IMGUI
struct br_file_saver_s;
typedef enum {
  file_saver_state_exploring,
  file_saver_state_accept,
  file_saver_state_cancle
} br_file_saver_state_t;

br_file_saver_state_t br_file_explorer(struct br_file_saver_s* fs);
struct br_file_saver_s* br_file_saver_malloc(char const* title, char const* location, br_plot_t const* plot);
void br_file_saver_get_path(struct br_file_saver_s const* fs, br_str_t* path);
br_plot_t const* br_file_saver_get_plot_instance(struct br_file_saver_s const* fs);
void br_file_saver_free(struct br_file_saver_s* fs);
#endif

#ifdef __cplusplus
}
#endif

