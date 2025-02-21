#include "src/br_ui.h"
#include "src/br_icons.h"
#include "src/br_math.h"
#include "src/br_help.h"
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
  br_vec2_t start;
  br_bb_t limit;
  int start_z, z;
  br_vec2_t psum;
  float scroll_y;
  float content_height;

  br_vec2_t padding;

  int font_size;
  br_color_t font_color;
  br_text_renderer_ancor_t text_ancor;

  int cur_resizable;

  int split_count;

  bool is_active;
  bool hide_border;
  bool hide_bg;
} brui_stack_el_t;

typedef struct {
  brui_stack_el_t* arr;
  size_t len, cap;

  int active_resizable;
  float* sliderf;
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
  br_extenti_t drag_old_ex;
} bruirs_t;

static BR_THREAD_LOCAL bruirs_t bruirs;
static BR_THREAD_LOCAL bruir_childrens_t bruir_childrens;

static BR_THREAD_LOCAL brui_stack_t _stack;
int __n__;

BR_THREAD_LOCAL char _scrach[2048];
#define TOP (_stack.arr[_stack.len ? _stack.len - 1 : 0])
#define TOP2 (_stack.arr[_stack.len > 1 ? _stack.len - 2 : 0])
#define Z (TOP.z++)
#define ZGL BR_Z_TO_GL(TOP.z++)
#define RETURN_IF_OUT1(HEIGHT, ...) do { \
  float h = (HEIGHT); \
  if (TOP.cur.y < TOP.limit.y) { BRUI_LOG("skip"); TOP.cur.y += h; return __VA_ARGS__; } \
  if (TOP.is_out || (TOP.cur.y + h) > TOP.limit.height) { TOP.cur.y += h; BRUI_LOG("skip: cur.y=%f", TOP.cur.y); TOP.is_out = true; return __VA_ARGS__; } \
} while(0)
#if 1
#define BRUI_LOG(fmt, ...) do { \
  if (_stack.log == false) break; \
  for (size_t i = 0; i < _stack.len; ++i) { \
    printf("  "); \
  } \
  if (_stack.len > 0) { \
    printf("[CUR:%.3f,%.3f][LIMIT:%.2f,%.2f,%.2f,%.2f][SCROLL:%.2f][START:%.2f %.2f][PSUM:%.2f %.2f] " fmt "\n", TOP.cur.x, TOP.cur.y, BR_BB_(TOP.limit), TOP.scroll_y, TOP.start.x, TOP.start.y, TOP.psum.x, TOP.psum.y, ##__VA_ARGS__); \
  } else { \
    printf(fmt "\n", ##__VA_ARGS__); \
  } \
} while(0)
#else
#define BRUI_LOG(...)
#endif

brui_stack_el_t brui_stack_el(void) {
  if (_stack.len > 0) {
    brui_stack_el_t new_el = TOP;
    new_el.psum.x += TOP.padding.x;
    new_el.z += 5;
    new_el.start = new_el.cur;
    //new_el.cur = br_vec2_add(TOP.cur, TOP.padding);
    new_el.cur.x = new_el.limit.min_x + new_el.psum.x;
    new_el.cur.y += TOP.padding.y;
    if (new_el.start.y < new_el.limit.min_y) new_el.start.y = new_el.limit.min_y;
    new_el.start.y += new_el.scroll_y;
    new_el.start_z = new_el.z;
    new_el.cur_resizable = 0;
    new_el.hide_border = false;
    new_el.hide_bg = false;
    new_el.content_height = 0;
    return new_el;
  } else {
    brui_stack_el_t root = {
      .limit = BR_EXTENTI_TOBB(brtl_viewport()),
      .padding = BR_VEC2(14, 14),
      .font_size = 26,
      .font_color = br_theme.colors.btn_txt_inactive,
      .text_ancor = br_text_renderer_ancor_left_up,
      .is_active = true
    };
    root.cur = root.padding;
    return root;
  }
}

void brui_begin(void) {
  _stack.log = brtl_key_pressed(BR_KEY_U);
  BRUI_LOG("begin");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(_stack, new_el);
  BRUI_LOG("beginafter");
//  brui_textf("TExti");
//  brui_textf("TExti");
//  brui_textf("TExti");
//  brui_textf("TExti");
//  brui_textf("TExti");
//  brui_textf("TExti");
//  brui_textf("TExti");
  return;
  brui_push();
    for (int i = 0; i < bruirs.len; ++i) {
      brui_textf("R#%d: cs: %f", i, bruirs.arr[i].full_height); 
    }
    TOP.limit.max_x = TOP.limit.min_x + 200;
    brui_button(BR_STRL("Bottun"));
    brui_button(BR_STRL("Bottun"));
    brui_button(BR_STRL("Bottun"));
    brui_button(BR_STRL("Bottun"));
    brui_button(BR_STRL("Bottun"));
    brui_button(BR_STRL("Bottun"));
    brui_checkbox(BR_STRL("CB"), &(bool) { false });
    brui_new_lines(1);
    brui_textf("TExti");
    brui_button_icon(BR_SIZEI(32, 32), br_icons.menu.size_32);
    brui_sliderf(BR_STRL("SLIDE"), &(float) { 1.5f });
    brui_vsplit(2);
      brui_text(BR_STRL("Left"));
    brui_vsplit_pop();
      brui_text(BR_STRL("Right"));
      brui_button(BR_STRL("HIHI"));
    brui_vsplit_end();
    brui_text(BR_STRL("LoooooL"));
  brui_pop();
}

void brui_end(void) {
  BR_ASSERT(_stack.len == 1);
  _stack.len = 0;
  BRUI_LOG("end");
}

void brui_push(void) {
  BRUI_LOG("push");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(_stack, new_el);
}

void brui_pop(void) {
  float width = BR_BBW(TOP.limit) - 2 * fminf(TOP2.psum.x, TOP.psum.x);
  float height = TOP.cur.y - TOP.start.y; //BR_BBH(TOP.limit) - 2 * TOP.psum.y + fminf(TOP.cur.y - TOP.limit.min_y, 0.f);
  if (TOP.cur.y > TOP.limit.max_y) {
    height -= TOP.cur.y - TOP.limit.max_y;
  }
  br_size_t size = BR_SIZE(width, height);
  int z = TOP.start_z - 1;
  br_extent_t ex = BR_EXTENT2(TOP.start, size);
  float content_height = TOP.content_height;

  if (height <= 0.f) {
    float t = TOP.cur.y - TOP.start.y;
    --_stack.len;
    TOP.cur.y += t + TOP.padding.y;
    TOP.content_height += content_height;
    return;
  }

  if (TOP.hide_bg == false) br_icons_draw(ex, BR_EXTENT(0,0,0,0), br_theme.colors.plot_menu_color, br_theme.colors.plot_menu_color, z - 1);
  if (TOP.hide_border == false) {
    BRUI_LOG("POP Draw border: %.2f %.2f %.2f %.2f", BR_EXTENT_(ex));
    br_color_t ec = br_theme.colors.ui_edge_inactive;
    br_color_t bc = br_theme.colors.ui_edge_bg_inactive;

    float edge = 4;
    float thick = edge / 4;

    if (TOP.hide_border == false && TOP.is_active && br_col_vec2_extent(ex, brtl_mouse_pos())) {
      ec = br_theme.colors.ui_edge_active;
      bc = br_theme.colors.ui_edge_bg_active;
    }
    br_icons_draw(BR_EXTENT(ex.x + edge, ex.y, ex.width - edge*2, thick), br_extra_icons.edge_t.size_8, ec, bc, Z);
    br_icons_draw(BR_EXTENT(ex.x + edge, ex.y + height - thick, ex.width - edge*2, thick), br_extra_icons.edge_b.size_8, ec, bc, Z);
    br_icons_draw(BR_EXTENT(ex.x, ex.y + edge, thick, height - 2*edge), br_extra_icons.edge_l.size_8, ec, bc, Z);
    br_icons_draw(BR_EXTENT(ex.x + ex.width - thick, ex.y + edge, thick, height - 2*edge), br_extra_icons.edge_r.size_8, ec, bc, Z);

    br_icons_draw(BR_EXTENT(ex.x, ex.y + height - edge, edge, edge), br_icons.edge.size_8, bc, ec, Z);
    br_icons_draw(BR_EXTENT(ex.x + ex.width - edge, ex.y + height - edge, edge, edge), br_extra_icons.edge_br.size_8, bc, ec, Z);
    br_icons_draw(BR_EXTENT(ex.x, ex.y, edge, edge), br_extra_icons.edge_tl.size_8, bc, ec, Z);
    br_icons_draw(BR_EXTENT(ex.x + ex.width - edge, ex.y, edge, edge), br_extra_icons.edge_tr.size_8, bc, ec, Z);
  }

  BRUI_LOG("Pre pop ex: [%.2f,%.2f,%.2f,%.2f]", BR_EXTENT_(ex));
  float tt = TOP.cur.y;
  --_stack.len;
  TOP.cur.y = tt + TOP.padding.y;
  TOP.cur.x = TOP.limit.min_x + TOP.psum.x;
  TOP.content_height += content_height;
  BRUI_LOG("pop");
}

static void brui_push_simple(void) {
  BRUI_LOG("push simp");
  brui_stack_el_t new_el = TOP; 
  br_da_push(_stack, new_el);
  BRUI_LOG("push simp post");
}

static void brui_pop_simple(void) {
  --_stack.len;
}

void brui_push_y(float y) {
  TOP.cur.y += y;
}

static inline bool brui_extent_is_good(br_extenti_t e, br_extenti_t parent) {
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
    ex = br_text_renderer_push2(tr, loc.x, loc.y, ZGL, TOP.font_size, TOP.font_color, fit, TOP.text_ancor);
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

bool brui_button(br_strv_t text) {
  float opt_height /* text + 2*1/2*padding + padding */ = (float)TOP.font_size + 2*TOP.padding.y;
  float opt_y = TOP.cur.y + opt_height;

  float button_max_x = TOP.limit.max_x - TOP.psum.x;
  float button_max_y = fminf(TOP.cur.y + opt_height, TOP.limit.max_y);
  br_bb_t button_limit = BR_BB(TOP.cur.y, TOP.cur.y, button_max_x, button_max_y);
  bool hovers = br_col_vec2_bb(button_limit, brtl_mouse_pos());
  brui_push_simple();
    TOP.limit.max_y = button_max_y;
    TOP.limit.max_x = button_max_x;
    brui_text_align_set(br_text_renderer_ancor_mid_up);
    brui_text_color_set(hovers ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive);
    brui_text(text);
  brui_pop_simple();
  TOP.content_height += opt_height;
  TOP.cur.y = opt_y;
  return hovers && brtl_mousel_pressed();
}

bool brui_checkbox(br_strv_t text, bool* checked) {
  float sz = (float)TOP.font_size * 0.6f;
  br_extent_t cb_extent = BR_EXTENT(TOP.cur.x, TOP.cur.y, sz, sz);
  float top_out /* neg or 0 */ = fminf(0.f, TOP.cur.y - TOP.limit.min_y); 
  float opt_height = (float)TOP.font_size + TOP.padding.y;
  float opt_cur_y = TOP.cur.y + opt_height;
  bool hover = false;

  brui_push_simple();
    br_extent_t icon = *checked ?  br_icons.cb_1.size_32 : br_icons.cb_0.size_32;
    if (cb_extent.y + cb_extent.height > TOP.limit.max_y) cb_extent.height = TOP.limit.max_y - cb_extent.y;
    if (cb_extent.height > 0) {
      hover = br_col_vec2_extent(cb_extent, brtl_mouse_pos());
      br_color_t bg = hover ? br_theme.colors.btn_hovered : br_theme.colors.btn_inactive;
      br_color_t fg = hover ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive;
      br_icons_draw(cb_extent, icon, bg, fg, Z);
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

bool brui_button_icon(br_sizei_t size, br_extent_t icon) {
  bool is_pressed = false;
  float out_top /* neg or 0 */ = fminf(0.f, TOP.cur.y - TOP.limit.min_y);
  float opt_height = (float)size.height + TOP.padding.y;
  float height = (float)size.height + out_top;
  br_extent_t ex = BR_EXTENT(TOP.cur.x, TOP.cur.y - out_top, (float)size.width + out_top, height);

  if (ex.height > 0 && ex.width > 0) {
    if (br_col_vec2_extent(ex, brtl_mouse_pos())) {
      br_color_t fg = br_theme.colors.btn_txt_hovered;
      br_color_t bg = br_theme.colors.btn_hovered;
      br_icons_draw(ex, icon, bg, fg, Z);
      is_pressed = brtl_mousel_pressed();
    } else {
      br_color_t fg = br_theme.colors.btn_txt_inactive;
      br_color_t bg = br_theme.colors.btn_inactive;
      br_icons_draw(ex, icon, bg, fg, Z);
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
    float to_sub = 0;

    brui_push();
      BRUI_LOG("Slider font: %d, pady: %.2f, lt: %.2f", TOP.font_size, TOP.padding.y, lt);
      TOP.limit.max_y = fminf(TOP.limit.max_y, opt_max_y);
      to_sub += (float)TOP.font_size - brui_text(text).height;
      TOP.cur.y -= to_sub;
      float h = (float)TOP.font_size - brui_textf("%f", *val).height;
      to_sub += h;
      TOP.cur.y -= h; 

      br_vec2_t mouse = brtl_mouse_pos();
      bool is_down = brtl_mousel_down();

      const float lw = BR_BBW(TOP.limit) - 2 * TOP.psum.x;
      br_extent_t line_extent = BR_EXTENT(TOP.cur.x, TOP.cur.y + (ss - lt)*0.5f, lw, lt);
      br_color_t lc = br_theme.colors.btn_inactive;
      br_extent_t slider_extent = BR_EXTENT(TOP.cur.x + (lw - ss)*.5f, TOP.cur.y, ss, ss);
      br_color_t sc = br_theme.colors.btn_txt_inactive;

      if (_stack.sliderf == val ||
          (_stack.sliderf == NULL && mouse.x > line_extent.x &&
          mouse.x < line_extent.x + line_extent.width &&
          mouse.y > line_extent.y - 3 &&
          mouse.y < line_extent.y + line_extent.height + 3)) {
        if (is_down) {
          lc = br_theme.colors.btn_active;
          sc = br_theme.colors.btn_txt_active;
          float speed = brtl_frame_time();
          float factor = br_float_clamp((mouse.x - line_extent.x) / line_extent.width, 0.f, 1.f) * speed + (1 - speed * 0.5f);
          *val *= factor;
          slider_extent.x = br_float_clamp(mouse.x, line_extent.x, line_extent.x + line_extent.width) - ss * 0.5f;
          _stack.sliderf = val;
        } else {
          lc = br_theme.colors.btn_hovered;
          sc = br_theme.colors.btn_txt_hovered;
          _stack.sliderf = NULL;
        }
      }

      BRUI_LOG("Slider inner %f %f %f %f", BR_EXTENT_(line_extent));
      if (slider_extent.y + slider_extent.height < TOP.limit.max_y) {
        if (slider_extent.y > TOP.limit.min_y) {
          br_icons_draw(line_extent, BR_EXTENT(0,0,0,0), lc, lc, Z);
          br_icons_draw(slider_extent, BR_EXTENT(0,0,0,0), sc, sc, Z);
        }
      }
      TOP.cur.y += ss + TOP.padding.y;
    brui_pop();
  brui_pop_simple();
  TOP.cur.y = opt_max_y + TOP.padding.y - to_sub;
  TOP.content_height += opt_height + TOP.padding.y;
  return false;
}

bool brui_vsplit(int n) {
  BRUI_LOG("vsplit %d", n);
  brui_stack_el_t top = TOP;
  float width = (BR_BBW(TOP.limit) - 2*TOP.psum.x) / (float)n;
  for (int i = 0; i < n; ++i) {
    brui_stack_el_t new_el = top;
    new_el.z += 5;
    new_el.psum.x = 0;
    new_el.limit.min_x = (float)i * width + top.cur.x;
    new_el.limit.max_x = new_el.limit.min_x + width;
    new_el.cur.x = new_el.limit.min.x;
    new_el.start.x = new_el.limit.min_x;
    new_el.cur_resizable = 0;
    new_el.hide_border = true;
    new_el.split_count = n;
    br_da_push(_stack, new_el);
  }
  return true;
}

void brui_vsplit_pop(void) {
  BRUI_LOG("vsplit pre pop");
  --_stack.len;
  BRUI_LOG("vsplit post pop");
}

void brui_vsplit_end(void) {
  size_t n = (size_t)TOP.split_count;
  --_stack.len;
  float max_cur = 0;
  for (size_t i = 0; i < n; ++i) {
    max_cur = fmaxf(_stack.arr[_stack.len + i].cur.y, max_cur);
  }
  TOP.cur.y = max_cur;
  TOP.cur.x = TOP.start.x + TOP.padding.x;
  BRUI_LOG("vsplit end");
}

void brui_scroll_bar(float bar_fract, float* bar_offset_fract) {
  float thick = 5.f;
  int z = Z;
  br_vec2_t mouse = brtl_mouse_pos();
  bool is_down = brtl_mousel_down();
  /*
  br_extent_t line_ex = BR_EXTENT(TOP.limit.x + TOP.limit.width + TOP.padding.x - thick, TOP.limit.y - TOP.padding.y, thick, TOP.limit.height);
  br_color_t lc = br_theme.colors.btn_inactive;
  float bar_height = TOP.limit.height * bar_fract;
  float bar_offset = (TOP.limit.height - bar_height) * *bar_offset_fract;
  br_extent_t bar_ex = BR_EXTENT(TOP.limit.x + TOP.limit.width - thick - 2.f + TOP.padding.x, TOP.limit.y + bar_offset - TOP.padding.y, thick + 2.f, bar_height);
  br_color_t bc = br_theme.colors.btn_txt_inactive;
  if (_stack.sliderf == bar_offset_fract || (_stack.sliderf == NULL && br_col_vec2_extent(line_ex, mouse))) {
    if (is_down) {
      *bar_offset_fract = br_float_clamp((mouse.y - TOP.limit.y) / TOP.limit.height, 0, 1);
      lc = br_theme.colors.btn_active;
      bc = br_theme.colors.btn_txt_active;
      _stack.sliderf = bar_offset_fract;
    } else {
      lc = br_theme.colors.btn_hovered;
      bc = br_theme.colors.btn_txt_hovered;
      _stack.sliderf = NULL;
    }
  }
  br_icons_draw(line_ex, BR_EXTENT(0,0,0,0), lc, lc, z + 10);
  br_icons_draw(bar_ex, BR_EXTENT(0,0,0,0), bc, bc, z + 11);
  */
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


// ---------------------------Resizables--------------------------
static void bruir_update_ancors(int index, float dx, float dy, float dw, float dh);
static int bruir_find_at(int index, br_vec2_t loc, br_vec2_t* out_local_pos);
static void bruir_update_extent(int index, br_extenti_t new_ex, bool force);

void brui_resizable_init(void) {
  brui_resizable_t screen = {
    .cur_extent = brtl_viewport(),
    .ancor = brui_ancor_all,
    .z = 0.f,
  };
  bruir_children_t children = { 0 };
  br_da_push_t(int, bruirs, screen);
  br_da_push_t(int, bruir_childrens, children);
}

int brui_resizable_new(br_extenti_t init_extent, int parent) {
  BR_ASSERT(bruirs.len > parent);
  //static int id = 0;

  
  brui_resizable_t new = {
    .cur_extent = init_extent,
    .z = bruir_childrens.arr[parent].len + 1,
    .parent = parent
  };
  int new_id = bruirs.len;
  br_da_push_t(int, bruirs, new);

  br_da_push_t(int, bruir_childrens.arr[parent], new_id);

  bruir_children_t children = { 0 };
  br_da_push_t(int, bruir_childrens, children);
  return new_id;
}

int brui_resizable_new2(br_extenti_t init_extent, int parent, brui_resizable_t template) {
  BR_ASSERT(bruirs.len > parent);

  brui_resizable_t new = template;
  new.cur_extent = init_extent;
  new.z = bruir_childrens.arr[parent].len + 1;
  new.parent = parent;
  int new_id = bruirs.len;
  br_da_push_t(int, bruirs, new);

  br_da_push_t(int, bruir_childrens.arr[parent], new_id);
  bruir_children_t children = { 0 };

  br_da_push_t(int, bruir_childrens, children);
  return new_id;
}

void brui_resizable_update(void) {
  bruir_update_extent(0, brtl_viewport(), false);
  br_vec2_t mouse_pos = brtl_mouse_pos();

  if (bruirs.drag_mode == brui_drag_mode_none) {
    br_vec2_t local_pos = { 0 };
    int index = bruir_find_at(0, mouse_pos, &local_pos);
    brui_resizable_t* hovered = &bruirs.arr[index];
    bool ml = brtl_mousel_down();
    bool mr = brtl_mouser_down();
    if (false == ml && false == mr) _stack.active_resizable = index;
    float scroll = brtl_mouse_scroll().y;
    if (brtl_key_ctrl() && ml == true) {
      float slack = 20;
      brui_drag_mode_t new_mode = brui_drag_mode_none;
      if (index != 0) {
        br_extenti_t ex = hovered->cur_extent;
        bruirs.drag_index = index;
        bruirs.drag_point = mouse_pos;
        if      (local_pos.x < slack)                    new_mode |= brui_drag_mode_left;
        else if (local_pos.x > (float)ex.width - slack)  new_mode |= brui_drag_mode_right;
        if      (local_pos.y < slack)                    new_mode |= brui_drag_mode_top;
        else if (local_pos.y > (float)ex.height - slack) new_mode |= brui_drag_mode_bottom;
        if      (new_mode == brui_drag_mode_none)        new_mode  = brui_drag_mode_move;
        bruirs.drag_mode = new_mode;
        bruirs.drag_old_ex = ex;
      }
    } else if (scroll != 0) {
      if (hovered->full_height > (float)hovered->cur_extent.height) {
        float diff = hovered->full_height / (float)hovered->cur_extent.height;
        float dir = scroll * 10.f;
        float move = dir / diff;
        hovered->scroll_offset_percent = br_float_clamp(hovered->scroll_offset_percent - move, 0.f, 1.f);
      } else {
        hovered->scroll_offset_percent = 0.f;
      }
    }
  } else {
    if (brtl_mousel_down()) {
      br_extenti_t new_extent = bruirs.drag_old_ex;
      if (bruirs.drag_mode == brui_drag_mode_move) {
        new_extent.pos = br_vec2i_sub(bruirs.drag_old_ex.pos, br_vec2_toi(br_vec2_sub(bruirs.drag_point, mouse_pos)));
      } else {
        if (bruirs.drag_mode & brui_drag_mode_left) {
          float dif = bruirs.drag_point.x - mouse_pos.x;
          new_extent.width  = bruirs.drag_old_ex.width  + (int)dif;
          new_extent.x      = bruirs.drag_old_ex.x      - (int)dif;
        } else if (bruirs.drag_mode & brui_drag_mode_right) {
          float dif = bruirs.drag_point.x - mouse_pos.x;
          new_extent.width  = bruirs.drag_old_ex.width  - (int)dif;
        }
        if (bruirs.drag_mode & brui_drag_mode_top) {
          float dif = bruirs.drag_point.y - mouse_pos.y;
          new_extent.y      = bruirs.drag_old_ex.y      - (int)dif;
          new_extent.height = bruirs.drag_old_ex.height + (int)dif;
        } else if (bruirs.drag_mode & brui_drag_mode_bottom) {
          float dif = bruirs.drag_point.y - mouse_pos.y;
          new_extent.height = bruirs.drag_old_ex.height - (int)dif;
        }
      }
      bruir_update_extent(bruirs.drag_index, new_extent, false);
    } else {
      bruirs.drag_index = 0;
      bruirs.drag_mode = brui_drag_mode_none;
      bruirs.drag_point = BR_VEC2(0, 0);
    }
  }
}

void bruir_resizable_refresh(int index) {
  bruir_update_extent(index, bruirs.arr[index].cur_extent, true);
}

static br_vec2_t bruir_pos_global(brui_resizable_t r) {
  if (r.parent == 0) {
    return BR_VEC2I_TOF(r.cur_extent.pos);
  }
  return br_vec2_add(bruir_pos_global(bruirs.arr[r.parent]), BR_VEC2I_TOF(r.cur_extent.pos));
}

void brui_resizable_push(int id) {
  brui_resizable_t* res = &bruirs.arr[id];
  BR_ASSERT(false == res->hidden);

  br_extent_t rex = BR_EXTENTI_TOF(res->cur_extent);
  brui_push();
  BRUI_LOG("resizablepre [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  TOP.psum = BR_VEC2(0, 0);
  TOP.cur = bruir_pos_global(*res);
  TOP.start = TOP.cur;
  TOP.limit = BR_BB(TOP.cur.x, TOP.cur.y, TOP.cur.x + rex.width, TOP.cur.y + rex.height);
  TOP.cur.y -= res->full_height / (float)res->cur_extent.height * res->scroll_offset_percent;
  brui_z_set(TOP.z + res->z * ((4*1024) >> (_stack.len)));
  TOP.cur_resizable = id;
  TOP.start_z = TOP.z;
  TOP.is_active = id == _stack.active_resizable;
  BRUI_LOG("resizablepost [%f %f %f %f] %f", BR_EXTENT_(rex), res->scroll_offset_percent);
  brui_push();
  TOP.hide_border = true;
  TOP.hide_bg = true;
  // DEBUg
//  brui_textf("content_height: %f", old_height);
//  brui_textf("scroll_offset: %f", res->scroll_offset);
}

void brui_resizable_pop(void) {
  brui_resizable_t* res = &bruirs.arr[TOP.cur_resizable];
  float full_height = res->full_height;
  float hidden_height = full_height - (float)res->cur_extent.height;
  if (hidden_height > 0.f && false == brtl_key_ctrl()) {
    brui_scroll_bar(br_float_clamp((float)res->cur_extent.height / full_height, 0, 2), &res->scroll_offset_percent);
  }
  TOP.cur.y = (float)res->cur_extent.y + (float)res->cur_extent.height;
  brui_pop();
  brui_pop();
}

int brui_resizable_active(void) {
  return _stack.active_resizable;
}

brui_resizable_t* brui_resizable_get(int id) {
  BR_ASSERT(id < bruirs.len);

  return &bruirs.arr[id];
}

static int bruir_find_at(int index, br_vec2_t loc, br_vec2_t* out_local_pos) {
  brui_resizable_t res = bruirs.arr[index];
  if (true == res.hidden) return -1;
  br_vec2_t local = BR_VEC2(loc.x - (float)res.cur_extent.x, loc.y - (float)res.cur_extent.y);
  if (local.x < 0) return -1;
  if (local.y < 0) return -1;
  if (local.x > (float)res.cur_extent.width) return -1;
  if (local.y > (float)res.cur_extent.height) return -1;

  bruir_children_t children = bruir_childrens.arr[index];
  int found = -1;
  int best_z = 0;
  for (int i = 0; i < children.len; ++i) {
    int cur_z = bruirs.arr[children.arr[i]].z;
    if (cur_z > best_z) {
      int cur = bruir_find_at(children.arr[i], local, out_local_pos);
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


static void bruir_update_extent(int index, br_extenti_t new_ex, bool force) {
  brui_resizable_t res = bruirs.arr[index];
  br_extenti_t old_ex = res.cur_extent;
  brui_resizable_t parent = bruirs.arr[res.parent];

  if (force || br_extenti_eq(new_ex, old_ex) == false) {
    if (index != 0) {
      new_ex.x = maxi32(mini32(new_ex.x, parent.cur_extent.width - new_ex.width), 0);
      new_ex.y = maxi32(mini32(new_ex.y, parent.cur_extent.height - new_ex.height), 0);
      bool new_is_good = brui_extent_is_good(new_ex, bruirs.arr[res.parent].cur_extent);
      bool old_is_good = brui_extent_is_good(res.cur_extent, bruirs.arr[res.parent].cur_extent);
      if (new_is_good == false && old_is_good == true) return;
    }

    bruirs.arr[index].cur_extent = new_ex;
    float dx = (float)old_ex.x      - (float)new_ex.x;
    float dy = (float)old_ex.y      - (float)new_ex.y;
    float dw = (float)old_ex.width  - (float)new_ex.width;
    float dh = (float)old_ex.height - (float)new_ex.height;
    bruir_update_ancors(0, dx, dy, dw, dh);
    bruir_children_t children = bruir_childrens.arr[index];
    for (int i = 0; i < children.len; ++i) {
      int child = children.arr[i];
      bool changed = false;
      br_extenti_t child_extent = bruirs.arr[child].cur_extent;
      if (child_extent.x > new_ex.width) {
        child_extent.x = 0;
        changed = true;
      }
      if (child_extent.x + child_extent.width > new_ex.width) {
        child_extent.width = new_ex.width - child_extent.x - 4;
        changed = true;
      }
      if (child_extent.width < 100) {
        child_extent.x = 0;
        child_extent.width = new_ex.width;
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
      if (child_extent.height < 100) {
        child_extent.y = 0;
        child_extent.height = new_ex.height;
        changed = true;
      }
      if (changed == true) bruir_update_extent(child, child_extent, false);
    }
  }
}

