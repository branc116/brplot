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

typedef enum {
  br_drag_mode_none = 0,
  br_drag_mode_left = 1,
  br_drag_mode_right = 2,
  br_drag_mode_top = 4,
  br_drag_mode_bottom = 8,
  br_drag_mode_move = 16,
  br_drag_mode_left_top = br_drag_mode_left | br_drag_mode_top,
  br_drag_mode_right_top = br_drag_mode_right | br_drag_mode_top,
  br_drag_mode_left_bottom = br_drag_mode_left | br_drag_mode_bottom,
  br_drag_mode_right_bottom = br_drag_mode_right | br_drag_mode_bottom,
} br_drag_mode_t;

typedef struct br_plot_2d_t {
  // graph_rect is in the graph coordinates.
  //            That is if you zoom in and out, graph_rect will change.
  br_extent_t graph_rect;

  float recoil;
  br_vec2_t mouse_pos;
  br_vec2_t zoom;
  br_vec2_t offset;
  br_vec2_t delta;
  bool show_closest, show_x_closest, show_y_closest;
} br_plot_2d_t;

typedef struct br_plot_3d_t {
  br_vec3_t eye, target, up;
  float fov_y, near_plane, far_plane;
} br_plot_3d_t;

typedef struct br_plot_t {
  struct {
    int* arr;
    int len, cap; 
  } groups_to_show;
  // graph_screen_rect is in the screen coordinates.
  //                   That is if you resize the whole plot, or move the plot around the screen this value will change.
  br_extenti_t graph_screen_rect;
  br_drag_mode_t drag_mode;
  br_vec2_t drag_point;
  br_extenti_t drag_rect_before;
  br_extenti_t drag_parent_extent;

  unsigned int texture_id;

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
} context_t;

extern context_t context;

br_vec2_t br_graph_to_screen(br_extent_t graph_rect, br_extenti_t screen_rect, br_vec2_t point);

typedef struct br_plotter_t br_plotter_t;

void br_plot_create_texture(br_plot_t* br);
void br_plot_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders);
void br_plot_screenshot(br_text_renderer_t* tr, br_plot_t* br, br_shaders_t* shaders, br_datas_t groups, char const* path);
void br_keybinding_handle_keys(br_plotter_t* br, br_plot_t* plot);


void read_input_start(br_plotter_t* br);
void read_input_main_worker(br_plotter_t* br);
int  read_input_read_next(void);
void read_input_stop(void);

int     ui_draw_button(br_text_renderer_t* tr, bool* is_pressed, float x, float y, int font_size, const char* str, ...);
void    ui_stack_buttons_init(br_vec2_t pos, float* scroll_position, int font_size);
void    ui_stack_set_size(br_vec2_t v);
int     ui_stack_buttons_add(br_text_renderer_t* tr, bool* is_pressed, const char* str, ...);
br_vec2_t ui_stack_buttons_end(void);

void    help_trim_zeros(char* buff);
void    help_draw_text(br_text_renderer_t* renderer, char const* text, br_vec2_t pos, int font_size, br_color_t color);
br_vec2_t help_measure_text(char const* txt, int font_size);
void    help_draw_fps(br_text_renderer_t* renderer, int posX, int posY);
void    help_load_default_font(void);

min_distances_t min_distances_get(br_vec2_t const* points, size_t points_len, br_vec2_t to);
void            min_distances_get1(min_distances_t* m, br_vec2_t const* points, size_t points_len, br_vec2_t to);


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

