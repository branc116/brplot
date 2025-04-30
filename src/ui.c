#include "src/br_ui.h"
#include "src/br_icons.h"
#include "src/br_math.h"
#include "src/br_pp.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"
#include "src/br_theme.h"
#include "src/br_da.h"
#include "src/br_tl.h"
#include "src/br_gl.h"
#include "src/br_shaders.h"

#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  br_vec2_t cur;
  br_bb_t limit;
  int start_z, z;
  br_vec2_t psum;
  float content_height;

  br_vec2_t padding;

  int font_size;
  br_color_t font_color;
  br_text_renderer_ancor_t text_ancor;

  int cur_resizable;

  float vsplit_max_height;

  bool is_active;
  bool hide_border;
  bool hide_bg;
} brui_stack_el_t;

typedef struct {
  brui_stack_el_t* arr;
  size_t len, cap;

  int active_resizable;
  void* sliderf;
  br_vec2_t drag_ancor_point;
  bool log;
} brui_stack_t;

typedef struct {
  int* arr;
  int len, cap;
} bruir_children_t;

typedef struct {
  bruir_children_t* arr;
  int len, cap;
} bruir_childrens_t;

typedef struct {
  brui_resizable_t* arr;
  int len, cap;

  brui_drag_mode_t drag_mode;
  int drag_index;
  br_vec2_t drag_point;
  br_extent_t drag_old_ex;
  int next_free;
} bruirs_t;

static BR_THREAD_LOCAL bruirs_t bruirs;
static BR_THREAD_LOCAL bruir_childrens_t bruir_childrens;

static BR_THREAD_LOCAL brui_stack_t brui__stack;
BR_THREAD_LOCAL int brui__n__;

BR_THREAD_LOCAL char brui__scrach[2048];
#define TOP (brui__stack.arr[brui__stack.len ? brui__stack.len - 1 : 0])
#define TOP2 (brui__stack.arr[brui__stack.len > 1 ? brui__stack.len - 2 : 0])
#define Z (TOP.z++)
#define ZGL BR_Z_TO_GL(TOP.z++)
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
    printf("[CUR:%.3f,%.3f][LIMIT:%.2f,%.2f,%.2f,%.2f][PSUM:%.2f %.2f] " fmt "\n", TOP.cur.x, TOP.cur.y, BR_BB_(TOP.limit), TOP.psum.x, TOP.psum.y, ##__VA_ARGS__); \
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
    new_el.z += 5;
    new_el.cur.x = new_el.limit.min_x + new_el.psum.x;
    new_el.cur.y += TOP.padding.y;
    new_el.start_z = new_el.z;
    new_el.cur_resizable = 0;
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

void brui_begin(void) {
  brui__stack.log = brtl_key_pressed(BR_KEY_U);
  BRUI_LOG("begin");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(brui__stack, new_el);
  BRUI_LOG("beginafter");
}

void brui_end(void) {
  BR_ASSERT(brui__stack.len == 1);
  brui__stack.len = 0;
  BRUI_LOG("end");
}

void brui_push(void) {
  BRUI_LOG("push");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(brui__stack, new_el);
}

void brui_pop(void) {
  float width = BR_BBW(TOP.limit) - 2 * fminf(TOP2.psum.x, TOP.psum.x);
  float height = TOP.content_height;
  br_size_t size = BR_SIZE(width, height);
  br_bb_t bb = BR_BB(TOP2.cur.x, TOP2.cur.y, TOP2.cur.x + size.width, TOP2.cur.y + size.height);
  float content_height = TOP.content_height;

  if (TOP.hide_bg == false) brui_background(bb, BR_THEME.colors.plot_menu_color);
  if (TOP.hide_border == false) brui_border(bb);

  BRUI_LOG("Pre pop ex: [%.2f,%.2f,%.2f,%.2f]", BR_BB_(bb));
  float tt = TOP.cur.y;
  --brui__stack.len;
  TOP.cur.y = tt + TOP.padding.y;
  TOP.cur.x = TOP.limit.min_x + TOP.psum.x;
  TOP.content_height += content_height + TOP.padding.y;
  BRUI_LOG("pop");
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

static inline bool brui_extent_is_good(br_extent_t e, br_extent_t parent) {
  return e.x >= 0 &&
    e.y >= 0 &&
    e.y + e.height <= parent.height &&
    e.x + e.width <= parent.width &&
    e.height > 100 &&
    e.width > 100;
}

br_size_t brui_text(br_strv_t strv) {
  BRUI_LOG("Text: %.*s", strv.len, strv.str);
  br_text_renderer_t* tr = brtl_text_renderer();
  br_vec2_t loc = TOP.cur;
  float out_top /* neg or 0 */ = fminf(TOP.cur.y - TOP.limit.min_y, 0.f); 
  float opt_height = (float)TOP.font_size + TOP.padding.y;
  
  br_size_t space_left = BR_SIZE(BR_BBW(TOP.limit) - 2 * TOP.psum.x - (TOP.cur.x - (TOP.limit.min_x + TOP.psum.x)), TOP.limit.max_y - TOP.cur.y + out_top);
  BRUI_LOG("TEXT: Space left %f %f", space_left.width, space_left.height);
  br_strv_t fit = br_text_renderer_fit(tr, space_left, TOP.font_size, strv);
  br_extent_t ex = { 0 };
  if (fit.len != 0) {
    if (TOP.text_ancor & br_text_renderer_ancor_y_mid) loc.y += BR_BBH(TOP.limit) * 0.5f;
    if (TOP.text_ancor & br_text_renderer_ancor_x_mid) loc.x += BR_BBW(TOP.limit) * 0.5f;
    br_bb_t text_limit = TOP.limit;
    text_limit.min_x += TOP.psum.x;
    text_limit.max_x -= TOP.psum.x;

    ex = br_text_renderer_push2(tr, BR_VEC3(loc.x, loc.y, ZGL), TOP.font_size, TOP.font_color, fit, text_limit, TOP.text_ancor);
  }
  TOP.cur.x = TOP.limit.min_x + TOP.psum.x;
  TOP.cur.y += opt_height;
  TOP.content_height += opt_height;
  return ex.size;
}

void brui_new_lines(int n) {
  float off = (float)TOP.font_size * (float)n;
  TOP.cur.y += off;
  TOP.content_height += off;
}

void brui_background(br_bb_t bb, br_color_t color) {
  br_icons_draw(bb, BR_BB(0,0,0,0), color, color, TOP.limit, TOP.start_z - 1);
}

void brui_border(br_bb_t bb) {
  br_color_t ec = BR_THEME.colors.ui_edge_inactive;
  br_color_t bc = BR_THEME.colors.ui_edge_bg_inactive;

  float edge = 0;
  float thick = BR_THEME.ui.border_thick;

  if (TOP.is_active && br_col_vec2_bb(bb, brtl_mouse_pos())) {
    ec = BR_THEME.colors.ui_edge_active;
    bc = BR_THEME.colors.ui_edge_bg_active;
  }

  br_icons_draw(BR_BB(bb.min_x + edge, bb.min_y, bb.max_x - edge, bb.min_y + thick), BR_EXTENT_TOBB(br_extra_icons.edge_t.size_8), ec, bc, TOP.limit, Z);
  br_icons_draw(BR_BB(bb.min_x + edge, bb.max_y - thick, bb.max_x - edge, bb.max_y), BR_EXTENT_TOBB(br_extra_icons.edge_b.size_8), ec, bc, TOP.limit, Z);
  br_icons_draw(BR_BB(bb.min_x, bb.min_y + edge, bb.min_x + thick, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_l.size_8), ec, bc, TOP.limit, Z);
  br_icons_draw(BR_BB(bb.max_x - thick, bb.min_y + edge, bb.max_x, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_r.size_8), ec, bc, TOP.limit, Z);
}

bool brui_collapsable(br_strv_t name, bool* expanded) {
  float opt_header_height /* text + 2*1/2*padding */ = (float)TOP.font_size + TOP.padding.y;
  brui_border(BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_header_height));
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
  brui_border(BR_BB(TOP.limit.min_x, TOP.limit.min_y - opt_header_height, TOP.limit.max_x, TOP.cur.y));
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
  bool hovers = br_col_vec2_bb(button_limit, brtl_mouse_pos());
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
    brui_border(BR_BB(TOP.cur.x, TOP.cur.y, TOP.limit.max_x - TOP.psum.x, TOP.cur.y + opt_height));
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
    br_extent_t icon = *checked ?  br_icons.cb_1.size_32 : br_icons.cb_0.size_32;
    //if (cb_extent.max_y > TOP.limit.max_y) cb_extent.max_y = TOP.limit.max_y;
    /*if (cb_extent.height > 0) */ {
      hover = br_col_vec2_bb(cb_extent, brtl_mouse_pos());
      br_color_t bg = hover ? BR_THEME.colors.btn_hovered : BR_THEME.colors.btn_inactive;
      br_color_t fg = hover ? BR_THEME.colors.btn_txt_hovered : BR_THEME.colors.btn_txt_inactive;
      br_icons_draw(cb_extent, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z);
      TOP.cur.x += TOP.padding.x + sz;
    }

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

void brui_img(unsigned int texture_id) {
  br_shader_img_t* img = brtl_shaders()->img;
  img->uvs.image_uv = brgl_framebuffer_to_texture(texture_id);
  TOP.z += 2;
  float gl_z = ZGL;
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
    if (TOP.is_active && br_col_vec2_bb(bb, brtl_mouse_pos())) {
      br_color_t fg = BR_THEME.colors.btn_txt_hovered;
      br_color_t bg = BR_THEME.colors.btn_hovered;
      br_icons_draw(bb, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z);
      is_pressed = brtl_mousel_pressed();
    } else {
      br_color_t fg = BR_THEME.colors.btn_txt_inactive;
      br_color_t bg = BR_THEME.colors.btn_inactive;
      br_icons_draw(bb, BR_EXTENT_TOBB(icon), bg, fg, TOP.limit, Z);
    }
  }
  TOP.cur.y += opt_height;
  TOP.content_height += opt_height;
  return is_pressed;
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

      if (brui__stack.sliderf == val ||
          (brui__stack.sliderf == NULL && mouse.x > line_bb.min_x &&
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
          brui__stack.sliderf = val;
        } else {
          lc = BR_THEME.colors.btn_hovered;
          sc = BR_THEME.colors.btn_txt_hovered;
          brui__stack.sliderf = NULL;
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
  if (brui__stack.sliderf == value) {
    if (brtl_mousel_down() == false) {
      brui__stack.sliderf = NULL;
    } else {
      *value *= 1.f - brtl_frame_time() * (brui__stack.drag_ancor_point.x - mouse_pos.x) / 1000.f;
      brui_background(bb, BR_THEME.colors.btn_active);
    }
  } else if (brui__stack.sliderf == NULL && brtl_mousel_down() && br_col_vec2_bb(bb, mouse_pos)) {
    brui__stack.sliderf = value;
    brui__stack.drag_ancor_point = mouse_pos;
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
  brui_border(bb);
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
  brui_border(bb);
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
    new_el.cur_resizable = 0;
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
    top.cur_resizable = 0;
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
  --brui__stack.len;
  TOP.vsplit_max_height = fmaxf(cur_max, cur_ch);
  BRUI_LOG("vsplit post pop");
}

void brui_vsplit_end(void) {
  float cur_max = TOP.vsplit_max_height;
  float cur_ch = TOP.content_height;
  --brui__stack.len;
  float max_ch = fmaxf(cur_max, cur_ch);
  TOP.cur.y += max_ch + TOP.padding.y;
  TOP.content_height += max_ch + TOP.padding.y;
  BRUI_LOG("vsplit end");
}

void brui_scroll_bar(float* bar_offset_fract) {
  float thick = TOP.padding.x * 0.5f;
  float slider_thick = TOP.padding.x * 0.8f;
  int z = Z;
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
  if (brui__stack.sliderf == bar_offset_fract || (brui__stack.sliderf == NULL && br_col_vec2_bb(line_bb, mouse))) {
    if (is_down) {
      *bar_offset_fract = br_float_clamp((mouse.y - TOP.limit.min_y) / limit_height, 0, 1);
      lc = BR_THEME.colors.btn_active;
      bc = BR_THEME.colors.btn_txt_active;
      brui__stack.sliderf = bar_offset_fract;
    } else {
      lc = BR_THEME.colors.btn_hovered;
      bc = BR_THEME.colors.btn_txt_hovered;
      brui__stack.sliderf = NULL;
    }
  }
  br_icons_draw(line_bb, BR_BB(0,0,0,0), lc, lc, TOP.limit, z + 10);
  br_icons_draw(slider_bb, BR_BB(0,0,0,0), bc, bc, TOP.limit, z + 11);
}

void brui_text_align_set(br_text_renderer_ancor_t ancor) {
  TOP.text_ancor = ancor;
}

void brui_text_color_set(br_color_t color) {
  TOP.font_color = color;
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

float brui_min_y(void) {
  return TOP.limit.min_y;
}

void brui_height_set(float value) {
  TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.cur.y + value);
  TOP.limit.min_y = fmaxf(TOP.limit.min_y, TOP.cur.y);
}

float brui_padding_x(void) {
  return TOP.padding.x;
}

void brui_padding_y_set(float value) {
  TOP.padding.y = value;
}

float brui_y(void) {
  return TOP.cur.y;
}

bool brui_active(void) {
  return TOP.is_active;
}


// ---------------------------Resizables--------------------------
static void bruir_update_ancors(int index, float dx, float dy, float dw, float dh);
static int bruir_find_at(int index, br_vec2_t loc, br_vec2_t* out_local_pos);
static void bruir_update_extent(int index, br_extent_t new_ex, bool force);

void brui_resizable_init(void) {
  brui_resizable_t screen = { 0 };
  screen.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
  screen.ancor = brui_ancor_all;
  screen.z = 0.f;
  screen.alloced = true;
  screen.next_free = -1;
  bruir_children_t children = { 0 };
  bruirs.next_free = -1;
  br_da_push_t(int, bruirs, screen);
  br_da_push_t(int, bruir_childrens, children);
}

void brui_resizable_deinit(void) {
  br_da_free(bruirs);
  for (int i = 0; i < bruir_childrens.len; ++i) {
    bruir_children_t* t = br_da_getp(bruir_childrens, i);
    br_da_free(*t);
  }
  br_da_free(bruir_childrens);
}

int brui_resizable_new(br_extent_t init_extent, int parent) {
  BR_ASSERT(bruirs.len > parent);
  
  int new_id = 0;
  if (bruirs.next_free == -1) {
    brui_resizable_t new = { 0 };
    new.target.cur_extent = init_extent;
    new.z = br_da_get(bruir_childrens, parent).len + 1;
    new.parent = parent;
    new.alloced = true;
    new.next_free = -1;
    new_id = bruirs.len;
    br_da_push_t(int, bruirs, new);

    bruir_children_t children = { 0 };
    br_da_push_t(int, bruir_childrens, children);
  } else {
    new_id = bruirs.next_free;
    brui_resizable_t* r = br_da_getp(bruirs, new_id);
    bruirs.next_free = r->next_free;
    *r = (brui_resizable_t) { 0 };
    r->target.cur_extent = init_extent;
    r->z = br_da_get(bruir_childrens, parent).len + 1;
    r->parent = parent;
    r->alloced = true;
    r->next_free = -1;
  }

  bruir_children_t* to_push = br_da_getp(bruir_childrens, parent);
  br_da_push_t(int, *to_push, new_id);
  if (parent > 0) bruir_update_extent(new_id, bruirs.arr[new_id].target.cur_extent, true);
  return new_id;
}

int brui_resizable_new2(br_extent_t init_extent, int parent, brui_resizable_t template) {
  BR_ASSERT(bruirs.len > parent);

  int new_id = 0;
  if (bruirs.next_free == -1) {
    brui_resizable_t new = template;
    new.target.cur_extent = init_extent;
    new.z = br_da_get(bruir_childrens, parent).len + 1;
    new.parent = parent;
    new.alloced = true;
    new.next_free = -1;
    new_id = bruirs.len;
    br_da_push_t(int, bruirs, new);

    bruir_children_t children = { 0 };
    br_da_push_t(int, bruir_childrens, children);
  } else {
    new_id = bruirs.next_free;
    brui_resizable_t* r = br_da_getp(bruirs, new_id);
    bruirs.next_free = r->next_free;
    *r = template;
    r->target.cur_extent = init_extent;
    r->z = br_da_get(bruir_childrens, parent).len + 1;
    r->parent = parent;
    r->alloced = true;
    r->next_free = -1;
  }

  bruir_children_t* to_push = br_da_getp(bruir_childrens, parent);
  br_da_push_t(int, *to_push, new_id);
  if (parent > 0) bruir_update_extent(new_id, bruirs.arr[new_id].target.cur_extent, true);
  return new_id;
}

void brui_resizable_delete(int handle) {
  brui_resizable_t* r = br_da_getp(bruirs, handle);
  bruir_children_t* c = br_da_getp(bruir_childrens, handle);
  br_da_free(*c);
  *r = (brui_resizable_t) { 0 };
  r->next_free = bruirs.next_free;
  bruirs.next_free = handle;
}

float brui_resizable_hidden_factor(brui_resizable_t* r) {
  if (r->parent == 0 || r->hidden_factor > 0.99f) return r->hidden_factor;
  else return fmaxf(brui_resizable_hidden_factor(&bruirs.arr[r->parent]), r->hidden_factor);
}

void brui_resizable_update(void) {
  bruir_update_extent(0, BR_EXTENTI_TOF(brtl_viewport()), false);
  br_vec2_t mouse_pos = brtl_mouse_pos();

  float lerp_speed = brtl_frame_time() * brtl_theme()->ui.animation_speed;
  for (int i = 0; i < bruirs.len; ++i) {
    bruirs.arr[i].hidden_factor = br_float_lerp(bruirs.arr[i].hidden_factor, bruirs.arr[i].target.hidden_factor, lerp_speed);
    bruirs.arr[i].scroll_offset_percent = br_float_lerp(bruirs.arr[i].scroll_offset_percent, bruirs.arr[i].target.scroll_offset_percent, lerp_speed);

    br_extent_t target_ex = bruirs.arr[i].target.cur_extent;
    float cur_hidden_factor = brui_resizable_hidden_factor(&bruirs.arr[i]);
    target_ex.size = br_size_scale(target_ex.size, 1.f - cur_hidden_factor);
    if (bruirs.arr[i].parent > 0) target_ex.pos = br_vec2_scale(target_ex.pos, 1.f - cur_hidden_factor);
    bruirs.arr[i].cur_extent = br_extent_lerp(bruirs.arr[i].cur_extent, target_ex, lerp_speed);
  }

  if (bruirs.drag_mode == brui_drag_mode_none) {
    br_vec2_t local_pos = { 0 };
    int index = bruir_find_at(0, mouse_pos, &local_pos);
    if (index <= 0) return;
    brui_resizable_t* hovered = br_da_getp(bruirs, index);
    bool ml = brtl_mousel_down();
    bool mr = brtl_mouser_down();
    if (false == ml && false == mr) brui__stack.active_resizable = index;
    float scroll = brtl_mouse_scroll().y;
    if (brtl_key_ctrl() && ml == true) {
      float slack = 20;
      brui_drag_mode_t new_mode = brui_drag_mode_none;
      if (index != 0) {
        br_extent_t ex = hovered->cur_extent;
        bruirs.drag_index = index;
        bruirs.drag_point = mouse_pos;
        if      (local_pos.x < slack)                    new_mode |= brui_drag_mode_left;
        else if (local_pos.x > (float)ex.width - slack)  new_mode |= brui_drag_mode_right;
        if      (local_pos.y < slack)                    new_mode |= brui_drag_mode_top;
        else if (local_pos.y > (float)ex.height - slack) new_mode |= brui_drag_mode_bottom;
        if      (new_mode == brui_drag_mode_none)        new_mode  = brui_drag_mode_move;
        bruirs.drag_mode = new_mode;
        bruirs.drag_old_ex = ex;
        brui__stack.sliderf = hovered;
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
      br_extent_t new_extent = bruirs.drag_old_ex;
      if (bruirs.drag_mode == brui_drag_mode_move) {
        new_extent.pos = br_vec2_sub(bruirs.drag_old_ex.pos, br_vec2_sub(bruirs.drag_point, mouse_pos));
      } else {
        if (bruirs.drag_mode & brui_drag_mode_left) {
          float dif = bruirs.drag_point.x - mouse_pos.x;
          new_extent.width  = bruirs.drag_old_ex.width  + dif;
          new_extent.x      = bruirs.drag_old_ex.x      - dif;
        } else if (bruirs.drag_mode & brui_drag_mode_right) {
          float dif = bruirs.drag_point.x - mouse_pos.x;
          new_extent.width  = bruirs.drag_old_ex.width  - dif;
        }
        if (bruirs.drag_mode & brui_drag_mode_top) {
          float dif = bruirs.drag_point.y - mouse_pos.y;
          new_extent.y      = bruirs.drag_old_ex.y      - dif;
          new_extent.height = bruirs.drag_old_ex.height + dif;
        } else if (bruirs.drag_mode & brui_drag_mode_bottom) {
          float dif = bruirs.drag_point.y - mouse_pos.y;
          new_extent.height = bruirs.drag_old_ex.height - dif;
        }
      }
      bruir_update_extent(bruirs.drag_index, new_extent, false);
    } else {
      bruirs.drag_index = 0;
      bruirs.drag_mode = brui_drag_mode_none;
      bruirs.drag_point = BR_VEC2(0, 0);
      brui__stack.sliderf = NULL;
    }
  }
}

void bruir_resizable_refresh(int index) {
  bruir_update_extent(index, br_da_get(bruirs, index).cur_extent, true);
}

static br_vec2_t bruir_pos_global(brui_resizable_t r) {
  if (r.parent == 0) {
    return BR_VEC2I_TOF(r.cur_extent.pos);
  }
  return br_vec2_add(bruir_pos_global(br_da_get(bruirs, r.parent)), BR_VEC2I_TOF(r.cur_extent.pos));
}

void brui_resizable_push(int id) {
  brui_resizable_t* res = br_da_getp(bruirs, id);

  br_extent_t rex = BR_EXTENTI_TOF(res->cur_extent);
  brui_push();
  BRUI_LOG("resizablepre [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  TOP.psum = BR_VEC2(0, 0);
  TOP.cur = bruir_pos_global(*res);
  TOP.limit = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + rex.width, TOP.cur.y + rex.height);
  float scroll_y = (res->full_height - (float)res->cur_extent.height) * res->scroll_offset_percent;
  brui_z_set(TOP.z + res->z * ((4*1024) >> (brui__stack.len)));
  TOP.cur_resizable = id;
  TOP.start_z = TOP.z;
  TOP.is_active = id == brui__stack.active_resizable;
  BRUI_LOG("resizablepost [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  TOP.hide_border = true;
  TOP.hide_bg = true;
  brui_push();
  TOP.cur.y -= scroll_y;
  TOP.cur_resizable = id;
}

void brui_resizable_pop(void) {
  brui_resizable_t* res = br_da_getp(bruirs, TOP.cur_resizable);
  float full_height = res->full_height = TOP.content_height;
  float hidden_height = full_height - (float)res->cur_extent.height;
  if (hidden_height > 0.f) {
    if (brui_resizable_hidden_factor(&bruirs.arr[TOP.cur_resizable]) < 0.01 && false == brtl_key_ctrl()) brui_scroll_bar(&res->scroll_offset_percent);
  } else res->scroll_offset_percent = 0.f;
  TOP.cur.y = (float)res->cur_extent.y + (float)res->cur_extent.height;
  TOP.content_height = (float)res->cur_extent.height;
  brui_pop();
  brui_pop();
  TOP.content_height = (float)res->cur_extent.height;
}

int brui_resizable_active(void) {
  return brui__stack.active_resizable;
}

void brui_resizable_show(int resizable_handle, bool show) {
  bruirs.arr[resizable_handle].target.hidden_factor = show ? 0.f : 1.f;
}

bool brui_resizable_is_hidden(int resizable_handle) {
  return bruirs.arr[resizable_handle].hidden_factor > 0.99f;
}

brui_resizable_t* brui_resizable_get(int id) {
  return br_da_getp(bruirs, id);
}

static int bruir_find_at(int index, br_vec2_t loc, br_vec2_t* out_local_pos) {
  brui_resizable_t res = br_da_get(bruirs, index);
  if (res.hidden_factor > 0.9f) return -1;
  br_vec2_t local = BR_VEC2(loc.x - (float)res.cur_extent.x, loc.y - (float)res.cur_extent.y);
  if (local.x < 0) return -1;
  if (local.y < 0) return -1;
  if (local.x > (float)res.cur_extent.width) return -1;
  if (local.y > (float)res.cur_extent.height) return -1;

  bruir_children_t children = br_da_get(bruir_childrens, index);
  int found = -1;
  int best_z = 0;
  for (int i = 0; i < children.len; ++i) {
    int index_c = br_da_get(children, i);
    int cur_z = br_da_get(bruirs, index_c).z;
    if (cur_z > best_z) {
      int cur = bruir_find_at(index_c, local, out_local_pos);
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

static void bruir_update_ancors(int index, float dx, float dy, float dw, float dh) {
  (void)index; (void)dx; (void)dy; (void)dw; (void)dh;
  // TODO
}

static void bruir_update_extent(int index, br_extent_t new_ex, bool force) {
  brui_resizable_t res = br_da_get(bruirs, index);
  br_extent_t old_ex = res.target.cur_extent;
  brui_resizable_t parent = br_da_get(bruirs, res.parent);

  if (force || br_extent_eq(new_ex, old_ex) == false) {
    if (index != 0) {
      new_ex.x = BR_MAX(BR_MIN(new_ex.x, parent.target.cur_extent.width - new_ex.width), 0);
      new_ex.y = BR_MAX(BR_MIN(new_ex.y, parent.target.cur_extent.height - new_ex.height), 0);
      bool new_is_good = brui_extent_is_good(new_ex, br_da_get(bruirs, res.parent).target.cur_extent);
      bool old_is_good = brui_extent_is_good(res.target.cur_extent, br_da_get(bruirs, res.parent).target.cur_extent);
      if (new_is_good == false && old_is_good == true) return;
    }

    br_da_getp(bruirs, index)->target.cur_extent = new_ex;
    float dx = (float)old_ex.x      - (float)new_ex.x;
    float dy = (float)old_ex.y      - (float)new_ex.y;
    float dw = (float)old_ex.width  - (float)new_ex.width;
    float dh = (float)old_ex.height - (float)new_ex.height;
    bruir_update_ancors(0, dx, dy, dw, dh);
    bruir_children_t children = br_da_get(bruir_childrens, index);
    for (int i = 0; i < children.len; ++i) {
      int child = br_da_get(children, i);
      bool changed = false;
      br_extent_t child_extent = br_da_get(bruirs, child).target.cur_extent;
      if (child_extent.x > new_ex.width) {
        child_extent.x = 0;
        changed = true;
      }
      if (child_extent.x + child_extent.width > new_ex.width) {
        child_extent.width = new_ex.width - child_extent.x - 4;
        changed = true;
      }
      if (child_extent.width < 10) {
        child_extent.x = 0;
        child_extent.width = 60;
        changed = true;
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
      if (changed == true) bruir_update_extent(child, child_extent, false);
    }
  }
}

#undef TOP
#undef TOP2
#undef Z
#undef ZGL
#undef RETURN_IF_OUT1
