#pragma once
#include "src/br_math.h"
#include "src/br_data.h"

#include <stddef.h>

#if !defined(BR_PLOT_KIND_T)
typedef enum {
  br_plot_kind_2d,
  br_plot_kind_3d
} br_plot_kind_t;
#define BR_PLOT_KIND_T
#endif

typedef struct br_plot_2d_t {
  float line_thickness;
  float grid_line_thickness;
  float grid_major_line_thickness;

  br_vec2d_t zoom;
  br_vec2d_t offset;
  bool show_closest, show_x_closest, show_y_closest;
} br_plot_2d_t;

typedef struct br_plot_3d_t {
  float line_thickness;
  float grid_line_thickness;
  float grid_major_line_thickness;

  int eye_ah, target_ah;
  br_vec3_t up;
  float fov_y, near_plane, far_plane;
} br_plot_3d_t;

typedef struct br_plot_data_t {
  int group_id;
  int thickness_multiplyer_ah;
} br_plot_data_t;

typedef struct br_plot_t {
  struct {
    br_plot_data_t* arr;
    int len, cap; 
  } data_info;
  int extent_handle;
  int menu_extent_handle;
  int legend_extent_handle;

  unsigned int texture_id;

  int selected_data_old;
  int selected_data;

  bool follow;
  bool jump_around;
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

typedef struct br_anims_t br_anims_t;
void br_plot_construct(br_anims_t* anims);

br_plot_data_t br_plot_data(int group_id);
bool br_plot_data_is_visible(br_plot_data_t pd);

void br_plot_remove_group(br_plot_t* plot, int group_id);
void br_plots_remove_group(br_plots_t plots, int group);

void br_plots_focus_visible(br_plots_t plot, br_datas_t groups);
void br_plot_focus_visible(br_plot_t* plot, br_datas_t groups, br_extent_t ex);

void br_plot_deinit(br_plot_t* plot);
void br_plot_create_texture(br_plot_t* br, br_extent_t ex);

void br_plot2d_move_screen_space(br_plot_t* plot, br_vec2_t delta, br_size_t extent);
void br_plot2d_zoom(br_plot_t* plot, br_vec2_t vec, br_extent_t screen_extent, br_vec2_t mouse_pos_screen);

br_vec2d_t   br_plot2d_to_plot  (br_plot_t* plot, br_vec2_t  vec, br_extent_t ex);
br_vec2_t    br_plot2d_to_screen(br_plot_t* plot, br_vec2d_t vec, br_extent_t ex);
br_extentd_t br_plot2d_extent_to_plot(br_plot_t plot, br_extent_t screen_extent);

br_vec3d_t br_plot3d_to_plot  (br_plot_t* plot, br_vec2_t  mouse_pos, br_extent_t ex);
br_vec2_t  br_plot3d_to_screen(br_plot_t* plot, br_vec3_t pos, br_extent_t ex);
