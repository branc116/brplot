#pragma once
#include "src/br_plot.h"
#include "src/br_shaders.h"
#include "src/br_filesystem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_plotter_t br_plotter_t;

typedef struct brui_file_manager_t {
  brsp_id_t file_selected;
  brsp_id_t path_id;
  br_fs_files_t cur_dir;
  int select_index;
  bool is_inited;
  bool is_open;
  bool show_hidden_files;
} brui_file_manager_t;

void draw_grid_numbers(br_text_renderer_t* r, br_plot_t* br);
void br_plot_update_variables(br_plotter_t* br, br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos);
bool br_plot_update_variables_2d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos);
bool br_plot_update_variables_3d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos);
void br_plot_remove_group(br_plots_t plots, int group);
void br_plot_update_context(br_plot_t* plot, br_vec2_t mouse_pos);
void br_plot_update_shader_values(br_plot_t* plot, br_shaders_t* shaders);
void brgui_draw_file_manager(brui_file_manager_t* state);


#ifdef __cplusplus
}
#endif

