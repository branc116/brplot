#pragma once
#include "src/br_plot.h"
#include "src/br_shaders.h"
#include "src/br_filesystem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_plotter_t br_plotter_t;

typedef enum brgui_file_manager_action_t {
  brgui_file_manager_load_font,
  brgui_file_manager_import_csv,
} brgui_file_manager_action_t;

typedef struct brgui_file_manager_t {
  brsp_id_t file_selected;
  brsp_id_t path_id;
  brgui_file_manager_action_t action;
  int select_index;
  bool is_inited;
  bool is_open;
  bool show_hidden_files;
  bool has_tabed;
  bool has_entered;

  br_fs_files_t cur_dir;
} brgui_file_manager_t;

typedef struct brgui_add_expression_t {
  brsp_id_t input_id;
  bool show;
} brgui_add_expression_t;

typedef struct brgui_show_data_t {
  int data_id;
  bool show;
} brgui_show_data_t;

typedef struct brgui_fm_result_t {
  bool is_selected;
  brsp_id_t selected_file;
  int resizable_handle;
} brgui_fm_result_t;

typedef enum br_csv_state_t {
  br_csv_state_init,
  br_csv_state_file_read_error,
  br_csv_state_parse_error,
  br_csv_state_ok
} br_csv_state_t;

typedef struct br_csv_header_t {
  br_strv_t* arr;
  size_t len, cap;
} br_csv_header_t;

typedef struct br_csv_cells_t {
  br_strv_t* arr;
  size_t len, cap;
} br_csv_cells_t;

typedef struct br_csv_rows_t {
  br_csv_cells_t* arr;
  size_t len, cap;

  size_t real_len;
} br_csv_rows_t;

typedef struct br_csv_parser_t {
  br_csv_header_t header;
  br_csv_rows_t rows;
  br_str_t file_path;
  br_str_t content;
  br_csv_state_t state;
} br_csv_parser_t;

typedef struct brgui_csv_reader_t {
  bool is_open;
  int first_coord;
  brsp_id_t read_id;
} brgui_csv_reader_t;

void br_plot_update_context(br_plot_t* plot, br_extent_t plot_screen_extent, br_vec2_t mouse_pos);
void br_plot_update_shader_values(br_plot_t* plot, br_shaders_t* shaders);
brgui_fm_result_t brgui_draw_file_manager(br_plotter_t* br, brsp_t* sp, brgui_file_manager_t* state);


#ifdef __cplusplus
}
#endif

