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
#include "src/br_da.h"

void emscripten_run_script(const char* script);
static void update_resolution(br_plotter_t* gv);
static void draw_left_panel(br_plotter_t* gv);
void br_gui_init_specifics_gui(br_plotter_t* br) {
  br_plot_instance_t plot = {
    .kind = br_plot_instance_kind_2d,
    .graph_screen_rect = { GRAPH_LEFT_PAD, 50, (float)GetScreenWidth() - GRAPH_LEFT_PAD - 60, (float)GetScreenHeight() - 110 },
    .follow = false,
    .dd = {
      .line_shader = br->shaders.line,
      .grid_shader = br->shaders.grid,
      .zoom = (Vector2) { .x = 1.f, .y = 1.f },
      .offset = { 0 }
    }
  };
  br_da_push_t(int, br->plots, plot);
}

BR_API void br_plotter_draw(br_plotter_t* br) {
  BeginDrawing();
  ClearBackground(BLACK);
  update_resolution(br);
  br_plotter_update_variables(br);
  help_draw_fps(0, 0);
  draw_grid_values(&br->plots.arr[0]);
  br_plotter_draw_grid(&br->plots.arr[0]);
  points_groups_draw(&br->groups, &br->plots.arr[0]);
  draw_left_panel(br);
  EndDrawing();
}

static float sp = 0.f;
static void draw_left_panel(br_plotter_t* br) {
#if 1
  br_plot_instance_t* plot = &br->plots.arr[0];
  ui_stack_buttons_init((Vector2){.x = 30.f, .y = 25.f}, NULL, context.font_scale * 15);
  ui_stack_buttons_add(&plot->follow, "Follow");
  if (ui_stack_buttons_add(NULL, "Export") == 2) {
    br_plotter_export(br, "test.brp");
  }
  if (ui_stack_buttons_add(NULL, "Export CSV") == 2) {
    br_plotter_export_csv(br, "test.csv");
  }
  if (context.debug_bounds) {
    ui_stack_buttons_add(&context.debug_bounds, "Debug view");
    ui_stack_buttons_add(&plot->jump_around, "Jump Around");
    //ui_stack_buttons_add(NULL, "Line draw calls: %d", gv->lines_mesh->draw_calls);
    //ui_stack_buttons_add(NULL, "Points drawn: %d", gv->lines_mesh->points_drawn);
    ui_stack_buttons_add(NULL, "Recoil: %f", plot->dd.recoil);
    if (2 == ui_stack_buttons_add(NULL, "Add test points")) {
      points_groups_add_test_points(&br->groups);
    }
  }
  Vector2 new_pos = ui_stack_buttons_end();
  new_pos.y += 50;
  ui_stack_buttons_init(new_pos, &sp, context.font_scale * 15);
  for(size_t j = 0; j < br->groups.len; ++j) {
    int res = 0;
    if (context.debug_bounds) {
      res = ui_stack_buttons_add(&br->groups.arr[j].is_selected, "Line #%d: %d;",
        br->groups.arr[j].group_id, br->groups.arr[j].len);
    } else {
      res = ui_stack_buttons_add(&br->groups.arr[j].is_selected, "Line #%d: %d", br->groups.arr[j].group_id, br->groups.arr[j].len);
    }
    if (res > 0) {
      if (IsKeyPressed(KEY_C)) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) points_group_clear(&br->groups, br->groups.arr[j].group_id);
        else                           points_group_empty(&br->groups.arr[j]);
      }
    }
  }
  ui_stack_buttons_end();
#endif
}

// TODO 2D/3D
static void update_resolution(br_plotter_t* br) {
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_instance_t* plot = &br->plots.arr[i];
    assert(plot->kind == br_plot_instance_kind_2d);
    float w = (float)GetScreenWidth() - (float)GRAPH_LEFT_PAD - 20.f, h = (float)GetScreenHeight() - 50.f;
    plot->graph_screen_rect.x = (float)GRAPH_LEFT_PAD;
    plot->graph_screen_rect.y = 25.f;
    plot->graph_screen_rect.width = w;
    plot->graph_screen_rect.height = h;
    plot->dd.line_shader->uvs.screen_uv = (Vector2) { (float)GetScreenWidth(), (float)GetScreenHeight() };
    plot->dd.grid_shader->uvs.screen_uv = (Vector2) { w, h };
  }
}

void br_plotter_screenshot(br_plotter_t* gv, char const * path) {
  assert(false);
#if 0
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
#endif
}
