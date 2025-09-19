#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_gl.h"
#include "src/br_gui.h"
#include "src/br_math.h"
#include "src/br_plot.h"
#include "src/br_resampling.h"
#include "src/br_tl.h"
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

void br_plot_update_shader_values(br_plot_t* plot, br_shaders_t* shaders) {
  br_extent_t const ex = BR_EXTENTI_TOF(plot->cur_extent);
  switch (plot->kind) {
    case br_plot_kind_2d: {
      TracyCFrameMarkStart("update_shader_values_2d");
      br_vec2d_t zoom = plot->dd.zoom;
      br_vec2d_t zoom_log = { .x = pow(10.0, -floor(log10(zoom.x))), .y = pow(10.0, -floor(log10(zoom.y))) };
      br_vec2d_t zoom_final = { .x = zoom.x * zoom_log.x, .y = zoom.y * zoom_log.y };
      shaders->grid->uvs.zoom_uv = BR_VEC2D_TOF(zoom_final);
      br_vec2d_t off_zoom = br_vec2d_mul(plot->dd.offset, zoom_log);
      br_vec2d_t off = br_vec2d_mul(off_zoom, BR_VEC2D(0.1f, 0.1f));
      br_vec2d_t off_final = br_vec2d_sub(off_zoom, BR_VEC2D(floor(off.x) * 10.0, floor(off.y) * 10.0));
      shaders->grid->uvs.offset_uv = BR_VEC2D_TOF(off_final);
      shaders->grid->uvs.screen_uv = ex.size.vec;
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

void br_plot_update_context(br_plot_t* plot, br_vec2_t mouse_pos) {
  br_extent_t ex = BR_EXTENTI_TOF(plot->cur_extent);
  if (plot->kind == br_plot_kind_2d) {
    float aspect = ex.width/ex.height;
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

