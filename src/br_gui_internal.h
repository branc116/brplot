#pragma once
#include "br_plot.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_plotter_t br_plotter_t;

void draw_grid_numbers(br_plot_t* br);
void br_plotter_update_context(br_plotter_t* br, Vector2 mouse_pos);
void br_plotter_update_variables(br_plotter_t* br);
bool br_plot_update_variables_2d(br_plot_t* plot, points_groups_t const groups, Vector2 mouse_pos);
bool br_plot_update_variables_3d(br_plot_t* plot, points_groups_t const groups, Vector2 mouse_pos);
void br_plot_remove_group(br_plots_t plots, int group);
void br_plot_update_context(br_plot_t* plot, Vector2 mouse_pos);
void br_plot_update_shader_values(br_plot_t* plot);

void br_gui_init_specifics_gui(br_plotter_t* br);
void br_gui_init_specifics_platform(br_plotter_t* br);
void br_gui_free_specifics(br_plotter_t* br);

#ifdef __cplusplus
}
#endif

