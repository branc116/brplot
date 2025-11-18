#pragma once
#include "src/br_pp.h"
#include "src/br_data.h"
#include "src/br_plot.h"
#include "src/br_data_generator.h"
#include "src/br_math.h"
#include "src/br_theme.h"
#include "src/br_permastate.h"
#include "src/br_ui.h"
#include "src/br_gui.h"
#include "src/br_platform.h"

typedef struct br_plotter_t br_plotter_t;
typedef struct q_commands q_commands;
typedef struct br_text_renderer_t br_text_renderer_t;

#if BR_HAS_HOTRELOAD
typedef struct br_hotreload_state_t {
  void (*func_loop)(struct br_plotter_t* gv);
  void (*func_loop_ui)(struct br_plotter_t* gv);
  void (*func_init)(struct br_plotter_t* gv);
  bool is_init_called;
  bool has_changed;
  void* handl;
  br_plotter_t* plotter;
} br_hotreload_state_t;
#endif

typedef enum br_plotter_entity_kind_t {
  br_plotter_entity_none,
  br_plotter_entity_plot_2d,
  br_plotter_entity_plot_3d,
  br_plotter_entity_ui,
  br_plotter_entity_text_input,
} br_plotter_entity_kind_t;

typedef struct br_plotter_t {
  br_datas_t groups;
  br_plots_t plots;
  br_shaders_t shaders;
  br_dagens_t dagens;
  bruirs_t resizables;
  br_csv_parser_t csv_parser;
  brsp_t sp;

  br_text_renderer_t* text;
  // Any thread can write to this q, only render thread can pop
  q_commands* commands;

  brpl_window_t win;

  struct {
    br_plotter_entity_kind_t active;
    union {
      int plot_id;
      int resizable_id;
      brsp_id_t input;
    };
  } action, hovered;

  struct {
    brpl_touch_point_t* arr;
    int* free_arr;

    int len, cap;
    int free_len;
    int free_next;

    double last_free_time;
    double last_touch_time;
  } touch_points;

  struct {
    br_vec2_t old_pos;
    br_vec2_t pos;
    br_vec2_t delta;
    bool dragging_left;
    bool dragging_right;
    bool click;
    bool active;
  } mouse;

  struct {
    bool ctrl_down;
    bool shift_down;
    bool alt_down;
    bool down[255];
  } key;

  struct {
    double old;
    double now;
    double frame;
  } time;

#if BR_HAS_HOTRELOAD
  br_hotreload_state_t hot_state;
#endif

  br_permastate_status_t loaded_status;
#if BR_HAS_SHADER_RELOAD
  bool shaders_dirty;
#endif
  bool should_close;
  bool exited;
  struct {
    br_theme_t theme;
    brsp_id_t csv_file_opened;
    brgui_file_manager_t fm_state;
    brgui_csv_reader_t csv_state;
    brsp_id_t font_path_id;
    brgui_add_expression_t add_expression;
    brgui_show_data_t show_data;
    struct {
      bool show;
      bool key_bindings;
      bool cli_help;
      bool c_help;
      bool python_help;
    } help;
    struct {
      int selected_frame;
      int selected_nid;
      bool show;
    } memory;
    bool dark_theme;
    bool multisampling;
    bool expand_file;
    bool expand_plots;
    bool expand_optimizations;
    bool expand_ui_styles;
    bool expand_export;
    bool expand_data;
    bool expand_about;
    bool show_license;
    bool show_about;
    bool show_log;
  } ui;
} br_plotter_t;

br_plotter_t* br_plotter_malloc(void);
void        br_plotter_init(br_plotter_t* br);
void        br_plotter_deinit(br_plotter_t* br);
void        br_plotter_free(br_plotter_t* plotter);

void        br_plotter_one_iter(br_plotter_t* br);
void        br_plotter_update(br_plotter_t* br);

void        br_plotter_resize(br_plotter_t* br, float width, float height);
br_datas_t* br_plotter_get_br_datas(br_plotter_t* br);
void        br_plotter_switch_2d(br_plotter_t* br);
void        br_plotter_switch_3d(br_plotter_t* br);
void        br_plots_update_variables(br_plotter_t* br, br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos);
int         br_plotter_add_plot_2d(br_plotter_t* br);
int         br_plotter_add_plot_3d(br_plotter_t* br);
void        br_plotter_remove_plot(br_plotter_t* br, int plot_index);
void        br_plotter_data_remove(br_plotter_t* br, int group_id);
void        br_plotter_export(br_plotter_t const* br, char const* path);
void        br_plotter_export_csv(br_plotter_t const* br, char const* path);
void        br_plotter_draw(br_plotter_t* br);
void        br_plotter_minimal(br_plotter_t* br);
void        br_plotter_frame_end(br_plotter_t* br);
void        br_plotter_datas_deinit(br_plotter_t* br);
void        br_plotter_datas_deinit_in_plot(br_plotter_t* br, int plot_id);
void        br_plotter_datas_empty_in_plot(br_plotter_t* br, int plot_id);

// Platform specific
void br_plotter_init_specifics_platform(br_plotter_t* br, int width, int height);
void br_plotter_deinit_specifics_platform(br_plotter_t* br);
void br_plotter_begin_drawing(br_plotter_t* br);
void br_plotter_end_drawing(br_plotter_t* br);

#if BR_HAS_HOTRELOAD
void br_hotreload_start(br_hotreload_state_t* s);
void br_hotreload_tick(br_hotreload_state_t* s);
void br_hotreload_tick_ui(br_hotreload_state_t* s);
#endif

void br_read_input_start(br_plotter_t* br);
void br_read_input_stop(void);

