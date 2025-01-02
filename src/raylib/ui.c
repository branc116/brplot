#include "external/X11/X.h"
#include "src/br_icons.h"
#include "src/br_math.h"
#include "src/br_plot.h"
#include "src/br_help.h"
#include "src/br_pp.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"
#include "src/br_theme.h"
#include "src/br_da.h"
#include "src/br_tl.h"

#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


//int ui_draw_text_box(bool focused, float x, float y, float font_size, br_vec2_t* size_out, char* str) {
//  return 0;
//}

static int ui_draw_button_va(br_text_renderer_t* tr, bool* is_pressed, float x, float y, int font_size, br_vec2_t* size_out, const char* str, va_list va) {
  int c = 0;
  char* scrach = br_scrach_get(128);
  vsprintf(scrach, str, va);
  float pad = 5.f;
  br_extent_t box  = br_text_renderer_push(tr, x + pad, y + pad, 0.9f, font_size, br_theme.colors.btn_txt_inactive, scrach);
  bool is_in = br_col_vec2_extent(box, brtl_mouse_get_pos());
  if (is_in) {
    bool is_p = brtl_mouse_is_pressed_l();
    c = is_p ? 2 : 1;
    if (is_p && is_pressed) {
      *is_pressed = !*is_pressed;
    }
  }
  if (is_pressed && *is_pressed) {
    //DrawRectangleRec(box, BLUE);
  } else if (is_in) {
    //DrawRectangleRec(box, RED);
  }
  br_scrach_free();
  if (size_out) *size_out = (br_vec2_t) { .x = box.width, .y = box.height };
  return c;
}

int ui_draw_button(br_text_renderer_t* tr, bool* is_pressed, float x, float y, int font_size, const char* str, ...) {
  va_list args;
  va_start(args, str);
  int ret = ui_draw_button_va(tr, is_pressed, x, y, font_size, NULL, str, args);
  va_end(args);
  return ret;
}

static BR_THREAD_LOCAL br_vec2_t stack_pos;
static BR_THREAD_LOCAL bool      stack_is_inited = false;
static BR_THREAD_LOCAL float*    stack_scroll_position;
static BR_THREAD_LOCAL br_vec2_t stack_button_size;
static BR_THREAD_LOCAL float     stack_offset;
static BR_THREAD_LOCAL int       stack_font_size;
static BR_THREAD_LOCAL int       stack_count;
static BR_THREAD_LOCAL br_vec2_t stack_maxsize;
static BR_THREAD_LOCAL bool      stack_size_set;
static BR_THREAD_LOCAL br_vec2_t stack_size;

void ui_stack_buttons_init(br_vec2_t pos, float* scroll_position, int font_size) {
  assert(!stack_is_inited);
  stack_pos = pos;
  stack_is_inited = true;
  stack_scroll_position = scroll_position;
  stack_button_size.y = (float)font_size + 5.f;
  stack_offset = -stack_button_size.y * (scroll_position == NULL ? 0.f : *scroll_position);
  stack_font_size = font_size;
  stack_count = 0;
  stack_maxsize = (br_vec2_t){0};
  stack_size_set = false;
  stack_size = (br_vec2_t){0};
}

int ui_stack_buttons_add(br_text_renderer_t* tr, bool* is_pressed, char const* str, ...) {
  assert(stack_is_inited);
  br_vec2_t s = stack_button_size;
  ++stack_count;
  if (stack_size_set) {
    if (stack_offset + s.y > stack_size.y) return 0;
  }
  if (stack_offset >= 0) {
    va_list args;
    va_start(args, str);
    int ret = ui_draw_button_va(tr, is_pressed, stack_pos.x, stack_pos.y + stack_offset, stack_font_size, &s, str, args);
    va_end(args);
    if (s.x > stack_maxsize.x) stack_maxsize.x = s.x;
    if (s.y > stack_maxsize.y) stack_maxsize.y = s.y;
    stack_offset += fmaxf(s.y, (float)stack_font_size + 2);
    return ret;
  }
  stack_offset += s.y;
  return 0;
}

void ui_stack_set_size(br_vec2_t v) {
  stack_size = v; 
  stack_size_set = true;
}

br_vec2_t ui_stack_buttons_end(void) {
  assert(stack_is_inited);
  stack_is_inited = false;
  if (stack_count == 0) return stack_pos;
  br_bb_t bb = BR_BB(stack_pos.x, stack_pos.y, stack_pos.x + stack_maxsize.x, stack_pos.y + stack_maxsize.y + stack_offset - stack_button_size.y);
  if (stack_size_set) {
    bb.max.x = bb.min.x + stack_size.x;
    bb.max.y = bb.min.y + stack_size.y;
  }
  if (stack_scroll_position != NULL) {
    if (br_col_vec2_extent(BR_BB_TOEX(bb), brtl_mouse_get_pos())) {
      float mouse_scroll = brtl_get_scroll().y;
      *stack_scroll_position += mouse_scroll;
    }
    *stack_scroll_position = minf((float)stack_count - 3.f , *stack_scroll_position);
    *stack_scroll_position = maxf(0.f, *stack_scroll_position);
  }
  //DrawBoundingBox(bb, RAYWHITE);
  return BR_VEC2(stack_pos.x, stack_pos.y + stack_offset);
}

typedef struct {
  br_vec2_t cur;
  br_extent_t max;
  br_size_t content;

  br_vec4_t padding;
  int font_size;
  br_color_t font_color;
  float z;
  br_text_renderer_ancor_t text_ancor;
} brui_stack_el_t;

typedef struct {
  brui_stack_el_t* arr;
  size_t len, cap;
} brui_stack_t;

static BR_THREAD_LOCAL brui_stack_t _stack;
#define TOP (_stack.arr[_stack.len - 1])

brui_stack_el_t brui_stack_el(br_extent_t max) {
  if (_stack.len > 0) {
    brui_stack_el_t new_el;
    memcpy(&new_el, &TOP, sizeof(new_el));
    new_el.z += 1;
    new_el.content = (br_size_t){ .width = TOP.padding.arr[0] + TOP.padding.arr[2], .height = TOP.padding.arr[1] + TOP.padding.arr[3] };
    new_el.max = max;
    new_el.cur = (br_vec2_t) { .x = TOP.padding.arr[0], .y = TOP.padding.arr[1] };
    return new_el;
  } else {
    return (brui_stack_el_t) {
      .cur = BR_VEC2(4, 4),
      .max = max,
      .content = BR_SIZE(0, 0),
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
  brui_stack_el_t* parent = &_stack.arr[_stack.len - 1];
  parent->content = BR_SIZE(TOP.content.width + parent->content.width, TOP.content.height + parent->content.height);
  parent->cur.y += TOP.content.height;
  parent->cur.x = parent->padding.x;

  if (br_col_vec2_extent(TOP.max, brtl_mouse_get_pos())) {
    br_extent_t top_padding = BR_EXTENT(TOP.max.x, TOP.max.y, TOP.max.width, TOP.padding.y * 0.5f);
    br_extent_t left_padding = BR_EXTENT(TOP.max.x, TOP.max.y, TOP.padding.x * 0.5f, TOP.max.height);
    br_extent_t bot_padding = BR_EXTENT(TOP.max.x, TOP.max.y + TOP.max.height - TOP.padding.y + TOP.padding.w * 0.5f, TOP.max.width, TOP.padding.w * 0.5f);
    br_extent_t right_padding = BR_EXTENT(TOP.max.x + TOP.max.width - TOP.padding.x + TOP.padding.z * 0.5f, TOP.max.y, TOP.padding.z * 0.5f, TOP.max.height);

    const float bc = 8/32.f;
    br_icons_draw(brtl_shaders()->icon, top_padding, br_icons_tc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, TOP.z - 0.5f);
    br_icons_draw(brtl_shaders()->icon, left_padding, br_icons_lc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, TOP.z - 0.5f);
    br_icons_draw(brtl_shaders()->icon, bot_padding, br_icons_bc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, TOP.z - 0.5f);
    br_icons_draw(brtl_shaders()->icon, right_padding, br_icons_rc(br_icons.cb_0.size_32, bc, 1.0f), br_theme.colors.plot_menu_color, br_theme.colors.btn_hovered, TOP.z - 0.5f);
  }
  br_icons_draw(brtl_shaders()->icon, TOP.max, BR_EXTENT(0,0,0,0), br_theme.colors.plot_menu_color, br_theme.colors.plot_menu_color, TOP.z - 1.9f);
  --_stack.len;
}

br_size_t brui_text(br_strv_t strv) {
  br_text_renderer_t* tr = brtl_text_renderer();
  br_vec2_t loc = BR_VEC2((float)TOP.cur.x, (float)TOP.cur.y);
  
  br_size_t space_left = br_size_subv(TOP.max.size, br_vec2_sub(TOP.cur, TOP.max.pos));
  br_strv_t fit = br_text_renderer_fit(tr, space_left, TOP.font_size, strv);
  float x = loc.x + (float)TOP.max.x;
  float y = loc.y + (float)TOP.max.y;
  if (TOP.text_ancor & br_text_renderer_ancor_y_mid) y += ((float)TOP.max.height - TOP.padding.w - TOP.padding.y) * 0.5f;
  if (TOP.text_ancor & br_text_renderer_ancor_x_mid) x += ((float)TOP.max.width - TOP.padding.z - TOP.padding.x) * 0.5f;
  br_extent_t ex = br_text_renderer_push2(tr, x, y, -TOP.z/255.f, TOP.font_size, TOP.font_color, fit, TOP.text_ancor);
  TOP.content.width = maxf(TOP.content.width, ex.width);
  TOP.content.height += ex.height;
  TOP.cur.x = TOP.padding.x;
  TOP.cur.y += ex.height + TOP.padding.y;
  return ex.size;
}

void brui_text_align(br_text_renderer_ancor_t ancor) {
  TOP.text_ancor = ancor;
}
void brui_text_color(br_color_t color) {
  TOP.font_color = color;
}

void brui_z_set(float z) {
  TOP.z = z;
}

bool brui_button(br_strv_t text) {
  br_vec2_t tl = TOP.cur;
  br_vec2_t mouse = brtl_mouse_get_pos();
  br_extent_t extent = BR_EXTENT(tl.x + TOP.max.x, tl.y + TOP.max.y, TOP.max.width - TOP.padding.x - TOP.padding.z, (float)TOP.font_size + TOP.padding.y + TOP.padding.w);
  bool hovers = br_col_vec2_extent(extent, mouse);

  br_icons_draw(brtl_shaders()->icon, extent, BR_EXTENT(0, 0, 0, 0), br_theme.colors.btn_inactive, br_theme.colors.btn_inactive, TOP.z);
  brui_push(BR_EXTENT(0, 0, extent.width, extent.height));
    brui_text_align(br_text_renderer_ancor_mid_mid);
    brui_text_color(hovers ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive);
    brui_text(text);
  brui_pop();
  TOP.cur.y += extent.height + TOP.padding.y;
  return hovers && brtl_mouse_is_pressed_l();
}

bool brui_checkbox(br_strv_t text, bool* checked) {
  br_extent_t icon = *checked ?  br_icons.cb_1.size_32 : br_icons.cb_0.size_32;
  float sz = (float)TOP.font_size * 0.6f;
  br_extent_t cb_extent = BR_EXTENT(TOP.cur.x + TOP.max.x, TOP.cur.y + TOP.max.y, sz, sz);
  bool hover = br_col_vec2_extent(cb_extent, brtl_mouse_get_pos());
  br_color_t bg = hover ? br_theme.colors.btn_hovered : br_theme.colors.btn_inactive;
  br_color_t fg = hover ? br_theme.colors.btn_txt_hovered : br_theme.colors.btn_txt_inactive;
  br_icons_draw(brtl_shaders()->icon, cb_extent, icon, bg, fg, TOP.z);
  TOP.cur.x += sz + TOP.padding.x;
  brui_text(text);

  if (hover && brtl_mouse_is_pressed_l()) {
    *checked = !*checked;
    return true;
  }
  return false;
}
