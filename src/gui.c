#include ".generated/icons.h"
#include "src/br_da.h"
#include "src/br_free_list.h"
#include "src/br_gl.h"
#include "src/br_gui_internal.h"
#include "src/br_math.h"
#include "src/br_permastate.h"
#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_str.h"
#include "src/br_string_pool.h"
#include "src/br_theme.h"
#include "src/br_tl.h"
#include "src/br_ui.h"
#include "src/br_resampling2.h"
#include "src/br_license.h"

static void draw_left_panel(br_plotter_t* br);
static bool brgui_draw_plot_menu(br_plot_t* plot, br_datas_t datas);
static void brgui_draw_legend(br_plot_t* plot, br_datas_t datas);
static void brgui_draw_file_browser(br_plot_t* plot, br_datas_t datas);
static void brgui_draw_file_browser(br_plot_t* plot, br_datas_t datas);
static void brgui_draw_debug_window(br_plotter_t* br);
static void brgui_draw_license(br_plotter_t* br);

void br_plotter_draw(br_plotter_t* br) {
  br_plotter_begin_drawing(br);
  for (int i = 0; i < br->plots.len; ++i) {
#define PLOT br_da_getp(br->plots, i)
    brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
    if (brui_resizable_is_hidden(PLOT->extent_handle)) continue;
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
      br_datas_draw(br->groups, PLOT);
      br_shaders_draw_all(br->shaders);
      draw_grid_numbers(br->text, PLOT);
    } else if (PLOT->kind == br_plot_kind_3d) {
      br_datas_draw(br->groups, PLOT);
      br_shaders_draw_all(br->shaders);
      smol_mesh_grid_draw(PLOT, &br->shaders);
      br_shaders_draw_all(br->shaders);
      draw_grid_numbers(br->text, PLOT);
    }
  }

  brgl_enable_framebuffer(0, br->win.size.width, br->win.size.height);
  brgl_clear(BR_COLOR_COMPF(BR_THEME.colors.bg));

  brui_begin();
    br_shaders_draw_all(br->shaders);
#if BR_HAS_HOTRELOAD
    br_hotreload_tick_ui(&br->hot_state);
#endif
    int to_remove = -1;
    for (int i = 0; i < br->plots.len; ++i) {
      if (brui_resizable_is_hidden(PLOT->extent_handle)) continue;
        brui_resizable_push(PLOT->extent_handle);
          brui_img(PLOT->texture_id);
          if (brgui_draw_plot_menu(PLOT, br->groups)) to_remove = i;
          brgui_draw_legend(PLOT, br->groups);
        brui_resizable_pop();
#undef PLOT
    }
    if (to_remove != -1) br_plotter_remove_plot(br, to_remove);
    draw_left_panel(br);
    brgui_draw_debug_window(br);
    brgui_draw_license(br);
    brui_resizable_update();
  brui_end();
  br_plotter_end_drawing(br);
}

static void brgui_draw_legend(br_plot_t* plot, br_datas_t datas) {
  if (brui_resizable_is_hidden(plot->legend_extent_handle)) return;
  brui_resizable_push(plot->legend_extent_handle);
    bool is_active = brui_active();
    int active_group = -1;
    bool pressed = brtl_mousel_pressed();
    br_plot_data_t* di = NULL;
    br_vec2_t mp = brtl_mouse_pos();
    brui_padding_y_set(1.f);
    for (int i = 0; i < plot->data_info.len; ++i) {
      bool active = is_active;
      br_plot_data_t* cdi = &plot->data_info.arr[i];
      br_data_t* data = br_data_get1(datas, cdi->group_id);
      active &= brui_y() < mp.y;
      brui_vsplitvp(3, BRUI_SPLITA((float)brui_text_size()), BRUI_SPLITA(brui_padding_x()), BRUI_SPLITR(1));
        if (plot->selected_data == data->group_id) {
          brui_icon((float)brui_text_size(), BR_EXTENT_TOBB(br_icon_cb_0((float)brui_text_size())), brtl_theme()->colors.btn_hovered, data->color);
        } else {
          brui_icon((float)brui_text_size(), BR_BB(0,0,0,0), data->color, data->color);
        }
      brui_vsplit_pop();
      brui_vsplit_pop();
        brui_text_align_set(br_text_renderer_ancor_left_mid);
        brui_height_set((float)brui_text_size());
        brui_text(brsp_get(*brtl_brsp(), data->name));
      brui_vsplit_end();
      active &= brui_y() >= mp.y;
      if (active) {
        active = false;
        if (cdi->thickness_multiplyer_target != 0) {
          if (pressed) {
            cdi->thickness_multiplyer_target = 0.f;
            plot->selected_data_old = cdi->group_id;
            plot->selected_data = -1;
          } else {
            active_group = data->group_id;
            di = cdi;
          }
        } else {
          if (brtl_mousel_pressed()) {
            cdi->thickness_multiplyer_target = 1.f;
          }
        }
      }
    }
    if (active_group != plot->selected_data_old) {
      if (di != NULL) di->thickness_multiplyer_target = 2.f;
      if (false == pressed) {
        if (plot->selected_data_old >= 0) {
          int index = 0;
          br_da_find(plot->data_info, group_id, plot->selected_data_old, index);
          if (index < plot->data_info.len) {
            plot->data_info.arr[index].thickness_multiplyer_target = 1.f;
          }
        }
      }
      plot->selected_data_old = active_group;
      plot->selected_data = active_group;
    }
  brui_resizable_pop();
}

static bool brgui_draw_plot_menu(br_plot_t* plot, br_datas_t datas) {
  int og_text_size = brui_text_size();
  int icon_size = og_text_size;
  bool ret = false;
  if (brui_resizable_is_hidden(plot->menu_extent_handle)) {
    if (brui_button_icon(BR_SIZEI(icon_size, icon_size), br_icon_menu((float)og_text_size))) brui_resizable_show(plot->menu_extent_handle, true);
  } else {
    brui_resizable_push(plot->menu_extent_handle);
      brui_vsplitvp(2, BRUI_SPLITA((float)icon_size), BRUI_SPLITR(1));
        char* c = br_scrach_get(4096);
        if (brui_button_icon(BR_SIZEI(icon_size, icon_size), br_icon_back((float)og_text_size))) brui_resizable_show(plot->menu_extent_handle, false);
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
      bool show_legend = false == brui_resizable_is_hidden(plot->legend_extent_handle);
      if (brui_checkbox(BR_STRL("Show Legend"), &show_legend)) {
        brui_resizable_show(plot->legend_extent_handle, show_legend);
      };
      for (size_t k = 0; k < datas.len; ++k) {
        bool is_shown = false;
        br_data_t* data = br_da_getp(datas, k);
        for (int j = 0; j < plot->data_info.len; ++j) {
          if (br_da_get(plot->data_info, j).group_id == data->group_id) {
            is_shown = true;
            break;
          }
        }
        br_strv_t name = brsp_get(*brtl_brsp(), data->name);
        sprintf(c, "%.*s ( #%d )", name.len, name.str, data->group_id);
        if (brui_checkbox(br_strv_from_c_str(c), &is_shown)) {
          if (false == is_shown) br_da_remove_feeld(plot->data_info, group_id, data->group_id);
          else {
            br_da_push_t(int, plot->data_info, BR_PLOT_DATA(data->group_id));
          }
        }
      }
      for (int j = 0; j < plot->data_info.len; ++j) {
        br_plot_data_t di = plot->data_info.arr[j];
        brui_textf("  thick: %.2f -> %.2f", di.thickness_multiplyer, di.thickness_multiplyer_target);
      }
      br_scrach_free();
      if (plot->kind == br_plot_kind_2d) {
        brui_textf("Offset: %f %f", BR_VEC2_(plot->dd.offset));
        brui_textf("Zoom: %f %f", BR_VEC2_(plot->dd.zoom));
      } else if (plot->kind == br_plot_kind_3d) {
        brui_sliderf2(BR_STRL("Fov"), &plot->ddd.fov_y);
      }
    brui_resizable_pop();
  }
  return ret;
}

static void draw_left_panel(br_plotter_t* br) {
  brui_resizable_push(br->resizables.menu_extent_handle);
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
        br_plot_t* plot = br_da_getp(br->plots, i);
        int n = sprintf(scrach, "%s Plot %d", plot->kind == br_plot_kind_2d ? "2D" : "3D", i);
        bool is_visible = !brui_resizable_is_hidden(plot->extent_handle);
        if (brui_checkbox(BR_STRV(scrach, (uint32_t)n), &is_visible)) brui_resizable_show(plot->extent_handle, is_visible);
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Optimizations"), &br->ui.expand_optimizations)) {
      brui_sliderf3(BR_STRL("min something"), brtl_min_sampling(), 4);
      brui_sliderf2(BR_STRL("cull min"), brtl_cull_min());
      brui_checkbox(BR_STRL("Debug"), brtl_debug());
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
      brui_sliderf2(BR_STRL("Animation Speed"), &BR_THEME.ui.animation_speed);
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
          br_strv_t name = brsp_get(*brtl_brsp(), data.name);
          int n = sprintf(scrach, "%.*s (%d) (%zu points)", name.len, name.str, data.group_id, data.len);
          brui_text(BR_STRV(scrach, (uint32_t)n));
          n = sprintf(scrach, "%.2fms (%.3f %.3f)", br_resampling2_get_draw_time(data.resampling)*1000.0f, br_resampling2_get_something(data.resampling), br_resampling2_get_something2(data.resampling));
          brui_text(BR_STRV(scrach, (uint32_t)n));
          brui_textf("name id: %d", data.name);
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

    if (brui_collapsable(BR_STRL("About"), &br->ui.expand_about)) {
      if (brui_button(BR_STRL("License"))) {
        br->ui.show_license = true;
      }
      brui_collapsable_end();
    }

    if (brui_button(BR_STRL("Exit"))) {
      br->should_close = true;
    }
    br_scrach_free();
  brui_resizable_pop();
}

static void brgui_draw_debug_window_rec(br_plotter_t* br, int handle, int depth) {
   brui_resizable_t r = brtl_bruirs()->arr[handle];
   brui_textf("%*s, %d Res: z: %d, max_z: %d, max_sib_z: %d, parent: %d", depth*2, "", handle, r.z, r.max_z, brui_resizable_sibling_max_z(handle), r.parent);
   brfl_foreach(i, *brtl_bruirs()) if (i != 0 && brtl_bruirs()->arr[i].parent == handle) brgui_draw_debug_window_rec(br, i, 1+depth);
}

static void brgui_draw_debug_window(br_plotter_t* br) {
  if (false == *brtl_debug()) return;
  if (brui_resizable_temp_push(BR_STRL("Debug")).just_created) {
    brui_ancor_set(brui_ancor_left);
  }
    brgui_draw_debug_window_rec(br, 0, 0);
    uint32_t chars_per_row = 8;
    brui_push();
      brui_textf("Pool size: %d, elements: %d, free_len: %d", brtl_brsp()->pool.len, brtl_brsp()->len, brtl_brsp()->free_len);
      for (int i = 0; i < brtl_brsp()->len; ++i) {
        brsp_node_t node = brtl_brsp()->arr[i];
        brui_textf("#%d |%c| start_index: %d, len: %d, cap: %d prev: %d, next: %d", i, brfl_is_free(*brtl_brsp(), i) ? ' ' : 'X', node.start_index, node.len, node.cap, node.prev_in_memory, node.next_in_memory);
      }
      char* buff = br_scrach_get(128);
        int cur_node_index = brtl_brsp()->first_in_memory;
        brsp_node_t cur_node = brtl_brsp()->arr[cur_node_index];
        for (uint32_t i = 0; i < brtl_brsp()->pool.len; i += chars_per_row) {
          brui_vsplitvp(2, BRUI_SPLITA(70), BRUI_SPLITR(1));
            brui_text_color_set(brtl_theme()->colors.btn_txt_inactive);
            brui_textf("0x%x", i);
          brui_vsplit_pop();
            char* cur = buff;
            for (uint32_t j = 0; j < chars_per_row && i + j < brtl_brsp()->pool.len; ++j) { 
              br_color_t text_color = brfl_is_free(*brtl_brsp(), cur_node_index) ? BR_COLOR(0x40, 0x40, 0x20, 0xFF) : BR_COLOR(0xE0, 0x20, 0xE0, 0xFF);
              brui_text_color_set(cur_node.len > 0 ? br_color_lighter(text_color, 10) : text_color);
              unsigned char c = (unsigned char)brtl_brsp()->pool.str[i + j];
              if (c < 16) {
                *cur++ = '0';
                *cur++ = c >= 10 ? 'A'+(c - 10) : ('0' + c);
                *cur++ = ' ';
              } else {
                char upper = c >> 4;
                char downer = c & 0x0F;
                *cur++ = upper >= 10 ? 'A'+(upper - 10) : ('0' + upper);
                *cur++ = downer >= 10 ? 'A'+(downer - 10) : ('0' + downer);
                *cur++ = ' ';
              }
              if (--cur_node.len == 0) {
                brui_text(BR_STRV(buff, (uint32_t)(cur - buff)));
                cur = buff;
              }
              if (0 == --cur_node.cap) {
                brui_text(BR_STRV(buff, (uint32_t)(cur - buff)));
                cur_node = brtl_brsp()->arr[cur_node.next_in_memory];
              }
            }
            brui_text(BR_STRV(buff, (uint32_t)(cur - buff)));
          brui_vsplit_end();
        }
      br_scrach_free();
    brui_pop();
  brui_resizable_pop();
}

static void brgui_draw_license(br_plotter_t* br) {
  if (false == br->ui.show_license) return;
  br_strv_t res_name = BR_STRL("License");
  bool delete = false;
  brui_resizable_temp_push_t tmp =  brui_resizable_temp_push(res_name);
    if (tmp.just_created) {
      tmp.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      tmp.res->target.cur_extent.width -= 100;
      tmp.res->target.cur_extent.height -= 100;
      tmp.res->target.cur_extent.x += 50;
      tmp.res->target.cur_extent.y += 50;
      for (int i = 0; i < br_license_lines; ++i) printf("%.*s\n", br_license[i].len, br_license[i].str);
    }
    if (false == brui_resizable_is_hidden(tmp.resizable_handle)) {
      for (int i = 0; i < br_license_lines; ++i) {
        brui_text(br_license[i]);
      }
    } else {
      delete = true;
    }
  brui_resizable_pop();
  if (true == delete) {
      br->ui.show_license = false;
      brui_resizable_temp_delete(res_name);
  }
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

