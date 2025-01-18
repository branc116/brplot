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
  br_extent_t limit;
  int start_z, z;
  br_size_t content;

  br_vec4_t padding;
  brui_ancor_t ancor;

  int font_size;
  br_color_t font_color;
  br_text_renderer_ancor_t text_ancor;

  int cur_resizable;

  bool is_out;
  bool is_active;
  bool hide_border;
} brui_stack_el_t;

typedef struct {
  brui_stack_el_t* arr;
  size_t len, cap;

  int active_resizable;
  float* sliderf;
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
BR_THREAD_LOCAL char _scrach[2048];
#define TOP (_stack.arr[_stack.len - 1])
#define Z (TOP.z++)
#define ZGL BR_Z_TO_GL(TOP.z++)
#define RETURN_IF_OUT(HEIGHT, ...) do { \
  if (TOP.cur.y < 0) { BRUI_LOG("skip"); TOP.cur.y += (HEIGHT) + TOP.padding.y; return __VA_ARGS__; } \
  if (TOP.is_out || (TOP.cur.y + (HEIGHT)) > TOP.limit.height) { BRUI_LOG("skip"); TOP.is_out = true; return __VA_ARGS__; } \
} while(0)
#if 0
#define BRUI_LOG(fmt, ...) do { \
  for (int i = 0; i < _stack.len; ++i) { \
    printf("  "); \
  } \
  printf(fmt "\n", ##__VA_ARGS__); \
} while(0)
#else
#define BRUI_LOG(...)
#endif

brui_stack_el_t brui_stack_el(void) {
  if (_stack.len > 0) {
    brui_stack_el_t new_el;
    memcpy(&new_el, &TOP, sizeof(new_el));
    new_el.z += 5;
    new_el.limit = BR_EXTENT2(br_vec2_add(TOP.limit.pos, TOP.cur), br_size_subv(TOP.limit.size, TOP.cur));
    new_el.cur = (br_vec2_t) { .x = TOP.padding.x, .y = TOP.padding.y };
    new_el.start_z = new_el.z;
    new_el.cur_resizable = 0;
    new_el.hide_border = false;
    new_el.content = BR_SIZE(0, 0);
    return new_el;
  } else {
    return (brui_stack_el_t) {
      .cur = BR_VEC2(4, 1),
      .limit = BR_EXTENTI_TOF(brtl_viewport()),
      .padding = BR_VEC4(4, 4, 4, 4),
      .font_size = 26,
      .font_color = br_theme.colors.btn_txt_inactive,
      .z = 0,
      .text_ancor = br_text_renderer_ancor_left_up
    };
  }

}

void brui_begin(void) {
  BRUI_LOG("begin");
  brui_stack_el_t new_el = brui_stack_el();
  br_da_push(_stack, new_el);
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

void brui_push_y(float y) {
  TOP.cur.x += y;
  TOP.content.height += y;
}

void brui_pop(void) {
  if (TOP.limit.height <= 0.f) { --_stack.len; return; }
  if (TOP.content.height <= 0.f) { --_stack.len; return; }
  float width = minf(maxf(TOP.content.width, TOP.cur.x), TOP.limit.width);
  float height = minf(maxf(TOP.content.height, TOP.cur.y), TOP.limit.height);
  int z = TOP.start_z - 1;
  br_extent_t ex = BR_EXTENT(TOP.limit.x, TOP.limit.y, minf(TOP.limit.width, TOP.content.width), minf(TOP.limit.height, TOP.content.height));

  br_icons_draw(ex, BR_EXTENT(0,0,0,0), br_theme.colors.plot_menu_color, br_theme.colors.plot_menu_color, z - 1);
  if (TOP.hide_border == false) {
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

  --_stack.len;
  TOP.cur.y += height;
  TOP.content.width += width;
  TOP.content.height += height;
  TOP.cur.x = TOP.padding.x;
  TOP.cur.y += TOP.padding.y;
  BRUI_LOG("pop");
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
  br_text_renderer_t* tr = brtl_text_renderer();
  br_vec2_t loc = TOP.cur;
  RETURN_IF_OUT((float)TOP.font_size, BR_SIZE(0,0));
  
  br_size_t space_left = br_size_subv(TOP.limit.size, TOP.cur);
  br_strv_t fit = br_text_renderer_fit(tr, space_left, TOP.font_size, strv);
  float x = loc.x + TOP.limit.x;
  float y = loc.y + TOP.limit.y;
  if (TOP.text_ancor & br_text_renderer_ancor_y_mid) y += (TOP.limit.height - TOP.padding.w - TOP.padding.y) * 0.5f;
  if (TOP.text_ancor & br_text_renderer_ancor_x_mid) x += (TOP.limit.width - TOP.padding.z - TOP.padding.x) * 0.5f;
  br_extent_t ex = br_text_renderer_push2(tr, x, y, ZGL, TOP.font_size, TOP.font_color, fit, TOP.text_ancor);
  TOP.content.width = maxf(TOP.content.width, TOP.padding.x + TOP.padding.z + ex.width);
  TOP.cur.x = TOP.padding.x;
  TOP.cur.y += (float)TOP.font_size + TOP.padding.y;
  return ex.size;
}

void brui_new_lines(int n) {
  float off = (float)TOP.font_size * (float)n;
  TOP.cur.y += off;
}

void brui_text_align_set(br_text_renderer_ancor_t ancor) {
  TOP.text_ancor = ancor;
}

void brui_text_color_set(br_color_t color) {
  TOP.font_color = color;
}

float brui_top_width(void) {
  return TOP.limit.width - TOP.cur.x;
}

void brui_size_set(br_size_t size) {
  TOP.limit.size = BR_SIZE(minf(size.width, TOP.limit.width), minf(size.height, TOP.limit.height));
  TOP.content = TOP.limit.size;
}

void  brui_cur_set(br_vec2_t pos) {
  TOP.cur = pos;
}

int brui_text_size(void) {
  return TOP.font_size;
}

void brui_text_size_set(int size) {
  TOP.font_size = size;
}

void brui_ancor_set(brui_ancor_t ancor) {
  TOP.ancor = ancor;
}

void brui_z_set(int z) {
  TOP.z = z;
}

bool brui_button(br_strv_t text) {
  br_vec2_t tl = TOP.cur;
  br_vec2_t mouse = brtl_mouse_pos();
  br_extent_t button_limit = BR_EXTENT(tl.x + TOP.limit.x, tl.y + TOP.limit.y, TOP.limit.width - TOP.padding.x - TOP.padding.z, (float)TOP.font_size + TOP.padding.y + TOP.padding.w);
  RETURN_IF_OUT(button_limit.height - TOP.padding.y, false);

  bool hovers = br_col_vec2_extent(button_limit, mouse);

  br_icons_draw(button_limit, BR_EXTENT(0, 0, 0, 0), br_theme.colors.btn_inactive, br_theme.colors.btn_inactive, Z);
  brui_push();
    brui_size_set(button_limit.size);
    brui_text_align_set(br_text_renderer_ancor_mid_mid);
    brui_text_color_set(hovers ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive);
    brui_text(text);
  brui_pop();
  return hovers && brtl_mousel_pressed();
}

bool brui_checkbox(br_strv_t text, bool* checked) {
  float sz = (float)TOP.font_size * 0.6f;
  br_extent_t cb_extent = BR_EXTENT(TOP.cur.x + TOP.limit.x, TOP.cur.y + TOP.limit.y, sz, sz);

  RETURN_IF_OUT((float)TOP.font_size, false);

  br_extent_t icon = *checked ?  br_icons.cb_1.size_32 : br_icons.cb_0.size_32;
  bool hover = br_col_vec2_extent(cb_extent, brtl_mouse_pos());
  br_color_t bg = hover ? br_theme.colors.btn_hovered : br_theme.colors.btn_inactive;
  br_color_t fg = hover ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive;
  br_icons_draw(cb_extent, icon, bg, fg, Z);
  TOP.cur.x += sz + TOP.padding.x;
  brui_text(text);

  if (hover && brtl_mousel_pressed()) {
    *checked = !*checked;
    return true;
  }
  return false;
}

void brui_img(unsigned int texture_id) {
  br_shader_img_t* img = brtl_shaders()->img;
  img->uvs.image_uv = brgl_framebuffer_to_texture(texture_id);
  br_extent_t ex = TOP.limit;
  float gl_z = ZGL;
  br_sizei_t screen = brtl_viewport().size;
  br_shader_img_push_quad(img, (br_shader_img_el_t[4]) {
      { .pos = BR_VEC42(br_vec2_stog(ex.pos,           screen), BR_VEC2(0, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_extent_tr(ex), screen), BR_VEC2(1, 1)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_extent_br(ex), screen), BR_VEC2(1, 0)), .z = gl_z },
      { .pos = BR_VEC42(br_vec2_stog(br_extent_bl(ex), screen), BR_VEC2(0, 0)), .z = gl_z },
  });
  br_shader_img_draw(img);
  img->len = 0;
}

bool brui_button_icon(br_sizei_t size, br_extent_t icon) {
  RETURN_IF_OUT((float)size.height, false);

  br_extent_t ex = BR_EXTENT(TOP.cur.x + TOP.limit.x, TOP.cur.y + TOP.limit.y, (float)size.width, (float)size.height);
  //br_extent_t ex = BR_EXTENT2(br_vec2_add(TOP.cur, TOP.max.pos), BR_SIZEI_TOF(size));
  if (TOP.ancor & brui_ancor_right) ex.x += TOP.limit.width - TOP.padding.x - TOP.padding.z - (float)size.width;
  if (br_col_vec2_extent(ex, brtl_mouse_pos())) {
    br_color_t fg = br_theme.colors.btn_txt_hovered;
    br_color_t bg = br_theme.colors.btn_hovered;
    br_icons_draw(ex, icon, bg, fg, Z);
    return brtl_mousel_pressed();
  } else {
    br_color_t fg = br_theme.colors.btn_txt_inactive;
    br_color_t bg = br_theme.colors.btn_inactive;
    br_icons_draw(ex, icon, bg, fg, Z);
    return false;
  }
}

bool brui_sliderf(br_strv_t text, float* val) {
  br_extent_t ex = BR_EXTENT(0, 0, TOP.limit.width - TOP.padding.x - TOP.padding.z, (TOP.padding.y + TOP.padding.w + (float)TOP.font_size * 3));
  RETURN_IF_OUT(ex.height, false);
  brui_push();
    brui_size_set(ex.size);
    brui_text(text);
    brui_textf("%f", *val);

    br_vec2_t mouse = brtl_mouse_pos();
    bool is_down = brtl_mousel_down();

    const float lt = 8.f;
    const float ss = lt + 3.f;
    br_extent_t line_extent = BR_EXTENT(TOP.limit.x + TOP.cur.x + ss*0.5f, TOP.limit.y + TOP.cur.y + 1.5f, TOP.limit.width - TOP.padding.z - ss, lt);
    br_color_t lc = br_theme.colors.btn_inactive;
    br_extent_t slider_extent = BR_EXTENT(TOP.limit.x + TOP.cur.x + (TOP.limit.width - ss)*.5f, TOP.limit.y + TOP.cur.y, ss, ss);
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

    br_icons_draw(line_extent, BR_EXTENT(0,0,0,0), lc, lc, Z);
    br_icons_draw(slider_extent, BR_EXTENT(0,0,0,0), sc, sc, Z);
  brui_pop();
  return false;
}

bool brui_vsplit(int n, float height) {
  BRUI_LOG("vsplit %d - %f", n, height);
  RETURN_IF_OUT(height, false);
  brui_stack_el_t* top = &TOP;
  float width = (TOP.limit.width - TOP.padding.x - TOP.padding.z)  / (float)n;
  for (int i = 0; i < n; ++i) {
    br_extent_t ex = BR_EXTENT((float)(n - i - 1) * width + top->cur.x + top->limit.x, top->limit.y + top->cur.y, width, height);
    brui_stack_el_t new_el;
    memcpy(&new_el, top, sizeof(new_el));
    new_el.z += 5;
    new_el.limit = ex;
    new_el.cur = (br_vec2_t) { .x = top->padding.x, .y = top->padding.y };
    new_el.start_z = new_el.z;
    new_el.cur_resizable = 0;
    new_el.hide_border = true;
    br_da_push(_stack, new_el);
  }
  return true;
}

void brui_vsplit_pop(void) {
  --_stack.len;
  BRUI_LOG("vsplit pop");
}

void brui_scroll_bar(float bar_fract, float* bar_offset_fract) {
  float thick = 5.f;
  int z = Z;
  br_vec2_t mouse = brtl_mouse_pos();
  bool is_down = brtl_mousel_down();

  br_extent_t line_ex = BR_EXTENT(TOP.limit.x + TOP.limit.width - thick, TOP.limit.y, thick, TOP.limit.height);
  br_color_t lc = br_theme.colors.btn_inactive;
  float bar_height = TOP.limit.height * bar_fract;
  float bar_offset = (TOP.limit.height - bar_height) * *bar_offset_fract;
  br_extent_t bar_ex = BR_EXTENT(TOP.limit.x + TOP.limit.width - thick - 2.f, TOP.limit.y + bar_offset, thick + 2.f, bar_height);
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
      if (hovered->content_height > (float)hovered->cur_extent.height) {
        hovered->scroll_offset = br_float_clamp(hovered->scroll_offset - scroll * 10.f, 0, bruirs.arr[index].content_height - (float)hovered->cur_extent.height);
      } else {
        hovered->scroll_offset = 0;
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

void brui_resizable_push(int id) {
  brui_resizable_t* res = &bruirs.arr[id];
  BR_ASSERT(false == res->hidden);

  TOP.cur.x = 0;
  TOP.cur.y = 0;
  //float old_height = res->content_height;

  brui_cur_set(BR_VEC2I_TOF(res->cur_extent.pos));
  brui_push();
  brui_size_set(BR_SIZEI_TOF(res->cur_extent.size));
  res->content_height = 0;
  TOP.content = BR_SIZEI_TOF(res->cur_extent.size);
  brui_z_set(TOP.z + res->z * ((4*1024) >> (_stack.len)));
  TOP.cur_resizable = id;
  TOP.start_z = TOP.z;
  TOP.cur.y += -res->scroll_offset;
  TOP.is_active = id == _stack.active_resizable;
  
  // DEBUg
//  brui_textf("content_height: %f", old_height);
//  brui_textf("scroll_offset: %f", res->scroll_offset);
}

void brui_resizable_pop(void) {
  brui_resizable_t* res = &bruirs.arr[TOP.cur_resizable];
  float hidden_height = res->content_height - (float)res->cur_extent.height;
  if (hidden_height > 0.f && false == brtl_key_ctrl()) {
    res->scroll_offset_percent = res->scroll_offset / hidden_height;
    brui_scroll_bar(br_float_clamp((float)res->cur_extent.height / res->content_height, 0, 1), &res->scroll_offset_percent);
    res->scroll_offset = res->scroll_offset_percent * hidden_height;
  }
  brui_pop();
  TOP.cur.x += TOP.padding.x;
  TOP.cur.y += TOP.padding.y;
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
      float old_hidden_height = res.content_height - (float)old_ex.height;
      float hidden_height = res.content_height - (float)new_ex.height;
      if (hidden_height > 0.f) {
        float scroll = old_hidden_height < 2 ? 0 : res.scroll_offset / old_hidden_height;
        bruirs.arr[index].scroll_offset = scroll * hidden_height;
      }
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

