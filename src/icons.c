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


#define SCREEN_TO_GL(point, screen) BR_VEC2((float)(point).x / (float)(screen).width * 2.f - 1.f, (float)(point).y / (float)(screen).height * 2.f - 1.f)
static _Thread_local GLuint icons_id = 0;

void br_icons_init(br_shader_icon_t* shader) {
  icons_id = brgl_load_texture(br_icons_atlas, 64, 64, BRGL_TEX_GRAY);
  shader->uvs.atlas_uv = icons_id;
}

// TODO: extent to bb
void br_icons_draw(br_shader_icon_t* shader, br_extent_t screen, br_extent_t atlas, br_color_t bg, br_color_t fg, float z) {
  br_sizei_t res = brtl_window_size();
  br_vec4_t* vecs = (br_vec4_t*)&shader->pos_vbo[3*4*shader->len];
  br_vec4_t* fgs = (br_vec4_t*)&shader->fg_vbo[3*4*shader->len];
  br_vec4_t* bgs = (br_vec4_t*)&shader->bg_vbo[3*4*shader->len];
  float* zs = (float*)&shader->z_vbo[3*shader->len];
  vecs[0] = (br_vec4_t) { .xy = br_vec2_stog(screen.pos, res), .zw = atlas.pos };
  vecs[1] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_tr(screen), res), .zw = br_extent_tr(atlas) };
  vecs[2] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_br(screen), res), .zw = br_extent_br(atlas) };
  vecs[3] = (br_vec4_t) { .xy = br_vec2_stog(screen.pos, res), .zw = atlas.pos };
  vecs[4] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_br(screen), res), .zw = br_extent_br(atlas) };
  vecs[5] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_bl(screen), res), .zw = br_extent_bl(atlas) };
  for (int i = 0; i < 6; ++i) bgs[i] = BR_COLOR_TO4(bg);
  for (int i = 0; i < 6; ++i) fgs[i] = BR_COLOR_TO4(fg);
  for (int i = 0; i < 6; ++i) zs[i]  = z;
  shader->len += 2;
}

void br_icons_deinit(void) {
  brgl_unload_texture(icons_id);
}

br_extent_t br_icons_y_mirror(br_extent_t icon) {
  icon.x += icon.width;
  icon.width *= -1.f;
  return icon;
}
