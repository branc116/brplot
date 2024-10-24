#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_help.h"

void br_plotter_init_specifics_platform(br_plotter_t* br) {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(br->width, br->height, "brplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  br->shaders = br_shaders_malloc();
  br->text = br_text_renderer_malloc(1024, 1024, br_font_data, &br->shaders.font);
}

void br_plotter_begin_drawing(br_plotter_t* br) {
  BeginDrawing();
  ClearBackground((Color) { 0, 0, 0, 1 });
}
void br_plotter_end_drawing(br_plotter_t* br) {
  EndDrawing();
}
