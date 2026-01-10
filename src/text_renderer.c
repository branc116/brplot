#include "src/br_text_renderer.h"
#include "src/br_da.h"
#include "src/br_free_list.h"
#include "src/br_gl.h"
#include "src/br_math.h"
#include "src/br_shaders.h"
#include "include/br_str_header.h"
#include "src/br_filesystem.h"
#include "src/br_memory.h"
#if !defined(BR_INCLUDE_ICONS_GEN_C)
#  define BR_INCLUDE_ICONS_GEN_C
#  include ".generated/icons.c"
#endif
#include ".generated/default_font.h"

#include "external/stb_rect_pack.h"
#include "external/stb_truetype.h"
#include "external/stb_ds.h"

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

typedef struct brtr_t {
  br_shaders_t* shaders;
  brtr_size_to_font_t* sizes;
  unsigned char* bitmap_pixels;
  int bitmap_pixels_len;
  int bitmap_pixels_height;
  int bitmap_pixels_width;
  unsigned int bitmap_texture_id;
  stbtt_pack_context pack_cntx;
  unsigned char const* font_data;
  unsigned char const* default_font_data;
  unsigned char*       custom_font_data;

  struct {stbtt_packedchar* arr; size_t len, cap; } baking_chars;
  struct {
    brtr_stack_el_t* arr;
    size_t len, cap;
  } stack;
  struct {
    br_extent_t* arr;
    int len, cap;

    int* free_arr;
    int free_len, free_next;
  } icons;

  bool is_dirty;
} brtr_t;

static BR_THREAD_LOCAL brtr_t brtr;

extern unsigned char const br_font_data[];
extern long long const br_font_data_size;

#if 0 // Maybe Usefull...
void br_icons_init(br_shader_icon_t* shader) {

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
#endif

static void brtr_icons_load(void);

void brtr_construct(int bitmap_width, int bitmap_height, br_shaders_t* shaders) {
  brtr.bitmap_pixels = BR_MALLOC((size_t)bitmap_width * (size_t)bitmap_height);
  brtr.shaders = shaders;
  brtr.bitmap_pixels_len = bitmap_width * bitmap_height;
  brtr.bitmap_pixels_width = bitmap_width;
  brtr.bitmap_pixels_height = bitmap_height;
  brtr.default_font_data = br_font_data;
  brtr.font_data = br_font_data;
  brtr.is_dirty = true;

  int res = stbtt_PackBegin(&brtr.pack_cntx, brtr.bitmap_pixels, bitmap_width, bitmap_height, 0, 1, NULL);
  stbtt_PackSetOversampling(&brtr.pack_cntx, 2, 2);
  if (res == 0) LOGE("Failed to load font");

  brtr_stack_el_t bot_el = {
    .ancor = br_dir_left_up,
    .font_size = 24,
    .z = 1,
    .forground = BR_COLOR(200, 200, 200, 255),
    .background = BR_COLOR(0,0,0,0),
  };
  br_da_push(brtr.stack, bot_el);

  brtr_icons_load();
}

bool brtr_font_load(br_strv_t path) {
  br_str_t content = { 0 };
  if (false == br_fs_read(path.str, &content)) return false;

  // Some TTF formats are not support by stbtt, of if font is not supported, don't event try to load that...
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
  brtr_free();

  // Set everythin to zero
  brtr.sizes = NULL;
  memset(&brtr.pack_cntx, 0, sizeof(brtr.pack_cntx));

  // Set new data
  brtr.custom_font_data = (unsigned char*)content.str;
  brtr.font_data = brtr.custom_font_data;
  brtr.bitmap_pixels = BR_MALLOC((size_t)brtr.bitmap_pixels_width * (size_t)brtr.bitmap_pixels_height);

  // Load new data
  int res = stbtt_PackBegin(&brtr.pack_cntx, brtr.bitmap_pixels, brtr.bitmap_pixels_width, brtr.bitmap_pixels_height, 0, 1, NULL);
  brtr.pack_cntx.skip_missing = true;
  if (res == 0) LOGE("Failed to load font");
  stbtt_PackSetOversampling(&brtr.pack_cntx, 2, 2);
  BR_LOGI("Loading font %.*s success, size=%ukB", path.len, path.str, content.len >> 10);
  brtr.is_dirty = true;
  brtr_icons_load();
  return true;
}

void brtr_free(void) {
  if (brtr.sizes != NULL) {
    for (long i = 0; i < stbds_hmlen(brtr.sizes); ++i) {
      stbds_hmfree(brtr.sizes[i].value);
    }
    stbds_hmfree(brtr.sizes);
  }
  stbtt_PackEnd(&brtr.pack_cntx);
  brgl_unload_texture(brtr.shaders->glyph->uvs.atlas_uv);
  brtr.shaders->glyph->uvs.atlas_uv = 0;
  br_da_free(brtr.baking_chars);
  if (brtr.custom_font_data) BR_FREE(brtr.custom_font_data);
  brfl_free(brtr.icons);
  BR_FREE(brtr.bitmap_pixels);
  brtr.bitmap_pixels = 0;
  br_da_free(brtr.stack);
}

static void brtr_bake_range(brtr_t* r, int size, br_u32 start, br_u32 len) {
  if (len == 0) return;
  r->is_dirty = true;
  br_da_reserve(r->baking_chars, (size_t)len);
  stbtt_PackFontRange(&r->pack_cntx, r->font_data, 0, (float)size, (br_i32)start, (br_i32)len, r->baking_chars.arr);
  ptrdiff_t k = stbds_hmgeti(r->sizes, size);
  if (k == -1) {
    stbds_hmput(r->sizes, size, NULL);
    k = stbds_hmgeti(r->sizes, size);
  }
  for (br_u32 j = 0; j < len; ++j) {
    br_u32 key = start + j;
    // NOTE: Even tho the len of baking_chars is not set, this is valid, because PackFontRange set the values for this.
    stbds_hmput(r->sizes[k].value, key, r->baking_chars.arr[j]);
  }
}

static void brtr_dump(void) {
  br_shader_glyph_t* g = brtr.shaders->glyph;
  if (brtr.is_dirty) {
    if (brtr.bitmap_texture_id) brgl_unload_texture(brtr.bitmap_texture_id);
    brtr.bitmap_texture_id = brgl_load_texture(brtr.bitmap_pixels, brtr.bitmap_pixels_width, brtr.bitmap_pixels_height, BRGL_TEX_GRAY, false);
    brtr.is_dirty = false;
    g->uvs.atlas_uv = brtr.bitmap_texture_id;
  }
  br_shader_glyph_draw(g);
}

brtr_stack_el_t* brtr_state_push(void) {
  //LOGE("PUSH: stack.len = %d", brtr.stack.len);
  BR_ASSERT(brtr.stack.len < 30);
  brtr_stack_el_t e = *brtr_state();
  br_da_push(brtr.stack, e);
  return brtr_state();
}

brtr_stack_el_t* brtr_state_pop(void) {
  //LOGE("POP: stack.len = %d", brtr.stack.len);
  BR_ASSERT(brtr.stack.len > 1);
  --brtr.stack.len;
  if (brtr.stack.len == 1) brtr_dump();
  return brtr_state();
}

brtr_stack_el_t* brtr_state(void) {
  return &brtr.stack.arr[brtr.stack.len - 1];
}

uint32_t brtr_texture_id(void) {
  return brtr.bitmap_texture_id;
}

static bool brtr_move_loc(brtr_size_to_font_t* s, br_u32 c, br_vec2_t* pos) {
  if ((char)c == '\n') {
    pos->y += (float)s->key * 1.1f;
    pos->x = 0;
    return true;
  }
  if ((char)c == '\r') return true;
  ptrdiff_t char_index = s->value == NULL ? -1 : stbds_hmgeti(s->value, c);
  if (char_index == -1) {
    brtr_bake_range(&brtr, s->key, c, 1);

    ptrdiff_t size_index = stbds_hmgeti(brtr.sizes, s->key);
    if (size_index != -1) *s = brtr.sizes[size_index];

    char_index = s->value == NULL ? -1 : stbds_hmgeti(s->value, c);
  }
  if (char_index == -1) pos->x += (float)s->key;
  else                  pos->x += s->value[char_index].value.xadvance;
  return true;
}

br_strv_t brtr_fit(br_strv_t text) {
  br_strv_t iter = text;
  BR_PROFILE("Text renderer fit") {
    brtr_stack_el_t* el = brtr_state();
    br_vec2_t loc = el->pos;
    brtr_size_to_font_t stof = { .key = el->font_size, .value = NULL };
    ptrdiff_t size_index = stbds_hmgeti(brtr.sizes, stof.key);

    if (size_index != -1) stof = brtr.sizes[size_index];
    BR_STRV_FOREACH_UTF8(iter, ch) {
      if (loc.x > el->limits.max.x) break;
      if (loc.y > el->limits.max.y) break;
      if (false == brtr_move_loc(&stof, ch, &loc)) break;
    }
  }
  return br_str_sub(text, 0, text.len - iter.len);
}

br_size_t brtr_measure(br_strv_t str) {
  br_vec2_t loc = { 0 };
  brtr_size_to_font_t f = { .key = brtr_state()->font_size };
  ptrdiff_t size_index = stbds_hmgeti(brtr.sizes, f.key);

  if (size_index != -1) f = brtr.sizes[size_index];
  BR_STRV_FOREACH_UTF8(str, ch) brtr_move_loc(&f, ch, &loc);
  return BR_SIZE(loc.x, loc.y + (float)brtr_state()->font_size);
}

br_extent_t brtr_push(br_strv_t text) {
  brtr_stack_el_t* el = brtr_state();
  int font_size = el->font_size;

  br_size_t size = brtr_measure(text);

  br_vec2_t start_pos = el->pos;
  br_vec2_t pos = el->pos;
  // This is some ttf bullshit, idk...
  pos.y -= (float)font_size*0.25f;

  if      (el->ancor & br_dir_mid_y) pos.y += (float)font_size * 0.5f;
  else if (el->ancor & br_dir_up)    pos.y += (float)font_size;

  if      (el->ancor & br_dir_mid_x) pos.x -= size.width * 0.5f;
  else if (el->ancor & br_dir_right) pos.x -= size.width;

  float width = fmaxf(el->pos.x, el->limits.max_x) - fmaxf(el->pos.x, el->limits.min_x);
  if      (el->justify & br_dir_mid_x) pos.x += width * 0.5f;
  else if (el->justify & br_dir_right) pos.x += width;

  float height = fmaxf(el->pos.y, el->limits.max_y) - fmaxf(el->pos.y, el->limits.min_y);
  if      (el->justify & br_dir_mid_y) pos.y += height * 0.5f;
  else if (el->justify & br_dir_down)  pos.y += height;
  /*
  */


  BR_PROFILE("text renderer push2") {
    br_size_t sz = el->viewport.size;
    br_vec4_t fg = BR_COLOR_TO4(el->forground);
    br_vec4_t bg = BR_COLOR_TO4(el->background);
    br_shader_glyph_t* glyph = brtr.shaders->glyph;
    br_bb_t limit = el->limits;

    float x = pos.x;
    float z = BR_Z_TO_GL(el->z);
    ptrdiff_t size_index = stbds_hmgeti(brtr.sizes, font_size);
    BR_ASSERT(size_index > -1);

    brtr_size_to_font_t s = brtr.sizes[size_index];
    BR_STRV_FOREACH_UTF8(text, ch) {
      if (ch == '\n') {
        pos.y += (float)font_size * 1.1f;
        pos.x = x;
        continue;
      }
      if (ch == '\r') continue;
      ptrdiff_t char_index = stbds_hmgeti(s.value, ch);
      if (char_index == -1) {
        ch = '?';
        char_index = stbds_hmgeti(s.value, ch);
        if (char_index == -1) {
          brtr_bake_range(&brtr, s.key, ch, 1);
          size_index = stbds_hmgeti(brtr.sizes, font_size);
          BR_ASSERT(size_index > -1);
          s = brtr.sizes[size_index];
        }
        char_index = stbds_hmgeti(s.value, ch);
        BR_ASSERT(char_index != -1);
      }

      stbtt_aligned_quad q;
      stbtt_packedchar pack = s.value[char_index].value;
      stbtt_GetPackedQuad(&pack, brtr.bitmap_pixels_width, brtr.bitmap_pixels_height, 0, &pos.x, &pos.y, &q, false);

      if (q.x0 > el->limits.max.x) continue;
      if (q.x1 < el->limits.min.x) continue;
      if (q.y0 > el->limits.max.y) continue;
      if (q.y1 < el->limits.min.y) continue;

      br_bb_t bb = BR_BB(q.x0, q.y0, q.x1, q.y1);
      br_bb_t tex = BR_BB(q.s0, q.t0, q.s1, q.t1);
      br_shader_glyph_push_quad(glyph, (br_shader_glyph_el_t[4]) {
          { .pos = BR_VEC42(br_vec2_stog(bb.min, sz), tex.min),             .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, bb.min),       .z = z },
          { .pos = BR_VEC42(br_vec2_stog(br_bb_tr(bb), sz), br_bb_tr(tex)), .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, br_bb_tr(bb)), .z = z },
          { .pos = BR_VEC42(br_vec2_stog(bb.max, sz), tex.max),             .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, bb.max),       .z = z },
          { .pos = BR_VEC42(br_vec2_stog(br_bb_bl(bb), sz), br_bb_bl(tex)), .fg = fg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, br_bb_bl(bb)), .z = z },
      });
    }
  }
  el->pos.x = pos.x;
  el->pos.y = pos.y;
  return BR_EXTENT(start_pos.x, pos.y, size.width, pos.y - start_pos.y);
}

void brtr_glyph_draw(br_bb_t where_screen, br_extent_t glyph) {
  brtr_stack_el_t* el = brtr_state();
  br_vec4_t bgc = BR_COLOR_TO4(el->background);
  br_vec4_t fgc = BR_COLOR_TO4(el->forground);
  float z = BR_Z_TO_GL(el->z);
  br_bb_t limit = el->limits;
  br_size_t sz = el->viewport.size;
  br_vec2_t a = where_screen.min, b = br_bb_tr(where_screen), c = where_screen.max, d = br_bb_bl(where_screen);
  br_vec2_t ag = glyph.pos, bg = br_extent_tr(glyph), cg = br_extent_br(glyph), dg = br_extent_bl(glyph);
  br_shader_glyph_push_quad(brtr.shaders->glyph, (br_shader_glyph_el_t[4]) {
    { .pos = BR_VEC42(br_vec2_stog(a, sz), ag), .fg = fgc, .bg = bgc, .clip_dists = br_bb_clip_dists(limit, a), .z = z },
    { .pos = BR_VEC42(br_vec2_stog(b, sz), bg), .fg = fgc, .bg = bgc, .clip_dists = br_bb_clip_dists(limit, b), .z = z },
    { .pos = BR_VEC42(br_vec2_stog(c, sz), cg), .fg = fgc, .bg = bgc, .clip_dists = br_bb_clip_dists(limit, c), .z = z },
    { .pos = BR_VEC42(br_vec2_stog(d, sz), dg), .fg = fgc, .bg = bgc, .clip_dists = br_bb_clip_dists(limit, d), .z = z },
  });
}

void brtr_rectangle_draw(br_bb_t where_screen) {
  brtr_stack_el_t* el = brtr_state();
  br_vec4_t bg = BR_COLOR_TO4(el->background);
  float z = BR_Z_TO_GL(el->z);
  br_bb_t limit = el->limits;
  br_vec2_t a = where_screen.min, b = br_bb_tr(where_screen), c = where_screen.max, d = br_bb_bl(where_screen);
  br_vec2_t center = br_vec2_scale(br_vec2_add(a, c), 0.5f);
  brtr.shaders->rect->uvs.viewport_uv = el->viewport.v4;
  br_shader_rect_push_quad(brtr.shaders->rect, (br_shader_rect_el_t[4]) {
    { .pos = BR_VEC42(a, center), .color = bg, .clip_dists = br_bb_clip_dists(limit, a), .z_and_round_factor = BR_VEC2(z, 0.1f) },
    { .pos = BR_VEC42(b, center), .color = bg, .clip_dists = br_bb_clip_dists(limit, b), .z_and_round_factor = BR_VEC2(z, 0.1f) },
    { .pos = BR_VEC42(c, center), .color = bg, .clip_dists = br_bb_clip_dists(limit, c), .z_and_round_factor = BR_VEC2(z, 0.1f) },
    { .pos = BR_VEC42(d, center), .color = bg, .clip_dists = br_bb_clip_dists(limit, d), .z_and_round_factor = BR_VEC2(z, 0.1f) },
  });
}

void brtr_triangle_draw(br_vec2_t a, br_vec2_t b, br_vec2_t c) {
  brtr_stack_el_t* el = brtr_state();
  br_vec4_t bg = BR_COLOR_TO4(el->background);
  br_vec2_t zero = BR_VEC2(0,0);
  float z = BR_Z_TO_GL(el->z);
  br_bb_t limit = el->limits;
  br_size_t sz = el->viewport.size;
  br_shader_glyph_push_tri(brtr.shaders->glyph, (br_shader_glyph_el_t[3]) {
    { .pos = BR_VEC42(br_vec2_stog(a, sz), zero), .fg = bg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, a), .z = z },
    { .pos = BR_VEC42(br_vec2_stog(b, sz), zero), .fg = bg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, b), .z = z },
    { .pos = BR_VEC42(br_vec2_stog(c, sz), zero), .fg = bg, .bg = bg, .clip_dists = br_bb_clip_dists(limit, c), .z = z },
  });
}

int brtr_icon_load(br_u8 const* buff, br_sizei_t buff_size, br_extenti_t icon_extent) {
  BR_ASSERT(icon_extent.x + icon_extent.width <= buff_size.width);
  BR_ASSERT(icon_extent.y + icon_extent.height <= buff_size.height);
  stbrp_rect rect = { .w = icon_extent.width, .h = icon_extent.height };
  stbrp_context* ctx = brtr.pack_cntx.pack_info;
  stbrp_pack_rects(ctx, &rect, 1);
  if (0 == rect.was_packed) return -1;
  br_u8* out = brtr.bitmap_pixels;
  for (int y = 0; y < rect.h; ++y) {
    for (int x = 0; x < rect.w; ++x) {
      out[(y+rect.y)*brtr.bitmap_pixels_width + (x+rect.x)] = buff[(y+icon_extent.y)*buff_size.width + (x+icon_extent.x)];
    }
  }
  brtr.is_dirty = true;
  br_extent_t tex_ex = BR_EXTENT(
    (float)rect.x/(float)brtr.bitmap_pixels_width,
    (float)rect.y/(float)brtr.bitmap_pixels_height,
    (float)rect.w/(float)brtr.bitmap_pixels_width,
    (float)rect.h/(float)brtr.bitmap_pixels_height);
  return brfl_push(brtr.icons, tex_ex);
}

br_extent_t brtr_icon(int icon) {
  return br_da_get(brtr.icons, icon);
}

static void brtr_icons_load(void) {
  size_t n = BR_ARR_LEN(br_icons.arr);
  for (size_t i = 0; i < n; ++i) {
    br_icons.arr[i].handle = brtr_icon_load(br_icons_atlas, BR_SIZEI(br_icons_atlas_width, br_icons_atlas_height), br_icons.arr[i].size);
  }
}
