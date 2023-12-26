#pragma once
#include "br_plot.h"

#ifdef __cplusplus
extern "C" {
#endif

void draw_grid_values(br_plot_t* br);
void graph_update_context(br_plot_t* br);
void graph_draw_grid(Shader shader, Rectangle screen_rect);
void update_shader_values(br_plot_t* br);
void update_variables(br_plot_t* br);
void br_gui_init_specifics_gui(br_plot_t* br);
void br_gui_init_specifics_platform(br_plot_t* br);
void br_gui_free_specifics(br_plot_t* br);

#ifdef __cplusplus
}
#endif

