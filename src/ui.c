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
  br_extent_t max;

  br_vec4_t padding;
  int font_size;
  br_color_t font_color;
  int start_z, z;
  br_text_renderer_ancor_t text_ancor;
  brui_ancor_t ancor;
} brui_stack_el_t;

typedef struct {
  brui_stack_el_t* arr;
  size_t len, cap;
} brui_stack_t;

static BR_THREAD_LOCAL brui_stack_t _stack;
#define TOP (_stack.arr[_stack.len - 1])
#define Z (TOP.z++)
#define ZGL BR_Z_TO_GL(TOP.z++)

brui_stack_el_t brui_stack_el(br_extent_t max) {
  if (_stack.len > 0) {
    brui_stack_el_t new_el;
    memcpy(&new_el, &TOP, sizeof(new_el));
    new_el.z += 5;
    new_el.max = max;
    new_el.cur = (br_vec2_t) { .x = TOP.padding.x, .y = TOP.padding.y };
    new_el.start_z = new_el.z;
    return new_el;
  } else {
    return (brui_stack_el_t) {
      .cur = BR_VEC2(4, 4),
      .max = max,
      .padding = BR_VEC4(4, 4, 4, 4),
      .font_size = 26,
      .font_color = br_theme.colors.btn_txt_inactive,
      .z = 0,
      .text_ancor = br_text_renderer_ancor_left_up
    };
  }

}

void brui_begin(void) {
  brui_stack_el_t new_el = brui_stack_el(BR_EXTENTI_TOF(brtl_viewport()));
  br_da_push(_stack, new_el);
}

void brui_end(void) {
  _stack.len = 0;
}

void brui_push(br_extent_t max) {
  br_extent_t new_max = BR_EXTENT(TOP.max.x + TOP.cur.x + max.x, TOP.max.y + TOP.cur.y + max.y, max.width, max.height);
  brui_stack_el_t new_el = brui_stack_el(new_max);
  br_da_push(_stack, new_el);
}

void brui_pop(void) {
  int z = TOP.start_z - 1;
  if (br_col_vec2_extent(TOP.max, brtl_mouse_pos())) {
    br_extent_t top_padding = BR_EXTENT(TOP.max.x, TOP.max.y, TOP.max.width, TOP.padding.y * 0.5f);
    br_extent_t left_padding = BR_EXTENT(TOP.max.x, TOP.max.y, TOP.padding.x * 0.5f, TOP.max.height);
    br_extent_t bot_padding = BR_EXTENT(TOP.max.x, TOP.max.y + TOP.max.height - TOP.padding.y + TOP.padding.w * 0.5f, TOP.max.width, TOP.padding.w * 0.5f);
    br_extent_t right_padding = BR_EXTENT(TOP.max.x + TOP.max.width - TOP.padding.x + TOP.padding.z * 0.5f, TOP.max.y, TOP.padding.z * 0.5f, TOP.max.height);

    const float bc = 8/32.f;
    br_icons_draw(brtl_shaders()->icon, top_padding, br_icons_tc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, Z);
    br_icons_draw(brtl_shaders()->icon, left_padding, br_icons_lc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, Z);
    br_icons_draw(brtl_shaders()->icon, bot_padding, br_icons_bc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, Z);
    br_icons_draw(brtl_shaders()->icon, right_padding, br_icons_rc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, Z);
  }
  br_icons_draw(brtl_shaders()->icon, TOP.max, BR_EXTENT(0,0,0,0), br_theme.colors.plot_menu_color, br_theme.colors.plot_menu_color, z - 1);
  --_stack.len;
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
  if (loc.y >= TOP.max.height) return BR_SIZE(0, 0);
  
  br_size_t space_left = br_size_subv(TOP.max.size, TOP.cur);
  br_strv_t fit = br_text_renderer_fit(tr, space_left, TOP.font_size, strv);
  float x = loc.x + TOP.max.x;
  float y = loc.y + TOP.max.y;
  if (TOP.text_ancor & br_text_renderer_ancor_y_mid) y += (TOP.max.height - TOP.padding.w - TOP.padding.y) * 0.5f;
  if (TOP.text_ancor & br_text_renderer_ancor_x_mid) x += (TOP.max.width - TOP.padding.z - TOP.padding.x) * 0.5f;
  br_extent_t ex = br_text_renderer_push2(tr, x, y, ZGL, TOP.font_size, TOP.font_color, fit, TOP.text_ancor);
  TOP.cur.x = TOP.padding.x;
  TOP.cur.y += ex.height + TOP.padding.y;
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
  br_extent_t extent = BR_EXTENT(tl.x + TOP.max.x, tl.y + TOP.max.y, TOP.max.width - TOP.padding.x - TOP.padding.z, (float)TOP.font_size + TOP.padding.y + TOP.padding.w);
  bool hovers = br_col_vec2_extent(extent, mouse);

  br_icons_draw(brtl_shaders()->icon, extent, BR_EXTENT(0, 0, 0, 0), br_theme.colors.btn_inactive, br_theme.colors.btn_inactive, Z);
  brui_push(BR_EXTENT(0, 0, extent.width, extent.height));
    brui_text_align_set(br_text_renderer_ancor_mid_mid);
    brui_text_color_set(hovers ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive);
    brui_text(text);
  brui_pop();
  TOP.cur.y += extent.height + TOP.padding.y;
  return hovers && brtl_mousel_pressed();
}

bool brui_checkbox(br_strv_t text, bool* checked) {
  float sz = (float)TOP.font_size * 0.6f;
  br_extent_t cb_extent = BR_EXTENT(TOP.cur.x + TOP.max.x, TOP.cur.y + TOP.max.y, sz, sz);
  if (cb_extent.height + TOP.cur.y > TOP.max.height) return false;

  br_extent_t icon = *checked ?  br_icons.cb_1.size_32 : br_icons.cb_0.size_32;
  bool hover = br_col_vec2_extent(cb_extent, brtl_mouse_pos());
  br_color_t bg = hover ? br_theme.colors.btn_hovered : br_theme.colors.btn_inactive;
  br_color_t fg = hover ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive;
  br_icons_draw(brtl_shaders()->icon, cb_extent, icon, bg, fg, Z);
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
  br_extent_t ex = TOP.max;
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
  if (TOP.cur.y + (float)size.height > TOP.max.height) return false;

  br_extent_t ex = BR_EXTENT(TOP.cur.x + TOP.max.x, TOP.cur.y + TOP.max.y, (float)size.width, (float)size.height);
  //br_extent_t ex = BR_EXTENT2(br_vec2_add(TOP.cur, TOP.max.pos), BR_SIZEI_TOF(size));
  if (TOP.ancor & brui_ancor_right) ex.x += TOP.max.width - TOP.padding.x - TOP.padding.z - (float)size.width;
  if (br_col_vec2_extent(ex, brtl_mouse_pos())) {
    br_color_t fg = br_theme.colors.btn_txt_hovered;
    br_color_t bg = br_theme.colors.btn_hovered;
    br_icons_draw(brtl_shaders()->icon, ex, icon, bg, fg, Z);
    return brtl_mousel_pressed();
  } else {
    br_color_t fg = br_theme.colors.btn_txt_inactive;
    br_color_t bg = br_theme.colors.btn_inactive;
    br_icons_draw(brtl_shaders()->icon, ex, icon, bg, fg, Z);
    return false;
  }
}


// ---------------------------Resizables--------------------------

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

static void bruir_update_ancors(int index, float dx, float dy, float dw, float dh);
static int bruir_find_at(int index, br_vec2_t loc, br_vec2_t* out_local_pos);
static void bruir_update_extent(int index, br_extenti_t new_ex);

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
  static int id = 0;

  brui_resizable_t new = {
    .cur_extent = init_extent,
    .z = ++id + bruirs.arr[parent].z + 1,
    .parent = parent
  };
  int new_id = bruirs.len;
  br_da_push_t(int, bruirs, new);

  br_da_push_t(int, bruir_childrens.arr[parent], new_id);

  bruir_children_t children = { 0 };
  br_da_push_t(int, bruir_childrens, children);
  return new_id;
}

void brui_resizable_update(void) {
  bruir_update_extent(0, brtl_viewport());

  br_vec2_t mouse_pos = brtl_mouse_pos();
  if (bruirs.drag_mode == brui_drag_mode_none) {
    if (brtl_mousel_down() == true) {
      br_vec2_t local_pos = { 0 };
      int index = bruir_find_at(0, mouse_pos, &local_pos);
      float slack = 25;
      brui_drag_mode_t new_mode = brui_drag_mode_none;
      if (index != 0) {
        br_extenti_t ex = bruirs.arr[index].cur_extent;
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
      bruir_update_extent(bruirs.drag_index, new_extent);
    } else {
      bruirs.drag_index = 0;
      bruirs.drag_mode = brui_drag_mode_none;
      bruirs.drag_point = BR_VEC2(0, 0);
    }
  }
}

void brui_resizable_push(int id) {
  brui_resizable_t res = bruirs.arr[id];
  BR_ASSERT(false == res.hidden);

  br_vec4_t padding = TOP.padding;
  TOP.cur.x -= padding.x;
  TOP.cur.y -= padding.y;

  brui_push(BR_EXTENTI_TOF(res.cur_extent));
  brui_z_set(TOP.z + res.z * ((4*1024) >> (4 * _stack.len)));

  TOP.start_z = TOP.z;
}

void brui_resizable_pop(void) {
  brui_pop();
  TOP.cur.x += TOP.padding.x;
  TOP.cur.y += TOP.padding.y;
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

static void bruir_update_extent(int index, br_extenti_t new_ex) {
  brui_resizable_t res = bruirs.arr[index];
  br_extenti_t old_ex = res.cur_extent;
  brui_resizable_t parent = bruirs.arr[res.parent];

  if (br_extenti_eq(new_ex, old_ex) == false) {
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
  }
}

