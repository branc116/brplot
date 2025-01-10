#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include ".generated/icons.c"
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#include "src/br_gl.h"
#include "src/br_icons.h"
#include "src/br_shaders.h"
#include "src/br_tl.h"
#include "src/br_math.h"
#include "src/br_pp.h"


static BR_THREAD_LOCAL GLuint icons_id = 0;

void br_icons_init(br_shader_icon_t* shader) {
  icons_id = brgl_load_texture(br_icons_atlas, br_icons_atlas_width, br_icons_atlas_height, BRGL_TEX_GRAY);
  shader->uvs.atlas_uv = icons_id;
}

// TODO: extent to bb
void br_icons_draw(br_shader_icon_t* shader, br_extent_t screen, br_extent_t atlas, br_color_t bg, br_color_t fg, int z) {
  br_sizei_t res = brtl_viewport().size;
  br_vec4_t bgv = BR_COLOR_TO4(bg);
  br_vec4_t fgv = BR_COLOR_TO4(fg);
  float gl_z = BR_Z_TO_GL(z);
  br_shader_icon_push_quad(shader, (br_shader_icon_el_t[4]) {
    { .pos = BR_VEC42(br_vec2_stog(screen.pos, res), atlas.pos), .bg  = bgv, .fg = fgv, .z = gl_z },
    { .pos = BR_VEC42(br_vec2_stog(br_extent_tr(screen), res), br_extent_tr(atlas)), .bg  = bgv, .fg = fgv, .z = gl_z },
    { .pos = BR_VEC42(br_vec2_stog(br_extent_br(screen), res), br_extent_br(atlas)), .bg  = bgv, .fg = fgv, .z = gl_z },
    { .pos = BR_VEC42(br_vec2_stog(br_extent_bl(screen), res), br_extent_bl(atlas)), .bg  = bgv, .fg = fgv, .z = gl_z },
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
