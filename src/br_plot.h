#pragma once
#include "src/br_shaders.h"
#include "src/br_data.h"
#include "src/br_text_renderer.h"
#include "src/br_math.h"
#include "src/br_data.h"

#include <stddef.h>

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
  br_extentd_t graph_rect;

  float line_thickness;
  float grid_line_thickness;
  float grid_major_line_thickness;

  br_vec2d_t mouse_pos;
  br_vec2d_t zoom;
  br_vec2d_t offset;
  bool show_closest, show_x_closest, show_y_closest;
} br_plot_2d_t;

typedef struct br_plot_3d_t {
  float line_thickness;
  float grid_line_thickness;
  float grid_major_line_thickness;

  br_vec3_t eye, target, up;
  float fov_y, near_plane, far_plane;
} br_plot_3d_t;

typedef struct br_plot_data_t {
  int group_id;
  float thickness_multiplyer;
  float thickness_multiplyer_target;
} br_plot_data_t;
#define BR_PLOT_DATA(GROUP_ID) ((br_plot_data_t) { .group_id = (GROUP_ID), .thickness_multiplyer = 0.f, .thickness_multiplyer_target = 1.f })
#define BR_PLOT_DATA_IS_VISIBLE(PD) ((PD).thickness_multiplyer > 0.1f)


typedef struct br_plot_t {
  struct {
    br_plot_data_t* arr;
    int len, cap; 
  } data_info;
  br_extenti_t cur_extent;
  int extent_handle;
  int menu_extent_handle;
  int legend_extent_handle;

  unsigned int texture_id;

  int selected_data_old;
  int selected_data;

  bool follow;
  bool jump_around;
  bool mouse_inside_graph;
  bool is_deleted;

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

typedef struct br_plotter_t br_plotter_t;

br_vec2_t br_graph_to_screen(br_extent_t graph_rect, br_extenti_t screen_rect, br_vec2_t point);

void br_plots_remove_group(br_plots_t plots, int group);

void br_plot_deinit(br_plot_t* plot);
void br_plot_create_texture(br_plot_t* br);
void br_plot_draw(br_plot_t* plot, br_datas_t datas);
void br_plot_screenshot(br_text_renderer_t* tr, br_plot_t* br, br_shaders_t* shaders, br_datas_t groups, char const* path);
void br_keybinding_handle_keys(br_plotter_t* br, br_plot_t* plot);

void read_input_start(br_plotter_t* br);
void read_input_main_worker(br_plotter_t* br);
int  read_input_read_next(void);
void read_input_stop(void);

min_distances_t min_distances_get(br_vec2_t const* points, size_t points_len, br_vec2_t to);
void            min_distances_get1(min_distances_t* m, br_vec2_t const* points, size_t points_len, br_vec2_t to);


#if BR_HAS_SHADER_RELOAD
// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(br_plotter_t* br);
#endif
#if BR_HAS_HOTRELOAD
void br_hotreload_start(br_hotreload_state_t* s);
void br_hotreload_tick_ui(br_hotreload_state_t* s);
#endif

#ifdef __cplusplus
}
#endif

