#pragma once
#include "br_data.h"
#include "br_plot.h"
#include "br_pp.h"
#include "br_data_generator.h"
#include "br_math.h"
#include "br_theme.h"

#ifdef __cplusplus
extern "C" {
#endif

struct br_plotter_t;
typedef struct q_commands q_commands;
typedef struct br_text_renderer_t br_text_renderer_t;
typedef struct GLFWwindow GLFWwindow;

#if BR_HAS_HOTRELOAD
typedef struct br_hotreload_state_t {
  LOCK(lock)
  void (*func_loop)(struct br_plotter_t* gv);
  void (*func_init)(struct br_plotter_t* gv);
  bool is_init_called;
  void* handl;
} br_hotreload_state_t;
#endif

typedef struct br_plotter_t {
  br_datas_t groups;
  br_plots_t plots;
  br_shaders_t shaders;
  br_dagens_t dagens;

  br_text_renderer_t* text;
  // Any thread can write to this q, only render thread can pop
  q_commands* commands;

  struct {
    GLFWwindow* glfw;
    br_sizei_t size;
    br_extenti_t viewport;
  } win;

  struct {
    br_vec2_t old_pos;
    br_vec2_t pos;
    br_vec2_t delta;
    br_vec2_t scroll;
    struct {
      bool left:1;
      bool right:1;
    } down;
    struct {
      bool left:1;
      bool right:1;
    } pressed;
  } mouse;

  struct {
    bool down[64];
    bool pressed[64];
    int mod;
  } key;

  struct {
    double old; 
    double now; 
    double frame; 
  } time;

#if BR_HAS_HOTRELOAD
  br_hotreload_state_t hot_state;
#endif
  int active_plot_index;
  int menu_extent_handle;

  bool loaded;
#if BR_HAS_SHADER_RELOAD
  bool shaders_dirty;
#endif
  bool should_close;
  bool switch_to_active;
  bool any_2d, any_3d;
  struct {
    br_theme_t theme;
    bool file_saver_inited;
    bool dark_theme;
    bool expand_plots;
    bool expand_optimizations;
    bool expand_ui_styles;
    bool expand_export;
    bool expand_data;
  } ui;
} br_plotter_t;

BR_API br_plotter_t* br_plotter_malloc(void);
BR_API void        br_plotter_init(br_plotter_t* br);
BR_API void        br_plotter_resize(br_plotter_t* br, float width, float height);
BR_API br_datas_t* br_plotter_get_br_datas(br_plotter_t* br);
BR_API void        br_plotter_switch_2d(br_plotter_t* br);
BR_API void        br_plotter_switch_3d(br_plotter_t* br);
BR_API void        br_plotter_set_bottom_left(br_plot_t* plot, float left, float bottom);
BR_API void        br_plotter_set_top_right(br_plot_t* plot, float right, float top);
BR_API void        br_plots_focus_visible(br_plots_t plot, br_datas_t groups);
BR_API void        br_plot_focus_visible(br_plot_t* plot, br_datas_t groups);
int                br_plotter_add_plot_2d(br_plotter_t* br);
int                br_plotter_add_plot_3d(br_plotter_t* br);
void               br_plotter_remove_plot(br_plotter_t* br, int plot_index);
void               br_plotter_export(br_plotter_t const* br, char const* path);
void               br_plotter_export_csv(br_plotter_t const* br, char const* path);
BR_API void        br_plotter_free(br_plotter_t* br);
BR_API void        br_plotter_draw(br_plotter_t* br);
BR_API void        br_plotter_minimal(br_plotter_t* br);
BR_API void        br_plotter_frame_end(br_plotter_t* br);
BR_API void        br_plotter_datas_deinit(br_plotter_t* br);

// Platform specific
void br_plotter_wait(br_plotter_t const* br);
void br_plotter_init_specifics_platform(br_plotter_t* br, int width, int height);
void br_plotter_begin_drawing(br_plotter_t* br);
void br_plotter_end_drawing(br_plotter_t* br);

#ifdef __cplusplus
}
#endif

