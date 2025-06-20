#include "src/br_da.h"
#include "src/br_gl.h"
#include "src/br_math.h"
#include "src/br_pp.h"
#include "src/br_shaders.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"

#include "external/stb_rect_pack.h"
#include "external/stb_truetype.h"
#include "external/stb_ds.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>

typedef struct {
  char key;
  stbtt_packedchar value;
} key_to_packedchar_t;

typedef struct {
  int key;
  key_to_packedchar_t* value;
} size_to_font;

typedef struct {
  int size;
  char ch;
} char_sz;

typedef struct {
  char_sz key;
  unsigned char value; 
} to_bake_t;

typedef struct br_text_renderer_t {
  br_shader_font_t** shader_f;
  br_shader_fontbg_t** shader_bg;
  size_to_font* sizes;
  to_bake_t* to_bake;
  unsigned char* bitmap_pixels;
  int bitmap_pixels_len;
  int bitmap_pixels_height;
  int bitmap_pixels_width;
  unsigned int bitmap_texture_id;
  stbtt_pack_context pack_cntx;
  unsigned char const* font_data;

  struct {stbtt_aligned_quad* arr; size_t len, cap; } tmp_quads;
} br_text_renderer_t;

br_text_renderer_t* br_text_renderer_malloc(int bitmap_width, int bitmap_height, unsigned char const* font_data, br_shader_font_t** shader) {
  size_t total_mem = sizeof(br_text_renderer_t) + (size_t)bitmap_width * (size_t)bitmap_height;
  struct tmp {
    br_text_renderer_t r;
    unsigned char data[];
  };
  
  struct tmp* t = BR_MALLOC(total_mem);
  t->r = (br_text_renderer_t) {
    .shader_f = shader,
    .sizes = NULL,
    .to_bake = NULL,
    .bitmap_pixels = t->data,
    .bitmap_pixels_len = bitmap_width * bitmap_height,
    .bitmap_pixels_width = bitmap_width,
    .bitmap_pixels_height = bitmap_height,
    .bitmap_texture_id = 0,
    .pack_cntx = {0},
    .font_data = font_data
  };
  int res = stbtt_PackBegin(&t->r.pack_cntx, t->r.bitmap_pixels, bitmap_width, bitmap_height, 0, 1, NULL);
  stbtt_PackSetOversampling(&t->r.pack_cntx, 2, 2);
  if (res == 0) fprintf(stderr, "Failed to pack begin\n");
  return &t->r;
}

void br_text_renderer_free(br_text_renderer_t* r) {
  stbds_hmfree(r->to_bake);
  for (long i = 0; i < stbds_hmlen(r->sizes); ++i) {
    stbds_hmfree(r->sizes[i].value);
  }
  stbds_hmfree(r->sizes);
  stbtt_PackEnd(&r->pack_cntx);
  brgl_unload_texture((*r->shader_f)->uvs.atlas_uv);
  br_da_free(r->tmp_quads);
  BR_FREE(r);
}

static int br_text_renderer_sort_by_size(const void* s1, const void* s2) {
  to_bake_t a = *(to_bake_t*)s1, b = *(to_bake_t*)s2;
  if (a.key.size < b.key.size) return -1;
  else if (a.key.size > b.key.size) return 1;
  else if (a.key.ch < b.key.ch) return -1;
  else if (a.key.ch > b.key.ch) return 1;
  else return 0;
}

void br_text_renderer_dump(br_text_renderer_t* r) {
  stbtt_packedchar charz[256] = {0};
  ptrdiff_t hm_len = stbds_hmlen(r->to_bake);
  if (hm_len > 0) {
    qsort(r->to_bake, (size_t)hm_len, sizeof(r->to_bake[0]), br_text_renderer_sort_by_size);
    int old_size = r->to_bake[0].key.size;
    int pack_from = 0;
    for (int i = 1; i < hm_len; ++i) {
      int new_size = r->to_bake[i].key.size;
      int p_len = r->to_bake[i].key.ch - r->to_bake[i - 1].key.ch;
      if (new_size == old_size && p_len < 10) continue;
      p_len = 1 + r->to_bake[i - 1].key.ch - r->to_bake[pack_from].key.ch;
      stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, (float)old_size, r->to_bake[pack_from].key.ch, p_len, &charz[0]);
      ptrdiff_t k = stbds_hmgeti(r->sizes, old_size);
      if (k == -1) {
        stbds_hmput(r->sizes, old_size, NULL);
        k = stbds_hmgeti(r->sizes, old_size);
      }
      for (int j = 0; j < p_len; ++j) {
        char key = ((char)(r->to_bake[pack_from].key.ch + j));
        stbds_hmput(r->sizes[k].value, key, charz[j]);
      }
      old_size = new_size;
      pack_from = i;
    }
    if (pack_from < hm_len) {
      int s_len = 1 + r->to_bake[hm_len - 1].key.ch - r->to_bake[pack_from].key.ch;
      stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, (float)old_size, r->to_bake[pack_from].key.ch, s_len, &charz[0]);
      ptrdiff_t k = stbds_hmgeti(r->sizes, old_size);
      if (k == -1) {
        stbds_hmput(r->sizes, old_size, NULL);
        k = stbds_hmgeti(r->sizes, old_size);
      }
      for (int j = 0; j < s_len; ++j) {
        char key = r->to_bake[pack_from].key.ch + (char)j;
        stbds_hmput(r->sizes[k].value, key, charz[j]);
      }
    }
    stbds_hmfree(r->to_bake);
    brgl_unload_texture(r->bitmap_texture_id);
    r->bitmap_texture_id = brgl_load_texture(r->bitmap_pixels, r->bitmap_pixels_width, r->bitmap_pixels_height, BRGL_TEX_GRAY);
  }
  br_shader_font_t* simp = *r->shader_f;
  simp->uvs.atlas_uv = r->bitmap_texture_id;
  br_shader_font_draw(simp);
  simp->len = 0;
}

static bool brtr_move_loc(br_text_renderer_t* tr, size_to_font s, char c, br_vec2_t* pos) {
  if (c == '\n') {
    pos->y += (float)s.key * 1.1f;
    pos->x = 0;
    return true;
  }
  if (c == '\r') return true;
  ptrdiff_t char_index = stbds_hmgeti(s.value, c);
  if (char_index == -1) {
    pos->x += (float)s.key;
  } else {
    stbtt_packedchar ch = s.value[char_index].value;
    stbtt_aligned_quad q;
    stbtt_GetPackedQuad(&ch, tr->bitmap_pixels_width, tr->bitmap_pixels_height, 0, &pos->x, &pos->y, &q, false);
  }
  return true;
}

br_strv_t br_text_renderer_fit(br_text_renderer_t* r, br_size_t size, int font_size, br_strv_t text) {
  ssize_t i = 0;
  BR_PROFILE("Text renderer fit") {
    br_vec2_t loc = { 0 };
    ptrdiff_t size_index = stbds_hmgeti(r->sizes, font_size);
    r->tmp_quads.len = 0;
    br_extent_t exf = BR_EXTENT(0, 0, (float)size.width, (float)size.height);
    if (size_index == -1) {
      // We don't have the font baked so be conservative
      for (; i < (ssize_t)text.len; ++i) {
        char c = text.str[i];
        if (c == '\n') {
          loc.y += (float)font_size * 1.1f;
          loc.x = 0;
        } else if (c == '\r') continue;
        else loc.x += (float)font_size * 1.1f;
        if (false == br_col_vec2_extent(exf, loc)) break;
      }
    } else {
      size_to_font f = r->sizes[size_index];
      for (; i < (ssize_t)text.len; ++i) {
        if (loc.x > size.width) break;
        if (loc.y > size.height) break;
        if (false == brtr_move_loc(r, f, text.str[i], &loc)) break;
      }
    }
  }
  return br_str_sub(text, 0, (uint32_t)(i < 0 ? 0 : i));
}

br_size_t br_text_renderer_measure(br_text_renderer_t* r, int font_size, br_strv_t str) {
  br_vec2_t loc = { 0 };
  ptrdiff_t size_index = stbds_hmgeti(r->sizes, font_size);
  ssize_t i = 0;
  if (size_index == -1) return BR_SIZE(0, 0);
  size_to_font f = r->sizes[size_index];
  for (; i < (ssize_t)str.len; ++i) brtr_move_loc(r, f, str.str[i], &loc);
  return BR_SIZE(loc.x, loc.y);
}

br_extent_t br_text_renderer_push0(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, const char* text) {
  return br_text_renderer_push(r, pos, font_size, color_fg, color_bg, text, BR_BB(0,0,10000,10000));
}

br_extent_t br_text_renderer_push_strv0(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, br_strv_t text) {
  return br_text_renderer_push_strv(r, pos, font_size, color_fg, color_bg, text, BR_BB(0,0,10000,10000));
}

br_extent_t br_text_renderer_push(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, const char* text, br_bb_t limit) {
  return br_text_renderer_push2(r, pos, font_size, color_fg, color_bg, br_strv_from_c_str(text), limit, br_text_renderer_ancor_left_up);
}

br_extent_t br_text_renderer_push_strv(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, br_strv_t text, br_bb_t limit) {
  return br_text_renderer_push2(r, pos, font_size, color_fg, color_bg, text, limit, br_text_renderer_ancor_left_up);
}

br_extent_t br_text_renderer_push2(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, br_strv_t text, br_bb_t limit, br_text_renderer_ancor_t ancor) {
  float min_y = FLT_MAX, max_y = FLT_MIN, min_x = FLT_MAX, max_x = FLT_MIN;
  float y_off = 0.f;
  float x_off = 0.f;
  BR_PROFILE("text renderer push2") {
    float x = pos.x, y = pos.y, z = pos.z;
    ptrdiff_t size_index = stbds_hmgeti(r->sizes, font_size);
    float og_x = pos.x;
    r->tmp_quads.len = 0;
    if (size_index == -1) {
      for (size_t i = 0; i < text.len; ++i) {
        stbds_hmput(r->to_bake, ((char_sz){ .size = font_size, .ch = text.str[i] }), 0);
      }
    } else {
      size_to_font s = r->sizes[size_index];
      for (size_t i = 0; i < text.len; ++i) {
        ptrdiff_t char_index = stbds_hmgeti(s.value, text.str[i]);
        if (text.str[i] == '\n') {
          pos.y += (float)font_size * 1.1f;
          pos.x = og_x;
          continue;
        }
        if (text.str[i] == '\r') continue;
        if (char_index == -1) {
          stbds_hmput(r->to_bake, ((char_sz){ .size = font_size, .ch = text.str[i] }), 0);
        } else {
          stbtt_aligned_quad q;
          stbtt_packedchar ch = s.value[char_index].value;
          stbtt_GetPackedQuad(&ch, r->bitmap_pixels_width, r->bitmap_pixels_height, 0, &pos.x, &pos.y, &q, false);
          br_da_push(r->tmp_quads, q);
        }
      }
    }
#define MMIN(x, y) (x < y ? x : y)
#define MMAX(x, y) (x > y ? x : y)
    for (size_t i = 0; i < r->tmp_quads.len; ++i) {
      min_y = MMIN(r->tmp_quads.arr[i].y0, min_y);
      min_x = MMIN(r->tmp_quads.arr[i].x0, min_x);
      max_y = MMAX(r->tmp_quads.arr[i].y1, max_y);
      max_x = MMAX(r->tmp_quads.arr[i].x1, max_x);
    }
#undef MMAX
#undef MMIN
    if (ancor & br_text_renderer_ancor_y_up)         y_off = min_y - y;
    else if (ancor & br_text_renderer_ancor_y_mid)   y_off = (max_y + min_y) * 0.5f - y;
    else if (ancor & br_text_renderer_ancor_y_down)  y_off = max_y - y;
    if (ancor & br_text_renderer_ancor_x_left)       x_off = min_x - x;
    else if (ancor & br_text_renderer_ancor_x_mid)   x_off = (max_x + min_x) * 0.5f - x;
    else if (ancor & br_text_renderer_ancor_x_right) x_off = max_x - x;
    br_sizei_t sz = brtl_viewport().size;
    br_vec4_t fg = BR_COLOR_TO4(color_fg);
    br_vec4_t bg = BR_COLOR_TO4(color_bg);
    for (size_t i = 0; i < r->tmp_quads.len; ++i) {
      br_bb_t bb = br_bb_sub(BR_BB(r->tmp_quads.arr[i].x0, r->tmp_quads.arr[i].y0, r->tmp_quads.arr[i].x1, r->tmp_quads.arr[i].y1), BR_VEC2(x_off, y_off));
      br_bb_t tex = BR_BB(r->tmp_quads.arr[i].s0, r->tmp_quads.arr[i].t0, r->tmp_quads.arr[i].s1, r->tmp_quads.arr[i].t1);
      br_shader_font_push_quad(*r->shader_f, (br_shader_font_el_t[4]) {
          { .pos = BR_VEC42(br_vec2_stog(bb.min, sz), tex.min),             .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, bb.min),       .z = z },
          { .pos = BR_VEC42(br_vec2_stog(br_bb_tr(bb), sz), br_bb_tr(tex)), .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, br_bb_tr(bb)), .z = z },
          { .pos = BR_VEC42(br_vec2_stog(bb.max, sz), tex.max),             .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, bb.max),       .z = z },
          { .pos = BR_VEC42(br_vec2_stog(br_bb_bl(bb), sz), br_bb_bl(tex)), .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, br_bb_bl(bb)), .z = z },
      });
    }
  }
  return BR_EXTENT(min_x - x_off, min_y - y_off, max_x - min_x, max_y - min_y);
}

