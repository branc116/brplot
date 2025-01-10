#include ".generated/icons.h"
#include "src/br_da.h"
#include "src/br_gl.h"
#include "src/br_gui_internal.h"
#include "src/br_math.h"
#include "src/br_permastate.h"
#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_str.h"
#include "src/br_theme.h"
#include "src/br_tl.h"
#include "src/br_ui.h"
#include "src/br_resampling2.h"

static void draw_left_panel(br_plotter_t* gv);
void br_gui_init_specifics_gui(br_plotter_t* br) {
  if (false == br->loaded) {
    br_plotter_add_plot_2d(br);
    br_plotter_add_plot_3d(br);
  }
}

BR_API void br_plotter_draw(br_plotter_t* br) {
  br_plotter_begin_drawing(br);
  brui_resizable_update();
  for (int i = 0; i < br->plots.len; ++i) {
#define PLOT (&br->plots.arr[i])
    brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
    if (true == r->hidden) continue;
    PLOT->cur_extent = r->cur_extent;
    br_plot_update_variables(br, PLOT, br->groups, brtl_mouse_pos());
    br_plot_update_context(PLOT, brtl_mouse_pos());
    br_plot_update_shader_values(PLOT, &br->shaders);
    brgl_enable_framebuffer(PLOT->texture_id, PLOT->cur_extent.width, PLOT->cur_extent.height);
    brgl_clear(BR_COLOR_COMPF(br_theme.colors.plot_bg));
    if (PLOT->kind == br_plot_kind_2d) {
      smol_mesh_grid_draw(PLOT, &br->shaders);
      br_shaders_draw_all(br->shaders); // TODO: This should be called whenever a other shader are being drawn.
      br_datas_draw(br->groups, PLOT, &br->shaders);
      br_shaders_draw_all(br->shaders);
      draw_grid_numbers(br->text, PLOT);
    } else if (PLOT->kind == br_plot_kind_3d) {
      br_datas_draw(br->groups, PLOT, &br->shaders);
      br_shaders_draw_all(br->shaders);
      smol_mesh_grid_draw(PLOT, &br->shaders);
      br_shaders_draw_all(br->shaders);
      draw_grid_numbers(br->text, PLOT);
    }
  }

  brgl_enable_framebuffer(0, br->win.size.width, br->win.size.height);
  brgl_clear(BR_COLOR_COMPF(br_theme.colors.bg));
  help_draw_fps(br->text, 0, 0);

  brui_begin();
    for (int i = 0; i < br->plots.len; ++i) {
      brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
      if (true == r->hidden) continue;
        brui_resizable_push(PLOT->extent_handle);
          brui_img(PLOT->texture_id);
          brui_resizable_t* menu_res = brui_resizable_get(PLOT->menu_extent_handle);
          if (menu_res->hidden == true) {
            if (brui_button_icon(BR_SIZEI(32, 32), br_icons.menu.size_32)) menu_res->hidden = false;
          } else {
            brui_resizable_push(PLOT->menu_extent_handle);
              char* c = br_scrach_get(4096);
              brui_ancor_set(brui_ancor_right_top);
              if (brui_button_icon(BR_SIZEI(32, 32), br_icons.back.size_32)) menu_res->hidden = true;

              brui_text_size_set(8);
              brui_new_lines(1);

              brui_text_size_set(16);
              brui_text_align_set(br_text_renderer_ancor_mid_up);
              int n = sprintf(c, "%s Plot #%d", PLOT->kind == br_plot_kind_2d ? "2D" : "3D", i);
              brui_text(BR_STRV(c, (uint32_t)n));

              brui_new_lines(1);

              brui_text_size_set(32);
              brui_text(BR_STRL("_______________________________________________________"));

              brui_text_size_set(26);
              brui_text_align_set(br_text_renderer_ancor_left_up);
              brui_checkbox(BR_STRL("Follow"), &PLOT->follow);
              for (size_t k = 0; k < br->groups.len; ++k) {
                bool is_shown = false;
                br_data_t* data = &br->groups.arr[k];
                for (int j = 0; j < PLOT->groups_to_show.len; ++j) {
                  if (PLOT->groups_to_show.arr[j] == data->group_id) {
                    is_shown = true;
                    break;
                  }
                }
                sprintf(c, "Data #%d", data->group_id);
                if (brui_checkbox(br_strv_from_c_str(c), &is_shown)) {
                  if (false == is_shown) br_da_remove(PLOT->groups_to_show, data->group_id);
                  else br_da_push_t(int, PLOT->groups_to_show, data->group_id);
                }
              }
              br_scrach_free();
            brui_resizable_pop();
          }
        brui_resizable_pop();
#undef PLOT
    }
    draw_left_panel(br);
  brui_end();
  br_plotter_end_drawing(br);
}

static void draw_left_panel(br_plotter_t* br) {
  brui_resizable_push(br->menu_extent_handle);
    brui_checkbox(BR_STRL("Debug"), &context.debug_bounds);
    if (brui_checkbox(BR_STRL("Dark Theme"), &br->dark_theme)) {
      if (br->dark_theme) br_theme_dark();
      else br_theme_light();
    }
    if (brui_button(BR_STRL("Export"))) {
      br_plotter_export(br, "test.brp");
    }
    if (brui_button(BR_STRL("Export CSV"))) {
      br_plotter_export_csv(br, "test.csv");
    }
    if (brui_button(BR_STRL("Exit"))) {
      br->should_close = true;
    }
    char* scrach = br_scrach_get(4096);
    for (int i = 0; i < br->plots.len; ++i) {
      int n = sprintf(scrach, "%s Plot %d", br->plots.arr[i].kind == br_plot_kind_2d ? "2D" : "3D", i);
      brui_resizable_t* r = brui_resizable_get(br->plots.arr[i].extent_handle);
      bool is_visible = !r->hidden;
      brui_checkbox(BR_STRV(scrach, (uint32_t)n), &is_visible);
      r->hidden = !is_visible;
    }
    brui_text_size_set(16);
    brui_sliderf(BR_STRL("min something"), &context.min_sampling);
    for (size_t i = 0; i < br->groups.len; ++i) {
      brui_push(BR_EXTENT(0, 0, brui_top_width(), 40));
        int n = sprintf(scrach, "Data %d (%zu points)", br->groups.arr[i].group_id, br->groups.arr[i].len);
        brui_text(BR_STRV(scrach, (uint32_t)n));
        n = sprintf(scrach, "%.1fms (%.3f %.3f)", br_resampling2_get_draw_time(br->groups.arr[i].resampling)*1000.0f, br_resampling2_get_something(br->groups.arr[i].resampling), br_resampling2_get_something2(br->groups.arr[i].resampling));
        brui_text(BR_STRV(scrach, (uint32_t)n));
      brui_pop();
    }
    br_scrach_free();
  brui_resizable_pop();
}

void br_plot_screenshot(br_text_renderer_t* tr, br_plot_t* plot, br_shaders_t* shaders, br_datas_t groups, char const* path) {
  (void)tr; (void)plot; (void)shaders; (void)groups; (void)path;
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

