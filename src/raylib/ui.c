#include "src/br_plot.h"
#include "src/br_help.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"

#include "raylib.h"

#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

//int ui_draw_text_box(bool focused, float x, float y, float font_size, br_vec2_t* size_out, char* str) {
//  return 0;
//}

static int ui_draw_button_va(br_text_renderer_t* tr, bool* is_pressed, float x, float y, int font_size, br_vec2_t* size_out, const char* str, va_list va) {
  int c = 0;
  char* scrach = br_scrach_get(128);
  vsprintf(scrach, str, va);
  float pad = 5.f;
  br_text_renderer_extent_t e = br_text_renderer_push(tr, x + pad, y + pad, font_size, BR_WHITE, scrach);
  br_extent_t box = BR_EXTENT(e.x0 - pad, e.y0 - pad, e.x1 - e.x0 + 2 * pad, e.y1 - e.y0 + 2 * pad);
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

static RL_THREAD_LOCAL br_vec2_t stack_pos;
static RL_THREAD_LOCAL bool stack_is_inited = false;
static RL_THREAD_LOCAL float* stack_scroll_position;
static RL_THREAD_LOCAL br_vec2_t stack_button_size;
static RL_THREAD_LOCAL float stack_offset;
static RL_THREAD_LOCAL int stack_font_size;
static RL_THREAD_LOCAL int stack_count;
static RL_THREAD_LOCAL br_vec2_t stack_maxsize;
static RL_THREAD_LOCAL bool stack_size_set;
static RL_THREAD_LOCAL br_vec2_t stack_size;

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
    stack_offset += s.y;
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
  BoundingBox bb = {.min = {stack_pos.x, stack_pos.y, 0.f}, .max = { stack_pos.x + stack_maxsize.x, stack_pos.y + stack_maxsize.y + stack_offset - stack_button_size.y, 0.f} };
  if (stack_size_set) {
    bb.max.x = bb.min.x + stack_size.x;
    bb.max.y = bb.min.y + stack_size.y;
  }
  if (stack_scroll_position != NULL) {
    if (CheckCollisionPointRec(GetMousePosition(), (Rectangle) {.x = bb.min.x, .y = bb.min.y, .width = bb.max.x - bb.min.x, .height = bb.max.y - bb.min.y })) {
      float mouse_scroll = brtl_get_scroll().y;
      *stack_scroll_position += mouse_scroll;
    }
    *stack_scroll_position = minf((float)stack_count - 3.f , *stack_scroll_position);
    *stack_scroll_position = maxf(0.f, *stack_scroll_position);
  }
  //DrawBoundingBox(bb, RAYWHITE);
  return BR_VEC2(stack_pos.x, stack_pos.y + stack_offset);
}

