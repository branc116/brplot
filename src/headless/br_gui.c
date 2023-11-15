#include "../br_plot.h"
#include "../br_gui_internal.h"

void br_gui_init_specifics(br_plot_t *br) {(void)br;}

void graph_draw(br_plot_t *br) {
  BeginDrawing();
  update_variables(br);
  help_draw_fps(0, 0);
  draw_grid_values(br);
  graph_draw_grid(br->gridShader.shader, br->graph_screen_rect);
  points_groups_draw(&br->groups, br->lines_mesh, br->quads_mesh, br->graph_rect);
  ClearBackground(BLACK);
  EndDrawing();
}
