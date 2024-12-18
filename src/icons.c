#include ".generated/icons.c"
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

void br_icons_draw(br_shader_icon_t* shader, br_extent_t screen, br_extent_t atlas, br_vec4_t bg, br_vec4_t fg) {
  br_sizei_t res = brtl_window_size();
  br_vec4_t* vecs = (br_vec4_t*)&shader->pos_vbo[3*4*shader->len];
  br_vec4_t* fgs = (br_vec4_t*)&shader->fg_vbo[3*4*shader->len];
  br_vec4_t* bgs = (br_vec4_t*)&shader->bg_vbo[3*4*shader->len];
  vecs[0] = (br_vec4_t) { .xy = br_vec2_stog(screen.pos, res), .zw = atlas.pos };
  vecs[1] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_top_right(screen), res), .zw = br_extent_top_right(atlas) };
  vecs[2] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_bottom_right(screen), res), .zw = br_extent_bottom_right(atlas) };
  vecs[3] = (br_vec4_t) { .xy = br_vec2_stog(screen.pos, res), .zw = atlas.pos };
  vecs[4] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_bottom_right(screen), res), .zw = br_extent_bottom_right(atlas) };
  vecs[5] = (br_vec4_t) { .xy = br_vec2_stog(br_extent_bottom_left(screen), res), .zw = br_extent_bottom_left(atlas) };
  for (int i = 0; i < 6; ++i) bgs[i] = (bg);
  for (int i = 0; i < 6; ++i) fgs[i] = (fg);
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
