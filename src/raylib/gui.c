#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_permastate.h"

#include "ui.c"

#include "raylib.h"

static void update_resolution(br_plotter_t* gv);
static void draw_left_panel(br_plotter_t* gv);
void br_gui_init_specifics_gui(br_plotter_t* br) {
  if (false == br->loaded) {
    br_plotter_add_plot_2d(br);
    br_plotter_add_plot_3d(br);
  }
}

BR_API void br_plotter_draw(br_plotter_t* br) {
  BeginDrawing();
  ClearBackground(BLACK);
  update_resolution(br);
  br_plotter_update_variables(br);
  help_draw_fps(0, 0);
  br_plot_t* plot = &br->plots.arr[br->active_plot_index];
  br_plot_update_variables(br, plot, br->groups, GetMousePosition());
  br_plot_update_shader_values(plot, &br->shaders);
  draw_grid_numbers(plot);
  smol_mesh_grid_draw(plot, &br->shaders);
  br_datas_draw(br->groups, plot, &br->shaders);
  draw_left_panel(br);
  EndDrawing();
}

static float sp = 0.f;
static void draw_left_panel(br_plotter_t* br) {
#if 1
  br_plot_t* plot = &br->plots.arr[0];
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
      br_datas_add_test_points(&br->groups);
    }
  }
  Vector2 new_pos = ui_stack_buttons_end();
  new_pos.y += 50;
  ui_stack_buttons_init(new_pos, &sp, context.font_scale * 15);
  for(size_t j = 0; j < br->groups.len; ++j) {
    int res = 0;
    bool yes = true;
    if (context.debug_bounds) {
      res = ui_stack_buttons_add(&yes, "Line #%d: %d;",
        br->groups.arr[j].group_id, br->groups.arr[j].len);
    } else {
      res = ui_stack_buttons_add(&yes, "Line #%d: %d", br->groups.arr[j].group_id, br->groups.arr[j].len);
    }
    if (res > 0) {
      if (IsKeyPressed(KEY_C)) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
          br_data_clear(&br->groups, &br->plots, br->groups.arr[j].group_id);
        }
        else {
          br_data_empty(&br->groups.arr[j]);
        }
      }
    }
  }
  ui_stack_buttons_end();
#endif
}

// TODO 2D/3D
static void update_resolution(br_plotter_t* br) {
  br_plot_t* plot = &br->plots.arr[br->active_plot_index];
  plot->resolution = (Vector2) { (float)GetScreenWidth(), (float)GetScreenHeight() };
  float w = (float)plot->resolution.x - GRAPH_LEFT_PAD - 25.f, h = (float)plot->resolution.y - 50.f;
  plot->graph_screen_rect.x = GRAPH_LEFT_PAD;
  plot->graph_screen_rect.y = 25.f;
  plot->graph_screen_rect.width = w;
  plot->graph_screen_rect.height = h;
}

void br_plot_screenshot(br_plot_t* plot, br_shaders_t* shaders, br_datas_t groups, char const* path) {
  float left_pad = 80.f;
  float bottom_pad = 80.f;
  plot->resolution = (Vector2){1280, 720};
  RenderTexture2D target = LoadRenderTexture((int)plot->resolution.x, (int)plot->resolution.y); // TODO: make this values user defined.
  plot->graph_screen_rect = (Rectangle){left_pad, 0.f, plot->resolution.x - left_pad, plot->resolution.y - bottom_pad};
  br_plot_update_context(plot, GetMousePosition());
  br_plot_update_shader_values(plot, shaders);
  BeginTextureMode(target);
    smol_mesh_grid_draw(plot, shaders);
    br_datas_draw(groups, plot, shaders);
    draw_grid_numbers(plot);
  EndTextureMode();
  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  ExportImage(img, path);
  UnloadImage(img);
  UnloadRenderTexture(target);
}
