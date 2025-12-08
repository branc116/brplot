#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_gl.h"
#include "src/br_gui.h"
#include "src/br_math.h"
#include "src/br_plot.h"
#include "src/br_resampling.h"
#include "src/br_ui.h"
#include "src/br_memory.h"
#include "src/br_anim.h"

static BR_THREAD_LOCAL struct {
  br_anims_t* anims;
} br_plot_state;

void br_plot_construct(br_anims_t* anims) {
  br_plot_state.anims = anims;
}

br_plot_data_t br_plot_data(int group_id) {
  br_plot_data_t ret;
  ret.group_id = group_id;
  ret.thickness_multiplyer_ah = br_animf_new(br_plot_state.anims, 0.f, 1.f);
  return ret;
}

bool br_plot_data_is_visible(br_plot_data_t pd) {
  return br_da_getp(br_plot_state.anims->all, pd.thickness_multiplyer_ah)->f.target > 0.1f;
}

void br_plot_deinit(br_plot_t* plot) {
  br_da_free(plot->data_info);
  brui_resizable_delete(plot->extent_handle);
  brgl_destroy_framebuffer(plot->texture_id);
}

void br_plot_create_texture(br_plot_t* br, br_extent_t extent) {
  br->texture_id = brgl_create_framebuffer((int)extent.width, (int)extent.height);
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

br_vec2d_t br_plot_2d_get_mouse_position(br_plot_t* plot, br_vec2_t screen_mouse_pos, br_extent_t ex) {
  br_vec2i_t mouse_pos = BR_VEC2_TOI(screen_mouse_pos);
  br_vec2i_t mp_in_graph = BR_VEC2I_SUB(mouse_pos, BR_VEC2_TOI(ex.pos));
  br_vec2i_t a = BR_VEC2I_SCALE(mp_in_graph, 2);
  br_vec2_t b = br_vec2i_tof(BR_VEC2I_SUB(BR_VEC2_TOI(ex.size.vec), a));
  br_vec2_t c = br_vec2_scale(b, 1.f/(float)ex.height);
  return BR_VEC2D(
    -c.x*plot->dd.zoom.x/2.0 + plot->dd.offset.x,
     c.y*plot->dd.zoom.y/2.0 + plot->dd.offset.y
  );
}

br_vec2d_t br_plot2d_to_plot(br_plot_t* plot, br_vec2_t vec, br_extent_t ex) {
  br_vec2_t vec_in_resizable = br_vec2_sub(vec, ex.pos);
  br_vec2_t vec_in_resizable2 = br_vec2_scale(vec_in_resizable, 2);
  br_vec2_t b = br_vec2_sub(ex.size.vec, vec_in_resizable2);
  br_vec2_t c = br_vec2_scale(b, 1.f/(float)ex.height);
  return BR_VEC2D(
    -(double)c.x*plot->dd.zoom.x/2.0 + plot->dd.offset.x,
     (double)c.y*plot->dd.zoom.y/2.0 + plot->dd.offset.y
  );
}

br_vec2_t br_plot2d_to_screen(br_plot_t* plot, br_vec2d_t vec, br_extent_t ex) {
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

br_vec3d_t br_plot3d_to_plot(br_plot_t* plot, br_vec2_t mouse_pos, br_extent_t ex) {
  br_mat_t per = br_mat_perspective(plot->ddd.fov_y, ex.width / ex.height, plot->ddd.near_plane, plot->ddd.far_plane);
  br_mat_t look = br_mat_look_at(plot->ddd.eye, plot->ddd.target, plot->ddd.up);
  br_mat_t mvp = br_mat_mul(look, per);
  br_mat_t imvp = br_mat_transpose(br_mat_inverse(mvp));
  br_vec2_t mp_ndc = br_vec2_div(br_vec2_sub(mouse_pos, ex.pos), ex.size.vec);
  mp_ndc = br_vec2_scale(mp_ndc, 2.f);
  mp_ndc = br_vec2_sub(mp_ndc, BR_VEC2(1.f, 1.f));
  br_vec4_t v = BR_VEC4(mp_ndc.x, -mp_ndc.y, 1.f, 1.f);
  br_vec4_t w = br_vec4_apply(v, imvp);
  br_vec4d_t wd = BR_VEC4_TOD(w);
  return br_vec3d_scale(wd.xyz, 1.0/wd.w);
}

br_vec2_t br_plot3d_to_screen(br_plot_t* plot, br_vec3_t pos, br_extent_t ex) {
  br_mat_t per = br_mat_perspective(plot->ddd.fov_y, ex.width / ex.height, plot->ddd.near_plane, plot->ddd.far_plane);
  br_mat_t look = br_mat_look_at(plot->ddd.eye, plot->ddd.target, plot->ddd.up);
  br_mat_t mvp = br_mat_mul(look, per);
  br_vec3_t w = br_vec3_transform_scale(pos, mvp);
  br_vec2_t w2 = w.xy;
  w2 = br_vec2_mul(w2, BR_VEC2(0.5f, -0.5f));
  w2 = br_vec2_add(w2, BR_VEC2(0.5f, 0.5f));
  w2 = br_vec2_mul(w2, ex.size.vec);
  w2 = br_vec2_add(w2, ex.pos);
  return w2;
}

void br_plot_update_context(br_plot_t* plot, br_extent_t plot_screen_extent, br_vec2_t mouse_pos) {
  if (plot->kind == br_plot_kind_2d) {
    float aspect = plot_screen_extent.width/plot_screen_extent.height;
    plot->dd.mouse_pos = br_plot_2d_get_mouse_position(plot, mouse_pos, plot_screen_extent);
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

void br_plots_focus_visible(br_plots_t plots, br_datas_t const groups) {
  for (int i = 0; i < plots.len; ++i) {
    if (plots.arr[i].kind != br_plot_kind_2d) continue;
    br_plot_focus_visible(&plots.arr[i], groups, brui_resizable_cur_extent(plots.arr[i].extent_handle));
  }
}

void br_plot_focus_visible(br_plot_t* plot, br_datas_t const groups, br_extent_t ex) {
  // TODO 2D/3D
  BR_ASSERT(plot->kind == br_plot_kind_2d);
  if (plot->data_info.len == 0) return;

  br_bb_t bb = { 0 };
  int n = 0;
  for (int i = 0; i < plot->data_info.len; ++i) {
    br_plot_data_t di = plot->data_info.arr[i];
    if (false == br_plot_data_is_visible(di)) continue;
    br_data_t* d = br_data_get1(groups, di.group_id);
    if (n > 0) {
      if (d->len > 0) {
        br_bb_t this_bb = br_bb_add(d->dd.bounding_box, BR_VEC2((float)d->dd.rebase_x, (float)d->dd.rebase_y));
        bb = br_bb_union(bb, this_bb);
        ++n;
      }
    } else {
      if (d->len > 0) {
        bb = br_bb_add(d->dd.bounding_box, BR_VEC2((float)d->dd.rebase_x, (float)d->dd.rebase_y));
        ++n;
      }
    }
  }
  if (n == 0) return;

  float new_width = BR_BBW(bb);
  float new_height = BR_BBH(bb);
  bb.max_x += new_width  * .1f;
  bb.max_y += new_height * .1f;
  bb.min_x -= new_width  * .1f;
  bb.min_y -= new_height * .1f;
  new_width = BR_BBW(bb);
  new_height = BR_BBH(bb);
  br_vec2_t bl = bb.min;
  if (0) {
    float maxSize = fmaxf(new_width, new_height);
    plot->dd.zoom.x = BR_EXTENTD_ASPECT(ex) * maxSize;
    plot->dd.zoom.y = new_height;
    plot->dd.offset.x = bl.x + maxSize / 2.f;
    plot->dd.offset.y = bl.y + maxSize / 2.f;
  } else {
    plot->dd.offset.x = bl.x + new_width / 2.f;
    plot->dd.offset.y = bl.y + new_height / 2.f;
    plot->dd.zoom.x = new_width;
    plot->dd.zoom.y = new_height;
  }
}

