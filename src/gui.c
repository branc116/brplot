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
BR_API void br_plotter_draw(br_plotter_t* br) {
  br_plotter_begin_drawing(br);
  brui_resizable_update();
  for (int i = 0; i < br->plots.len; ++i) {
#define PLOT br_da_getp(br->plots, i)
    brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
    if (true == r->hidden) continue;
    PLOT->cur_extent = r->cur_extent;
    if (PLOT->extent_handle == brui_resizable_active()) {
      br_plot_update_variables(br, PLOT, br->groups, brtl_mouse_pos());
      br_plot_update_context(PLOT, brtl_mouse_pos());
    }
    br_plot_update_shader_values(PLOT, &br->shaders);
    brgl_enable_framebuffer(PLOT->texture_id, PLOT->cur_extent.width, PLOT->cur_extent.height);
    brgl_clear(BR_COLOR_COMPF(BR_THEME.colors.plot_bg));
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
  brgl_clear(BR_COLOR_COMPF(BR_THEME.colors.bg));
  help_draw_fps(br->text, 0, 0);

  brgl_enable_clip_distance();
  brui_begin();
    int to_remove = -1;
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
              int icon_size = 32;
              int og_text_size = brui_text_size();
              brui_vsplitvp(2, BRUI_SPLITA((float)icon_size), BRUI_SPLITR(1));
                char* c = br_scrach_get(4096);
                if (brui_button_icon(BR_SIZEI(icon_size, icon_size), br_icons.back.size_32)) menu_res->hidden = true;
              brui_vsplit_pop();
                brui_text_size_set(og_text_size / 2);
                brui_text_align_set(br_text_renderer_ancor_mid_mid);
                brui_maxy_set(brui_min_y() + (float)icon_size);
                brui_textf("%s Plot #%d", PLOT->kind == br_plot_kind_2d ? "2D" : "3D", i);
              brui_vsplit_end();
              brui_text_size_set(og_text_size);
              brui_text(BR_STRL("_______________________________________________________"));
              if (brui_button(BR_STRL("Remove Plot"))) {
                to_remove = i;
              }
              brui_text_size_set(og_text_size/3*2);
              brui_checkbox(BR_STRL("Follow"), &PLOT->follow);
              for (size_t k = 0; k < br->groups.len; ++k) {
                bool is_shown = false;
                br_data_t* data = br_da_getp(br->groups, k);
                for (int j = 0; j < PLOT->groups_to_show.len; ++j) {
                  if (br_da_get(PLOT->groups_to_show, j) == data->group_id) {
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
    if (to_remove != -1) br_plotter_remove_plot(br, to_remove);
    draw_left_panel(br);
  brui_end();
  br_plotter_end_drawing(br);
}

static void draw_left_panel(br_plotter_t* br) {
  brui_resizable_push(br->menu_extent_handle);
    char* scrach = br_scrach_get(4096);
    if (brui_collapsable(BR_STRL("Plots"), &br->ui.expand_plots)) {
      brui_vsplit(2);
        if (brui_button(BR_STRL("Add 2D"))) {
          br_plotter_add_plot_2d(br);
        }
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Add 3D"))) {
          br_plotter_add_plot_3d(br);
        }
      brui_vsplit_end();

      for (int i = 0; i < br->plots.len; ++i) {
        int n = sprintf(scrach, "%s Plot %d", br_da_get(br->plots, i).kind == br_plot_kind_2d ? "2D" : "3D", i);
        brui_resizable_t* r = brui_resizable_get(br_da_get(br->plots, i).extent_handle);
        bool is_visible = !r->hidden;
        brui_checkbox(BR_STRV(scrach, (uint32_t)n), &is_visible);
        r->hidden = !is_visible;
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Optimizations"), &br->ui.expand_optimizations)) {
      brui_sliderf3(BR_STRL("min something"), &context.min_sampling, 4);
      brui_sliderf2(BR_STRL("cull min"), &context.cull_min);
      brui_checkbox(BR_STRL("Debug"), &context.debug_bounds);
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("UI Styles"), &br->ui.expand_ui_styles)) {
      if (brui_checkbox(BR_STRL("Dark Theme"), &br->ui.dark_theme)) {
        if (br->ui.dark_theme) br_theme_dark();
        else br_theme_light();
      }
      brui_vsplit(2);
        brui_sliderf(BR_STRL("padding.y"), &BR_THEME.ui.padding.y);
      brui_vsplit_pop();
        brui_sliderf(BR_STRL("padding.x"), &BR_THEME.ui.padding.x);
      brui_vsplit_end();
      brui_sliderf2(BR_STRL("thick"), &BR_THEME.ui.border_thick);
      brui_slideri(BR_STRL("Font Size"), &BR_THEME.ui.font_size);
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Export"), &br->ui.expand_export)) {
      if (brui_button(BR_STRL("Brp format"))) {
        br_plotter_export(br, "test.brp");
      }
      if (brui_button(BR_STRL("CSV"))) {
        br_plotter_export_csv(br, "test.csv");
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Data"), &br->ui.expand_data)) {
      brui_text_size_set((int)((float)brui_text_size() * 0.8f));
      for (size_t i = 0; i < br->groups.len; ++i) {
        brui_push();
          br_data_t data = br_da_get(br->groups, i);
          int n = sprintf(scrach, "Data %d (%zu points)", data.group_id, data.len);
          brui_text(BR_STRV(scrach, (uint32_t)n));
          n = sprintf(scrach, "%.1fms (%.3f %.3f)", br_resampling2_get_draw_time(data.resampling)*1000.0f, br_resampling2_get_something(data.resampling), br_resampling2_get_something2(data.resampling));
          brui_text(BR_STRV(scrach, (uint32_t)n));
        brui_pop();
      }
      brui_collapsable_end();
    }

    if (brui_button(BR_STRL("Exit"))) {
      br->should_close = true;
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

