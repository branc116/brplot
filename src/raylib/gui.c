#include ".generated/icons.h"
#include "src/br_icons.h"
#include "src/br_math.h"
#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_permastate.h"
#include "src/br_tl.h"
#include "src/raylib/ui.c"
#include "src/br_gl.h"

static void draw_left_panel(br_plotter_t* gv);
void br_gui_init_specifics_gui(br_plotter_t* br) {
  if (false == br->loaded) {
    br_plotter_add_plot_2d(br);
    br_plotter_add_plot_3d(br);
  }
}

bool br_icon_button(br_extent_t pos, br_extent_t atlas, br_vec4_t bg, bool hide_when_not_near) {
  br_vec4_t fg = BR_VEC4(200, 200, 200, 255); 
  if (br_col_vec2_extent(pos, brtl_mouse_get_pos())) {
    bg = BR_VEC4(45, 45, 45, bg.w);
    br_icons_draw(brtl_shaders()->icon, pos, atlas, bg, fg);
    return brtl_mouse_is_pressed_l();
  } else {
    if (hide_when_not_near) {
      float dist2 = br_vec2_dist2(pos.pos, brtl_mouse_get_pos());
      if (dist2 < 10000) {
        fg.x = fg.y = fg.z = fg.w = (unsigned char)((10000.f - dist2) / 10000.f * 255.f); 
        br_icons_draw(brtl_shaders()->icon, pos, atlas, bg, fg);
      }
    } else br_icons_draw(brtl_shaders()->icon, pos, atlas, bg, fg);
    return false;
  }
}

BR_API void br_plotter_draw(br_plotter_t* br) {
  br_plotter_begin_drawing(br);

#define PLOT (&br->plots.arr[br->active_plot_index])
  PLOT->drag_parent_extent.pos = BR_VEC2I(50, 50);
  PLOT->drag_parent_extent.size = br_sizei_sub(br->win.size, BR_SIZEI(100, 100));
  brgl_enable(GL_BLEND);
  br_plot_update_variables(br, PLOT, br->groups, brtl_mouse_get_pos());
  br_plot_update_context(PLOT, brtl_mouse_get_pos());
  br_plot_update_shader_values(PLOT, &br->shaders);
  brgl_enable_framebuffer(PLOT->texture_id, PLOT->graph_screen_rect.width, PLOT->graph_screen_rect.height);
  brgl_clear();
  draw_grid_numbers(br->text, PLOT);
  br_datas_draw(br->groups, PLOT, &br->shaders);
  smol_mesh_grid_draw(PLOT, &br->shaders);

  brgl_enable_framebuffer(0, br->win.size.width, br->win.size.height);
  brgl_disable(GL_BLEND);
  brgl_enable_depth_test();
  brgl_blend_func(GL_SRC_ALPHA, GL_DST_ALPHA);
  brgl_blend_equation(GL_MAX);
  brgl_clear();
  help_draw_fps(br->text, 0, 0);
  smol_mesh_img_draw(PLOT, &br->shaders);

  if (PLOT->draw_settings == false) {
    br_extent_t icon_ex = BR_EXTENT((float)PLOT->graph_screen_rect.x + (float)PLOT->graph_screen_rect.width - 32.f - 5.f, (float)PLOT->graph_screen_rect.y + 5.f, 32.f, 32.f);
    if (br_icon_button(icon_ex, br_icons.menu.size_32, BR_VEC4(0, 0, 0, 1), true)) {
        PLOT->drag_mode = br_drag_mode_none;
        PLOT->draw_settings = true;
    }
  } else {
    br_extent_t panel = BR_EXTENTPS(br_extent_tr(BR_EXTENTI_TOF(PLOT->graph_screen_rect), 200, 0), BR_SIZE(200, p.height));
    br_vec4_t panel_color = BR_VEC4(0.1f,0.1f,0.1f,2.f);
    br_icons_draw(br->shaders.icon, panel, BR_EXTENT(0,0,0,0), panel_color, panel_color);
    panel_color.w = 3;
    if (br_icon_button(BR_EXTENTPS(br_extent_tl(panel, 4, 4), BR_SIZE(32, 32)), br_icons_y_mirror(br_icons.back.size_32), panel_color, false)) {
        PLOT->drag_mode = br_drag_mode_none;
        PLOT->draw_settings = false;
    }
  }
#undef PLOT
  draw_left_panel(br);
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
