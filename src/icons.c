#include ".generated/icons.c"
#include ".generated/default_font.h"

#include "src/br_gl.h"
#include "src/br_icons.h"
#include "src/br_shaders.h"
#include "src/br_tl.h"
#include "src/br_math.h"
#include "src/br_pp.h"

static BR_THREAD_LOCAL GLuint icons_id = 0;

struct br_extra_icons_t br_extra_icons;

void br_icons_init(br_shader_icon_t* shader) {
  icons_id = brgl_load_texture(br_icons_atlas, br_icons_atlas_width, br_icons_atlas_height, BRGL_TEX_GRAY);
  shader->uvs.atlas_uv = icons_id;

#define I br_icons.edge.size_8
  br_extra_icons = (struct br_extra_icons_t) {
    .edge_tl.size_8 = BR_EXTENT(I.x, I.y + I.height, I.width, -I.height),
    .edge_tr.size_8 = BR_EXTENT(I.x + I.width, I.y + I.height, -I.width, -I.height),
    .edge_br.size_8 = BR_EXTENT(I.x + I.width, I.y, -I.width, I.height),

    .edge_b.size_8 = BR_EXTENT(I.x + 7.5f/8.0f * I.width, I.y + 6.5f/8.0f*I.height, 0.01f/8.0f*I.width, 1.4f/8.f*I.height),
    .edge_t.size_8 = BR_EXTENT(I.x + 7.5f/8.0f * I.width, I.y + 7.9f/8.0f*I.height, 0.01f/8.0f*I.width, -1.4f/8.f*I.height),
    .edge_l.size_8 = BR_EXTENT(I.x, I.y, I.width * 1.4f/8.0f, I.height * 0.01f/8.0f),
    .edge_r.size_8 = BR_EXTENT(I.x + 1.4f/8.0f * I.width, I.y, -I.width * 1.4f/8.0f, I.height * 0.01f/8.0f),
  };
#undef I
}

void br_icons_draw(br_bb_t screen, br_bb_t atlas, br_color_t bg, br_color_t fg, br_bb_t limit, int z) {
  br_sizei_t res = brtl_viewport().size;
  br_vec4_t bgv = BR_COLOR_TO4(bg);
  br_vec4_t fgv = BR_COLOR_TO4(fg);
  float gl_z = BR_Z_TO_GL(z);

  br_vec2_t stl = screen.min;
  br_vec2_t str = br_bb_tr(screen);
  br_vec2_t sbr = screen.max;
  br_vec2_t sbl = br_bb_bl(screen);

  br_vec2_t atl = atlas.min;
  br_vec2_t atr = br_bb_tr(atlas);
  br_vec2_t abr = atlas.max;
  br_vec2_t abl = br_bb_bl(atlas);
  br_shader_icon_push_quad(brtl_shaders()->icon, (br_shader_icon_el_t[4]) {
    { .pos = BR_VEC42(br_vec2_stog(stl, res), atl), .bg  = bgv, .fg = fgv, .clip_dists = br_bb_clip_dists(limit, stl), .z = gl_z },
    { .pos = BR_VEC42(br_vec2_stog(str, res), atr), .bg  = bgv, .fg = fgv, .clip_dists = br_bb_clip_dists(limit, str), .z = gl_z },
    { .pos = BR_VEC42(br_vec2_stog(sbr, res), abr), .bg  = bgv, .fg = fgv, .clip_dists = br_bb_clip_dists(limit, sbr), .z = gl_z },
    { .pos = BR_VEC42(br_vec2_stog(sbl, res), abl), .bg  = bgv, .fg = fgv, .clip_dists = br_bb_clip_dists(limit, sbl), .z = gl_z },
  });
}

void br_icons_deinit(void) {
  brgl_unload_texture(icons_id);
}

br_extent_t br_icons_y_mirror(br_extent_t icon) {
  icon.x += icon.width;
  icon.width *= -1.f;
  return icon;
}

br_extent_t br_icons_top(br_extent_t icon, float percent) {
  icon.height *= percent;
  return icon;
}

br_extent_t br_icons_bot(br_extent_t icon, float percent);
br_extent_t br_icons_left(br_extent_t icon, float percent);
br_extent_t br_icons_right(br_extent_t icon, float percent);

br_extent_t br_icons_tc(br_extent_t icon, float top, float center) {
  icon.height *= top;
  icon.x += icon.width * 0.5f * (1.f - center);
  icon.width *= center;
  return icon;
}

br_extent_t br_icons_lc(br_extent_t icon, float left, float center) {
  icon.width *= left;
  icon.y += icon.height * 0.5f * (1.f - center);
  icon.height *= center;
  return icon;
}

br_extent_t br_icons_bc(br_extent_t icon, float top, float center) {
  icon.y += icon.height * (1.f - top);
  icon.height *= top;
  icon.x += icon.width * 0.5f * (1.f - center);
  icon.width *= center;
  return icon;
}

br_extent_t br_icons_rc(br_extent_t icon, float left, float center) {
  icon.x += icon.width * (1.f - left);
  icon.width *= left;
  icon.y += icon.height * 0.5f * (1.f - center);
  icon.height *= center;
  return icon;
}
