#pragma once
#include "raylib.h"
#include <stddef.h>
#include <stdio.h>
#include "br_shaders.h"
#include "br_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_hotreload_state_t br_hotreload_state_t;

#if !defined(BR_PLOT_KIND_T)
typedef enum {
  br_plot_kind_2d,
  br_plot_kind_3d
} br_plot_kind_t;
#define BR_PLOT_KIND_T
#endif

typedef struct br_plot_2d_t {
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

typedef struct br_plot_3d_t {
  Vector3 eye, target, up;
  float fov_y, near_plane, far_plane;
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
  bool is_deleted;
  bool is_visible;

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

typedef struct context_t {
  float font_scale;
  bool debug_bounds;
  size_t alloc_size, alloc_count, alloc_total_size, alloc_total_count, alloc_max_size, alloc_max_count, free_of_unknown_memory;
  char buff[128];
  struct {
    float min_dist_sqr;
    float min_dist_loc;
    br_data_t* closest_to;
  } hover;
  br_datas_t* datas;
} context_t;

extern context_t context;

Vector2 br_graph_to_screen(Rectangle graph_rect, Rectangle screen_rect, Vector2 point);

typedef struct br_plotter_t br_plotter_t;

void br_plot_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders);
void br_plot_screenshot(br_plot_t* br, br_shaders_t* shaders, br_datas_t groups, char const* path);
void br_keybinding_handle_keys(br_plotter_t* br, br_plot_t* plot);

void read_input_start(br_plotter_t* br);
void read_input_main_worker(br_plotter_t* br);
int  read_input_read_next(void);
void read_input_stop(void);

void            help_trim_zeros(char* buff);
void            help_draw_text(const char *text, Vector2 pos, float fontSize, Color color);
Vector2         help_measure_text(const char* txt, float font_size);
void            help_draw_fps(int posX, int posY);
void            help_load_default_font(void);
min_distances_t min_distances_get(Vector2 const* points, size_t points_len, Vector2 to);
void            min_distances_get1(min_distances_t* m, Vector2 const* points, size_t points_len, Vector2 to);


#if BR_HAS_SHADER_RELOAD
// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(br_plotter_t* br);
#endif
#if BR_HAS_HOTRELOAD
void br_hotreload_start(br_hotreload_state_t* s);
#endif

#ifdef __cplusplus
}
#endif

