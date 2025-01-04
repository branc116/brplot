#include ".generated/icons.h"
#include "src/br_da.h"
#include "src/br_gl.h"
#include "src/br_gui_internal.h"
#include "src/br_icons.h"
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

static void draw_left_panel(br_plotter_t* gv);
void br_gui_init_specifics_gui(br_plotter_t* br) {
  if (false == br->loaded) {
    br_plotter_add_plot_2d(br);
    br_plotter_add_plot_3d(br);
  }
}

bool br_icon_button(br_extent_t pos, br_extent_t atlas, bool hide_when_not_near, int z) {
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
      br_shaders_draw_all(br->shaders);
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

  for (int i = 0; i < br->plots.len; ++i) {
    brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
    if (true == r->hidden) continue;
    brui_begin();
      brui_resizable_push(PLOT->extent_handle);
        brui_img(PLOT->texture_id);
        if (PLOT->draw_settings == false) {
          brui_ancor_set(brui_ancor_top_right);
          if (brui_button_icon(BR_SIZEI(32, 32), br_icons.menu.size_32)) PLOT->draw_settings = true;
        } else {
          brui_resizable_push(PLOT->menu_extent_handle);
            char* c = br_scrach_get(4096);
            if (brui_button_icon(BR_SIZEI(32, 32), br_icons.back.size_32)) PLOT->draw_settings = false;
            brui_checkbox(br_strv_from_literal("Follow", &PLOT->follow));
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
          brui_resizable_pop();
        }
      brui_resizable_pop();
    brui_end();

//    if (PLOT->draw_settings == false) {
//      br_extent_t icon_ex = BR_EXTENT((float)PLOT->cur_extent.x + (float)PLOT->cur_extent.width - 32.f - 5.f, (float)PLOT->cur_extent.y + 5.f, 32.f, 32.f);
//      if (br_icon_button(icon_ex, br_icons.menu.size_32, true, 1)) {
//          PLOT->draw_settings = true;
//      }
//    } else {
//      br_extent_t p = BR_EXTENTI_TOF(PLOT->cur_extent);
//      br_extent_t panel = BR_EXTENTPS(br_extent_tr2(p, 200, 0), BR_SIZE(200, p.height));
//      br_icons_draw(br->shaders.icon, panel, BR_EXTENT(0,0,0,0), br_theme.colors.plot_menu_color, br_theme.colors.plot_menu_color, 3);
//      brui_resizable_push(PLOT->menu_extent_handle);
//      if (br_icon_button(BR_EXTENTPS(br_extent_tl2(panel, 4, 4), BR_SIZE(32, 32)), br_icons_y_mirror(br_icons.back.size_32), false, 4)) {
//          PLOT->draw_settings = false;
//      }
//      brui_begin();
//        brui_z_set(12);
//        brui_push(BR_EXTENTPS(br_extent_tr2(p, 190, 40), BR_SIZE(180, p.height - 50)));
//          brui_checkbox(br_strv_from_literal("Follow"), &PLOT->follow);
//          char* c = br_scrach_get(4096);
//          for (size_t k = 0; k < br->groups.len; ++k) {
//            bool is_shown = false;
//            br_data_t* data = &br->groups.arr[k];
//            for (int j = 0; j < PLOT->groups_to_show.len; ++j) {
//              if (PLOT->groups_to_show.arr[j] == data->group_id) {
//                is_shown = true;
//                break;
//              }
//            }
//
//            sprintf(c, "Data #%d", data->group_id);
//            if (brui_checkbox(br_strv_from_c_str(c), &is_shown)) {
//              if (false == is_shown) br_da_remove(PLOT->groups_to_show, data->group_id);
//              else br_da_push_t(int, PLOT->groups_to_show, data->group_id);
//            }
//          }
//          br_scrach_free();
//        brui_pop();
//      brui_end();
//    }
    brui_resizable_pop();
#undef PLOT
  }
  draw_left_panel(br);
  br_plotter_end_drawing(br);
}

static float sp = 0.f;
static void draw_left_panel(br_plotter_t* br) {
  brui_begin();
    brui_push(BR_EXTENTI_TOF(brui_resizable_get(br->menu_extent_handle)->cur_extent));
      char* scrach = br_scrach_get(4096);
      for (int i = 0; i < br->plots.len; ++i) {
        sprintf(scrach, "%s Plot %d", br->plots.arr[i].kind == br_plot_kind_2d ? "2D" : "3D", i);
        brui_resizable_t* r = brui_resizable_get(br->plots.arr[i].extent_handle);
        bool is_visible = !r->hidden;
        brui_checkbox(br_strv_from_c_str(scrach), &is_visible);
        r->hidden = !is_visible;
      }
      br_scrach_free();
      brui_checkbox(br_strv_from_literal("Debug"), &context.debug_bounds);
      if (brui_checkbox(br_strv_from_literal("Dark Theme"), &br->dark_theme)) {
        if (br->dark_theme) br_theme_dark();
        else br_theme_light();
      }
      if (brui_button(br_strv_from_literal("Export"))) {
        br_plotter_export(br, "test.brp");
      }
      if (brui_button(br_strv_from_literal("Export CSV"))) {
        br_plotter_export_csv(br, "test.csv");
      }
      if (brui_button(br_strv_from_literal("Exit"))) {
        br->should_close = true;
      }
    brui_pop();
  brui_end();
  return;
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
      if (brtl_key_pressed(BR_KEY_C)) {
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

