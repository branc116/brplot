#include "src/br_plot.h"
#include "src/br_help.h"

#define RAYMATH_STATIC_INLINE
#include "raylib.h"
#include "raymath.h"

#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

//int ui_draw_text_box(bool focused, float x, float y, float font_size, Vector2* size_out, char* str) {
//  return 0;
//}

static int ui_draw_button_va(bool* is_pressed, float x, float y, float font_size, Vector2* size_out, const char* str, va_list va) {
  int c = 0;
  vsprintf(context.buff, str, va);
  float pad = 5.f;
  Vector2 size = Vector2AddValue(help_measure_text(context.buff, font_size), 2.f * pad);
  if (size_out) *size_out = size;
  Rectangle box = { x, y, size.x, size.y };
  bool is_in = CheckCollisionPointRec(GetMousePosition(), box);
  if (is_in) {
    bool is_p = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    c = is_p ? 2 : 1;
    if (is_p && is_pressed) {
      *is_pressed = !*is_pressed;
    }
  }
  if (is_pressed && *is_pressed) {
    DrawRectangleRec(box, BLUE);
  } else if (is_in) {
    DrawRectangleRec(box, RED);
  }
  help_draw_text(context.buff, (Vector2){x + pad, y + pad}, font_size, WHITE);
  return c;
}

int ui_draw_button(bool* is_pressed, float x, float y, float font_size, const char* str, ...) {
  va_list args;
  va_start(args, str);
  int ret = ui_draw_button_va(is_pressed, x, y, font_size, NULL, str, args);
  va_end(args);
  return ret;
}

static RL_THREAD_LOCAL Vector2 stack_pos;
static RL_THREAD_LOCAL bool stack_is_inited = false;
static RL_THREAD_LOCAL float* stack_scroll_position;
static RL_THREAD_LOCAL Vector2 stack_button_size;
static RL_THREAD_LOCAL float stack_offset;
static RL_THREAD_LOCAL float stack_font_size;
static RL_THREAD_LOCAL int stack_count;
static RL_THREAD_LOCAL Vector2 stack_maxsize;
static RL_THREAD_LOCAL bool stack_size_set;
static RL_THREAD_LOCAL Vector2 stack_size;

void ui_stack_buttons_init(Vector2 pos, float* scroll_position, float font_size) {
  assert(!stack_is_inited);
  stack_pos = pos;
  stack_is_inited = true;
  stack_scroll_position = scroll_position;
  stack_button_size = help_measure_text("T", font_size);
  stack_button_size.y += 5.f;
  stack_offset = -stack_button_size.y * (scroll_position == NULL ? 0.f : *scroll_position);
  stack_font_size = font_size;
  stack_count = 0;
  stack_maxsize = (Vector2){0};
  stack_size_set = false;
  stack_size = (Vector2){0};
}

int ui_stack_buttons_add(bool* is_pressed, char const* str, ...) {
  assert(stack_is_inited);
  Vector2 s = stack_button_size;
  ++stack_count;
  if (stack_size_set) {
    if (stack_offset + s.y > stack_size.y) return 0;
  }
  if (stack_offset >= 0) {
    va_list args;
    va_start(args, str);
    int ret = ui_draw_button_va(is_pressed, stack_pos.x, stack_pos.y + stack_offset, stack_font_size, &s, str, args);
    va_end(args);
    if (s.x > stack_maxsize.x) stack_maxsize.x = s.x;
    if (s.y > stack_maxsize.y) stack_maxsize.y = s.y;
    stack_offset += s.y;
    return ret;
  }
  stack_offset += s.y;
  return 0;
}

void ui_stack_set_size(Vector2 v) {
  stack_size = v; 
  stack_size_set = true;
}

Vector2 ui_stack_buttons_end(void) {
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
      Vector2 mouse_scroll = GetMouseWheelMoveV();
      *stack_scroll_position += mouse_scroll.y;
    }
    *stack_scroll_position = minf((float)stack_count - 3.f , *stack_scroll_position);
    *stack_scroll_position = maxf(0.f, *stack_scroll_position);
  }
  DrawBoundingBox(bb, RAYWHITE);
  return (Vector2) { stack_pos.x, stack_pos.y + stack_offset };
}

