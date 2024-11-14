#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_permastate.h"
#include "src/raylib/ui.c"
#include "src/br_gl.h"

static void update_resolution(br_plotter_t* gv);
static void draw_left_panel(br_plotter_t* gv);
void br_gui_init_specifics_gui(br_plotter_t* br) {
  if (false == br->loaded) {
    br_plotter_add_plot_2d(br);
    br_plotter_add_plot_3d(br);
  }
}

BR_API void br_plotter_draw(br_plotter_t* br) {
  br_plotter_begin_drawing(br);
  update_resolution(br);
  br_plotter_update_variables(br);
  help_draw_fps(br->text, 0, 0);
#define PLOT (&br->plots.arr[br->active_plot_index])
  br_plot_update_variables(br, PLOT, br->groups, brtl_mouse_get_pos());
  br_plot_update_shader_values(PLOT, &br->shaders);
  brgl_enable_framebuffer(PLOT->texture_id);
  draw_grid_numbers(br->text, PLOT);
  br_datas_draw(br->groups, PLOT, &br->shaders);
  smol_mesh_grid_draw(PLOT, &br->shaders);
  brgl_enable_framebuffer(0);
#undef PLOT
  draw_left_panel(br);
  br_text_renderer_dump(br->text);
  br_plotter_end_drawing(br);
}

static float sp = 0.f;
static void draw_left_panel(br_plotter_t* br) {
  br_plot_t* plot = &br->plots.arr[0];
  ui_stack_buttons_init(BR_VEC2(30.f, 25.f), NULL, (int)(context.font_scale * 15));
  ui_stack_buttons_add(br->text, &plot->follow, "Follow");
  if (ui_stack_buttons_add(br->text, NULL, "Export") == 2) {
    br_plotter_export(br, "test.brp");
  }
  if (ui_stack_buttons_add(br->text, NULL, "Export CSV") == 2) {
    br_plotter_export_csv(br, "test.csv");
  }
  if (context.debug_bounds) {
    ui_stack_buttons_add(br->text, &context.debug_bounds, "Debug view");
    ui_stack_buttons_add(br->text, &plot->jump_around, "Jump Around");
    //ui_stack_buttons_add(NULL, "Line draw calls: %d", gv->lines_mesh->draw_calls);
    //ui_stack_buttons_add(NULL, "Points drawn: %d", gv->lines_mesh->points_drawn);
    ui_stack_buttons_add(br->text, NULL, "Recoil: %f", plot->dd.recoil);
    if (2 == ui_stack_buttons_add(br->text, NULL, "Add test points")) {
      br_datas_add_test_points(&br->groups);
    }
  }
  br_vec2_t new_pos = ui_stack_buttons_end();
  new_pos.y += 50;
  ui_stack_buttons_init(new_pos, &sp, (int)(context.font_scale * 15));
  for(size_t j = 0; j < br->groups.len; ++j) {
    int res = 0;
    bool yes = true;
    if (context.debug_bounds) {
      res = ui_stack_buttons_add(br->text, &yes, "Line #%d: %d;",
        br->groups.arr[j].group_id, br->groups.arr[j].len);
    } else {
      res = ui_stack_buttons_add(br->text, &yes, "Line #%d: %d", br->groups.arr[j].group_id, br->groups.arr[j].len);
    }
    if (res > 0) {
      if (brtl_key_is_pressed(BR_KEY_C)) {
        if (brtl_key_shift()) {
          br_data_clear(&br->groups, &br->plots, br->groups.arr[j].group_id);
        }
        else {
          br_data_empty(&br->groups.arr[j]);
        }
      }
    }
  }
  ui_stack_buttons_end();
}

// TODO 2D/3D
static void update_resolution(br_plotter_t* br) {
  br_plot_t* plot = &br->plots.arr[br->active_plot_index];
  plot->resolution = br->win.size;
  int w = plot->resolution.width - 400 - 25, h = plot->resolution.height - 50;
  plot->graph_screen_rect = BR_EXTENTI(400, 25, w, h);
}

void br_plot_screenshot(br_text_renderer_t* tr, br_plot_t* plot, br_shaders_t* shaders, br_datas_t groups, char const* path) {
#// TODO: Make this more better...
//  int left_pad = 80;
//  int bottom_pad = 80;
//  plot->resolution = BR_SIZEI(1280, 720);
//  RenderTexture2D target = LoadRenderTexture(plot->resolution.width, plot->resolution.height); // TODO: make this values user defined.
//  plot->graph_screen_rect = BR_EXTENTI(left_pad, 0, plot->resolution.width - left_pad, plot->resolution.height - bottom_pad);
//  br_plot_update_context(plot, brtl_mouse_get_pos());
//  br_plot_update_shader_values(plot, shaders);
//  BeginTextureMode(target);
//    smol_mesh_grid_draw(plot, shaders);
//    br_datas_draw(groups, plot, shaders);
//    draw_grid_numbers(tr, plot);
//  EndTextureMode();
//  Image img = LoadImageFromTexture(target.texture);
//  ImageFlipVertical(&img);
//  ExportImage(img, path);
//  UnloadImage(img);
//  UnloadRenderTexture(target);
}
