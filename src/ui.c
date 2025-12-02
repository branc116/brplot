#include "src/br_pp.h"
#include "src/br_string_pool.h"
#include "src/br_ui.h"
#include "src/br_icons.h"
#include "src/br_math.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"
#include "src/br_theme.h"
#include "src/br_da.h"
#if !defined(BR_WANTS_GL)
#  define BR_WANTS_GL 1
#endif
#include "src/br_platform.h"
#include "src/br_shaders.h"
#include "src/br_free_list.h"
#include "src/br_memory.h"
#include "src/br_str.h"
#include "src/br_anim.h"

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
static BR_THREAD_LOCAL bruir_children_t brui__temp_children = { 0 };

#define BR_THEME (*brui__stack.theme)
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
#if 0
#  define BRUI_LOGI(...) do { \
  BR_STACKTRACE(); \
  LOGI(__VA_ARGS__); \
} while(0)
#  define BRUI_LOGV LOGI
#else
#  define BRUI_LOGI(...)
#  define BRUI_LOGV(...)
#endif

void brui_construct(br_theme_t* theme, bruirs_t* rs, brsp_t* sp, br_text_renderer_t* tr, br_shaders_t* shaders, br_anims_t* anims) {
  brui__stack.theme = theme;
  brui__stack.rs = rs;
  brui__stack.sp = sp;
  brui__stack.tr = tr;
  brui__stack.shaders = shaders;
  brui__stack.anims = anims;
  ACPARM.text.offset_ahandle = br_anim_newf(anims, 0, 0);
}

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
    br_extent_t viewport = brui__stack.rs->arr[0].cur_extent;
    brui_stack_el_t root = {
      .limit = BR_EXTENT_TOBB(viewport),
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
static BR_THREAD_LOCAL struct ___tracy_source_location_data brui_begin_end_bench = { "UI Begin End", "Static",  __FILE__, (uint32_t)__LINE__, 0 };
TracyCZoneCtx brui_begin_end_ctx;
#endif

void brui_begin(void) {
#if TRACY_ENABLE
  brui_begin_end_ctx = ___tracy_emit_zone_begin_callstack(&brui_begin_end_bench, 0, true );
#endif
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
  brui__stack.log = false;
}

void brui_finish(void) {
  BR_ASSERT(brui__stack.len == 0);
  br_da_free(brui__stack);
  br_da_free(brui__temp_children);
}

void brui_push(void) {
  BRUI_LOG("push");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(brui__stack, new_el);
  TOP.background_color = BR_THEME.colors.plot_menu_color;
}

brui_state_t brui_pop(void) {
  float width = BR_BBW(TOP.limit) - 2 * fminf(TOP2.psum.x, TOP.psum.x);
  float height = TOP.content_height;
  br_size_t size = BR_SIZE(width, height);
  br_bb_t bb = BR_BB(TOP2.cur.x, TOP2.cur.y, TOP2.cur.x + size.width, TOP2.cur.y + size.height);
  float content_height = TOP.content_height;
  bool clicked = false;
  bool hovered = br_col_vec2_bb(bb, brui__stack.mouse_pos);

  if (TOP.hide_bg == false) brui_background(bb, BR_THEME.colors.plot_menu_color);
  if (TOP.hide_border == false) brui_border1(bb);
  if (TOP.is_active && brui__stack.mouse_clicked && hovered) {
    clicked = true;
  }

  BRUI_LOG("Pre pop ex: [%.2f,%.2f,%.2f,%.2f]", BR_BB_(bb));
  float tt = TOP.cur.y;
  brui_pop_simple();
  TOP.cur.y = tt + TOP.padding.y;
  TOP.cur.x = TOP.limit.min_x + TOP.psum.x;
  TOP.content_height += content_height + TOP.padding.y;
  BRUI_LOG("pop");
  return (brui_state_t) { .clicked = clicked, .hovered = hovered, .bb = bb };
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
    br_text_renderer_t* tr = brui__stack.tr;
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


void brui_text_at(br_strv_t strv, br_vec2_t at) {
  br_text_renderer_t* tr = brui__stack.tr;
  br_size_t size = br_text_renderer_measure(tr, TOP.font_size, strv);
  size.height += (float)TOP.font_size;
  br_bb_t text_limit = TOP.limit;
  float padd = TOP.padding.x * 2;
  br_vec2_t at_og = at;
  br_color_t bg = br_color_lighter(TOP.background_color, BR_THEME.colors.highlite_factor);
  br_bb_t rect = { 0 };
  br_vec2_t b = { 0 }, c = { 0 };

  if (at.x - size.width - padd * 3 > text_limit.min_x) {
    at.x -= size.width + padd * 2;
    at.y -= size.height * 0.5f;
    rect = BR_BB(at.x - padd, at.y, at.x + size.width, at.y + (float)TOP.font_size);
    b = BR_VEC2(rect.max_x, rect.min_y);
    c = BR_VEC2(rect.max_x, rect.max_y);
  } else if (at.y - size.height > text_limit.min_y) {
    at.x = text_limit.min_x + padd;
    at.y -= size.height * 1.5f;
    rect = BR_BB(at.x - padd, at.y, at.x + size.width + padd, at.y + (float)TOP.font_size);
    b.x = at_og.x - (float)TOP.font_size*.5f;
    c.x = at_og.x + (float)TOP.font_size*.5f;
    float diffb = rect.min_x - b.x;
    float diffc = c.x - rect.max_x;
    float diff = br_float_max(diffb, diffc);
    if (diff > 0) {
      rect.max_y += diff;
      rect.min_y += diff;
    }
    b.y = rect.max_y;
    c.y = rect.max_y;
    if (diffb > 0) {
      b.x = rect.min_x;
      b.y -= diffb;
    }
    if (diffc > 0) {
      c.x = rect.max_x;
      c.y -= diffc;
    }
  }
  brui_rectangle(rect, text_limit, bg, Z);
  brui_triangle(at_og, b, c, text_limit, bg, Z);
  br_text_renderer_push2(tr, BR_VEC3(rect.min_x + padd, rect.min_y, ZGL), TOP.font_size, TOP.font_color, bg, strv, text_limit,  br_text_renderer_ancor_left_up);
}

bool brui_text_input(brsp_id_t str_id) {
  brsp_t* sp = brui__stack.sp;
  br_strv_t strv = brsp_get(*sp, str_id);
  br_text_renderer_t* tr = brui__stack.tr;
  bool is_active = brui_action_typing == ACTION && str_id == ACPARM.text.id;

  br_vec2_t loc                = TOP.cur;
  if (is_active) loc.x         -= br_anim_getf(brui__stack.anims, ACPARM.text.offset_ahandle);
  float out_top /* neg or 0 */ = fminf(TOP.cur.y - TOP.limit.min_y, 0.f);
  float opt_height             = (float)TOP.font_size + TOP.padding.y;
  br_size_t space_left         = BR_SIZE(TOP.limit.max_x - loc.x, TOP.limit.max_y - TOP.cur.y + out_top);
  br_strv_t fit                = br_text_renderer_fit(tr, space_left, TOP.font_size, strv);
  br_extent_t ex               = BR_EXTENT(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.cur.x, (float)TOP.font_size);
  br_bb_t text_limit           = TOP.limit;
  float half_thick             = 1.0f;
  bool changed                 = false;

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
    br_size_t size = br_text_renderer_measure(tr, TOP.font_size, br_str_sub(strv, 0, (uint32_t)ACPARM.text.cursor_pos));
    loc.x += size.width - 1.0f;
    text_limit.min_x -= 1.f;
    brui_rectangle(BR_BB(loc.x - half_thick, loc.y, loc.x + half_thick, loc.y + (float)text_height), text_limit, TOP.font_color, TOP.z + 2);
    int off_ahandle = ACPARM.text.offset_ahandle;
    float off = br_anim_getf(brui__stack.anims, off_ahandle);
    if (size.width - off > ex.width) br_anim_setf(brui__stack.anims, off_ahandle, size.width - ex.width / 2);
    if (size.width - off < 0)        br_anim_setf(brui__stack.anims, off_ahandle, br_float_max(0.f, size.width - ex.width / 2));
    changed = ACPARM.text.changed;
    ACPARM.text.changed = false;
  } else {
    if (TOP.is_active) {
      if (brui__stack.mouse_clicked) {
        if (br_col_vec2_bb(BR_EXTENT_TOBB(ex), brui__stack.mouse_pos)) {
          ACTION = brui_action_typing;
          ACPARM.text.cursor_pos = 0;
          ACPARM.text.id = str_id;
          br_anim_setf(brui__stack.anims, ACPARM.text.offset_ahandle, 0.f);
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
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
  br_icons_draw(bb, BR_BB(0,0,0,0), color, color, TOP.limit, TOP.start_z, viewport);
}

void brui_border1(br_bb_t bb) {
  brui_border2(bb, TOP.is_active && br_col_vec2_bb(bb, brui__stack.mouse_pos));
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
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);

  br_icons_draw(BR_BB(bb.min_x + edge, bb.min_y, bb.max_x - edge, bb.min_y + thick), BR_EXTENT_TOBB(br_extra_icons.edge_t.size_8), ec, bc, TOP.limit, z, viewport);
  br_icons_draw(BR_BB(bb.min_x + edge, bb.max_y - thick, bb.max_x - edge, bb.max_y), BR_EXTENT_TOBB(br_extra_icons.edge_b.size_8), ec, bc, TOP.limit, z, viewport);
  br_icons_draw(BR_BB(bb.min_x, bb.min_y + edge, bb.min_x + thick, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_l.size_8), ec, bc, TOP.limit, z, viewport);
  br_icons_draw(BR_BB(bb.max_x - thick, bb.min_y + edge, bb.max_x, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_r.size_8), ec, bc, TOP.limit, z, viewport);
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
  bool hovers = (br_col_vec2_bb(button_limit, brui__stack.mouse_pos) && TOP.is_active);
  bool is_selected = hovers || brui__stack.select_next;
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
    brui_text_color_set(is_selected ? BR_THEME.colors.btn_txt_hovered : BR_THEME.colors.btn_txt_inactive);
    brui_background(BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height),
      is_selected ? BR_THEME.colors.btn_hovered : BR_THEME.colors.btn_inactive
    );
    brui_border2(BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height), is_selected);
    brui_text(text);
  brui_pop_simple();
  TOP.content_height += opt_height + TOP.padding.y;
  TOP.cur.y = opt_y;
  return TOP.is_active && hovers && false == brui__stack.ctrl_down && brui__stack.mouse_clicked;
}

bool brui_checkbox(br_strv_t text, bool* checked) {
  float sz = (float)TOP.font_size * 0.6f;
  br_bb_t cb_extent = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + sz, TOP.cur.y + sz);
  float top_out /* neg or 0 */ = fminf(0.f, TOP.cur.y - TOP.limit.min_y);
  float opt_height = (float)TOP.font_size + TOP.padding.y;
  float opt_cur_y = TOP.cur.y + opt_height;
  bool hover = false;
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);


  brui_push_simple();
    br_extent_t icon = *checked ?  br_icon_cb_1((float)TOP.font_size) : br_icon_cb_0((float)TOP.font_size);
    hover = br_col_vec2_bb(cb_extent, brui__stack.mouse_pos);
    br_color_t bg = hover ? BR_THEME.colors.btn_hovered : BR_THEME.colors.btn_inactive;
    br_color_t fg = hover ? BR_THEME.colors.btn_txt_hovered : BR_THEME.colors.btn_txt_inactive;
    br_icons_draw(cb_extent, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z, viewport);
    TOP.cur.x += TOP.padding.x + sz;

    TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.cur.y + opt_height + top_out);
    brui_text(text);
  brui_pop_simple();

  TOP.cur.y = opt_cur_y;
  TOP.content_height += opt_height;

  if (hover && brui__stack.mouse_clicked) {
    *checked = !*checked;
    return true;
  }
  return false;
}

void brui_img_hack(unsigned int texture_id) {
  br_shader_img_t* img = brui__stack.shaders->img;
  img->uvs.image_uv = texture_id;
  TOP.z++;
  float gl_z = BR_Z_TO_GL(TOP.start_z + 1);
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
  br_shader_img_push_quad(img, (br_shader_img_el_t[4]) {
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.min,       viewport), BR_VEC2(0, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_tr(TOP.limit), viewport), BR_VEC2(1, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.max,       viewport), BR_VEC2(1, 0)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_bl(TOP.limit), viewport), BR_VEC2(0, 0)), .z = gl_z },
  });
  br_shader_img_draw(img);
  img->len = 0;
}

void brui_img(unsigned int texture_id) {
  br_shader_img_t* img = brui__stack.shaders->img;
  img->uvs.image_uv = brgl_framebuffer_to_texture(texture_id);
  TOP.z++;
  float gl_z = BR_Z_TO_GL(TOP.start_z + 1);
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
  br_shader_img_push_quad(img, (br_shader_img_el_t[4]) {
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.min,       viewport), BR_VEC2(0, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_tr(TOP.limit), viewport), BR_VEC2(1, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(TOP.limit.max,       viewport), BR_VEC2(1, 0)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_bb_bl(TOP.limit), viewport), BR_VEC2(0, 0)), .z = gl_z },
  });
  br_shader_img_draw(img);
  img->len = 0;
}

void brui_icon(float size, br_bb_t icon, br_color_t forground, br_color_t background) {
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
  br_icons_draw(BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + size, TOP.cur.y + size), icon, forground, background, TOP.limit, Z, viewport);
}

bool brui_button_icon(br_sizei_t size, br_extent_t icon) {
  bool is_pressed = false;
  float out_top /* neg or 0 */ = fminf(0.f, TOP.cur.y - TOP.limit.min_y);
  float opt_height = (float)size.height + TOP.padding.y;
  float height = (float)size.height + out_top;
  br_bb_t bb = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + (float)size.width, TOP.cur.y + height);
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);

  if (bb.max_y > bb.min_y && bb.max_x > bb.min_x) {
    bool hovered = (TOP.is_active && br_col_vec2_bb(bb, brui__stack.mouse_pos));
    if (brui__stack.select_next || hovered) {
      br_color_t fg = BR_THEME.colors.btn_txt_hovered;
      br_color_t bg = BR_THEME.colors.btn_hovered;
      br_icons_draw(bb, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z, viewport);
    } else {
      br_color_t fg = BR_THEME.colors.btn_txt_inactive;
      br_color_t bg = BR_THEME.colors.btn_inactive;
      br_icons_draw(bb, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z, viewport);
    }
    is_pressed = brui__stack.mouse_clicked && hovered;
  }
  brui__stack.select_next = false;
  TOP.cur.y += opt_height;
  TOP.content_height += opt_height;
  return is_pressed;
}

bool brui_triangle(br_vec2_t a, br_vec2_t b, br_vec2_t c, br_bb_t limit, br_color_t color, int z) {
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
  br_icons_draw_triangle(a, b, c, BR_BB(0,0,0,0), color, color, limit, z, viewport);
  return true;
}

bool brui_rectangle(br_bb_t bb, br_bb_t limit, br_color_t color, int z) {
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
  br_icons_draw(bb, BR_BB(0,0,0,0), color, color, limit, z, viewport);
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

      br_vec2_t mouse = brui__stack.mouse_pos;
      bool is_down = brui__stack.mouse_clicked;

      const float lw = BR_BBW(TOP.limit) - 2 * TOP.psum.x;
      br_bb_t line_bb = BR_BB(TOP.cur.x, TOP.cur.y + (ss - lt)*0.5f, TOP.cur.x + lw, TOP.cur.y + (ss + lt)*0.5f);
      br_color_t lc = BR_THEME.colors.btn_inactive;
      br_bb_t slider_bb = BR_BB(TOP.cur.x + (lw - ss)*.5f, TOP.cur.y, TOP.cur.x + (lw + ss)*.5f, TOP.cur.y + ss);
      br_color_t sc = BR_THEME.colors.btn_txt_inactive;

      if (ACTION == brui_action_sliding && ACPARM.slider.value == val) {
        lc = BR_THEME.colors.btn_active;
        sc = BR_THEME.colors.btn_txt_active;
        float speed = brui__stack.frame_time;
        float factor = br_float_clamp((mouse.x - line_bb.min_x) / lw, 0.f, 1.f) * speed + (1 - speed * 0.5f);
        *val *= factor;
        slider_bb.min_x = br_float_clamp(mouse.x, line_bb.min_x, line_bb.max_x) - ss * 0.5f;
        slider_bb.max_x = slider_bb.min_x + ss;
      } else if (ACTION == brui_action_none && br_col_vec2_bb(line_bb, mouse)) {
        lc = BR_THEME.colors.btn_hovered;
        sc = BR_THEME.colors.btn_txt_hovered;
        if (is_down) {
          ACPARM.slider.value = val;
          ACTION = brui_action_sliding;
        }
      }

      BRUI_LOG("Slider inner %f %f %f %f", BR_BB_(line_bb));
      if (slider_bb.max_y < TOP.limit.max_y) {
        if (slider_bb.min_y > TOP.limit.min_y) {
          br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
          br_icons_draw(line_bb, BR_BB(0,0,0,0), lc, lc, TOP.limit, Z, viewport);
          br_icons_draw(slider_bb, BR_BB(0,0,0,0), sc, sc, TOP.limit, Z, viewport);
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
  br_size_t size = br_text_renderer_measure(brui__stack.tr, TOP.font_size, value_str);
  size.width += TOP.padding.x * 2.f;
  float avaliable_width = BR_BBW(TOP.limit) - TOP.psum.x * 2.f;
  br_bb_t bb = BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height);
  br_vec2_t mouse_pos = brui__stack.mouse_pos;
  if (ACTION == brui_action_sliding && ACPARM.slider.value == value) {
    *value *= 1.f - brui__stack.frame_time * (ACPARM.slider.drag_ancor_point.x - mouse_pos.x) / 1000.f;
    brui_background(bb, BR_THEME.colors.btn_active);
  } else if (ACTION == brui_action_none && brui__stack.mouse_clicked && br_col_vec2_bb(bb, mouse_pos)) {
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
  br_size_t size = br_text_renderer_measure(brui__stack.tr, TOP.font_size, value_str);
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
  br_vec2_t mouse = brui__stack.mouse_pos;
  bool is_down = brui__stack.mouse_clicked;

  float limit_height = BR_BBH(TOP.limit);
  float hidden_height = TOP.content_height - limit_height;
  float slider_height = limit_height * (limit_height / (limit_height + hidden_height));
  float slider_upper = *bar_offset_fract * (limit_height - slider_height);
  float slider_downer = (1.f - *bar_offset_fract) * (limit_height - slider_height);

  br_bb_t line_bb = BR_BB(TOP.limit.max_x - thick, TOP.limit.min_y, TOP.limit.max_x, TOP.limit.max_y);
  br_bb_t slider_bb = BR_BB(TOP.limit.max_x - slider_thick, TOP.limit.min_y + slider_upper, TOP.limit.max_x, TOP.limit.max_y - slider_downer);
  br_color_t lc = BR_THEME.colors.btn_inactive;
  br_color_t bc = BR_THEME.colors.btn_txt_inactive;
  if (ACTION == brui_action_sliding && ACPARM.slider.value == bar_offset_fract) {
    lc = BR_THEME.colors.btn_active;
    bc = BR_THEME.colors.btn_txt_active;
    *bar_offset_fract = br_float_clamp((mouse.y - TOP.limit.min_y) / limit_height, 0, 1);
  } else if (ACTION == brui_action_none && br_col_vec2_bb(line_bb, mouse)) {
    lc = BR_THEME.colors.btn_hovered;
    bc = BR_THEME.colors.btn_txt_hovered;
    ACTION = brui_action_none;
    if (is_down) {
      ACTION = brui_action_sliding;
      ACPARM.slider.value = bar_offset_fract;
    }
  }
  br_sizei_t viewport = BR_SIZE_TOI(brui__stack.rs->arr[0].cur_extent.size);
  br_icons_draw(line_bb, BR_BB(0,0,0,0), lc, lc, TOP.limit, Z, viewport);
  br_icons_draw(slider_bb, BR_BB(0,0,0,0), bc, bc, TOP.limit, Z, viewport);
}

void brui_text_align_set(br_text_renderer_ancor_t ancor) {
  TOP.text_ancor = ancor;
}

void brui_text_color_set(br_color_t color) {
  TOP.font_color = color;
}

static void brui_resizable_set_ancor(int res_id, int sibling_id, brui_ancor_t ancor);
void brui_ancor_set(brui_ancor_t ancor) {
  brui_resizable_set_ancor(TOP.cur_resizable, 0, ancor);
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
  brui_resizable_t* res = br_da_getp(*brui__stack.rs, TOP.cur_resizable);
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

void brui_mouse_clicked(bool is_mouse_clicked) {
  brui__stack.mouse_clicked = is_mouse_clicked;
}

void brui_mouse_pos(br_vec2_t mouse_pos) {
  brui__stack.mouse_pos = mouse_pos;
}

void brui_ctrl_down(bool is_down) {
  brui__stack.ctrl_down = is_down;
}

void brui_log(bool should_log) {
  LOGI("LOG. Action: %d", ACTION);

  brui__stack.log = ACTION == brui_action_none && should_log;
}

void brui_frame_time(float frame_time) {
  brui__stack.frame_time = frame_time;
}

void brui_resizables(bruirs_t* rs) {
  brui__stack.rs = rs;
}

void brui_sp(brsp_t* sp) {
  brui__stack.sp = sp;
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
static int bruir_find_at(bruirs_t* rs, int index, br_vec2_t loc, br_vec2_t* out_local_pos);
static void bruir_update_extent(bruirs_t* rs, int index, br_extent_t new_ex, bool force, bool animate);

void brui_resizable_init(bruirs_t* rs, br_extent_t extent) {
  brui_resizable_t screen = { 0 };
  screen.cur_extent = extent;
  screen.ancor = brui_ancor_all;
  screen.z = 0.f;
  (void)brfl_push(*rs, screen);
}

void brui_resizable_deinit(void) {
  brfl_free(*brui__stack.rs);
}

static int brui_resizable_new0(bruirs_t* rs, brui_resizable_t* new, br_extent_t init_extent, int parent) {
  LOGI("Create new resizable");
  BR_ASSERTF(rs->len > parent, "Len: %d, parent: %d", rs->len > parent, parent);
  BR_ASSERT(rs->free_arr[parent] == -1);
  new->target.cur_extent = init_extent;
  new->z = brui_resizable_children_top_z(rs, parent) + 1;
  new->parent = parent;
  new->title_height_ah = br_anim_newf(brui__stack.anims, 0, 0);
  int new_id = brfl_push(*rs, *new);

  if (parent > 0) bruir_update_extent(rs, new_id, rs->arr[new_id].target.cur_extent, true, true);
  return new_id;
}

int brui_resizable_new(bruirs_t* rs, br_extent_t init_extent, int parent) {
  brui_resizable_t new = { 0 };
  new.title_enabled = true;

  return brui_resizable_new0(rs, &new, init_extent, parent);
}

int brui_resizable_new2(bruirs_t* rs, br_extent_t init_extent, int parent, brui_resizable_t template) {
  brui_resizable_t new = template;
  return brui_resizable_new0(rs, &new, init_extent, parent);
}

static void brui_resizable_check_parents(bruirs_t* rs);
void brui_resizable_delete(int handle) {
  BRUI_LOGI("Delete: %d", handle);
  brui_resizable_t resizable = br_da_get(*brui__stack.rs, handle);
  if (resizable.parent != handle) {
    bruir_children_t children = brui_resizable_children_temp(handle);
    for (int i = 0; i < children.len; ++i) brui_resizable_delete(children.arr[i]);

    if (resizable.ancor != brui_ancor_none) brui_resizable_set_ancor(handle, 0, brui_ancor_none);
  }

  brsp_remove(brui__stack.sp, brui__stack.rs->arr[handle].title_id);
  brfl_remove(*brui__stack.rs, handle);
  brui_resizable_check_parents(brui__stack.rs);
}

float brui_resizable_hidden_factor(bruirs_t* rs, brui_resizable_t* r) {
  if (r->parent == 0 || r->hidden_factor > 0.99f) return r->hidden_factor;
  else return fmaxf(brui_resizable_hidden_factor(rs, &rs->arr[r->parent]), r->hidden_factor);
}

static bool brui_snap_area(brui_ancor_t ancor, br_bb_t bb, br_color_t base_color, br_vec2_t mouse_pos, int r_id, int sibling_id, bruirs_t* rs, float light_f, br_bb_t limit) {
  br_color_t color = base_color;
  bool is_in = false;
  if (br_col_vec2_bb(bb, mouse_pos)) {
    rs->drag_point = mouse_pos;
    color = br_color_lighter(color, light_f);
    if (brui__stack.len == 0) {
      brui_resizable_set_ancor(r_id, sibling_id, ancor);
    }
    is_in = true;
  }
  if (brui__stack.len > 0) {
    brui_rectangle(bb, limit, color, BR_Z_MAX);
  }
  return is_in;
}

static bool brui_snap_areas(br_vec2_t mouse_pos, int r_id, bruirs_t* rs) {
  if (brui__stack.snap_cooldown > 0) return false;
  brui_resizable_t* r = br_da_getp(*rs, r_id);
  brui_resizable_t p = br_da_get(*rs, r->parent);
  br_extent_t pex = brui_resizable_cur_extent(r->parent);
  br_bb_t pbb = BR_EXTENT_TOBB(pex);
  br_color_t base_color = BR_COLOR(25, 200, 25, 64);
  float light_f = 1.4f;

  float snap_sib_height = 7;
  float snap_sib_width = 30;
  if (p.tag == brui_resizable_tag_ancor_helper) return false;
  brfl_foreach(i, *brui__stack.rs) {
    if (i == 0) continue;
    if (i == r_id) continue;
    brui_resizable_t* sib = br_da_getp(*brui__stack.rs, i);
    if (sib->parent != r->parent) continue;
    if (sib->hidden_factor > 0.1) continue;
    br_extent_t sex = brui_resizable_cur_extent(i);
    br_bb_t snap_bb = { 0 };
    {
      snap_bb.min.y = sex.y - snap_sib_height;
      snap_bb.max.y = sex.y + snap_sib_height;
      snap_bb.min.x = sex.x + sex.width / 2 - snap_sib_width;
      snap_bb.max.x = sex.x + sex.width / 2 + snap_sib_width;
      if (brui_snap_area(brui_ancor_top, snap_bb, base_color, mouse_pos, r_id, i, rs, light_f, pbb)) return true;
      snap_bb.min.y += sex.height;
      snap_bb.max.y += sex.height;
      if (brui_snap_area(brui_ancor_bottom, snap_bb, base_color, mouse_pos, r_id, i, rs, light_f, pbb)) return true;
    }
    {
      snap_bb.min.x = sex.x - snap_sib_height;
      snap_bb.max.x = sex.x + snap_sib_height;
      snap_bb.min.y = sex.y + sex.height / 2 - snap_sib_width;
      snap_bb.max.y = sex.y + sex.height / 2 + snap_sib_width;
      if (brui_snap_area(brui_ancor_left, snap_bb, base_color, mouse_pos, r_id, i, rs, light_f, pbb)) return true;
      snap_bb.min.x += sex.width;
      snap_bb.max.x += sex.width;
      if (brui_snap_area(brui_ancor_right, snap_bb, base_color, mouse_pos, r_id, i, rs, light_f, pbb)) return true;
    }
  }

  br_vec2_t center = BR_VEC2(pex.x + pex.width * .5f, pex.y + pex.height * .5f);
  float halfs = 10.f;
  float pad = 2.f;

  brui_snap_area(brui_ancor_all,          BR_BB(center.x - halfs, center.y - halfs, center.x + halfs, center.y + halfs), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_right,        BR_BB(center.x + halfs + pad, center.y - halfs, center.x + halfs + pad + halfs, center.y + halfs), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_left,         BR_BB(center.x - halfs - pad - halfs, center.y - halfs, center.x - pad - halfs, center.y + halfs), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_top,          BR_BB(center.x - halfs, center.y - halfs - pad - halfs, center.x + halfs, center.y - halfs - pad), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_bottom,       BR_BB(center.x - halfs, center.y + halfs + pad, center.x + halfs, center.y + halfs + pad + halfs), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_right_top,    BR_BB(center.x + halfs + pad, center.y - halfs - pad - halfs, center.x + halfs + pad + halfs, center.y - halfs - pad), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_left_top,     BR_BB(center.x - halfs - pad - halfs, center.y - halfs - pad - halfs, center.x - halfs - pad, center.y - halfs - pad), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_right_bottom, BR_BB(center.x + halfs + pad, center.y + halfs + pad, center.x + halfs + pad + halfs, center.y + halfs + pad + halfs), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  brui_snap_area(brui_ancor_left_bottom,  BR_BB(center.x - halfs - pad - halfs, center.y + halfs + pad, center.x - halfs - pad, center.y + halfs + pad + halfs), base_color, mouse_pos, r_id, 0, rs, light_f, pbb);
  return br_col_vec2_bb(BR_BB(center.x - 3*halfs - pad, center.y - 3*halfs - pad, center.x + 3*halfs + pad, center.y + 3*halfs + pad), mouse_pos);
}


void brui_resizable_update(bruirs_t* rs, br_extent_t viewport) {
  bruir_update_extent(rs, 0, viewport, false, true);
  rs->arr[0].current.cur_extent = rs->arr[0].target.cur_extent;
  float lerp_speed = brui__stack.frame_time * BR_THEME.ui.animation_speed;
  lerp_speed = br_float_clamp(lerp_speed, brui__stack.frame_time, 1.f);
  brui__stack.snap_cooldown -= brui__stack.frame_time;

  if (rs->drag_mode == brui_drag_mode_none) {
    br_vec2_t local;
    rs->active_resizable = bruir_find_at(rs, 0, brui__stack.mouse_pos, &local);
    (void)local;
  }

  brfl_foreach(i, *rs) {
    brui_resizable_t* res = br_da_getp(*rs, i);

    res->hidden_factor = br_float_lerp(res->hidden_factor, res->target.hidden_factor, lerp_speed);
    res->scroll_offset_percent = br_float_lerp(res->scroll_offset_percent, res->target.scroll_offset_percent, lerp_speed);
    br_extent_t target_ex = res->target.cur_extent;
    float cur_hidden_factor = brui_resizable_hidden_factor(rs, res);

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
    res->target.cur_extent = target_ex;
    target_ex.size  = br_size_scale(target_ex.size, 1.f - cur_hidden_factor);
    target_ex.pos   = br_vec2_scale(target_ex.pos, 1.f - cur_hidden_factor);
    br_extent_t old = res->cur_extent;
    /*if (i == 0 || (i != rs->drag_index && (res->ancor & (brui_ancor_bottom | brui_ancor_right)) && !(res->ancor & (brui_ancor_left | brui_ancor_top)))) {
      res->cur_extent = target_ex;
    } else*/ {
      res->cur_extent = br_extent_lerp(res->cur_extent, target_ex, lerp_speed);
    }
    if (false == br_extent_eq(res->cur_extent, old)) {
      brfl_foreach(j, *rs) {
        brui_resizable_t child = br_da_get(*rs, j);
        if (child.parent != i) continue;
        bruir_update_extent(rs, j, child.target.cur_extent, true, true);
      }
    }
  }

  brfl_foreach(i, *rs) {
    brui_resizable_t* r = br_da_getp(*rs, i);
    r->current.was_draw_last_frame = true;
    if (r->tag == brui_resizable_tag_ancor_helper) r->max_z = 0;
  }

  brfl_foreach(i, *rs) {
    brui_resizable_t* r = br_da_getp(*rs, i);
    brui_resizable_t* parent = br_da_getp(*rs, r->parent);
    if (parent->tag == brui_resizable_tag_ancor_helper) parent->max_z = br_i_max(parent->max_z, r->max_z);
  }

  for (ptrdiff_t i = 0; i < stbds_hmlen(bruir__temp_res); ++i) {
    brui_resizable_temp_state_t* state = &bruir__temp_res[i].value;
    if (state->is_deleted == true) continue;
    if (false == state->was_drawn) {
      brui_resizable_delete(state->resizable_handle);
      state->is_deleted = true;
    } else {
      state->was_drawn = false;
    }
  }
}

static void brui_resizable_check_parents(bruirs_t* rs) {
#if BR_DEBUG
  brfl_foreach(i, *rs) {
    int parent = i;
    for (int i = 0; i < 16; ++i) {
      if (parent == 0) goto next;
      BR_ASSERT(false == brfl_is_free(*rs, parent));
      parent = rs->arr[parent].parent;
      if (parent == i) goto next;
    }
    brui_resizable_t r = br_da_get(*rs, i);

    br_strv_t title;
    if (r.title_id > 0) brsp_get(*brui__stack.sp, r.title_id);
    if (r.title_id <= 0 || title.str == 0 && title.len == 0 || title.len > 0xFFFFFF) title = BR_STRL("Unnnamed"); 
    BR_ASSERTF(0, "Resizable has no parent: id=%d, name=%.*s, parent_id=%d", i, title.len, title.str, parent);
next:;
  }
  brfl_foreach(i, *rs) {
    brui_resizable_t r = br_da_get(*rs, i);
    if (r.tag == brui_resizable_tag_ancor_helper) {
      int count = 0;
      static BR_THREAD_LOCAL br_str_t s = { 0 };
      s.len = 0;
      brfl_foreach(j, *rs) {
        brui_resizable_t r2 = br_da_get(*rs, j);
        if (r2.parent == j) {
          count = 2;
          break;
        }
        if (r2.parent == i) {
          count += 1;
          br_str_push_int(&s, j);
          br_str_push_char(&s, ' ');
        }
      }
      br_strv_t title = brsp_get(*brui__stack.sp, r.title_id);
      if (r.title_id <= 0 || title.str == 0 && title.len == 0 || title.len > 0xFFFFFF) title = BR_STRL("Unnnamed"); 
      BR_ASSERTF(count == 2, "Thing with id of %d `%.*s` had %d children ( %.*s )", i, title.len, title.str, count, s.len, s.str);
    }
  }
  BRUI_LOGV("Check ok");
#else
  (void)rs;
#endif
}

void brui_resizable_mouse_move(bruirs_t* rs, br_vec2_t mouse_pos) {
  int drag_index = rs->drag_index;
  if (rs->drag_mode == brui_drag_mode_none) return;

  brui_resizable_t* r = br_da_getp(*rs, drag_index);
  br_extent_t new_extent = rs->drag_old_ex;
  bool animate = false;
  if (rs->drag_mode == brui_drag_mode_move) {
    new_extent.pos = br_vec2_sub(rs->drag_old_ex.pos, br_vec2_sub(rs->drag_point, mouse_pos));
    if (r->ancor != brui_ancor_none) {
      if (br_vec2_len2(br_vec2_sub(new_extent.pos, rs->drag_old_ex.pos)) > 10*10) {
        rs->drag_old_ex.pos = rs->drag_point = mouse_pos;
        brui_resizable_set_ancor(drag_index, 0, brui_ancor_none);
      }
    }

    if (r->tag != brui_resizable_tag_ancor_helper) {
      animate = brui_snap_areas(mouse_pos, drag_index, rs);
    }
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
  bruir_update_extent(rs, drag_index, new_extent, false, animate);
}

static br_strv_t brui_ancor_to_str(brui_ancor_t ancor);
void brui_resizable_mouse_pressl(bruirs_t* rs, br_vec2_t mouse_pos, bool ctrl_down) {
  if (rs->active_resizable < 0) return;
  brui_resizable_t* hovered = br_da_getp(*rs, rs->active_resizable);
  bool title_shown = br_anim_getf(brui__stack.anims, hovered->title_height_ah) > 0.1f;
  brui_drag_mode_t new_mode = brui_drag_mode_none;
  if ((ctrl_down || title_shown)) {
    float slack = 20;
    if (rs->active_resizable != 0) {
      br_extent_t ex = hovered->cur_extent;
      rs->drag_index = rs->active_resizable;
      rs->drag_point = mouse_pos;
      if (title_shown) new_mode = brui_drag_mode_move;
      else {
        br_vec2_t local_pos = brui_resizable_local(rs, rs->active_resizable, mouse_pos);
        if      (local_pos.x < slack)                    new_mode |= brui_drag_mode_left;
        else if (local_pos.x > (float)ex.width - slack)  new_mode |= brui_drag_mode_right;
        if      (local_pos.y < slack)                    new_mode |= brui_drag_mode_top;
        else if (local_pos.y > (float)ex.height - slack) new_mode |= brui_drag_mode_bottom;
        if      (new_mode == brui_drag_mode_none)        new_mode  = brui_drag_mode_move;
      }
      if (!(new_mode & brui_drag_mode_move)) {
        brui_resizable_t* parent = br_da_getp(*brui__stack.rs, hovered->parent);
        while (!(new_mode & brui_drag_mode_move) && parent->tag == brui_resizable_tag_ancor_helper) {
          BRUI_LOGI("Hover ancor: %s, parent->tag = %d", brui_ancor_to_str(hovered->ancor).str, parent->tag);
          if (hovered->ancor & brui_ancor_top) {
            if (new_mode & brui_drag_mode_bottom) new_mode = brui_drag_mode_move;
          } else if (hovered->ancor & brui_ancor_bottom) {
            if (new_mode & brui_drag_mode_top) new_mode = brui_drag_mode_move;
          } else if (hovered->ancor & brui_ancor_left) {
            if (new_mode & brui_drag_mode_right) new_mode = brui_drag_mode_move;
          } else if (hovered->ancor & brui_ancor_right) {
            if (new_mode & brui_drag_mode_left) new_mode = brui_drag_mode_move;
          }
          rs->drag_index = hovered->parent;
          rs->active_resizable = hovered->parent;
          hovered = parent;
          parent = br_da_getp(*brui__stack.rs, hovered->parent);
          BRUI_LOGI("Hover ancor: %s, parent->tag = %d", brui_ancor_to_str(hovered->ancor).str, parent->tag);
        }
      }
      rs->drag_mode = new_mode;
      rs->drag_old_ex = hovered->current.cur_extent;

      ACTION = brui_action_sliding;
      ACPARM.slider.value = hovered;
    }
  }
}

void brui_resizable_mouse_releasel(bruirs_t* rs, br_vec2_t mouse_pos) {
  (void)mouse_pos;
  rs->drag_index = 0;
  rs->drag_mode = brui_drag_mode_none;
  rs->drag_point = BR_VEC2(0, 0);
  ACTION = brui_action_none;
}

void brui_resizable_mouse_scroll(bruirs_t* rs, br_vec2_t delta) {
  brui_resizable_t* hovered = &rs->arr[rs->active_resizable];
  if (hovered->full_height > (float)hovered->cur_extent.height) {
    float speed = 50.f / (hovered->full_height - (float)hovered->cur_extent.height);
    hovered->target.scroll_offset_percent = br_float_clamp(hovered->target.scroll_offset_percent - delta.y * speed, 0.f, 1.f);
  } else {
    hovered->target.scroll_offset_percent = 0.f;
  }
}

void brui_resizable_page(bruirs_t* rs, br_vec2_t delta) {
  brui_resizable_t* hovered = &rs->arr[rs->active_resizable];
  float hidden_height = hovered->full_height - hovered->cur_extent.height;
  float page_percent = hovered->cur_extent.height / hidden_height;
  page_percent *= 0.9f;
  hovered->target.scroll_offset_percent = br_float_clamp(hovered->current.scroll_offset_percent + delta.y * page_percent, 0.f, 1.f);
}

void brui_resizable_scroll_percent_set(bruirs_t* rs, float percent) {
  rs->arr[rs->active_resizable].target.scroll_offset_percent = percent;
}

br_vec2_t brui_resizable_local(bruirs_t* rs, int id, br_vec2_t global_pos) {
  while (id > 0) {
    brui_resizable_t r = rs->arr[id];
    global_pos = br_vec2_sub(global_pos, r.cur_extent.pos);
    id = r.parent;
  }
  return global_pos;
}

void bruir_resizable_refresh(bruirs_t* rs, int index) {
  bruir_update_extent(rs, index, br_da_get(*rs, index).cur_extent, true, true);
}

static br_vec2_t bruir_pos_global(brui_resizable_t r) {
  if (r.parent == 0) return BR_VEC2I_TOF(r.cur_extent.pos);
  brui_resizable_t par = br_da_get(*brui__stack.rs, r.parent);
  br_vec2_t p = br_vec2_add(bruir_pos_global(par), r.cur_extent.pos);
  float hidden_heigth = par.full_height - par.cur_extent.height;
  if (hidden_heigth > 0) p.y -= par.scroll_offset_percent * hidden_heigth;
  return p;
}

static void brui_resizable_decrement_z(bruirs_t* rs, brui_resizable_t* res) {
  int cur_z = res->z;
  if (cur_z <= 1) return;

  bruir_children_t c = brui_resizable_children_temp(res->parent);
  for (int i = 0; i < c.len; ++i) {
    int sibling_index = c.arr[i];
    brui_resizable_t* sibling = br_da_getp(*rs, sibling_index);
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

static bool brui_resizable_increment_z(bruirs_t* rs, int resizable_handle) {
  brui_resizable_t* res = br_da_getp(*rs, resizable_handle);
  bool any_bigger = false;
  bool any_equal = false;
  brfl_foreach(sibling_index, *rs) {
    if (sibling_index == resizable_handle) continue;
    brui_resizable_t* sibling = br_da_getp(*rs, sibling_index);
    if (sibling->parent != res->parent) continue;
    if (sibling->z > res->z) any_bigger = true;
    if (sibling->z == res->z) any_equal = true;
    if (sibling->z - 1 != res->z) continue;
    res->max_z = 0;
    sibling->max_z = 0;
    ++res->z;
    --sibling->z;
    return true;
  }
  if (true  == any_equal);
  else if (false == any_bigger) return false;
  // We didn't find any sibling that has z value == cur->z + 1 so we can just increment cur z
  ++res->z;
  res->max_z = 0;
  return true;
}

void brui_resizable_move_on_top(bruirs_t* rs, int r_handle) {
  brui_resizable_t* res = br_da_getp(*rs, r_handle);
  while (res->parent != 0) {
    r_handle = res->parent;
    res = br_da_getp(*rs, r_handle);
  }
  while (brui_resizable_increment_z(rs, r_handle));
}

int brui_resizable_sibling_max_z(int id) {
  if (id == 0) return 0;
  brui_resizable_t* res = &brui__stack.rs->arr[id];
  bruir_children_t siblings = brui_resizable_children_temp(res->parent);
  int max_z = 0;
  for (int i = 0; i < siblings.len; ++i) {
    int sibling_id = siblings.arr[i];
    if (sibling_id == id) continue;
    brui_resizable_t* sibling = &brui__stack.rs->arr[sibling_id];
    if (sibling->z >= res->z) continue;
    int sib_z = sibling->max_z;
    max_z = sib_z > max_z ? sib_z : max_z;
  }
  return max_z;
}

bruir_children_t brui_resizable_children_temp(int resizable_handle) {
  brui__temp_children.len = 0;
  brfl_foreach(i, *brui__stack.rs) {
    if (resizable_handle == br_da_get(*brui__stack.rs, i).parent) {
      br_da_push_t(int, brui__temp_children, i);
    }
  }
  return brui__temp_children;
}

int brui_resizable_children_count(int resizable_handle) {
  int sum = 0;
  brfl_foreach(i, *brui__stack.rs) if (resizable_handle == br_da_get(*brui__stack.rs, i).parent) ++sum;
  return sum;
}

int brui_resizable_children_top_z(bruirs_t* rs, int resizable_handle) {
  int top_z = 0;
  brfl_foreach(i, *rs) if (resizable_handle == br_da_get(*rs, i).parent) top_z = br_i_max(top_z, br_da_get(*rs, i).z);
  return top_z;
}

br_extent_t brui_resizable_cur_extent(int resizable_handle) {
  br_extent_t ce = brui__stack.rs->arr[resizable_handle].cur_extent;
  if (resizable_handle > 0) {
    int parent = brui__stack.rs->arr[resizable_handle].parent;
    br_extent_t pe = brui_resizable_cur_extent(parent);
    ce.x += pe.x;
    ce.y += pe.y;
  }
  return ce;
}

static br_strv_t brui_ancor_to_str(brui_ancor_t ancor) {
  static BR_THREAD_LOCAL br_str_t buff = { 0 };
  buff.len = 0;
  if (brui_ancor_none  == ancor) br_str_push_c_str(&buff, "none");
  if (brui_ancor_left   & ancor) br_str_push_c_str(&buff, "left ");
  if (brui_ancor_right  & ancor) br_str_push_c_str(&buff, "right ");
  if (brui_ancor_top    & ancor) br_str_push_c_str(&buff, "top ");
  if (brui_ancor_bottom & ancor) br_str_push_c_str(&buff, "bottom ");
  if (buff.len == 0) br_str_push_int(&buff, ancor);
  br_str_push_zero(&buff);
  return br_str_as_view(buff);
}

static void brui_resizable_set_ancor(int res_id, int sibling_id, brui_ancor_t ancor) {
  (void)brui_ancor_to_str;
  bruirs_t* rs = brui__stack.rs;
  brui_resizable_t* res = br_da_getp(*brui__stack.rs, res_id);
  int parent_id = res->parent;
  brui_resizable_t* parent = br_da_getp(*brui__stack.rs, parent_id);
  brui_resizable_t* sibling = br_da_getp(*brui__stack.rs, sibling_id);

  BRUI_LOGI("Ancor %d and %d: %s", res_id, sibling_id, brui_ancor_to_str(ancor).str);
  if (res->ancor == brui_ancor_none) res->ancor_none_extent = res->current.cur_extent;
  if (sibling_id != 0) brui__stack.snap_cooldown = 1.f;

  res->ancor = ancor;

  if (0 != sibling_id) {
    brsp_id_t title_id = brsp_push(brui__stack.sp, br_scrach_printf("Ancor %d and %d", res_id, sibling_id));
    br_extent_t new_extent = sibling->cur_extent;
    if (ancor & (brui_ancor_top  | brui_ancor_bottom)) new_extent.height += res->cur_extent.height;
    else                                               new_extent.height = br_float_max(res->cur_extent.height, new_extent.height);
    if (ancor & (brui_ancor_left | brui_ancor_right))  new_extent.width  += res->cur_extent.width;
    else                                               new_extent.width  = br_float_max(res->cur_extent.width,  new_extent.width);
    new_extent.height = br_float_min(parent->cur_extent.height, new_extent.height);
    new_extent.width = br_float_min(parent->cur_extent.width, new_extent.width);

    brui_resizable_t new = {
      .current = {
        .tag = brui_resizable_tag_ancor_helper,
        .title_id = title_id,
        .cur_extent = new_extent,
        .parent = sibling->parent,
        .was_draw_last_frame = true,
        .ancor_none_extent = sibling->target.cur_extent,
        .title_enabled = false,
        .z = sibling->z > res->z ? sibling->z : res->z
      },
    };
    new.target.cur_extent = new_extent;
    int new_id = brfl_push(*brui__stack.rs, new);
    res = br_da_getp(*brui__stack.rs, res_id);
    sibling = br_da_getp(*brui__stack.rs, sibling_id);
    res->parent = new_id;
    res->cur_extent.pos = br_vec2_sub(res->cur_extent.pos, sibling->cur_extent.pos);
    res->target.cur_extent.pos = br_vec2_sub(res->target.cur_extent.pos, sibling->cur_extent.pos);
    sibling->ancor_none_extent = sibling->target.cur_extent;
    sibling->parent = new_id;
    sibling->cur_extent.pos = BR_VEC2(0, 0);
    sibling->target.cur_extent.pos = BR_VEC2(0, 0);
    rs->drag_point = brui__stack.mouse_pos;
    rs->drag_old_ex = res->cur_extent;
    brui_resizable_check_parents(brui__stack.rs);
    if      (ancor == brui_ancor_top)    sibling->ancor = brui_ancor_bottom;
    else if (ancor == brui_ancor_bottom) sibling->ancor = brui_ancor_top;
    else if (ancor == brui_ancor_left)   sibling->ancor = brui_ancor_right;
    else if (ancor == brui_ancor_right)  sibling->ancor = brui_ancor_left;
  } else {
    //if (res->tag == brui_resizable_tag_ancor_helper) return;
    if (parent->tag == brui_resizable_tag_ancor_helper) {
      int gp = parent->parent;
      if (parent->parent == -1) return;
      parent->parent = parent_id;
      brui_resizable_t* gparent = br_da_getp(*rs, gp);
      res->parent = gp;
      res->current.cur_extent.pos = br_vec2_add(res->current.cur_extent.pos, parent->current.cur_extent.pos);
      res->target.cur_extent.pos = br_vec2_add(res->target.cur_extent.pos, parent->current.cur_extent.pos);
      rs->drag_old_ex = res->target.cur_extent;

      brfl_foreach(i, *rs) {
        brui_resizable_t* sib = &rs->arr[i];
        if (sib->parent != parent_id) continue;
        if (i == parent_id) continue;
        sib->target.cur_extent.size = sib->ancor_none_extent.size;
        sib->target.cur_extent.pos = br_vec2_add(sib->target.cur_extent.pos, parent->current.cur_extent.pos);
        sib->current.cur_extent.pos = br_vec2_add(sib->current.cur_extent.pos, parent->current.cur_extent.pos);
        float r = sib->current.cur_extent.x + sib->target.cur_extent.width;
        float b = sib->current.cur_extent.y + sib->target.cur_extent.height;
        if (r > gparent->cur_extent.width) sib->target.cur_extent.pos.x -= r - gparent->cur_extent.width;
        if (b > gparent->cur_extent.height) sib->target.cur_extent.pos.y -= b - gparent->cur_extent.height;
        sib->ancor = parent->ancor;
        sib->parent = gp;
        if (sib->z > res->z) {
          int tmp = sib->z;
          sib->z = res->z;
          res->z = tmp;
        }
        break;
      }

      while (gparent->tag == brui_resizable_tag_ancor_helper) {
        if (parent->z > res->z) {
          int tmp = parent->z;
          parent->z = res->z;
          res->z = tmp;
        }
        res->parent = gparent->parent;
        parent->max_z = 0;
        parent = gparent;
        res->current.cur_extent.pos = br_vec2_add(res->current.cur_extent.pos, parent->current.cur_extent.pos);
        res->target.cur_extent.pos = br_vec2_add(res->target.cur_extent.pos, parent->current.cur_extent.pos);
        gp = gparent->parent;
        gparent = br_da_getp(*rs, gp);
      }
#if BR_DEBUG
      brfl_foreach(i, *rs) {
        brui_resizable_t r = br_da_get(*rs, i);
        if (i == r.parent) continue;
        BR_ASSERTF(r.parent != parent_id, "i == %d, parent = %d", i, parent_id);
      }
#endif
      brui_resizable_delete(parent_id);
      rs->drag_old_ex = res->target.cur_extent;
      rs->drag_old_ex = res->target.cur_extent;
      brui_resizable_increment_z(rs, res_id);
      brui_resizable_check_parents(rs);
    } else {
      if (ancor == brui_ancor_none) {
        res->target.cur_extent = res->ancor_none_extent;
      }
    }
  }
}

brui_resizable_t* brui_resizable_push(int id) {
  bruirs_t* rs = brui__stack.rs;
  brui_resizable_t* res = br_da_getp(*rs, id);

  res->was_draw_last_frame = true;
  br_extent_t rex = BR_EXTENTI_TOF(res->cur_extent);
  int cur_z = TOP.z;
  float title_height = br_anim_getf(brui__stack.anims, res->title_height_ah);
  bool is_menu_shown = title_height > .01f;
  brui_push();
  BRUI_LOG("resizablepre [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  TOP.psum = BR_VEC2(0, 0);
  br_vec2_t cur_p = bruir_pos_global(*res);
  TOP.cur = cur_p;
  TOP.limit = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + rex.width, TOP.cur.y + rex.height);
  float scroll_y = (res->full_height - (float)res->cur_extent.height) * res->scroll_offset_percent;

  int max_z_id = id;
  int ancor_parent_id = res->parent;
  while (ancor_parent_id > 0) {
    brui_resizable_t* parent = br_da_getp(*rs, ancor_parent_id);
    if (parent->tag == brui_resizable_tag_ancor_helper) {
      max_z_id = res->parent;
      ancor_parent_id = parent->parent;
    }
    else break;
  }
  brui_z_set(cur_z + brui_resizable_sibling_max_z(max_z_id) + (is_menu_shown ? 5 : 15));

  TOP.cur_resizable = id;

  TOP.start_z = TOP.z;
  TOP.is_active = id == rs->active_resizable;
  BRUI_LOG("resizablepost [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  TOP.hide_border = true;
  TOP.hide_bg = true;
  brui_push();
  TOP.cur_resizable = id;
  br_vec2_t mp = brui__stack.mouse_pos;
  float title_full_height = (float)TOP.font_size + 2.f * TOP.padding.y;
  float new_title_height = 0.f;
  if (res->title_enabled && false == brui__stack.ctrl_down && TOP.is_active) {
    if (br_col_vec2_extent(BR_EXTENT2(cur_p, BR_SIZE(res->cur_extent.width, title_full_height * 0.2f)), mp)) {
      new_title_height = title_full_height;
    } else if (false == br_col_vec2_extent(BR_EXTENT2(cur_p, BR_SIZE(res->cur_extent.width, title_full_height * 2.f)), mp)) {
      new_title_height = 0;
    }
  }
  br_anim_setf(brui__stack.anims, res->title_height_ah, new_title_height);
  if (is_menu_shown) {
    brui_push_simple();
      TOP.start_z = res->max_z + 2;
      TOP.z = TOP.start_z;
      TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.limit.min_y + title_height);
      float button_width = 20.f;
      brui_vsplitvp(5, BRUI_SPLITR(1), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width));
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Z-"))) brui_resizable_decrement_z(rs, res);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Z+"))) brui_resizable_increment_z(rs, id);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("[]"))) brui_resizable_set_ancor(id, 0, res->ancor == brui_ancor_all ? brui_ancor_none : brui_ancor_all);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("X"))) res->target.hidden_factor = 1.f;
      brui_vsplit_end();
      brui_background(TOP.limit, BR_THEME.colors.plot_bg);
    brui_pop_simple();
  }
  TOP.cur.y -= scroll_y;
  brui_max_z = 0;
  TOP.limit.min_y += title_height;
  TOP.cur.y += title_height;
  return res;
}

void brui_resizable_pop(void) {
  bruirs_t* rs = brui__stack.rs;
  int cur_res = TOP.cur_resizable;
  brui_resizable_t* res = br_da_getp(*rs, cur_res);

  float full_height = res->full_height = TOP.content_height;
  float hidden_height = full_height - (float)res->cur_extent.height;
  if (hidden_height > 0.f) {
    if (brui_resizable_hidden_factor(rs, &rs->arr[cur_res]) < 0.01 && false == brui__stack.ctrl_down) brui_scroll_bar(&res->target.scroll_offset_percent);
  } else res->target.scroll_offset_percent = 0.f;
  TOP.cur.y = (float)res->cur_extent.y + (float)res->cur_extent.height;
  TOP.content_height = (float)res->cur_extent.height;
  brui_pop();
  brui_pop();
  res->max_z = brui_max_z;
  TOP.content_height = (float)res->cur_extent.height;
  if (cur_res == rs->active_resizable && brui__stack.rs->drag_mode == brui_drag_mode_move) {
    brui_snap_areas(brui__stack.mouse_pos, cur_res, rs);
  }
}

void brui_resizable_show(int resizable_handle, bool show) {
  br_da_getp(*brui__stack.rs, resizable_handle)->target.hidden_factor = show ? 0.f : 1.f;
}

void brui_resizable_maximize(int resizable_handle, bool maximize) {
  br_da_getp(*brui__stack.rs, resizable_handle)->ancor = maximize ? brui_ancor_all : brui_ancor_none;
}

bool brui_resizable_is_hidden(int resizable_handle) {
  return br_da_get(*brui__stack.rs, resizable_handle).hidden_factor > 0.99f;
}

br_vec2_t brui_resizable_to_global(int resizable_handle, br_vec2_t pos) {
  if (resizable_handle == 0) return pos;
  brui_resizable_t* r = br_da_getp(*brui__stack.rs, resizable_handle);
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

  ptrdiff_t index = stbds_hmgeti(bruir__temp_res, hash);
  bool just_created = false;
  brui_resizable_temp_state_t state = { 0 };
  brsp_id_t title_id = -1;
  if (index < 0) {
    state.was_drawn = true;
    state.resizable_handle = brui_resizable_new(brui__stack.rs, BR_EXTENT(0, 0, 100, 100), 0);
    state.is_deleted = false;
    stbds_hmput(bruir__temp_res, hash, state);
    title_id = brsp_push(brui__stack.sp, id);
    just_created = true;
  } else {
    state = bruir__temp_res[index].value;
    if (state.is_deleted) {
      just_created = true;
      bruir__temp_res[index].value.is_deleted = false;
      bruir__temp_res[index].value.resizable_handle = brui_resizable_new(brui__stack.rs, BR_EXTENT(0, 0, 100, 100), 0);
      state = bruir__temp_res[index].value;
      title_id = brsp_push(brui__stack.sp, id);
    }
    bruir__temp_res[index].value.was_drawn = true;
  }

  brui_resizable_temp_last = state.resizable_handle;
  brui_resizable_t* res = brui_resizable_push(state.resizable_handle);
  if (title_id > -1) res->title_id = title_id;
  if (just_created) {
    res->target.cur_extent = BR_EXTENTI_TOF(brui__stack.rs->arr[0].cur_extent);
    res->target.cur_extent.width -= 100;
    res->target.cur_extent.height -= 100;
    res->target.cur_extent.x += 50;
    res->target.cur_extent.y += 50;
  }
  return (brui_resizable_temp_push_t) { .res = res, .resizable_handle = state.resizable_handle, .just_created = just_created };
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
    handle = bruir__temp_res[index].value.resizable_handle;
    brui_resizable_delete(handle);
    (void)stbds_hmdel(bruir__temp_res, hash);
  }
}

void brui_resizable_temp_delete_all(void) {
  bruirs_t* rs = brui__stack.rs;
  ptrdiff_t len = stbds_hmlen(bruir__temp_res);
  for (int i = 0; i < len; ++i) {
    if (bruir__temp_res[i].value.is_deleted) continue;
    brfl_remove(*rs, bruir__temp_res[i].value.resizable_handle);
  }
  br_anim_delete(brui__stack.anims, ACPARM.text.offset_ahandle);
}

static int bruir_find_at(bruirs_t* rs, int index, br_vec2_t loc, br_vec2_t* out_local_pos) {
  brui_resizable_t res = br_da_get(*rs, index);
  if (res.current.hidden_factor > 0.9f) return -1;
  if (index != 0 && false == res.current.was_draw_last_frame) return -1;
  if (br_anim_getf(brui__stack.anims, res.current.title_height_ah) > 0.1f) return index;
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
      int cur = bruir_find_at(rs, i, local, out_local_pos);
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

static void bruir_update_extent(bruirs_t* rs, int index, br_extent_t new_ex, bool force, bool animate) {
  (void)animate;
  brui_resizable_check_parents(brui__stack.rs);
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
    /*
    if (false == animate) {
      res->current.cur_extent.pos = new_ex.pos;
      if (rs->drag_mode != brui_drag_mode_move && index == rs->drag_index) {
        res->current.cur_extent.size = new_ex.size;
      }
    }
    */
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
      if (changed == true) bruir_update_extent(rs, i, child_extent, force, true);
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
