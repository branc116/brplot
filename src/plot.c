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
  br_anim_delete(br_plot_state.anims, plot->ddd.eye_ah);
  br_anim_delete(br_plot_state.anims, plot->ddd.target_ah);
}

void br_plot_create_texture(br_plot_t* br, br_extent_t extent) {
  br->texture_id = brgl_create_framebuffer((int)extent.width, (int)extent.height);
}

void br_plot2d_move_screen_space(br_plot_t* plot, br_vec2_t delta, br_size_t plot_screen_size) {
  float height = plot_screen_size.height;
  br_vec2d_t zoom = br_anim2d(br_plot_state.anims, plot->dd.zoom_ah);
  br_vec2d_t real_delta = br_vec2d_mul(zoom, br_vec2d_mul(BR_VEC2_TOD(delta), BR_VEC2D(-1.0/height, 1.0/height)));
  br_vec2d_t new_offset = br_vec2d_add(real_delta, br_anim2d_get_target(br_plot_state.anims, plot->dd.offset_ah));
  br_anim2d_set(br_plot_state.anims, plot->dd.offset_ah, new_offset);
}

static br_vec2d_t br_plot2d_to_plot_target(br_plot_t* plot, br_vec2_t vec, br_extent_t ex) {
  br_vec2_t vec_in_resizable = br_vec2_sub(vec, ex.pos);
  br_vec2_t vec_in_resizable2 = br_vec2_scale(vec_in_resizable, 2);
  br_vec2_t b = br_vec2_sub(ex.size.vec, vec_in_resizable2);
  br_vec2_t c = br_vec2_scale(b, 1.f/(float)ex.height);
  br_vec2d_t zoom = br_anim2d_get_target(br_plot_state.anims, plot->dd.zoom_ah);
  br_vec2d_t offset = br_anim2d_get_target(br_plot_state.anims, plot->dd.offset_ah);
  return BR_VEC2D(
    -(double)c.x*zoom.x/2.0 + offset.x,
     (double)c.y*zoom.y/2.0 + offset.y
  );
}

void br_plot2d_zoom(br_plot_t* plot, br_vec2_t vec, br_extent_t screen_extent, br_vec2_t mouse_pos_screen) {
  // TODO: br_plot2d_to_plot should prob be br_plot2d_to_plot_target...
  br_vec2d_t old = br_plot2d_to_plot_target(plot, mouse_pos_screen, screen_extent);
  bool any = false;
  br_vec2d_t zoom = br_anim2d(br_plot_state.anims, plot->dd.zoom_ah);
  if (false == br_float_near_zero(vec.x)) {
    float mw_scale = (1 + vec.x/10);
    zoom.x *= mw_scale;
    any = true;
  }
  if (false == br_float_near_zero(vec.y)) {
    float mw_scale = (1 + vec.y/10);
    zoom.y *= mw_scale;
    any = true;
  }
  if (false == any) return;

  br_anim2d_set(br_plot_state.anims, plot->dd.zoom_ah, zoom);
  br_vec2d_t now = br_plot2d_to_plot_target(plot, mouse_pos_screen, screen_extent);

  br_vec2d_t offset = br_anim2d_get_target(br_plot_state.anims, plot->dd.offset_ah);
  offset.x -= now.x - old.x;
  offset.y -= now.y - old.y;
  br_anim2d_set(br_plot_state.anims, plot->dd.offset_ah, offset);
}

br_vec2d_t br_plot2d_to_plot(br_plot_t* plot, br_vec2_t vec, br_extent_t ex) {
  br_vec2_t vec_in_resizable = br_vec2_sub(vec, ex.pos);
  br_vec2_t vec_in_resizable2 = br_vec2_scale(vec_in_resizable, 2);
  br_vec2_t b = br_vec2_sub(ex.size.vec, vec_in_resizable2);
  br_vec2_t c = br_vec2_scale(b, 1.f/(float)ex.height);
  br_vec2d_t zoom = br_anim2d(br_plot_state.anims, plot->dd.zoom_ah);
  br_vec2d_t offset = br_anim2d(br_plot_state.anims, plot->dd.offset_ah);
  return BR_VEC2D(
    -(double)c.x*zoom.x/2.0 + offset.x,
     (double)c.y*zoom.y/2.0 + offset.y
  );
}

br_vec2_t br_plot2d_to_screen(br_plot_t* plot, br_vec2d_t vec, br_extent_t ex) {
  br_vec2d_t d = vec;
  br_vec2d_t zoom = br_anim2d(br_plot_state.anims, plot->dd.zoom_ah);
  br_vec2d_t offset = br_anim2d(br_plot_state.anims, plot->dd.offset_ah);
  d = br_vec2d_sub(d, offset);
  d = br_vec2d_scale(d, 2.0);
  d = br_vec2d_mul(d, BR_VEC2D(-1/zoom.x, 1/zoom.y));
  br_vec2_t f = BR_VEC2((float)d.x, (float)d.y);
  f = br_vec2_scale(f, ex.height);
  f = br_vec2_sub(ex.size.vec, f);
  f = br_vec2_scale(f, 0.5f);
  f = br_vec2_add(f, ex.pos);
  return f;
}

br_vec3d_t br_plot3d_to_plot(br_plot_t* plot, br_vec2_t mouse_pos, br_extent_t ex) {
  br_mat_t per = br_mat_perspective(plot->ddd.fov_y, ex.width / ex.height, plot->ddd.near_plane, plot->ddd.far_plane);
  br_vec3_t eye = br_anim3(br_plot_state.anims, plot->ddd.eye_ah);
  br_vec3_t target = br_anim3(br_plot_state.anims, plot->ddd.target_ah);
  br_mat_t look = br_mat_look_at(eye, target, plot->ddd.up);
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
  br_vec3_t eye = br_anim3(br_plot_state.anims, plot->ddd.eye_ah);
  br_vec3_t target = br_anim3(br_plot_state.anims, plot->ddd.target_ah);
  br_mat_t look = br_mat_look_at(eye, target, plot->ddd.up);
  br_mat_t mvp = br_mat_mul(look, per);
  br_vec3_t w = br_vec3_transform_scale(pos, mvp);
  br_vec2_t w2 = w.xy;
  w2 = br_vec2_mul(w2, BR_VEC2(0.5f, -0.5f));
  w2 = br_vec2_add(w2, BR_VEC2(0.5f, 0.5f));
  w2 = br_vec2_mul(w2, ex.size.vec);
  w2 = br_vec2_add(w2, ex.pos);
  return w2;
}

br_extentd_t br_plot2d_extent_to_plot(br_plot_t plot, br_extent_t screen_extent) {
  BR_ASSERT(plot.kind == br_plot_kind_2d);
  br_vec2d_t zoom = br_anim2d(br_plot_state.anims, plot.dd.zoom_ah);
  br_vec2d_t offset = br_anim2d(br_plot_state.anims, plot.dd.offset_ah);
  float aspect = screen_extent.width/screen_extent.height;
  return BR_EXTENTD(
    (-aspect*zoom.x/2.0 + offset.x),
    (zoom.y/2.0 + offset.y),
    aspect*zoom.x,
    zoom.y);
}

void br_plot_remove_group(br_plot_t* plot, int group_id) {
  for (int j = 0; j < plot->data_info.len; ++j) {
    br_plot_data_t pd = br_da_get(plot->data_info, j);
    if (pd.group_id == group_id) br_anim_delete(br_plot_state.anims, pd.thickness_multiplyer_ah);
  }
  br_da_remove_feeld(plot->data_info, group_id, group_id);
}

void br_plots_remove_group(br_plots_t plots, int group) {
  for (int i = 0; i < plots.len; ++i) {
    br_plot_remove_group(br_da_getp(plots, i), group);
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
  BR_ASSERTF(plot->kind == br_plot_kind_2d, "Plot kind: %d", plot->kind);
  if (plot->data_info.len == 0) return;
  (void)ex;

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

  br_vec2d_t offset = BR_VEC2D(bl.x + new_width / 2.0, bl.y + new_height / 2.0);
  br_vec2d_t zoom  = BR_VEC2D(new_width, new_height);
  br_anim2d_set(br_plot_state.anims, plot->dd.offset_ah, offset);
  br_anim2d_set(br_plot_state.anims, plot->dd.zoom_ah, zoom);
}

