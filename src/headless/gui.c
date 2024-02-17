#include "src/br_plot.h"
#include "src/br_gui_internal.h"

void br_gui_init_specifics_gui(br_plot_t *br) {(void)br;}

void graph_draw(br_plot_t *br) {
  BeginDrawing();
  update_variables(br);
  help_draw_fps(0, 0);
  draw_grid_values(br);
  graph_draw_grid(br->gridShader.shader, br->graph_screen_rect);
  points_groups_draw(&br->groups, (points_groups_draw_in_t) {
    .mouse_pos_graph = br->mouse_graph_pos,
    .rect = br->graph_rect,
    .line_mesh = br->lines_mesh,
    .quad_mesh = br->quads_mesh,
    .line_mesh_3d = br->lines_mesh_3d
  });
  ClearBackground(BLACK);
  EndDrawing();
}

void graph_screenshot(br_plot_t* br, const char* path) {
  (void)br; (void)path;
  LOGI("Grabbing screenshot in headless, NOP\n");
  return;
}
