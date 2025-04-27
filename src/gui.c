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
static bool brgui_draw_plot_menu(br_plot_t* plot, br_datas_t datas);
static void brgui_draw_legend(br_plot_t* plot, br_datas_t datas);

void br_plotter_draw(br_plotter_t* br) {
  br_plotter_begin_drawing(br);
  brui_resizable_update();
  for (int i = 0; i < br->plots.len; ++i) {
#define PLOT br_da_getp(br->plots, i)
    brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
    if (r->hidden_factor > 0.9f) continue;
    PLOT->cur_extent = BR_EXTENT_TOI(r->cur_extent);
    PLOT->mouse_inside_graph = PLOT->extent_handle == brui_resizable_active();
    br_plot_update_variables(br, PLOT, br->groups, brtl_mouse_pos());
    br_plot_update_context(PLOT, brtl_mouse_pos());
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

  brgl_enable_clip_distance();
  brui_begin();
    int to_remove = -1;
    for (int i = 0; i < br->plots.len; ++i) {
      brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
      if (r->hidden_factor > 0.9f) continue;
        brui_resizable_push(PLOT->extent_handle);
          brui_img(PLOT->texture_id);
          if (brgui_draw_plot_menu(PLOT, br->groups)) to_remove = i;
          brgui_draw_legend(PLOT, br->groups);
        brui_resizable_pop();
#undef PLOT
    }
    if (to_remove != -1) br_plotter_remove_plot(br, to_remove);
    draw_left_panel(br);
  brui_end();
  br_plotter_end_drawing(br);
}

static void brgui_draw_legend(br_plot_t* plot, br_datas_t datas) {
  if (brui_resizable_get(plot->legend_extent_handle)->hidden_factor > 0.9f) return;
  brui_resizable_push(plot->legend_extent_handle);
    bool is_active = brui_active();
    int active_group = -1;
    br_vec2_t mp = brtl_mouse_pos();
    brui_padding_y_set(1.f);
    brui_text_size_set(brui_text_size() / 5 * 3);
    for (int i = 0; i < plot->groups_to_show.len; ++i) {
      bool active = is_active;
      br_data_t* data = br_data_get1(datas, plot->groups_to_show.arr[i]);
      active &= brui_y() < mp.y;
      brui_vsplitvp(3, BRUI_SPLITA((float)brui_text_size()), BRUI_SPLITA(brui_padding_x()), BRUI_SPLITR(1));
        if (plot->selected_data == data->group_id) {
          brui_icon((float)brui_text_size(), BR_EXTENT_TOBB(br_icons.cb_0.size_32), brtl_theme()->colors.btn_hovered, data->color);
        } else {
          brui_icon((float)brui_text_size(), BR_BB(0,0,0,0), data->color, data->color);
        }
      brui_vsplit_pop();
      brui_vsplit_pop();
        brui_text_align_set(br_text_renderer_ancor_left_mid);
        brui_height_set((float)brui_text_size());
        brui_text(br_str_as_view(data->name));
      brui_vsplit_end();
      active &= brui_y() >= mp.y;
      if (active) {
        active = false;
        active_group = data->group_id;
        plot->selected_data_influence_target = 1.f;
      }
    }
    if (active_group != plot->selected_data_old) {
      plot->selected_data_old = active_group;
      plot->selected_data = active_group;
      plot->selected_data_influence_target = 1.f;
      plot->selected_data_influence = 0.f;
    }
  brui_resizable_pop();
}

static bool brgui_draw_plot_menu(br_plot_t* plot, br_datas_t datas) {
  brui_resizable_t* menu_res = brui_resizable_get(plot->menu_extent_handle);
  int og_text_size = brui_text_size();
  int icon_size = og_text_size;
  bool ret = false;
  if (menu_res->hidden_factor > 0.9f) {
    if (brui_button_icon(BR_SIZEI(icon_size, icon_size), br_icons.menu.size_32)) menu_res->target.hidden_factor = 0.0f;
  } else {
    brui_resizable_push(plot->menu_extent_handle);
      brui_vsplitvp(2, BRUI_SPLITA((float)icon_size), BRUI_SPLITR(1));
        char* c = br_scrach_get(4096);
        if (brui_button_icon(BR_SIZEI(icon_size, icon_size), br_icons.back.size_32)) menu_res->target.hidden_factor = 1.0f;
      brui_vsplit_pop();
        brui_text_size_set(og_text_size);
        brui_text_align_set(br_text_renderer_ancor_mid_mid);
        brui_maxy_set(brui_min_y() + (float)icon_size);
        brui_textf("%s Plot", plot->kind == br_plot_kind_2d ? "2D" : "3D");
      brui_vsplit_end();
      brui_text(BR_STRL("_______________________________________________________"));
      if (brui_button(BR_STRL("Remove Plot"))) {
        ret = true;
      }
      if (plot->kind == br_plot_kind_2d) {
        brui_sliderf2(BR_STRL("Line Thickness"), &plot->dd.line_thickness);
        brui_vsplit(2);
          brui_sliderf2(BR_STRL("Grid Thick"), &plot->dd.grid_line_thickness);
        brui_vsplit_pop();
          brui_sliderf2(BR_STRL("Major Grid Thick"), &plot->dd.grid_major_line_thickness);
        brui_vsplit_end();
      }
      brui_text_size_set(og_text_size/5*4);
      brui_checkbox(BR_STRL("Follow"), &plot->follow);
      bool hide_legend = brui_resizable_get(plot->legend_extent_handle)->target.hidden_factor > 0.9f;
      if (brui_checkbox(BR_STRL("Hide Legend"), &hide_legend)) {
        brui_resizable_get(plot->legend_extent_handle)->target.hidden_factor = hide_legend ? 1.f : 0.f;
      };
      for (size_t k = 0; k < datas.len; ++k) {
        bool is_shown = false;
        br_data_t* data = br_da_getp(datas, k);
        for (int j = 0; j < plot->groups_to_show.len; ++j) {
          if (br_da_get(plot->groups_to_show, j) == data->group_id) {
            is_shown = true;
            break;
          }
        }
        sprintf(c, "Data #%d", data->group_id);
        if (brui_checkbox(br_strv_from_c_str(c), &is_shown)) {
          if (false == is_shown) br_da_remove(plot->groups_to_show, data->group_id);
          else br_da_push_t(int, plot->groups_to_show, data->group_id);
        }
      }
      br_scrach_free();
      if (plot->kind == br_plot_kind_2d) {
        brui_textf("Offset: %f %f", BR_VEC2_(plot->dd.offset));
        brui_textf("Zoom: %f %f", BR_VEC2_(plot->dd.zoom));
      }
    brui_resizable_pop();
  }
  return ret;
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
        bool is_visible = r->target.hidden_factor < 0.9f;
        brui_checkbox(BR_STRV(scrach, (uint32_t)n), &is_visible);
        r->target.hidden_factor = is_visible ? 0.f : 1.f;
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Optimizations"), &br->ui.expand_optimizations)) {
      brui_sliderf3(BR_STRL("min something"), &br_context.min_sampling, 4);
      brui_sliderf2(BR_STRL("cull min"), &br_context.cull_min);
      brui_checkbox(BR_STRL("Debug"), &br_context.debug_bounds);
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
          switch (data.kind) {
            case br_data_kind_2d: {
              brui_textf("2D rebase: %.3f, %.3f", data.dd.rebase_x, data.dd.rebase_y);
              br_bb_t bb = br_bb_add(data.dd.bounding_box, BR_VEC2((float)data.dd.rebase_x, (float)data.dd.rebase_y));
              brui_textf("   box: (%.3f, %.3f) (%.3f, %3.f)", BR_BB_(bb));
            } break;
            case br_data_kind_3d: {
              brui_textf("3D rebase: %.3f, %.3f, %.3f", data.ddd.rebase_x, data.ddd.rebase_y, data.ddd.rebase_z);
            } break;
          }
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

