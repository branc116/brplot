#include "src/br_da.h"
#include "src/br_gl.h"
#include "src/br_gui_internal.h"
#include "src/br_math.h"
#include "src/br_plot.h"
#include "src/br_resampling2.h"
#include "src/br_tl.h"
#include "src/br_ui.h"

#include <assert.h>

static void br_plot_2d_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders);
static void br_plot_3d_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders);

void br_plot_deinit(br_plot_t* plot) {
  br_da_free(plot->groups_to_show);
  brui_resizable_delete(plot->extent_handle);
  brui_resizable_delete(plot->menu_extent_handle);
  brui_resizable_delete(plot->legend_extent_handle);
  brgl_destroy_framebuffer(plot->texture_id);
}

void br_plot_create_texture(br_plot_t* br) {
  br->texture_id = brgl_create_framebuffer(br->cur_extent.width, br->cur_extent.height);
}

void br_plot_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders) {
  switch (plot->kind) {
    case br_plot_kind_2d: br_plot_2d_draw(plot, datas, shaders); break;
    case br_plot_kind_3d: br_plot_3d_draw(plot, datas, shaders); break;
    default: BR_ASSERT(0);
  }
}

void br_plot_update_variables(br_plotter_t* br, br_plot_t* plot, br_datas_t groups, br_vec2_t mouse_pos) {
  switch (plot->kind) {
    case br_plot_kind_2d: {
      if (br_plot_update_variables_2d(plot, groups, mouse_pos))
        br_keybinding_handle_keys(br, plot);
    } break;
    case br_plot_kind_3d: {
      if (br_plot_update_variables_3d(plot, groups, mouse_pos))
        br_keybinding_handle_keys(br, plot);
    } break;
    default: BR_ASSERT(0);
  }
}

bool br_plot_update_variables_2d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos) {
  BR_ASSERT(plot->kind == br_plot_kind_2d);
  if (plot->follow) {
    br_extent_t sr = plot->dd.graph_rect;
    br_vec2_t middle = BR_VEC2(sr.x + sr.width/2, sr.y - sr.height/2);
    for (size_t i = 0; i < groups.len; ++i) {
      br_data_t* pg = &groups.arr[i];
      size_t gl = pg->len;
      if (pg->kind != br_data_kind_2d || gl == 0) continue;
      plot->dd.delta.x += ((middle.x - pg->dd.xs[gl - 1]))/1000.f;
      plot->dd.delta.y += ((middle.y - pg->dd.ys[gl - 1]))/1000.f;
    }
    plot->dd.offset.x -= plot->dd.delta.x;
    plot->dd.offset.y -= plot->dd.delta.y;
    plot->dd.delta.x *= plot->dd.recoil;
    plot->dd.delta.y *= plot->dd.recoil;
  } else {
    plot->dd.delta = BR_VEC2(0.f, 0.f);
  }
  if (plot->mouse_inside_graph) {
    // TODO: Move this to br_keybindings.c
    // Stuff related to zoom
    {
      float mw = -brtl_mouse_scroll().y;
      br_vec2_t old = plot->dd.mouse_pos;
      bool any = false;
      if (false == br_float_near_zero(mw)) {
        float mw_scale = (1 + mw/10);
        if (brtl_key_down(BR_KEY_X)) {
          plot->dd.zoom.x *= mw_scale;
        } else if (brtl_key_down(BR_KEY_Y)) {
          plot->dd.zoom.y *= mw_scale;
        } else {
          plot->dd.zoom.x *= mw_scale;
          plot->dd.zoom.y *= mw_scale;
        }
        any = true;
      }
      if (brtl_key_down(BR_KEY_X) && brtl_key_shift()) any = true, plot->dd.zoom.x *= 1.1f;
      if (brtl_key_down(BR_KEY_Y) && brtl_key_shift()) any = true, plot->dd.zoom.y *= 1.1f;
      if (brtl_key_down(BR_KEY_X) && brtl_key_ctrl())  any = true, plot->dd.zoom.x *= .9f;
      if (brtl_key_down(BR_KEY_Y) && brtl_key_ctrl())  any = true, plot->dd.zoom.y *= .9f;
      if (any) {
        br_plot_update_context(plot, mouse_pos);
        br_vec2_t now = plot->dd.mouse_pos;
        plot->dd.offset.x -= now.x - old.x;
        plot->dd.offset.y -= now.y - old.y;
      }
    }
    if (false && plot->jump_around) {
      double t = brtl_time();
      plot->cur_extent.x += (int)(100.f * (float)sin(t));
      plot->cur_extent.y += (int)(77.f * (float)cos(t));
      plot->cur_extent.width += (int)(130.f * (float)sin(t));
      plot->cur_extent.height += (int)(177.f * (float)cos(t));
    }
    if (brtl_mouser_down()) {
      br_vec2_t delt = brtl_mouse_delta();
      float height = (float)plot->cur_extent.height;
      plot->dd.offset.x -= plot->dd.zoom.x*delt.x/height;
      plot->dd.offset.y += plot->dd.zoom.y*delt.y/height;
      return false;
    } else return true;
  }
  return false;
}

bool br_plot_update_variables_3d(br_plot_t* plot, br_datas_t const groups, br_vec2_t mouse_pos) {
  (void)groups; (void)mouse_pos;
  BR_ASSERT(plot->kind == br_plot_kind_3d);
  if (!plot->mouse_inside_graph) return false;
  if (brtl_mouser_down()) {
    br_vec2_t m = brtl_mouse_delta();
    br_vec2_t md = br_vec2_scale(BR_VEC2(m.x, m.y), -0.003f);
    br_vec3_t zeroed = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
    br_vec3_t rotated_up = br_vec3_rot(zeroed, plot->ddd.up, md.x);
    br_vec3_t right = br_vec3_cross(plot->ddd.up, br_vec3_normalize(zeroed));
    br_vec3_t rotated_right = br_vec3_rot(rotated_up, right, md.y);
    if (fabsf(br_vec3_dot(rotated_right, plot->ddd.up)) > 0.94f) plot->ddd.eye = br_vec3_add(rotated_up,    plot->ddd.target);
    else                                                               plot->ddd.eye = br_vec3_add(rotated_right, plot->ddd.target);
    plot->ddd.eye = br_vec3_add(rotated_right, plot->ddd.target);
    return false;
  }
  {
    float mw = brtl_mouse_scroll().y;
    float mw_scale = (1 + mw/10);
    br_vec3_t zeroed = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
    float len = br_vec3_len(zeroed);
    len *= mw_scale;
    plot->ddd.eye = br_vec3_add(br_vec3_scale(br_vec3_normalize(zeroed), len), plot->ddd.target);
  }
  return true;
}


void br_plot_update_shader_values(br_plot_t* plot, br_shaders_t* shaders) {
  br_extent_t const ex = BR_EXTENTI_TOF(plot->cur_extent);
  switch (plot->kind) {
    case br_plot_kind_2d: {
      TracyCFrameMarkStart("update_shader_values_2d");
      br_vec2_t zoom = plot->dd.zoom;
      br_vec2_t zoom_log = { .x = powf(10.f, -floorf(log10f(zoom.x))), .y = powf(10.f, -floorf(log10f(zoom.y))) };
      br_vec2_t zoom_final = { .x = zoom.x * zoom_log.x, .y = zoom.y * zoom_log.y };
      shaders->grid->uvs.zoom_uv = zoom_final;
      br_vec2_t off_zoom = br_vec2_mul(plot->dd.offset, zoom_log);
      br_vec2_t off = br_vec2_mul(off_zoom, BR_VEC2(0.1f, 0.1f));
      shaders->grid->uvs.offset_uv = br_vec2_sub(off_zoom, BR_VEC2(floorf(off.x) * 10.f, floorf(off.y) * 10.f));
      shaders->grid->uvs.screen_uv = ex.size.vec;
      shaders->line->uvs.zoom_uv = plot->dd.zoom;
      shaders->line->uvs.offset_uv = plot->dd.offset;
      shaders->line->uvs.screen_uv = ex.size.vec;
      TracyCFrameMarkEnd("update_shader_values_2d");
    } break;
    case br_plot_kind_3d: {
      TracyCFrameMarkStart("update_shader_values_3d");
      br_vec2_t re = (br_vec2_t) { .x = ex.width, .y = ex.height };
      br_vec3_t eye_zero = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
      float eye_scale = 10.f * powf(10.f, -floorf(log10f(fmaxf(fmaxf(fabsf(eye_zero.x), fabsf(eye_zero.y)), fabsf(eye_zero.z)))));
      br_vec3_t eye_final = br_vec3_add(br_vec3_scale(eye_zero, eye_scale), plot->ddd.target);
      br_mat_t per = br_mat_perspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
      br_mat_t look_grid = br_mat_look_at(eye_final, plot->ddd.target, plot->ddd.up);
      br_mat_t look = br_mat_look_at(plot->ddd.eye, plot->ddd.target, plot->ddd.up);
      shaders->grid_3d->uvs.m_mvp_uv = br_mat_mul(look_grid, per);
      shaders->grid_3d->uvs.eye_uv = eye_final;
      shaders->grid_3d->uvs.look_dir_uv = br_vec3_normalize(br_vec3_sub(plot->ddd.target, plot->ddd.eye));

      shaders->line_3d_simple->uvs.m_mvp_uv = shaders->line_3d->uvs.m_mvp_uv = br_mat_mul(look, per);
      shaders->line_3d->uvs.eye_uv = plot->ddd.eye;
      TracyCFrameMarkEnd("update_shader_values_3d");
    } break;
    default: BR_ASSERT(0);
  }
}

br_vec2_t br_plot_2d_get_mouse_position(br_plot_t* plot, br_vec2_t screen_mouse_pos) {
  br_extenti_t ex = plot->cur_extent;
  br_vec2i_t mouse_pos = BR_VEC2_TOI(screen_mouse_pos);
  br_vec2i_t mp_in_graph = BR_VEC2I_SUB(mouse_pos, ex.pos);
  br_vec2i_t a = BR_VEC2I_SCALE(mp_in_graph, 2);
  br_vec2_t b = br_vec2i_tof(BR_VEC2I_SUB(plot->cur_extent.size.vec, a));
  br_vec2_t c = br_vec2_scale(b, 1.f/(float)ex.height);
  return  BR_VEC2(
    -c.x*plot->dd.zoom.x/2.f + plot->dd.offset.x,
     c.y*plot->dd.zoom.y/2.f + plot->dd.offset.y
  );
}

void br_plot_update_context(br_plot_t* plot, br_vec2_t mouse_pos) {
  br_extent_t ex = BR_EXTENTI_TOF(plot->cur_extent);
  if (plot->kind == br_plot_kind_2d) {
    float aspect = ex.width/ex.height;
    plot->dd.mouse_pos = br_plot_2d_get_mouse_position(plot, mouse_pos);
    plot->dd.graph_rect = BR_EXTENT(
      -aspect*plot->dd.zoom.x/2.f + plot->dd.offset.x,
      plot->dd.zoom.y/2.f + plot->dd.offset.y,
      aspect*plot->dd.zoom.x,
      plot->dd.zoom.y);
  } else {
    // TODO 2D/3D
    //assert(false);
  }
}

void br_plot_remove_group(br_plots_t plots, int group) {
  for (int i = 0; i < plots.len; ++i) {
    br_da_remove(plots.arr[i].groups_to_show, group);
  }
}

static void br_plot_2d_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders) {
  TracyCFrameMarkStart("br_datas_draw_2d");
  for (int j = 0; j < plot->groups_to_show.len; ++j) {
    int group = plot->groups_to_show.arr[j];
    br_data_t const* g = br_data_get1(datas, group);
    if (g->len == 0) continue;
    resampling2_draw(g->resampling, g, plot, shaders);
  }
}

static void br_plot_3d_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders) {
  TracyCFrameMarkStart("br_datas_draw_3d");
  for (int j = 0; j < plot->groups_to_show.len; ++j) {
    int group = plot->groups_to_show.arr[j];
    br_data_t const* g = br_data_get1(datas, group);
    if (g->len == 0) continue;
    resampling2_draw(g->resampling, g, plot, shaders);
  }
  TracyCFrameMarkEnd("br_datas_draw_3d");
}

