#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_gl.h"
#include "src/br_gui.h"
#include "src/br_math.h"
#include "src/br_plot.h"
#include "src/br_resampling.h"
#include "src/br_ui.h"
#include "src/br_memory.h"

static void br_plot_2d_draw(br_plot_t* plot, br_datas_t datas);
static void br_plot_3d_draw(br_plot_t* plot, br_datas_t datas);

void br_plot_deinit(br_plot_t* plot) {
  br_da_free(plot->data_info);
  brui_resizable_delete(plot->extent_handle);
  brgl_destroy_framebuffer(plot->texture_id);
}

void br_plot_create_texture(br_plot_t* br) {
  br->texture_id = brgl_create_framebuffer(br->cur_extent.width, br->cur_extent.height);
}

void br_plot_draw(br_plot_t* plot, br_datas_t datas) {
  switch (plot->kind) {
    case br_plot_kind_2d: br_plot_2d_draw(plot, datas); break;
    case br_plot_kind_3d: br_plot_3d_draw(plot, datas); break;
    default: BR_UNREACHABLE("Plot kind %d is not handled", plot->kind);
  }
}

void br_plot2d_move_screen_space(br_plot_t* plot, br_vec2_t delta, br_size_t plot_screen_size) {
  float height = plot_screen_size.height;
  plot->dd.offset.x -= plot->dd.zoom.x*delta.x/height;
  plot->dd.offset.y += plot->dd.zoom.y*delta.y/height;
}

void br_plot2d_zoom(br_plot_t* plot, br_vec2_t vec, br_extent_t screen_extent, br_vec2_t mouse_pos_screen) {
  br_vec2d_t old = plot->dd.mouse_pos;
  bool any = false;
  if (false == br_float_near_zero(vec.x)) {
    float mw_scale = (1 + vec.x/10);
    plot->dd.zoom.x *= mw_scale;
    any = true;
  }
  if (false == br_float_near_zero(vec.y)) {
    float mw_scale = (1 + vec.y/10);
    plot->dd.zoom.y *= mw_scale;
    any = true;
  }
  if (false == any) return;

  br_plot_update_context(plot, screen_extent, mouse_pos_screen);
  br_vec2d_t now = plot->dd.mouse_pos;
  plot->dd.offset.x -= now.x - old.x;
  plot->dd.offset.y -= now.y - old.y;
}

void br_plot_update_shader_values(br_plot_t* plot, br_shaders_t* shaders) {
  br_extent_t const ex = BR_EXTENTI_TOF(plot->cur_extent);
  switch (plot->kind) {
    case br_plot_kind_2d: break;
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
      shaders->grid_3d->uvs.target_uv = plot->ddd.target;
      shaders->grid_3d->uvs.look_dir_uv = br_vec3_normalize(br_vec3_sub(plot->ddd.target, plot->ddd.eye));

      shaders->line_3d_simple->uvs.m_mvp_uv = shaders->line_3d->uvs.m_mvp_uv = br_mat_mul(look, per);
      shaders->line_3d->uvs.eye_uv = plot->ddd.eye;
      TracyCFrameMarkEnd("update_shader_values_3d");
    } break;
    default: BR_ASSERT(0);
  }
}

br_vec2d_t br_plot_2d_get_mouse_position(br_plot_t* plot, br_vec2_t screen_mouse_pos) {
  br_extenti_t ex = plot->cur_extent;
  br_vec2i_t mouse_pos = BR_VEC2_TOI(screen_mouse_pos);
  br_vec2i_t mp_in_graph = BR_VEC2I_SUB(mouse_pos, ex.pos);
  br_vec2i_t a = BR_VEC2I_SCALE(mp_in_graph, 2);
  br_vec2_t b = br_vec2i_tof(BR_VEC2I_SUB(plot->cur_extent.size.vec, a));
  br_vec2_t c = br_vec2_scale(b, 1.f/(float)ex.height);
  return BR_VEC2D(
    -c.x*plot->dd.zoom.x/2.0 + plot->dd.offset.x,
     c.y*plot->dd.zoom.y/2.0 + plot->dd.offset.y
  );
}

br_vec2d_t br_plot2d_to_plot(br_plot_t* plot, br_vec2_t vec) {
  br_extent_t ex = brui_resizable_cur_extent(plot->extent_handle);
  br_vec2_t vec_in_resizable = br_vec2_sub(vec, ex.pos);
  br_vec2_t vec_in_resizable2 = br_vec2_scale(vec_in_resizable, 2);
  br_vec2_t b = br_vec2_sub(ex.size.vec, vec_in_resizable2);
  br_vec2_t c = br_vec2_scale(b, 1.f/(float)ex.height);
  return BR_VEC2D(
    -(double)c.x*plot->dd.zoom.x/2.0 + plot->dd.offset.x,
     (double)c.y*plot->dd.zoom.y/2.0 + plot->dd.offset.y
  );
}

br_vec2_t br_plot2d_to_screen(br_plot_t* plot, br_vec2d_t vec) {
  br_extent_t ex = brui_resizable_cur_extent(plot->extent_handle);

  br_vec2d_t d = vec;
  d = br_vec2d_sub(d, plot->dd.offset);
  d = br_vec2d_scale(d, 2.0);
  d = br_vec2d_mul(d, BR_VEC2D(-1/plot->dd.zoom.x, 1/plot->dd.zoom.y));
  br_vec2_t f = BR_VEC2((float)d.x, (float)d.y);
  f = br_vec2_scale(f, ex.height);
  f = br_vec2_sub(ex.size.vec, f);
  f = br_vec2_scale(f, 0.5f);
  f = br_vec2_add(f, ex.pos);
  return f;
}

void br_plot_update_context(br_plot_t* plot, br_extent_t plot_screen_extent, br_vec2_t mouse_pos) {
  if (plot->kind == br_plot_kind_2d) {
    float aspect = plot_screen_extent.width/plot_screen_extent.height;
    plot->dd.mouse_pos = br_plot_2d_get_mouse_position(plot, mouse_pos);
    plot->dd.graph_rect = BR_EXTENTD(
      (-aspect*plot->dd.zoom.x/2.0 + plot->dd.offset.x),
      (plot->dd.zoom.y/2.0 + plot->dd.offset.y),
      (aspect*plot->dd.zoom.x),
      (plot->dd.zoom.y));
  } else {
    // TODO 2D/3D
    // BR_TODO("Update context for 3D");
  }
}

void br_plots_remove_group(br_plots_t plots, int group) {
  for (int i = 0; i < plots.len; ++i) {
    br_da_remove_feeld(plots.arr[i].data_info, group_id, group);
  }
}

static void br_plot_2d_draw(br_plot_t* plot, br_datas_t datas) {
  TracyCFrameMarkStart("br_datas_draw_2d");
  for (int j = 0; j < plot->data_info.len; ++j) {
    br_plot_data_t di = plot->data_info.arr[j];
    if (false == BR_PLOT_DATA_IS_VISIBLE(di)) continue;
    br_data_t const* g = br_data_get1(datas, di.group_id);
    if (g->len == 0) continue;
    g->resampling->culler.args.screen_size = BR_VEC2I_TOD(plot->cur_extent.size.vec);
    br_resampling_draw(g->resampling, g, plot, &di);
  }
}

static void br_plot_3d_draw(br_plot_t* plot, br_datas_t datas) {
  TracyCFrameMarkStart("br_datas_draw_3d");
  for (int j = 0; j < plot->data_info.len; ++j) {
    br_plot_data_t di = plot->data_info.arr[j];
    if (false == BR_PLOT_DATA_IS_VISIBLE(di)) continue;
    br_data_t const* g = br_data_get1(datas, di.group_id);
    if (g->len == 0) continue;
    br_resampling_draw(g->resampling, g, plot, &di);
  }
  TracyCFrameMarkEnd("br_datas_draw_3d");
}

