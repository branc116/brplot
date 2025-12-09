#include "src/br_da.h"
#include "src/br_gl.h"
#include "src/br_math.h"
#include "src/br_pp.h"
#include "src/br_shaders.h"
#include "include/br_str_header.h"
#include "src/br_text_renderer.h"
#include "src/br_filesystem.h"
#include "src/br_memory.h"

#include "external/stb_rect_pack.h"
#include "external/stb_truetype.h"
#include "external/stb_ds.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>

typedef struct {
  br_u32 key;
  stbtt_packedchar value;
} brtr_key_to_packedchar_t;

typedef struct {
  br_i32 key;
  brtr_key_to_packedchar_t* value;
} brtr_size_to_font_t;

typedef struct {
  int size;
  br_u32 ch;
} brtr_char_sz_t;

typedef struct {
  brtr_char_sz_t key;
  br_u32 value;
} brtr_to_bake_t;

typedef struct br_text_renderer_t {
  br_shader_font_t** shader_f;
  brtr_size_to_font_t* sizes;
  brtr_to_bake_t* to_bake;
  unsigned char* bitmap_pixels;
  int bitmap_pixels_len;
  int bitmap_pixels_height;
  int bitmap_pixels_width;
  unsigned int bitmap_texture_id;
  stbtt_pack_context pack_cntx;
  unsigned char const* font_data;
  unsigned char const* default_font_data;
  unsigned char* custom_font_data;
  br_sizei_t viewport;

  struct {stbtt_aligned_quad* arr; size_t len, cap; } tmp_quads;
  struct {stbtt_packedchar* arr; size_t len, cap; } baking_chars;
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
    .default_font_data = font_data,
    .font_data = font_data
  };
  int res = stbtt_PackBegin(&t->r.pack_cntx, t->r.bitmap_pixels, bitmap_width, bitmap_height, 0, 1, NULL);
  stbtt_PackSetOversampling(&t->r.pack_cntx, 2, 2);
  if (res == 0) LOGE("Failed to load font");
  return &t->r;
}

bool br_text_renderer_load_font(br_text_renderer_t* r, br_strv_t path) {
  br_str_t content = { 0 };
  if (false == br_fs_read(path.str, &content)) return false;

  // Some TTF formats are not support by stbtt, of if font is not supported, don't event try to load that font...
  stbtt_fontinfo info = { 0 };
  stbtt_InitFont(&info, (unsigned char*)content.str, stbtt_GetFontOffsetForIndex((unsigned char*)content.str, 0));
  uint8_t *data = info.data;
  int32_t index_map = info.index_map;
  uint8_t* addr = data + index_map + 0;
  uint16_t format = (uint16_t)((uint16_t)(addr[0])*256 + (uint16_t)addr[1]);
  if (format != 0 && format != 6 && format != 4 && format != 12 && format != 13) {
    LOGI("Font format %d not supported..", format);
    return false;
  }

  // Free old
  stbds_hmfree(r->to_bake);
  for (long i = 0; i < stbds_hmlen(r->sizes); ++i) {
    stbds_hmfree(r->sizes[i].value);
  }
  stbds_hmfree(r->sizes);
  stbtt_PackEnd(&r->pack_cntx);
  br_da_free(r->tmp_quads);
  br_da_free(r->baking_chars);
  if (r->custom_font_data) BR_FREE(r->custom_font_data);

  // Set everythin to zero
  r->sizes = NULL;
  r->to_bake = NULL;
  memset(&r->pack_cntx, 0, sizeof(r->pack_cntx));

  // Set new data
  r->custom_font_data = (unsigned char*)content.str;
  r->font_data = r->custom_font_data;

  // Load new data
  int res = stbtt_PackBegin(&r->pack_cntx, r->bitmap_pixels, r->bitmap_pixels_width, r->bitmap_pixels_height, 0, 1, NULL);
  r->pack_cntx.skip_missing = true;
  if (res == 0) LOGE("Failed to load font");
  stbtt_PackSetOversampling(&r->pack_cntx, 2, 2);
  r->tmp_quads.len = 0;
  BR_LOGI("Loading font %.*s success, size=%ukB", path.len, path.str, content.len >> 10);
  return true;
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
  br_da_free(r->baking_chars);
  if (r->custom_font_data) BR_FREE(r->custom_font_data);
  BR_FREE(r);
}

static int br_text_renderer_sort_by_size(const void* s1, const void* s2) {
  brtr_to_bake_t a = *(brtr_to_bake_t*)s1, b = *(brtr_to_bake_t*)s2;
  if (a.key.size < b.key.size) return -1;
  else if (a.key.size > b.key.size) return 1;
  else if (a.key.ch < b.key.ch) return -1;
  else if (a.key.ch > b.key.ch) return 1;
  else return 0;
}

static void br_text_renderer_bake_range(br_text_renderer_t* r, int size, br_u32 start, br_u32 len) {
  r->baking_chars.len = 0;
  br_da_reserve(r->baking_chars, (size_t)len);
  r->baking_chars.len = (size_t)len;
  stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, (float)size, (br_i32)start, (br_i32)len, br_da_getp(r->baking_chars, 0));
  ptrdiff_t k = stbds_hmgeti(r->sizes, size);
  if (k == -1) {
    stbds_hmput(r->sizes, size, NULL);
    k = stbds_hmgeti(r->sizes, size);
  }
  for (br_u32 j = 0; j < len; ++j) {
    br_u32 key = start + j;
    stbds_hmput(r->sizes[k].value, key, br_da_get(r->baking_chars, j));
  }
}

void br_text_renderer_dump(br_text_renderer_t* r) {
  ptrdiff_t hm_len = stbds_hmlen(r->to_bake);
  if (hm_len > 0) {
    qsort(r->to_bake, (size_t)hm_len, sizeof(r->to_bake[0]), br_text_renderer_sort_by_size);
    int old_size = r->to_bake[0].key.size;
    int pack_from = 0;
    for (int i = 1; i < hm_len; ++i) {
      int new_size = r->to_bake[i].key.size;
      br_u32 p_len = r->to_bake[i].key.ch - r->to_bake[i - 1].key.ch;
      if (new_size == old_size && p_len < 10) continue;
      p_len = 1 + r->to_bake[i - 1].key.ch - r->to_bake[pack_from].key.ch;
      br_text_renderer_bake_range(r, old_size, r->to_bake[pack_from].key.ch, p_len);
      old_size = new_size;
      pack_from = i;
    }
    if (pack_from < hm_len) {
      br_u32 s_len = 1 + r->to_bake[hm_len - 1].key.ch - r->to_bake[pack_from].key.ch;
      br_text_renderer_bake_range(r, old_size, r->to_bake[pack_from].key.ch, s_len);
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

uint32_t br_text_renderer_texture_id(br_text_renderer_t* r) {
  return r->bitmap_texture_id;
}

static bool brtr_move_loc(brtr_size_to_font_t s, br_u32 c, br_vec2_t* pos) {
  if ((char)c == '\n') {
    pos->y += (float)s.key * 1.1f;
    pos->x = 0;
    return true;
  }
  if ((char)c == '\r') return true;
  ptrdiff_t char_index = s.value == NULL ? -1 : stbds_hmgeti(s.value, c);
  if (char_index == -1) pos->x += (float)s.key;
  else                  pos->x += s.value[char_index].value.xadvance;
  return true;
}

br_strv_t br_text_renderer_fit(br_text_renderer_t* r, br_size_t size, int font_size, br_strv_t text) {
  br_strv_t iter = text;
  BR_PROFILE("Text renderer fit") {
    br_vec2_t loc = { 0 };
    brtr_size_to_font_t stof = { .key = font_size, .value = NULL };
    ptrdiff_t size_index = stbds_hmgeti(r->sizes, font_size);

    if (size_index != -1) stof = r->sizes[size_index];
    BR_STRV_FOREACH_UTF8(iter, ch) {
      if (loc.x > size.width) break;
      if (loc.y > size.height) break;
      if (false == brtr_move_loc(stof, ch, &loc)) break;
    }
  }
  return br_str_sub(text, 0, text.len - iter.len);
}

br_size_t br_text_renderer_measure(br_text_renderer_t* r, int font_size, br_strv_t str) {
  br_vec2_t loc = { 0 };
  brtr_size_to_font_t f = { 0 };
  ptrdiff_t size_index = stbds_hmgeti(r->sizes, font_size);

  if (size_index == -1) return BR_SIZE(0, 0);
  f = r->sizes[size_index];
  BR_STRV_FOREACH_UTF8(str, ch) brtr_move_loc(f, ch, &loc);
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
  float min_x = FLT_MAX, max_x = FLT_MIN;
  float x_off = 0.f;
  if      (ancor & br_text_renderer_ancor_y_mid)  pos.y -= (float)font_size * 0.5f;
  else if (ancor & br_text_renderer_ancor_y_down) pos.y -= (float)font_size;
  pos.y += (float)font_size*0.75f;
  BR_PROFILE("text renderer push2") {
    float x = pos.x, z = pos.z;
    ptrdiff_t size_index = stbds_hmgeti(r->sizes, font_size);
    float og_x = pos.x;
    r->tmp_quads.len = 0;
    if (size_index == -1) {
      BR_STRV_FOREACH_UTF8(text, ch) stbds_hmput(r->to_bake, ((brtr_char_sz_t){ .size = font_size, .ch = ch }), 0);
    } else {
      brtr_size_to_font_t s = r->sizes[size_index];
      BR_STRV_FOREACH_UTF8(text, ch) {
        if (ch == '\n') {
          pos.y += (float)font_size * 1.1f;
          pos.x = og_x;
          continue;
        }
        if (ch == '\r') continue;
        ptrdiff_t char_index = stbds_hmgeti(s.value, ch);
        if (char_index == -1) {
          stbds_hmput(r->to_bake, ((brtr_char_sz_t){ .size = font_size, .ch = ch }), 0);
        } else {
          stbtt_aligned_quad q;
          stbtt_packedchar pack = s.value[char_index].value;
          stbtt_GetPackedQuad(&pack, r->bitmap_pixels_width, r->bitmap_pixels_height, 0, &pos.x, &pos.y, &q, false);
          br_da_push(r->tmp_quads, q);
        }
      }
    }
#define MMIN(x, y) (x < y ? x : y)
#define MMAX(x, y) (x > y ? x : y)
    for (size_t i = 0; i < r->tmp_quads.len; ++i) {
      min_x = MMIN(r->tmp_quads.arr[i].x0, min_x);
      max_x = MMAX(r->tmp_quads.arr[i].x1, max_x);
    }
#undef MMAX
#undef MMIN
    if (ancor & br_text_renderer_ancor_x_left)       x_off = min_x - x;
    else if (ancor & br_text_renderer_ancor_x_mid)   x_off = (max_x + min_x) * 0.5f - x;
    else if (ancor & br_text_renderer_ancor_x_right) x_off = max_x - x;
    br_sizei_t sz = r->viewport;
    br_vec4_t fg = BR_COLOR_TO4(color_fg);
    br_vec4_t bg = BR_COLOR_TO4(color_bg);
    for (size_t i = 0; i < r->tmp_quads.len; ++i) {
      br_bb_t bb = BR_BB(r->tmp_quads.arr[i].x0, r->tmp_quads.arr[i].y0, r->tmp_quads.arr[i].x1, r->tmp_quads.arr[i].y1);
      bb.min_x -= x_off;
      bb.max_x -= x_off;
      br_bb_t tex = BR_BB(r->tmp_quads.arr[i].s0, r->tmp_quads.arr[i].t0, r->tmp_quads.arr[i].s1, r->tmp_quads.arr[i].t1);
      br_shader_font_push_quad(*r->shader_f, (br_shader_font_el_t[4]) {
          { .pos = BR_VEC42(br_vec2_stog(bb.min, sz), tex.min),             .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, bb.min),       .z = z },
          { .pos = BR_VEC42(br_vec2_stog(br_bb_tr(bb), sz), br_bb_tr(tex)), .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, br_bb_tr(bb)), .z = z },
          { .pos = BR_VEC42(br_vec2_stog(bb.max, sz), tex.max),             .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, bb.max),       .z = z },
          { .pos = BR_VEC42(br_vec2_stog(br_bb_bl(bb), sz), br_bb_bl(tex)), .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, br_bb_bl(bb)), .z = z },
      });
    }
  }
  return BR_EXTENT(min_x - x_off, pos.y, max_x - min_x, (float)font_size);
}

void br_text_renderer_viewport_set(br_text_renderer_t* r, br_sizei_t viewport) {
  r->viewport = viewport;
}
