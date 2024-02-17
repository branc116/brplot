#include "src/br_plot.h"
#include "src/br_gui_internal.h"

#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "raylib.h"

void emscripten_run_script(const char* script);
static void update_resolution(br_plot_t* gv);
static void draw_left_panel(br_plot_t* gv);
void br_gui_init_specifics_gui(br_plot_t* br) {(void)br;}

BR_API void graph_draw(br_plot_t* br) {
  BeginDrawing();
  ClearBackground(BLACK);
  update_resolution(br);
  update_variables(br);
  help_draw_fps(0, 0);
  draw_grid_values(br);
  graph_draw_grid(br->gridShader.shader, br->graph_screen_rect);
  points_groups_draw(&br->groups, (points_groups_draw_in_t) {
    .mouse_pos_graph = br->mouse_graph_pos,
    .rect = br->graph_rect,
    .line_mesh = br->lines_mesh,
    .quad_mesh = br->quads_mesh,
    .line_mesh_3d = br->lines_mesh_3d,
    .show_x_closest = br->show_x_closest,
    .show_y_closest = br->show_y_closest,
    .show_closest = br->show_closest
  });
  draw_left_panel(br);
  EndDrawing();
}

static float sp = 0.f;
static void draw_left_panel(br_plot_t* gv) {
  ui_stack_buttons_init((Vector2){.x = 30.f, .y = 25.f}, NULL, context.font_scale * 15);
  ui_stack_buttons_add(&gv->follow, "Follow");
  if (ui_stack_buttons_add(NULL, "Export") == 2) {
    graph_export(gv, "test.brp");
  }
  if (ui_stack_buttons_add(NULL, "Export CSV") == 2) {
    graph_export_csv(gv, "test.csv");
  }
  if (context.debug_bounds) {
    ui_stack_buttons_add(&context.debug_bounds, "Debug view");
    ui_stack_buttons_add(&gv->jump_around, "Jump Around");
    ui_stack_buttons_add(NULL, "Line draw calls: %d", gv->lines_mesh->draw_calls);
    ui_stack_buttons_add(NULL, "Points drawn: %d", gv->lines_mesh->points_drawn);
    ui_stack_buttons_add(NULL, "Recoil: %f", gv->recoil);
    if (2 == ui_stack_buttons_add(NULL, "Add test points")) {
      points_groups_add_test_points(&gv->groups);
    }
  }
  Vector2 new_pos = ui_stack_buttons_end();
  new_pos.y += 50;
  ui_stack_buttons_init(new_pos, &sp, context.font_scale * 15);
  for(size_t j = 0; j < gv->groups.len; ++j) {
    int res = 0;
    if (context.debug_bounds) {
      res = ui_stack_buttons_add(&gv->groups.arr[j].is_selected, "Line #%d: %d;",
        gv->groups.arr[j].group_id, gv->groups.arr[j].len);
    } else {
      res = ui_stack_buttons_add(&gv->groups.arr[j].is_selected, "Line #%d: %d", gv->groups.arr[j].group_id, gv->groups.arr[j].len);
    }
    if (res > 0) {
      if (IsKeyPressed(KEY_C)) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) points_group_clear(&gv->groups, gv->groups.arr[j].group_id);
        else                           points_group_empty(&gv->groups.arr[j]);
      }
    }
  }
  ui_stack_buttons_end();
}

static void update_resolution(br_plot_t* gv) {
  gv->uvScreen.x = (float)GetScreenWidth();
  gv->uvScreen.y = (float)GetScreenHeight();
  float w = gv->uvScreen.x - (float)GRAPH_LEFT_PAD - 20.f, h = gv->uvScreen.y - 50.f;
  gv->graph_screen_rect.x = (float)GRAPH_LEFT_PAD;
  gv->graph_screen_rect.y = 25.f;
  gv->graph_screen_rect.width = w;
  gv->graph_screen_rect.height = h;
}

void graph_screenshot(br_plot_t* gv, char const * path) {
  float left_pad = 80.f;
  float bottom_pad = 80.f;
  Vector2 is = {1280, 720};
  RenderTexture2D target = LoadRenderTexture((int)is.x, (int)is.y); // TODO: make this values user defined.
  gv->graph_screen_rect = (Rectangle){left_pad, 0.f, is.x - left_pad, is.y - bottom_pad};
  gv->uvScreen = (Vector2){is.x, is.y};
  graph_update_context(gv);
  update_shader_values(gv);
  BeginTextureMode(target);
    graph_draw_grid(gv->gridShader.shader, gv->graph_screen_rect);
    points_groups_draw(&gv->groups, (points_groups_draw_in_t) { .mouse_pos_graph = gv->mouse_graph_pos,
        .rect = gv->graph_rect,
        .line_mesh = gv->lines_mesh,
        .quad_mesh = gv->quads_mesh,
        .line_mesh_3d = gv->lines_mesh_3d
    });
    draw_grid_values(gv);
  EndTextureMode();
  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  ExportImage(img, path);
  UnloadImage(img);
  UnloadRenderTexture(target);

#if defined(PLATFORM_WEB)
  // Download file from MEMFS (emscripten memory filesystem)
  // saveFileFromMEMFSToDisk() function is defined in raylib/src/shell.html
  emscripten_run_script(TextFormat("saveFileFromMEMFSToDisk('%s','%s')", GetFileName(path), GetFileName(path)));
#endif
}
