#include "src/br_pp.h"
#include "src/br_string_pool.h"
#include "src/br_ui.h"
#include "src/br_icons.h"
#include "src/br_math.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"
#include "src/br_theme.h"
#include "src/br_da.h"
#include "src/br_tl.h"
#include "src/br_gl.h"
#include "src/br_shaders.h"
#include "src/br_free_list.h"

#include "external/stb_ds.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static BR_THREAD_LOCAL brui_stack_t brui__stack;
static BR_THREAD_LOCAL brui_resizable_temp_t* bruir__temp_res = NULL;
static BR_THREAD_LOCAL int brui_max_z = 0;
BR_THREAD_LOCAL int brui__n__;

BR_THREAD_LOCAL char brui__scrach[2048];
#define TOP (brui__stack.arr[brui__stack.len ? brui__stack.len - 1 : 0])
#define ACTION (brui__stack.action.kind)
#define ACPARM (brui__stack.action.args)
#define TOP2 (brui__stack.arr[brui__stack.len > 1 ? brui__stack.len - 2 : 0])
#define Z (brui_max_z = br_i_max(brui_max_z, TOP.z), ++TOP.z)
#define ZGL BR_Z_TO_GL(Z)
#define RETURN_IF_OUT1(HEIGHT, ...) do { \
  float h = (HEIGHT); \
  if (TOP.cur.y < TOP.limit.y) { BRUI_LOG("skip"); TOP.cur.y += h; return __VA_ARGS__; } \
  if (TOP.is_out || (TOP.cur.y + h) > TOP.limit.height) { TOP.cur.y += h; BRUI_LOG("skip: cur.y=%f", TOP.cur.y); TOP.is_out = true; return __VA_ARGS__; } \
} while(0)
#if 1
#define BRUI_LOG(fmt, ...) do { \
  if (brui__stack.log == false) break; \
  for (size_t i = 0; i < brui__stack.len; ++i) { \
    printf("  "); \
  } \
  if (brui__stack.len > 0) { \
    printf("[CUR:%.3f,%.3f][LIMIT:%.2f,%.2f,%.2f,%.2f][PSUM:%.2f %.2f][TOPZ:%d] " fmt "\n", TOP.cur.x, TOP.cur.y, BR_BB_(TOP.limit), TOP.psum.x, TOP.psum.y, TOP.z, ##__VA_ARGS__); \
  } else { \
    printf(fmt "\n", ##__VA_ARGS__); \
  } \
} while(0)
#else
#define BRUI_LOG(...)
#endif

brui_stack_el_t brui_stack_el(void) {
  if (brui__stack.len > 0) {
    brui_stack_el_t new_el = TOP;
    new_el.psum.x += TOP.padding.x;
    new_el.cur.x = new_el.limit.min_x + new_el.psum.x;
    new_el.cur.y += TOP.padding.y;
    new_el.start_z = new_el.z;
    new_el.hide_border = false;
    new_el.hide_bg = false;
    new_el.content_height = new_el.padding.y;
    return new_el;
  } else {
    brui_stack_el_t root = {
      .limit = BR_EXTENTI_TOBB(brtl_viewport()),
      .padding = BR_THEME.ui.padding,
      .font_size = BR_THEME.ui.font_size,
      .font_color = BR_THEME.colors.btn_txt_inactive,
      .text_ancor = br_text_renderer_ancor_left_up,
      .is_active = true
    };
    root.cur = root.padding;
    root.content_height = root.padding.y;
    return root;
  }
}

#if TRACY_ENABLE
BR_THREAD_LOCAL static struct ___tracy_source_location_data brui_begin_end_bench = { "UI Begin End", "Static",  __FILE__, (uint32_t)__LINE__, 0 };
TracyCZoneCtx brui_begin_end_ctx;
#endif

void brui_begin(void) {
#if TRACY_ENABLE
  brui_begin_end_ctx = ___tracy_emit_zone_begin_callstack(&brui_begin_end_bench, 0, true );
#endif
  brui__stack.log = ACTION == brui_action_none && brtl_key_pressed('u');
  BRUI_LOG("begin");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(brui__stack, new_el);
  BRUI_LOG("beginafter");
}

void brui_end(void) {
  BR_ASSERT(brui__stack.len == 1);
  brui__stack.len = 0;
  BRUI_LOG("end");
#if TRACY_ENABLE
  ___tracy_emit_zone_end(brui_begin_end_ctx);
#endif
}

void brui_push(void) {
  BRUI_LOG("push");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(brui__stack, new_el);
  TOP.background_color = BR_THEME.colors.plot_menu_color;
}

bool brui_pop(void) {
  float width = BR_BBW(TOP.limit) - 2 * fminf(TOP2.psum.x, TOP.psum.x);
  float height = TOP.content_height;
  br_size_t size = BR_SIZE(width, height);
  br_bb_t bb = BR_BB(TOP2.cur.x, TOP2.cur.y, TOP2.cur.x + size.width, TOP2.cur.y + size.height);
  float content_height = TOP.content_height;
  bool clicked = false;

  if (TOP.hide_bg == false) brui_background(bb, BR_THEME.colors.plot_menu_color);
  if (TOP.hide_border == false) brui_border1(bb);
  if (TOP.is_active && brtl_mousel_pressed() && br_col_vec2_bb(bb, brtl_mouse_pos())) {
    clicked = true;
  }

  BRUI_LOG("Pre pop ex: [%.2f,%.2f,%.2f,%.2f]", BR_BB_(bb));
  float tt = TOP.cur.y;
  brui_pop_simple();
  TOP.cur.y = tt + TOP.padding.y;
  TOP.cur.x = TOP.limit.min_x + TOP.psum.x;
  TOP.content_height += content_height + TOP.padding.y;
  BRUI_LOG("pop");
  return clicked;
}

void brui_push_simple(void) {
  BRUI_LOG("push simp");
  brui_stack_el_t new_el = TOP;
  br_da_push(brui__stack, new_el);
  BRUI_LOG("push simp post");
}

void brui_pop_simple(void) {
  --brui__stack.len;
}

void brui_push_y(float y) {
  TOP.cur.y += y;
}

void brui_select_next(void) {
  brui__stack.select_next = true;
}

static inline bool brui_extent_is_good(br_extent_t e, br_extent_t parent) {
  return e.x >= 0 &&
    e.y >= 0 &&
    e.y + e.height <= parent.height &&
    e.x + e.width <= parent.width &&
    e.height > 100 &&
    e.width > 100;
}

br_size_t brui_text(br_strv_t strv) {
  br_extent_t ex = { 0 };
  BR_PROFILE("brui_text") {
    BRUI_LOG("Text: %.*s", strv.len, strv.str);
    br_text_renderer_t* tr = brtl_text_renderer();
    br_vec2_t loc = TOP.cur;
    float out_top /* neg or 0 */ = fminf(TOP.cur.y - TOP.limit.min_y, 0.f);
    float opt_height = (float)TOP.font_size + TOP.padding.y;

    br_size_t space_left = BR_SIZE(BR_BBW(TOP.limit) - 2 * TOP.psum.x - (TOP.cur.x - (TOP.limit.min_x + TOP.psum.x)), TOP.limit.max_y - TOP.cur.y + out_top);
    BRUI_LOG("TEXT: Space left %f %f", space_left.width, space_left.height);
    br_strv_t fit = br_text_renderer_fit(tr, space_left, TOP.font_size, strv);
    if (fit.len != 0) {
      if (TOP.text_ancor & br_text_renderer_ancor_y_mid) loc.y += BR_BBH(TOP.limit) * 0.5f;
      if (TOP.text_ancor & br_text_renderer_ancor_x_mid) loc.x += BR_BBW(TOP.limit) * 0.5f;
      br_bb_t text_limit = TOP.limit;
      text_limit.min_x += TOP.psum.x;
      text_limit.max_x -= TOP.psum.x;

      ex = br_text_renderer_push2(tr, BR_VEC3(loc.x, loc.y, BR_Z_TO_GL(TOP.z + 1)), TOP.font_size, TOP.font_color, TOP.background_color, fit, text_limit, TOP.text_ancor);
    }
    TOP.cur.x = TOP.limit.min_x + TOP.psum.x;
    TOP.cur.y += opt_height;
    TOP.content_height += opt_height;
  }
  return ex.size;
}

bool brui_text_input(brsp_id_t str_id) {
  brsp_t* sp = brtl_brsp();
  br_strv_t strv = brsp_get(*sp, str_id);
  bool changed = false;
  BRUI_LOG("Text: %.*s", strv.len, strv.str);
  br_text_renderer_t* tr = brtl_text_renderer();
  bool is_active = brui_action_typing == ACTION && str_id == ACPARM.text.id;

  br_vec2_t loc = TOP.cur;
  if (is_active) loc.x -= ACPARM.text.offset_x;
  float out_top /* neg or 0 */ = fminf(TOP.cur.y - TOP.limit.min_y, 0.f);
  float opt_height = (float)TOP.font_size + TOP.padding.y;
  br_size_t space_left = BR_SIZE(TOP.limit.max_x - loc.x, TOP.limit.max_y - TOP.cur.y + out_top);
  br_strv_t fit = br_text_renderer_fit(tr, space_left, TOP.font_size, strv);
  br_extent_t ex = BR_EXTENT(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.cur.x, (float)TOP.font_size);
  br_bb_t text_limit = TOP.limit;
  float text_height = (float)TOP.font_size;
  if (fit.len != 0) {
    if (TOP.text_ancor & br_text_renderer_ancor_y_mid) loc.y += BR_BBH(TOP.limit) * 0.5f;
    if (TOP.text_ancor & br_text_renderer_ancor_x_mid) loc.x += BR_BBW(TOP.limit) * 0.5f;
    text_limit.min_x += TOP.psum.x;
    text_limit.max_x -= TOP.psum.x;

    text_height = br_text_renderer_push2(tr, BR_VEC3(loc.x, loc.y, BR_Z_TO_GL(TOP.z + 1)), TOP.font_size, TOP.font_color, TOP.background_color, fit, text_limit, TOP.text_ancor).height;
  }
  TOP.cur.x = TOP.limit.min_x + TOP.psum.x;
  TOP.cur.y += opt_height;
  TOP.content_height += opt_height;
  if (is_active) {
    int cp = ACPARM.text.cursor_pos;
    float half_thick = 1.0f;
    br_pressed_chars_t pressed = brtl_pressed_chars();
    brtl_pressed_clear();
    for (size_t i = 0; i < pressed.len; ++i) {
      brtl_pressed_char_t p = br_da_get(pressed, i);
      if (brtl_key_ctrl()) {
        if (p.key == 'v') {
          br_strv_t content = brtl_clipboard();
          for (uint32_t j = 0; j < content.len; ++j) {
            brsp_insert_char(brtl_brsp(), str_id, cp, (uint8_t)content.str[j]);
            cp = ++ACPARM.text.cursor_pos;
          }
          changed = true;
        }
      } else {
        uint32_t lp = p.key;
        if (p.is_special) {
          if (lp == BR_KEY_LEFT) while (ACPARM.text.cursor_pos > 0 && (strv.str[--ACPARM.text.cursor_pos] & 0b11000000) == 0b10000000);
          else if (lp == BR_KEY_RIGHT) while (ACPARM.text.cursor_pos < (int)strv.len && (strv.str[++ACPARM.text.cursor_pos] & 0b11000000) == 0b10000000);
          else if (lp == BR_KEY_ESCAPE) ACTION = brui_action_none;
          else if (lp == BR_KEY_DELETE) {
            changed = brsp_remove_utf8_after(sp, str_id, ACPARM.text.cursor_pos) > 0;
          } else if (lp == BR_KEY_BACKSPACE) {
            while (ACPARM.text.cursor_pos > 0 && ((strv.str[--ACPARM.text.cursor_pos] & 0b11000000) == 0b10000000));
            changed = brsp_remove_utf8_after(sp, str_id, ACPARM.text.cursor_pos) > 0;
          } else if (lp == BR_KEY_HOME) {
            ACPARM.text.cursor_pos = 0;
            changed = true;
          }
          else if (lp == BR_KEY_END) {
            ACPARM.text.cursor_pos = (int)brsp_get(*sp, str_id).len;
            changed = true;
          }
        } else if (p.key >= 32 && p.key != 127) {
          int inserted = brsp_insert_unicode(brtl_brsp(), str_id, cp, p.key);
          ACPARM.text.cursor_pos += inserted;
          changed = true;
        }
      }
      strv = brsp_get(*brtl_brsp(), str_id);
    }
    if ((uint32_t)ACPARM.text.cursor_pos > strv.len) ACPARM.text.cursor_pos = (int)strv.len;
    br_size_t size = br_text_renderer_measure(tr, TOP.font_size, br_str_sub(strv, 0, (uint32_t)ACPARM.text.cursor_pos));
    loc.x += size.width - 1.0f;
    text_limit.min_x -= 1.f;
    brui_rectangle(BR_BB(loc.x - half_thick, loc.y, loc.x + half_thick, loc.y + (float)text_height), text_limit, TOP.font_color, TOP.z + 2);
    if (loc.x - (ACPARM.text.offset_x_target - ACPARM.text.offset_x) > TOP.limit.max_x - 20.f) ACPARM.text.offset_x_target += 20.f;
    if (loc.x + (ACPARM.text.offset_x - ACPARM.text.offset_x_target) < TOP.limit.min_x + 20.f) ACPARM.text.offset_x_target = br_float_clamp(ACPARM.text.offset_x_target-20.f, 0.f, ACPARM.text.offset_x_target);
  } else {
    if (TOP.is_active) {
      if (brtl_mousel_pressed()) {
        br_bb_t bb = BR_EXTENT_TOBB(ex);
        if (br_col_vec2_bb(bb, brtl_mouse_pos())) {
          ACTION = brui_action_typing;
          ACPARM.text.cursor_pos = 0;
          ACPARM.text.id = str_id;
        }
      }
    }
  }
  return changed;
}

void brui_new_lines(int n) {
  float off = ((float)TOP.font_size + TOP.padding.y) * (float)n;
  TOP.cur.y += off;
  TOP.content_height += off;
}

void brui_background(br_bb_t bb, br_color_t color) {
  br_icons_draw(bb, BR_BB(0,0,0,0), color, color, TOP.limit, TOP.start_z);
}

void brui_border1(br_bb_t bb) {
  brui_border2(bb, TOP.is_active && br_col_vec2_bb(bb, brtl_mouse_pos()));
}

void brui_border2(br_bb_t bb, bool active) {
  br_color_t ec = BR_THEME.colors.ui_edge_inactive;
  br_color_t bc = BR_THEME.colors.ui_edge_bg_inactive;

  float edge = 0;
  float thick = BR_THEME.ui.border_thick;

  if (active) {
    ec = BR_THEME.colors.ui_edge_active;
    bc = BR_THEME.colors.ui_edge_bg_active;
  }

  int z = Z;
  br_icons_draw(BR_BB(bb.min_x + edge, bb.min_y, bb.max_x - edge, bb.min_y + thick), BR_EXTENT_TOBB(br_extra_icons.edge_t.size_8), ec, bc, TOP.limit, z);
  br_icons_draw(BR_BB(bb.min_x + edge, bb.max_y - thick, bb.max_x - edge, bb.max_y), BR_EXTENT_TOBB(br_extra_icons.edge_b.size_8), ec, bc, TOP.limit, z);
  br_icons_draw(BR_BB(bb.min_x, bb.min_y + edge, bb.min_x + thick, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_l.size_8), ec, bc, TOP.limit, z);
  br_icons_draw(BR_BB(bb.max_x - thick, bb.min_y + edge, bb.max_x, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_r.size_8), ec, bc, TOP.limit, z);
}

bool brui_collapsable(br_strv_t name, bool* expanded) {
  float opt_header_height /* text + 2*1/2*padding */ = (float)TOP.font_size + TOP.padding.y;
  brui_border1(BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_header_height));
  brui_push_simple();
    brui_vsplitvp(2, BRUI_SPLITR(1), BRUI_SPLITA((float)TOP.font_size));
      TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.cur.y + opt_header_height);
      TOP.limit.min_y = fmaxf(TOP.limit.min_y, TOP.cur.y);
      brui_text_align_set(br_text_renderer_ancor_mid_mid);
      brui_text(name);
    brui_vsplit_pop();
      if (*expanded) {
        if (brui_button(BR_STRL("V"))) {
          *expanded = false;
        }
      } else {
        if (brui_button(BR_STRL("<"))) {
          *expanded = true;
        }
      }
    brui_vsplit_end();
  brui_pop_simple();
  TOP.cur.y += opt_header_height + TOP.padding.y;
  TOP.content_height += opt_header_height + TOP.padding.y;
  if (*expanded) {
    brui_push_simple();
      TOP.limit.min_x = TOP.cur.x;
      TOP.limit.max_x = TOP.limit.max_x - TOP.psum.x;
      TOP.limit.min_y = fmaxf(TOP.limit.min_y, TOP.cur.y);
      TOP.content_height = TOP.padding.y;
      TOP.cur.x = TOP.limit.min_x + TOP.padding.x;
      TOP.cur.y = TOP.cur.y + TOP.padding.y;
      TOP.psum.x = TOP.padding.x;
  }
  return *expanded;
}

void brui_collapsable_end(void) {
  float ch = TOP.content_height;
  float opt_header_height /* text + 2*1/2*padding */ = (float)TOP.font_size + TOP.padding.y;
  brui_border1(BR_BB(TOP.limit.min_x, TOP.limit.min_y - opt_header_height, TOP.limit.max_x, TOP.cur.y));
  brui_pop_simple();
  TOP.content_height += ch + TOP.padding.y;
  TOP.cur.y += ch + TOP.padding.y;
}

bool brui_button(br_strv_t text) {
  // BUTTON
  float opt_height /* text + 2*1/2*padding */ = (float)TOP.font_size + TOP.padding.y;
  float opt_y = TOP.cur.y + opt_height + TOP.padding.y;

  float button_max_x = TOP.limit.max_x - TOP.psum.x;
  float button_max_y = fminf(TOP.cur.y + opt_height, TOP.limit.max_y);
  br_bb_t button_limit = BR_BB(TOP.cur.x, TOP.cur.y, button_max_x, button_max_y);
  bool hovers = brui__stack.select_next || (br_col_vec2_bb(button_limit, brtl_mouse_pos()) && TOP.is_active);
  brui__stack.select_next = false;
  BRUI_LOG("button_limit: %.2f %.2f %.2f %.2f", BR_BB_(button_limit));
  brui_push_simple();
    TOP.limit.min_x = TOP.cur.x;
    TOP.limit.min_y = fmaxf(TOP.cur.y, TOP.limit.min_y);
    TOP.limit.max_y = button_max_y;
    TOP.limit.max_x = button_max_x;
    TOP.psum.x = 0;
    TOP.padding.y *= 0.5f;
    brui_text_align_set(br_text_renderer_ancor_mid_mid);
    brui_text_color_set(hovers ? BR_THEME.colors.btn_txt_hovered : BR_THEME.colors.btn_txt_inactive);
    brui_background(BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height),
      hovers ? BR_THEME.colors.btn_hovered : BR_THEME.colors.btn_inactive
    );
    brui_border2(BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height), hovers);
    brui_text(text);
  brui_pop_simple();
  TOP.content_height += opt_height + TOP.padding.y;
  TOP.cur.y = opt_y;
  return TOP.is_active && hovers && brtl_key_ctrl() == false && brtl_mousel_pressed();
}

bool brui_checkbox(br_strv_t text, bool* checked) {
  float sz = (float)TOP.font_size * 0.6f;
  br_bb_t cb_extent = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + sz, TOP.cur.y + sz);
  float top_out /* neg or 0 */ = fminf(0.f, TOP.cur.y - TOP.limit.min_y);
  float opt_height = (float)TOP.font_size + TOP.padding.y;
  float opt_cur_y = TOP.cur.y + opt_height;
  bool hover = false;

  brui_push_simple();
    br_extent_t icon = *checked ?  br_icon_cb_1((float)TOP.font_size) : br_icon_cb_0((float)TOP.font_size);
    hover = br_col_vec2_bb(cb_extent, brtl_mouse_pos());
    br_color_t bg = hover ? BR_THEME.colors.btn_hovered : BR_THEME.colors.btn_inactive;
    br_color_t fg = hover ? BR_THEME.colors.btn_txt_hovered : BR_THEME.colors.btn_txt_inactive;
    br_icons_draw(cb_extent, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z);
    TOP.cur.x += TOP.padding.x + sz;

    TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.cur.y + opt_height + top_out);
    brui_text(text);
  brui_pop_simple();

  TOP.cur.y = opt_cur_y;
  TOP.content_height += opt_height;

  if (hover && brtl_mousel_pressed()) {
    *checked = !*checked;
    return true;
  }
  return false;
}

void brui_img_hack(unsigned int texture_id) {
  br_shader_img_t* img = brtl_shaders()->img;
  img->uvs.image_uv = texture_id;
  TOP.z++;
  float gl_z = BR_Z_TO_GL(TOP.start_z + 1);
  br_sizei_t screen = brtl_viewport().size;
  br_shader_img_push_quad(img, (br_shader_img_el_t[4]) {
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.min,       screen), BR_VEC2(0, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_tr(TOP.limit), screen), BR_VEC2(1, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.max,       screen), BR_VEC2(1, 0)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_bl(TOP.limit), screen), BR_VEC2(0, 0)), .z = gl_z },
  });
  br_shader_img_draw(img);
  img->len = 0;
}

void brui_img(unsigned int texture_id) {
  br_shader_img_t* img = brtl_shaders()->img;
  img->uvs.image_uv = brgl_framebuffer_to_texture(texture_id);
  TOP.z++;
  float gl_z = BR_Z_TO_GL(TOP.start_z + 1);
  br_sizei_t screen = brtl_viewport().size;
  br_shader_img_push_quad(img, (br_shader_img_el_t[4]) {
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.min,       screen), BR_VEC2(0, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_tr(TOP.limit), screen), BR_VEC2(1, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.max,       screen), BR_VEC2(1, 0)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_bl(TOP.limit), screen), BR_VEC2(0, 0)), .z = gl_z },
  });
  br_shader_img_draw(img);
  img->len = 0;
}

void brui_icon(float size, br_bb_t icon, br_color_t forground, br_color_t background) {
  br_icons_draw(BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + size, TOP.cur.y + size), icon, forground, background, TOP.limit, Z);
}

bool brui_button_icon(br_sizei_t size, br_extent_t icon) {
  bool is_pressed = false;
  float out_top /* neg or 0 */ = fminf(0.f, TOP.cur.y - TOP.limit.min_y);
  float opt_height = (float)size.height + TOP.padding.y;
  float height = (float)size.height + out_top;
  br_bb_t bb = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + (float)size.width, TOP.cur.y + height);

  if (bb.max_y > bb.min_y && bb.max_x > bb.min_x) {
    bool hovered = (TOP.is_active && br_col_vec2_bb(bb, brtl_mouse_pos()));
    if (brui__stack.select_next || hovered) {
      br_color_t fg = BR_THEME.colors.btn_txt_hovered;
      br_color_t bg = BR_THEME.colors.btn_hovered;
      br_icons_draw(bb, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z);
    } else {
      br_color_t fg = BR_THEME.colors.btn_txt_inactive;
      br_color_t bg = BR_THEME.colors.btn_inactive;
      br_icons_draw(bb, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z);
    }
    is_pressed = brtl_mousel_pressed() && hovered;
  }
  brui__stack.select_next = false;
  TOP.cur.y += opt_height;
  TOP.content_height += opt_height;
  return is_pressed;
}

bool brui_rectangle(br_bb_t bb, br_bb_t limit, br_color_t color, int z) {
  br_icons_draw(bb, BR_BB(0,0,0,0), color, color, limit, z);
  return true;
}

bool brui_sliderf(br_strv_t text, float* val) {
  // SLIDERF
  BRUI_LOG("Slider");
  brui_push_simple();
    TOP.padding.y *= 0.1f;
    TOP.font_size = (int)((float)TOP.font_size * 0.8f);
    const float lt = (float)TOP.font_size * 0.2f;
    const float ss = lt + 3.f;
    const float opt_height /* 2*text + 4*padding + ss */ = 2 * (float)TOP.font_size + 4 * TOP.padding.y + ss;
    const float opt_max_y = TOP.cur.y + opt_height;

    brui_push();
      BRUI_LOG("Slider font: %d, pady: %.2f, lt: %.2f", TOP.font_size, TOP.padding.y, lt);
      TOP.limit.max_y = fminf(TOP.limit.max_y, opt_max_y);
      brui_text(text);
      brui_textf("%f", *val);

      br_vec2_t mouse = brtl_mouse_pos();
      bool is_down = brtl_mousel_down();

      const float lw = BR_BBW(TOP.limit) - 2 * TOP.psum.x;
      br_bb_t line_bb = BR_BB(TOP.cur.x, TOP.cur.y + (ss - lt)*0.5f, TOP.cur.x + lw, TOP.cur.y + (ss + lt)*0.5f);
      br_color_t lc = BR_THEME.colors.btn_inactive;
      br_bb_t slider_bb = BR_BB(TOP.cur.x + (lw - ss)*.5f, TOP.cur.y, TOP.cur.x + (lw + ss)*.5f, TOP.cur.y + ss);
      br_color_t sc = BR_THEME.colors.btn_txt_inactive;

      if ((ACTION == brui_action_sliding && ACPARM.slider.value == val) ||
          (ACTION != brui_action_sliding && mouse.x > line_bb.min_x &&
          mouse.x < line_bb.max_x &&
          mouse.y > line_bb.min_y - 3 &&
          mouse.y < line_bb.max_y + 3)) {
        if (is_down) {
          lc = BR_THEME.colors.btn_active;
          sc = BR_THEME.colors.btn_txt_active;
          float speed = brtl_frame_time();
          float factor = br_float_clamp((mouse.x - line_bb.min_x) / lw, 0.f, 1.f) * speed + (1 - speed * 0.5f);
          *val *= factor;
          slider_bb.min_x = br_float_clamp(mouse.x, line_bb.min_x, line_bb.max_x) - ss * 0.5f;
          slider_bb.max_x = slider_bb.min_x + ss;
          ACPARM.slider.value = val;
          ACTION = brui_action_sliding;
        } else {
          lc = BR_THEME.colors.btn_hovered;
          sc = BR_THEME.colors.btn_txt_hovered;
          ACPARM.slider.value = NULL;
          ACTION = brui_action_none;
        }
      }

      BRUI_LOG("Slider inner %f %f %f %f", BR_BB_(line_bb));
      if (slider_bb.max_y < TOP.limit.max_y) {
        if (slider_bb.min_y > TOP.limit.min_y) {
          br_icons_draw(line_bb, BR_BB(0,0,0,0), lc, lc, TOP.limit, Z);
          br_icons_draw(slider_bb, BR_BB(0,0,0,0), sc, sc, TOP.limit, Z);
        }
      }
      TOP.cur.y += ss + TOP.padding.y;
      TOP.content_height = opt_height + TOP.padding.y;
    brui_pop();
  brui_pop_simple();
  TOP.cur.y = opt_max_y + TOP.padding.y;
  TOP.content_height += opt_height + TOP.padding.y;
  return false;
}

bool brui_sliderf2(br_strv_t text, float* value) {
  return brui_sliderf3(text, value, 2);
}

bool brui_sliderf3(br_strv_t text, float* value, int percision) {
  float opt_height /* text + 2*1/2*padding */ = (float)TOP.font_size + TOP.padding.y;
  int n = sprintf(brui__scrach, "%.*f", percision, *value);
  br_strv_t value_str = BR_STRV(brui__scrach, (uint32_t)n);
  br_size_t size = br_text_renderer_measure(brtl_text_renderer(), TOP.font_size, value_str);
  size.width += TOP.padding.x * 2.f;
  float avaliable_width = BR_BBW(TOP.limit) - TOP.psum.x * 2.f;
  br_bb_t bb = BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height);
  br_vec2_t mouse_pos = brtl_mouse_pos();
  if (ACTION == brui_action_sliding && ACPARM.slider.value == value) {
    if (brtl_mousel_down() == false) {
      ACTION = brui_action_none;
    } else {
      *value *= 1.f - brtl_frame_time() * (ACPARM.slider.drag_ancor_point.x - mouse_pos.x) / 1000.f;
      brui_background(bb, BR_THEME.colors.btn_active);
    }
  } else if (ACTION != brui_action_sliding && brtl_mousel_down() && br_col_vec2_bb(bb, mouse_pos)) {
    ACTION = brui_action_sliding;
    ACPARM.slider.value = value;
    ACPARM.slider.drag_ancor_point = mouse_pos;
  }
  brui_push_simple();
    TOP.limit.min_y = fmaxf(TOP.cur.y, TOP.limit.min_y);
    TOP.limit.max_y = fminf(TOP.limit.min_y + opt_height, TOP.limit.max_y);
    brui_text_align_set(br_text_renderer_ancor_mid_mid);
    if (size.width < avaliable_width) {
      brui_vsplitvp(2, BRUI_SPLITR(1), BRUI_SPLITA(size.width));
        brui_text(text);
      brui_vsplit_pop();
        brui_text(value_str);
      brui_vsplit_end();
    }
  brui_pop_simple();
  brui_border1(bb);
  TOP.content_height += opt_height + TOP.padding.y;
  TOP.cur.y += opt_height + TOP.padding.y;
  return false;
}

bool brui_slideri(br_strv_t text, int* value) {
  float opt_height /* text + 2*1/2*padding */ = (float)TOP.font_size + TOP.padding.y;
  int n = sprintf(brui__scrach, "%d", *value);
  br_strv_t value_str = BR_STRV(brui__scrach, (uint32_t)n);
  br_size_t size = br_text_renderer_measure(brtl_text_renderer(), TOP.font_size, value_str);
  size.width += TOP.padding.x * 2.f;
  float avaliable_width = BR_BBW(TOP.limit) - TOP.psum.x * 2.f;
  br_bb_t bb = BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height);
  brui_push_simple();
    TOP.limit.min_y = fmaxf(TOP.cur.y, TOP.limit.min_y);
    TOP.limit.max_y = fminf(TOP.limit.min_y + opt_height, TOP.limit.max_y);
    brui_text_align_set(br_text_renderer_ancor_mid_mid);
    if (size.width < avaliable_width) {
      brui_vsplitvp(4, BRUI_SPLITR(1), BRUI_SPLITA(size.width), BRUI_SPLITA((float)TOP.font_size), BRUI_SPLITA((float)TOP.font_size));
        brui_text(text);
      brui_vsplit_pop();
        brui_text(value_str);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("-"))) --(*value);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("+"))) ++(*value);
      brui_vsplit_end();
    }
  brui_pop_simple();
  brui_border1(bb);
  TOP.content_height += opt_height + TOP.padding.y;
  TOP.cur.y += opt_height + TOP.padding.y;
  return false;
}

void brui_vsplit(int n) {
  BRUI_LOG("vsplit %d", n);
  brui_stack_el_t top = TOP;
  float width = (BR_BBW(TOP.limit) - 2*TOP.psum.x) / (float)n;
  for (int i = 0; i < n; ++i) {
    brui_stack_el_t new_el = top;
    new_el.z += 5;
    new_el.psum.x = 0;
    new_el.limit.min_x = (float)(n - i - 1) * width + top.cur.x;
    new_el.limit.max_x = new_el.limit.min_x + width;
    new_el.cur.x = new_el.limit.min.x;
    new_el.hide_border = true;
    new_el.vsplit_max_height = 0;
    new_el.content_height = 0;
    br_da_push(brui__stack, new_el);
  }
}

void brui_vsplitvp(int n, ...) {
  float absolute = 0;
  float relative = 0;

  {
    va_list ap;
    // Find the sum....
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
      brui_split_t t = va_arg(ap, brui_split_t);
      switch (t.kind) {
        case brui_split_absolute: absolute += t.absolute; break;
        case brui_split_relative: relative += t.relative; break;
      }
    }
    va_end(ap);
  }

  float off = TOP.cur.x - (TOP.limit.min_x + TOP.psum.x);
  float space_left = BR_BBW(TOP.limit) - off - TOP.psum.x * 2;
  float space_after_abs = space_left - absolute;
  if (space_after_abs < 0) {
    brui_vsplit(n);
    return;
  }
  {
    va_list ap;
    va_start(ap, n);
    brui_stack_el_t top = TOP;
    top.z += 5;
    top.psum.x = 0;
    top.hide_border = true;
    top.vsplit_max_height = 0;
    top.content_height = 0;
    float cur_x = TOP.cur.x;
    for (int i = 0; i < n; ++i) {
      brui_stack_el_t new_el = top;
      brui_split_t t = va_arg(ap, brui_split_t);
      float width = 0;
      switch (t.kind) {
        case brui_split_relative: width = space_after_abs * (t.relative / relative); break;
        case brui_split_absolute: width = t.absolute; break;
      }
      new_el.limit.min_x = cur_x;
      new_el.limit.max_x = new_el.limit.min_x + width;
      new_el.cur.x = new_el.limit.min.x;
      cur_x += width;
      br_da_push(brui__stack, new_el);
    }
    va_end(ap);

  }

  // reverse
  {
    size_t start = brui__stack.len - (size_t)n, end = brui__stack.len - 1;
    while (start < end) {
      brui_stack_el_t tmp = br_da_get(brui__stack, start);
      br_da_set(brui__stack, start, br_da_get(brui__stack, end));
      br_da_set(brui__stack, end, tmp);
      start += 1;
      end -= 1;
    }
  }
}

void brui_vsplit_pop(void) {
  BRUI_LOG("vsplit pre pop");
  float cur_max = TOP.vsplit_max_height;
  float cur_ch = TOP.content_height;
  brui_pop_simple();
  TOP.vsplit_max_height = fmaxf(cur_max, cur_ch);
  BRUI_LOG("vsplit post pop");
}

void brui_vsplit_end(void) {
  float cur_max = TOP.vsplit_max_height;
  float cur_ch = TOP.content_height;
  brui_pop_simple();
  float max_ch = fmaxf(cur_max, cur_ch);
  TOP.cur.y += max_ch + TOP.padding.y;
  TOP.content_height += max_ch + TOP.padding.y;
  BRUI_LOG("vsplit end");
}

void brui_scroll_bar(float* bar_offset_fract) {
  float thick = TOP.padding.x * 0.5f;
  float slider_thick = TOP.padding.x * 0.8f;
  br_vec2_t mouse = brtl_mouse_pos();
  bool is_down = brtl_mousel_down();

  float limit_height = BR_BBH(TOP.limit);
  float hidden_height = TOP.content_height - limit_height;
  float slider_height = limit_height * (limit_height / (limit_height + hidden_height));
  float slider_upper = *bar_offset_fract * (limit_height - slider_height);
  float slider_downer = (1.f - *bar_offset_fract) * (limit_height - slider_height);

  br_bb_t line_bb = BR_BB(TOP.limit.max_x - thick, TOP.limit.min_y, TOP.limit.max_x, TOP.limit.max_y);
  br_bb_t slider_bb = BR_BB(TOP.limit.max_x - slider_thick, TOP.limit.min_y + slider_upper, TOP.limit.max_x, TOP.limit.max_y - slider_downer);
  br_color_t lc = BR_THEME.colors.btn_inactive;
  br_color_t bc = BR_THEME.colors.btn_txt_inactive;
  if ((ACTION == brui_action_sliding && ACPARM.slider.value == bar_offset_fract) || (ACTION == brui_action_none && br_col_vec2_bb(line_bb, mouse))) {
    if (is_down) {
      *bar_offset_fract = br_float_clamp((mouse.y - TOP.limit.min_y) / limit_height, 0, 1);
      lc = BR_THEME.colors.btn_active;
      bc = BR_THEME.colors.btn_txt_active;
      ACTION = brui_action_sliding;
      ACPARM.slider.value = bar_offset_fract;
    } else {
      lc = BR_THEME.colors.btn_hovered;
      bc = BR_THEME.colors.btn_txt_hovered;
      ACTION = brui_action_none;
    }
  }
  br_icons_draw(line_bb, BR_BB(0,0,0,0), lc, lc, TOP.limit, Z);
  br_icons_draw(slider_bb, BR_BB(0,0,0,0), bc, bc, TOP.limit, Z);
}

void brui_text_align_set(br_text_renderer_ancor_t ancor) {
  TOP.text_ancor = ancor;
}

void brui_text_color_set(br_color_t color) {
  TOP.font_color = color;
}

static void brui_resizable_set_ancor(brui_resizable_t* res, brui_ancor_t ancor);
void brui_ancor_set(brui_ancor_t ancor) {
  brui_resizable_set_ancor(br_da_getp(*brtl_bruirs(), TOP.cur_resizable), ancor);
}

int brui_text_size(void) {
  return TOP.font_size;
}

void brui_text_size_set(int size) {
  TOP.font_size = size;
}

void brui_z_set(int z) {
  TOP.z = z;
}

void brui_maxy_set(float value) {
  TOP.limit.max_y = value;
}

void brui_scroll_move(float value) {
  brui_resizable_t* res = br_da_getp(*brtl_bruirs(), TOP.cur_resizable);
  float hidden_height = res->full_height - res->current.cur_extent.height;
  if (hidden_height <= 0) return;

  float cur_scroll_px = hidden_height * res->target.scroll_offset_percent;
  float next_scroll_px = cur_scroll_px + value;
  if (next_scroll_px < 0) next_scroll_px = 0.f;
  if (next_scroll_px > hidden_height) next_scroll_px = hidden_height;
  res->target.scroll_offset_percent = next_scroll_px / hidden_height;
}

float brui_min_y(void) {
  return TOP.limit.min_y;
}

float brui_max_y(void) {
  return TOP.limit.max_y;
}

void brui_height_set(float value) {
  TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.cur.y + value);
  TOP.limit.min_y = fmaxf(TOP.limit.min_y, TOP.cur.y);
}

float brui_padding_x(void) {
  return TOP.padding.x;
}

float brui_padding_y(void) {
  return TOP.padding.y;
}

void brui_padding_y_set(float value) {
  TOP.padding.y = value;
}

float brui_y(void) {
  return TOP.cur.y;
}

float brui_width(void) {
  return TOP.limit.max_x - TOP.limit.min_x;
}

float brui_local_y(void) {
  return TOP.cur.y - TOP.limit.min_y;
}

bool brui_active(void) {
  return TOP.is_active;
}

void brui_debug(void) {
  brui__stack.log = true;
}

brui_action_t* brui_action(void) {
  return &brui__stack.action;
}

void brui_action_stop(void) {
  ACTION = brui_action_none;
}

brui_stack_t* brui_stack(void) {
  return &brui__stack;
}

// ---------------------------Resizables--------------------------
static int bruir_find_at(int index, br_vec2_t loc, br_vec2_t* out_local_pos);
static void bruir_update_extent(int index, br_extent_t new_ex, bool force, bool animate);

void brui_resizable_init(void) {
  brui_resizable_t screen = { 0 };
  screen.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
  screen.ancor = brui_ancor_all;
  screen.z = 0.f;
  (void)brfl_push(*brtl_bruirs(), screen);
}

void brui_resizable_deinit(void) {
  brfl_free(*brtl_bruirs());
}

int brui_resizable_new(br_extent_t init_extent, int parent) {
  BR_ASSERT(brtl_bruirs()->len > parent);
  BR_ASSERT(brtl_bruirs()->free_arr[parent] == -1);

  brui_resizable_t new = { 0 };
  new.target.cur_extent = init_extent;
  new.title_enabled = true;
  new.z = brui_resizable_children_top_z(parent) + 1;
  new.parent = parent;
  int new_id = brfl_push(*brtl_bruirs(), new);

  if (parent > 0) bruir_update_extent(new_id, brtl_bruirs()->arr[new_id].target.cur_extent, true, true);
  return new_id;
}

int brui_resizable_new2(br_extent_t init_extent, int parent, brui_resizable_t template) {
  BR_ASSERTF(brtl_bruirs()->len > parent, "Len: %d, parent: %d", brtl_bruirs()->len > parent, parent);
  BR_ASSERT(brtl_bruirs()->free_arr[parent] == -1);

  brui_resizable_t new = template;
  new.target.cur_extent = init_extent;
  new.z = brui_resizable_children_top_z(parent) + 1;
  new.parent = parent;
  int new_id = brfl_push(*brtl_bruirs(), new);

  if (parent > 0) bruir_update_extent(new_id, brtl_bruirs()->arr[new_id].target.cur_extent, true, true);
  return new_id;
}

void brui_resizable_delete(int handle) {
  bruir_children_t children = brui_resizable_children_temp(handle);
  for (int i = 0; i < children.len; ++i) brui_resizable_delete(children.arr[i]);

  brfl_remove(*brtl_bruirs(), handle);
}

float brui_resizable_hidden_factor(brui_resizable_t* r) {
  if (r->parent == 0 || r->hidden_factor > 0.99f) return r->hidden_factor;
  else return fmaxf(brui_resizable_hidden_factor(&brtl_bruirs()->arr[r->parent]), r->hidden_factor);
}

static bool brui_snap_area(brui_ancor_t ancor, br_bb_t bb, br_color_t base_color, br_vec2_t mouse_pos, brui_resizable_t* r, bruirs_t* rs, float light_f, br_bb_t limit) {
  br_color_t color = base_color;
  brui_rectangle(bb, limit, color, BR_Z_MAX);
  if (br_col_vec2_bb(bb, mouse_pos)) {
    rs->drag_point = mouse_pos;
    color = br_color_lighter(color, light_f);
    brui_resizable_set_ancor(r, ancor);
    return true;
  }
  return false;
}

void brui_resizable_update(void) {
  bruir_update_extent(0, BR_EXTENTI_TOF(brtl_viewport()), false, false);
  br_vec2_t mouse_pos = brtl_mouse_pos();
  bruirs_t* rs = brtl_bruirs();
  rs->arr[0].current.cur_extent = rs->arr[0].target.cur_extent;
  float lerp_speed = brtl_frame_time() * brtl_theme()->ui.animation_speed;
  lerp_speed = br_float_clamp(lerp_speed, brtl_frame_time(), 1.f);

  if (brui_action_typing == ACTION) ACPARM.text.offset_x = br_float_lerp(ACPARM.text.offset_x, ACPARM.text.offset_x_target, lerp_speed);

  brfl_foreach(i, *rs) {
    brui_resizable_t* res = br_da_getp(*rs, i);

    res->hidden_factor = br_float_lerp(res->hidden_factor, res->target.hidden_factor, lerp_speed);
    res->scroll_offset_percent = br_float_lerp(res->scroll_offset_percent, res->target.scroll_offset_percent, lerp_speed);
    res->title_height = br_float_lerp2(res->title_height, res->target.title_height, lerp_speed);
    br_extent_t target_ex = res->target.cur_extent;
    float cur_hidden_factor = brui_resizable_hidden_factor(res);

    br_extent_t pex = br_da_get(*rs, res->parent).cur_extent;
    pex.x = 0;
    pex.y = 0;

    if (res->ancor == brui_ancor_right) {
      target_ex = pex;
      target_ex.width *= 0.5f;
      target_ex.x += target_ex.width;
    } else if (res->ancor == brui_ancor_left) {
      target_ex = pex;
      target_ex.width *= 0.5f;
    } else if (res->ancor == brui_ancor_bottom) {
      target_ex = pex;
      target_ex.height *= 0.5f;
      target_ex.y += target_ex.height;
    } else if (res->ancor == brui_ancor_top) {
      target_ex = pex;
      target_ex.height *= 0.5f;
    } else if (res->ancor == brui_ancor_right_top) {
      target_ex.x = pex.width - res->current.cur_extent.width;
      target_ex.y = 0;
    } else if (res->ancor == brui_ancor_left_top) {
      target_ex.x = 0;
      target_ex.y = 0;
    } else if (res->ancor == brui_ancor_right_bottom) {
      target_ex.x = pex.width - res->current.cur_extent.width;
      target_ex.y = pex.height - res->current.cur_extent.height;
    } else if (res->ancor == brui_ancor_left_bottom) {
      target_ex.x = 0;
      target_ex.y = pex.height - res->current.cur_extent.height;
    } else if (res->ancor == brui_ancor_all) {
      target_ex = pex;
    }
    target_ex.size  = br_size_scale(target_ex.size, 1.f - cur_hidden_factor);
    target_ex.pos   = br_vec2_scale(target_ex.pos, 1.f - cur_hidden_factor);
    br_extent_t old = res->cur_extent;
    if (i != rs->drag_index && (res->ancor & (brui_ancor_bottom | brui_ancor_right)) && !(res->ancor & (brui_ancor_left | brui_ancor_top))) {
      res->cur_extent = target_ex;
    } else {
      res->cur_extent = br_extent_lerp(res->cur_extent, target_ex, lerp_speed);
    }
    if (false == br_extent_eq(res->cur_extent, old)) {
      brfl_foreach(j, *rs) {
        brui_resizable_t child = br_da_get(*rs, j); 
        if (child.parent != i) continue;
        bruir_update_extent(j, child.target.cur_extent, true, true);
      }
    }
  }

  if (brtl_bruirs()->drag_mode == brui_drag_mode_none) {
    br_vec2_t local_pos = { 0 };
    int index = bruir_find_at(0, mouse_pos, &local_pos);
    if (index <= 0) {
      brui__stack.active_resizable = 0;
      return;
    }
    brui_resizable_t* hovered = br_da_getp(*rs, index);
    if (brtl_key_pressed(BR_KEY_HOME)) hovered->target.scroll_offset_percent = 0.f;
    if (brtl_key_pressed(BR_KEY_END))  hovered->target.scroll_offset_percent = 1.f;
    if (brtl_key_pressed(BR_KEY_PAGE_UP)) {
      float hidden_height = hovered->full_height - hovered->cur_extent.height;
      float page_percent = hovered->cur_extent.height / hidden_height;
      page_percent *= 0.9f;
      hovered->target.scroll_offset_percent = br_float_clamp(hovered->current.scroll_offset_percent - page_percent, 0.f, 1.f);
    }
    if (brtl_key_pressed(BR_KEY_PAGE_DOWN)) {
      float hidden_height = hovered->full_height - hovered->cur_extent.height;
      float page_percent = hovered->cur_extent.height / hidden_height;
      page_percent *= 0.9f;
      hovered->target.scroll_offset_percent = br_float_clamp(hovered->current.scroll_offset_percent + page_percent, 0.f, 1.f);
    }

    bool ml = brtl_mousel_down();
    bool mr = brtl_mouser_down();
    if (false == ml && false == mr) brui__stack.active_resizable = index;
    float scroll = brtl_mouse_scroll().y;
    bool title_shown = hovered->title_height > 0.1f;
    if ((brtl_key_ctrl() || title_shown) && ml == true) {
      float slack = 20;
      brui_drag_mode_t new_mode = brui_drag_mode_none;
      if (index != 0) {
        br_extent_t ex = hovered->cur_extent;
        rs->drag_index = index;
        rs->drag_point = mouse_pos;
        if (title_shown) new_mode = brui_drag_mode_move;
        else {
          if      (local_pos.x < slack)                    new_mode |= brui_drag_mode_left;
          else if (local_pos.x > (float)ex.width - slack)  new_mode |= brui_drag_mode_right;
          if      (local_pos.y < slack)                    new_mode |= brui_drag_mode_top;
          else if (local_pos.y > (float)ex.height - slack) new_mode |= brui_drag_mode_bottom;
          if      (new_mode == brui_drag_mode_none)        new_mode  = brui_drag_mode_move;
        }
        brtl_bruirs()->drag_mode = new_mode;
        brtl_bruirs()->drag_old_ex = hovered->current.cur_extent;

        ACTION = brui_action_sliding;
        ACPARM.slider.value = hovered;
      }
    } else if (scroll != 0) {
      if (hovered->full_height > (float)hovered->cur_extent.height) {
        float speed = 50.f / (hovered->full_height - (float)hovered->cur_extent.height);
        hovered->target.scroll_offset_percent = br_float_clamp(hovered->target.scroll_offset_percent - scroll * speed, 0.f, 1.f);
      } else {
        hovered->target.scroll_offset_percent = 0.f;
      }
    }
  } else {
    if (brtl_mousel_down()) {
      brui_resizable_t* r = br_da_getp(*rs, rs->drag_index);
      br_extent_t new_extent = rs->drag_old_ex;
      bool animate = false;
      if (rs->drag_mode == brui_drag_mode_move) {
        new_extent.pos = br_vec2_sub(rs->drag_old_ex.pos, br_vec2_sub(rs->drag_point, mouse_pos));
        if (r->ancor != brui_ancor_none) {
          if (br_vec2_len2(br_vec2_sub(new_extent.pos, rs->drag_old_ex.pos)) > 10*10) {
            //new_extent.pos = br_vec2_add(br_vec2_sub(r->target.cur_extent.pos, r->current.cur_extent.pos), br_vec2_sub(rs->drag_old_ex.pos, br_vec2_sub(rs->drag_point, mouse_pos)));
            rs->drag_old_ex.pos = rs->drag_point = mouse_pos;
            brui_resizable_set_ancor(r, brui_ancor_none);
            r->target.cur_extent.pos = r->cur_extent.pos = mouse_pos;
          }
        }
        brui_resizable_t p = br_da_get(*rs, r->parent);
        br_extent_t pex = p.current.cur_extent;
        br_bb_t pbb = BR_EXTENT_TOBB(pex);
        br_vec2_t center = BR_VEC2(pex.x + pex.width * .5f, pex.y + pex.height * .5f);
        float halfs = 10.f;
        float pad = 2.f;
        br_color_t base_color = BR_COLOR(25, 128, 25, 128);
        float light_f = 0.4f;

        brui_snap_area(brui_ancor_all, BR_BB(center.x - halfs, center.y - halfs, center.x + halfs, center.y + halfs), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_right, BR_BB(center.x + halfs + pad, center.y - halfs, center.x + halfs + pad + halfs, center.y + halfs), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_left, BR_BB(center.x - halfs - pad - halfs, center.y - halfs, center.x - pad - halfs, center.y + halfs), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_top, BR_BB(center.x - halfs, center.y - halfs - pad - halfs, center.x + halfs, center.y - halfs - pad), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_bottom, BR_BB(center.x - halfs, center.y + halfs + pad, center.x + halfs, center.y + halfs + pad + halfs), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_right_top, BR_BB(center.x + halfs + pad, center.y - halfs - pad - halfs, center.x + halfs + pad + halfs, center.y - halfs - pad), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_left_top, BR_BB(center.x - halfs - pad - halfs, center.y - halfs - pad - halfs, center.x - halfs - pad, center.y - halfs - pad), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_right_bottom, BR_BB(center.x + halfs + pad, center.y + halfs + pad, center.x + halfs + pad + halfs, center.y + halfs + pad + halfs), base_color, mouse_pos, r, rs, light_f, pbb);
        brui_snap_area(brui_ancor_left_bottom, BR_BB(center.x - halfs - pad - halfs, center.y + halfs + pad, center.x - halfs - pad, center.y + halfs + pad + halfs), base_color, mouse_pos, r, rs, light_f, pbb);
        animate = br_col_vec2_bb(BR_BB(center.x - 3*halfs - pad, center.y - 3*halfs - pad, center.x + 3*halfs + pad, center.y + 3*halfs + pad), mouse_pos);
      } else {
        if (rs->drag_mode & brui_drag_mode_left) {
          float dif = rs->drag_point.x - mouse_pos.x;
          new_extent.width  = rs->drag_old_ex.width  + dif;
          new_extent.x      = rs->drag_old_ex.x      - dif;
        } else if (rs->drag_mode & brui_drag_mode_right) {
          float dif = rs->drag_point.x - mouse_pos.x;
          new_extent.width  = rs->drag_old_ex.width  - dif;
        }
        if (rs->drag_mode & brui_drag_mode_top) {
          float dif = rs->drag_point.y - mouse_pos.y;
          new_extent.y      = rs->drag_old_ex.y      - dif;
          new_extent.height = rs->drag_old_ex.height + dif;
        } else if (rs->drag_mode & brui_drag_mode_bottom) {
          float dif = rs->drag_point.y - mouse_pos.y;
          new_extent.height = rs->drag_old_ex.height - dif;
        }
      }
      bruir_update_extent(rs->drag_index, new_extent, false, animate);
    } else {
      rs->drag_index = 0;
      brtl_bruirs()->drag_mode = brui_drag_mode_none;
      brtl_bruirs()->drag_point = BR_VEC2(0, 0);
      ACTION = brui_action_none;
    }
  }

  brfl_foreach(i, *brtl_bruirs()) {
    br_da_getp(*brtl_bruirs(), i)->current.was_draw_last_frame = false;
  }
}

void bruir_resizable_refresh(int index) {
  bruir_update_extent(index, br_da_get(*brtl_bruirs(), index).cur_extent, true, true);
}

static br_vec2_t bruir_pos_global(brui_resizable_t r) {
  if (r.parent == 0) return BR_VEC2I_TOF(r.cur_extent.pos);
  brui_resizable_t par = br_da_get(*brtl_bruirs(), r.parent);
  br_vec2_t p = br_vec2_add(bruir_pos_global(par), r.cur_extent.pos);
  float hidden_heigth = par.full_height - par.cur_extent.height;
  if (hidden_heigth > 0) p.y -= par.scroll_offset_percent * hidden_heigth;
  return p;
}

static void brui_resizable_decrement_z(brui_resizable_t* res) {
  int cur_z = res->z;
  if (cur_z <= 1) return;

  bruir_children_t c = brui_resizable_children_temp(res->parent);
  for (int i = 0; i < c.len; ++i) {
    int sibling_index = c.arr[i];
    brui_resizable_t* sibling = br_da_getp(*brtl_bruirs(), sibling_index);
    if (sibling->z + 1 != res->z) continue;
    res->max_z = 0;
    sibling->max_z = 0;
    --res->z;
    ++sibling->z;
    return;
  }
  // We didn't find any sibling that has z value == cur->z - 1 so we can just decrement cur z
  --res->z;
  res->max_z = 0;
}

static void brui_resizable_increment_z(brui_resizable_t* res) {
  bool any_bigger = false;
  bruir_children_t c = brui_resizable_children_temp(res->parent);
  for (int i = 0; i < c.len; ++i) {
    int sibling_index = c.arr[i];
    brui_resizable_t* sibling = br_da_getp(*brtl_bruirs(), sibling_index);
    if (sibling->z > res->z) any_bigger = true;
    if (sibling->z - 1 != res->z) continue;
    res->max_z = 0;
    sibling->max_z = 0;
    ++res->z;
    --sibling->z;
    return;
  }
  if (false == any_bigger) return;
  // We didn't find any sibling that has z value == cur->z + 1 so we can just increment cur z
  ++res->z;
  res->max_z = 0;
}

int brui_resizable_sibling_max_z(int id) {
  if (id == 0) return 0;
  brui_resizable_t* res = &brtl_bruirs()->arr[id];
  bruir_children_t siblings = brui_resizable_children_temp(res->parent);
  int max_z = 0;
  for (int i = 0; i < siblings.len; ++i) {
    int sibling_id = siblings.arr[i];
    if (sibling_id == id) continue;
    brui_resizable_t* sibling = &brtl_bruirs()->arr[sibling_id];
    if (sibling->z >= res->z) continue;
    int sib_z = sibling->max_z;
    max_z = sib_z > max_z ? sib_z : max_z;
  }
  return max_z;
}

bruir_children_t brui_resizable_children_temp(int resizable_handle) {
  BR_THREAD_LOCAL static bruir_children_t temp = { 0 };
  temp.len = 0;
  brfl_foreach(i, *brtl_bruirs()) {
    if (resizable_handle == br_da_get(*brtl_bruirs(), i).parent) {
      br_da_push_t(int, temp, i);
    }
  }
  return temp;
}

int brui_resizable_children_count(int resizable_handle) {
  int sum = 0;
  brfl_foreach(i, *brtl_bruirs()) if (resizable_handle == br_da_get(*brtl_bruirs(), i).parent) ++sum;
  return sum;
}

int brui_resizable_children_top_z(int resizable_handle) {
  int top_z = 0;
  brfl_foreach(i, *brtl_bruirs()) if (resizable_handle == br_da_get(*brtl_bruirs(), i).parent) top_z = br_i_max(top_z, br_da_get(*brtl_bruirs(), i).z);
  return top_z;
}

static void brui_resizable_set_ancor(brui_resizable_t* res, brui_ancor_t ancor) {
  if (ancor == brui_ancor_none) {
    res->target.cur_extent = res->ancor_none_extent;
  } else {
    if (res->ancor == brui_ancor_none) res->ancor_none_extent = res->current.cur_extent;
  }

  res->ancor = ancor;
}

brui_resizable_t* brui_resizable_push(int id) {
  brui_resizable_t* res = br_da_getp(*brtl_bruirs(), id);
  res->was_draw_last_frame = true;
  br_extent_t rex = BR_EXTENTI_TOF(res->cur_extent);
  int cur_z = TOP.z;
  bool is_menu_shown = res->target.title_height > .01f;
  brui_push();
  BRUI_LOG("resizablepre [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  TOP.psum = BR_VEC2(0, 0);
  TOP.cur = bruir_pos_global(*res);
  TOP.limit = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + rex.width, TOP.cur.y + rex.height);
  float scroll_y = (res->full_height - (float)res->cur_extent.height) * res->scroll_offset_percent;
  brui_z_set(cur_z + brui_resizable_sibling_max_z(id) + (is_menu_shown ? 5 : 15));
  TOP.cur_resizable = id;
  TOP.start_z = TOP.z;
  TOP.is_active = id == brui__stack.active_resizable;
  BRUI_LOG("resizablepost [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  TOP.hide_border = true;
  TOP.hide_bg = true;
  brui_push();
  TOP.cur_resizable = id;
  br_vec2_t mp = brtl_mouse_pos();
  if (res->title_enabled && false == brtl_key_ctrl() && TOP.is_active) {
    if (br_da_get(*brtl_bruirs(), res->parent).title_height < 0.1f) {
      br_vec2_t cur_p = bruir_pos_global(*res);
      float y_dist = mp.y - cur_p.y;
      float x_dist = mp.x - cur_p.x;
      float slack = 30.f;
      float title_full_height = (float)TOP.font_size + 2.f * TOP.padding.y;
      if (y_dist > 0.f && y_dist < .2f * title_full_height && x_dist > 0.f && x_dist < res->cur_extent.width) {
        res->target.title_height = title_full_height;
      } else if (y_dist > title_full_height + slack || y_dist < -slack || x_dist < -slack  || x_dist > res->cur_extent.width + slack) {
        res->target.title_height= 0.f;
      }
    }
  } else res->target.title_height= 0.f;
  if (is_menu_shown) {
    brui_push_simple();
      TOP.start_z = res->max_z + 2;
      TOP.z = TOP.start_z;
      TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.limit.min_y + res->title_height);
      float button_width = 20.f;
      brui_vsplitvp(5, BRUI_SPLITR(1), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width));
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Z-"))) brui_resizable_decrement_z(res);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Z+"))) brui_resizable_increment_z(res);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("[]"))) brui_resizable_set_ancor(res, res->ancor == brui_ancor_all ? brui_ancor_none : brui_ancor_all);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("X"))) res->target.hidden_factor = 1.f;
      brui_vsplit_end();
      brui_background(TOP.limit, BR_THEME.colors.plot_bg);
    brui_pop_simple();
  }
  TOP.cur.y -= scroll_y;
  brui_max_z = 0;
  TOP.limit.min_y += res->title_height;
  TOP.cur.y += res->title_height;
  return res;
}

void brui_resizable_pop(void) {
  brui_resizable_t* res = br_da_getp(*brtl_bruirs(), TOP.cur_resizable);
  float full_height = res->full_height = TOP.content_height;
  float hidden_height = full_height - (float)res->cur_extent.height;
  if (hidden_height > 0.f) {
    if (brui_resizable_hidden_factor(&brtl_bruirs()->arr[TOP.cur_resizable]) < 0.01 && false == brtl_key_ctrl()) brui_scroll_bar(&res->target.scroll_offset_percent);
  } else res->target.scroll_offset_percent = 0.f;
  TOP.cur.y = (float)res->cur_extent.y + (float)res->cur_extent.height;
  TOP.content_height = (float)res->cur_extent.height;
  brui_pop();
  brui_pop();
  res->max_z = brui_max_z;
  TOP.content_height = (float)res->cur_extent.height;
}

int brui_resizable_active(void) {
  return brui__stack.active_resizable;
}

void brui_resizable_show(int resizable_handle, bool show) {
  br_da_getp(*brtl_bruirs(), resizable_handle)->target.hidden_factor = show ? 0.f : 1.f;
}

void brui_resizable_maximize(int resizable_handle, bool maximize) {
  br_da_getp(*brtl_bruirs(), resizable_handle)->ancor = maximize ? brui_ancor_all : brui_ancor_none;
}

bool brui_resizable_is_hidden(int resizable_handle) {
  return br_da_get(*brtl_bruirs(), resizable_handle).hidden_factor > 0.99f;
}

br_vec2_t brui_resizable_to_global(int resizable_handle, br_vec2_t pos) {
  if (resizable_handle == 0) return pos;
  brui_resizable_t* r = brui_resizable_get(resizable_handle);
  pos = br_vec2_sub(pos, r->cur_extent.pos);
  float hidden_heigth = r->full_height - r->cur_extent.height;
  if (hidden_heigth > 0) pos.y += r->scroll_offset_percent * hidden_heigth;
  return brui_resizable_to_global(r->parent, pos);
}


static BR_THREAD_LOCAL int brui_resizable_temp_last = -1;
static BR_THREAD_LOCAL br_strv_t brui_resizable_temp_last_str;
brui_resizable_temp_push_t brui_resizable_temp_push(br_strv_t id) {
  // TODO: Handle hash collisions

  if (-1 != brui_resizable_temp_last) LOGF("Can't have temp resizables in temp resizables, yet");
  brui_resizable_temp_last_str = id;
#if defined(BR_IS_SIZE_T_32_BIT)
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeef);
#else
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeefdeadbeef);
#endif
  int res_handle = -1;

  ptrdiff_t index = stbds_hmgeti(bruir__temp_res, hash);
  bool just_created = false;
  if (index < 0) {
    res_handle = brui_resizable_new(BR_EXTENT(0, 0, 100, 100), 0);
    index = stbds_hmput(bruir__temp_res, hash, res_handle);
    just_created = true;
  } else {
    res_handle = bruir__temp_res[index].value;
  }


  brui_resizable_temp_last = res_handle;
  brui_resizable_t* res = brui_resizable_push(res_handle);
  return (brui_resizable_temp_push_t) { .res = res, .resizable_handle = res_handle, .just_created = just_created };
}

bool brui_resizable_temp_pop(void) {
  int id = brui_resizable_temp_last;
  br_strv_t handle = brui_resizable_temp_last_str;
  bool is_hidden = brui_resizable_is_hidden(id);
  brui_resizable_pop();
  brui_resizable_temp_last = -1;
  if (is_hidden) {
    brui_resizable_temp_delete(handle);
    return true;
  }
  return false;
}

void brui_resizable_temp_delete(br_strv_t id) {
#if defined(BR_IS_SIZE_T_32_BIT)
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeef);
#else
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeefdeadbeef);
#endif
  int handle = -1;
  ptrdiff_t index = stbds_hmgeti(bruir__temp_res, hash);
  if (index >= 0) {
    handle = bruir__temp_res[index].value;
    brui_resizable_delete(handle);
    (void)stbds_hmdel(bruir__temp_res, hash);
  }
}

void brui_resizable_temp_delete_all(void) {
  bruirs_t* rs = brtl_bruirs();
  ptrdiff_t len = stbds_hmlen(bruir__temp_res);
  for (int i = 0; i < len; ++i) {
    brfl_remove(*rs, bruir__temp_res[i].value);
  }
}

brui_resizable_t* brui_resizable_get(int id) {
  return br_da_getp(*brtl_bruirs(), id);
}

static int bruir_find_at(int index, br_vec2_t loc, br_vec2_t* out_local_pos) {
  bruirs_t* rs = brtl_bruirs();
  brui_resizable_t res = br_da_get(*rs, index);
  if (res.current.hidden_factor > 0.9f) return -1;
  if (index != 0 && false == res.current.was_draw_last_frame) return -1;
  if (res.current.title_height > 0.1f) return index;
  br_vec2_t local = BR_VEC2(loc.x - (float)res.current.cur_extent.x, loc.y - (float)res.current.cur_extent.y);
  if (local.x < 0) return -1;
  if (local.y < 0) return -1;
  if (local.x > (float)res.current.cur_extent.width) return -1;
  if (local.y > (float)res.current.cur_extent.height) return -1;

  int found = -1;
  int best_z = 0;
  brfl_foreach(i, *rs) {
    brui_resizable_t* c = br_da_getp(*rs, i);
    if (c->parent != index) continue;
    int cur_z = c->z;
    if (cur_z > best_z) {
      int cur = bruir_find_at(i, local, out_local_pos);
      if (cur > 0) {
          best_z = cur_z;
          found = cur;
      }
    }
  }
  if (found == -1) {
    *out_local_pos = local;
    return index;
  }
  return found;
}

static void bruir_update_extent(int index, br_extent_t new_ex, bool force, bool animate) {
  bruirs_t* rs = brtl_bruirs();
  brui_resizable_t* res = br_da_getp(*rs, index);
  br_extent_t old_ex = res->target.cur_extent;
  if (force || br_extent_eq(new_ex, old_ex) == false) {
    if (index != 0) {
      brui_resizable_t parent = br_da_get(*rs, res->parent);
      float max_x = BR_MAX(parent.cur_extent.width - new_ex.width, parent.target.cur_extent.width - new_ex.width);
      float max_y = BR_MAX(parent.cur_extent.height - new_ex.height, parent.target.cur_extent.height - new_ex.height);
      new_ex.x = BR_MAX(BR_MIN(new_ex.x, max_x),  0);
      new_ex.y = BR_MAX(BR_MIN(new_ex.y, max_y), 0);
      bool new_is_good = brui_extent_is_good(new_ex, parent.cur_extent);
      new_is_good |= brui_extent_is_good(new_ex, parent.target.cur_extent);
      bool old_is_good = brui_extent_is_good(res->cur_extent, parent.cur_extent);
      if (new_is_good == false && old_is_good == true) return;
    }

    res->target.cur_extent = new_ex;
    if (false == animate) {
      res->current.cur_extent.pos = new_ex.pos;
      if (rs->drag_mode != brui_drag_mode_move && index == rs->drag_index) {
        res->current.cur_extent.size = new_ex.size;
      }
    }
    brfl_foreach(i, *rs) {
      if (i == 0) continue;
      brui_resizable_t* child = br_da_getp(*rs, i);
      if (child->parent != index) continue;
      bool changed = false;
      br_extent_t child_extent = child->target.cur_extent;
      if (child_extent.x > new_ex.width) {
        child_extent.x = 0;
        changed = true;
      }
      if (child_extent.x + child_extent.width > new_ex.width) {
        if (child->ancor & brui_ancor_right) child_extent.x -= child_extent.x + child_extent.width - new_ex.width;
        else child_extent.width = new_ex.width - child_extent.x - 4;

        changed = true;
      }
      if (child_extent.width < 10) {
        child_extent.x = 0;
        child_extent.width = 60;
        changed = true;
      }
      if (child_extent.x < 0) {
        child_extent.width += child_extent.x;
        child_extent.x = 0;
      }
      if (child_extent.width > new_ex.width) {
        child_extent.width = new_ex.width / 2;
        changed = true;
      }

      if (child_extent.y >= new_ex.height) {
        child_extent.y = 0;
        changed = true;
      }
      if (child_extent.y + child_extent.height > new_ex.height) {
        child_extent.height = new_ex.height - child_extent.y - 4;
        changed = true;
      }
      if (child_extent.height < 10) {
        child_extent.y = 0;
        child_extent.height = 60;
        changed = true;
      }
      if (child_extent.height > new_ex.height) {
        child_extent.height = new_ex.height / 2;
        changed = true;
      }
      if (changed == true) bruir_update_extent(i, child_extent, force, false);
    }
  }
}

#undef TOP
#undef TOP2
#undef Z
#undef ZGL
#undef RETURN_IF_OUT1
#undef ACTION
#undef ACPARM
