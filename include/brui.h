/*
 *  brui - Author: Branimir Ričko
 *    The MIT License (MIT)
 *    Copyright © Branimir Ričko [branc116]
 *
 *    Permission is hereby granted, free of charge, to any person  obtaining a copy of
 *    this software and associated  documentation files (the   “Software”), to deal in
 *    the Software without  restriction,  including without  limitation  the rights to
 *    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *    the Software,  and to permit persons to whom the Software is furnished to do so,
 *    subject to the following conditions:
 *
 *    The above  copyright notice and this  permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED   “AS IS”,   WITHOUT WARRANTY OF ANY KIND,   EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   IN NO EVENT SHALL THE AUTHORS OR
 *    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER
 *    IN  AN  ACTION  OF CONTRACT,   TORT OR OTHERWISE,   ARISING FROM,   OUT OF OR IN
 *    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Brui - Brui is a cross platform UI library.
 * It should work on Linux, Windows, Mac and Web.
 * Example:
 * ```main.c
#define BRUI_IMPLEMENTATION
#include <brui.h>

int main(void) {
  brui_window_t window = { 0 };
  while (false == window->pl.should_close) {
    while (brpl_event_frame_next != brui_event_next(&window).kind); // You can also handle some of the events 
    brui_frame_start(&window);
      if (brui_buttonf("Hello")) printf("Hello world\n");
    brui_frame_end(&window);
  }
}
 * ```
 * Compile as: cc main.c
 * Functions that are part of simple api can be found by following a tag *BRUI_API*
 * */
#if !defined(BR_INCLUDE_BRUI_H)
#define BR_INCLUDE_BRUI_H

#include <stdbool.h>

#if !defined(BR_EXPORT)
#  if defined(__EMSCRIPTEN__)
#    include <emscripten.h>
#    define BR_EXPORT EMSCRIPTEN_KEEPALIVE
#  elif defined(__GNUC__)
#    define BR_EXPORT __attribute__((visibility ("default")))
#  elif defined(_WIN32)
#    define BR_EXPORT __declspec(dllexport)
#  else
#    define BR_EXPORT
#  endif
#endif
#include "include/brplat_header.h"
#include "include/br_str_header.h"
#include "include/br_string_pool_header.h"
#include "src/br_text_renderer.h"
#include "src/br_math.h"
#include "src/br_shaders.h"
#include "src/br_anim.h"
#include "src/br_theme.h"

typedef enum {
  brui_drag_mode_none = 0,
  brui_drag_mode_left = 1,
  brui_drag_mode_right = 2,
  brui_drag_mode_top = 4,
  brui_drag_mode_bottom = 8,
  brui_drag_mode_move = 16,
  brui_drag_mode_left_top = brui_drag_mode_left | brui_drag_mode_top,
  brui_drag_mode_right_top = brui_drag_mode_right | brui_drag_mode_top,
  brui_drag_mode_left_bottom = brui_drag_mode_left | brui_drag_mode_bottom,
  brui_drag_mode_right_bottom = brui_drag_mode_right | brui_drag_mode_bottom,
} brui_drag_mode_t;

typedef enum {
  brui_ancor_none = 0,
  brui_ancor_left = 1,
  brui_ancor_right = 2,
  brui_ancor_top = 4,
  brui_ancor_bottom = 8,
  brui_ancor_left_top = brui_ancor_left | brui_ancor_top,
  brui_ancor_right_top = brui_ancor_right | brui_ancor_top,
  brui_ancor_left_bottom = brui_ancor_left | brui_ancor_bottom,
  brui_ancor_right_bottom = brui_ancor_right | brui_ancor_bottom,
  brui_ancor_all = brui_ancor_left | brui_ancor_right | brui_ancor_top | brui_ancor_bottom,

  brui_ancor_hidden = 16
} brui_ancor_t;

typedef enum brui_action_kind_t {
  brui_action_none = 0,
  brui_action_sliding,
  brui_action_typing
} brui_action_kind_t;

typedef struct brui_pop_t {
  br_bb_t bb;
  bool clicked;
  bool hovered;
} brui_pop_t;

typedef struct brui_action_text_t {
  brsp_id_t id;
  int cursor_pos;
  int offset_ahandle;
  bool changed;
} brui_action_text_t;

typedef struct brui_action_t {
  brui_action_kind_t kind;
  struct {
    void* value;
    br_vec2_t drag_ancor_point;
  } slider;
  brui_action_text_t text;
} brui_action_t;

typedef struct {
  brui_ancor_t ancor;
  int z, max_z;
  int parent;
  int tag;
  brsp_id_t title_id;
  int title_height_ah;
  int scroll_offset_percent_ah;
  int cur_extent_ah;
  float full_height;
  br_extent_t ancor_none_extent;
  bool is_hoverd;
  bool scroll_bar;
  bool title_enabled;
  bool was_draw_last_frame;
} brui_resizable_t;

typedef struct brui_resizable_temp_push_t {
  brui_resizable_t* res;
  int resizable_handle;
  bool just_created;
} brui_resizable_temp_push_t;

typedef struct bruirs_t {
  brui_resizable_t* arr;
  int len, cap;
  int* free_arr;
  int free_len;
  int free_next;

  int menu_extent_handle;
  brui_drag_mode_t drag_mode;
  int drag_index;
  int active_resizable;
  br_vec2_t drag_point;
  br_extent_t drag_old_ex;
} bruirs_t;

typedef struct br_theme_t br_theme_t;
typedef struct br_shaders_t br_shaders_t;
typedef struct br_anims_t br_anims_t;

typedef struct bruir_children_t {
  int* arr;
  int len, cap;
} bruir_children_t;

typedef struct brui_split_t {
  enum {
    brui_split_absolute,
    brui_split_relative,
  } kind;
  union {
    float relative;
    float absolute;
  };
} brui_split_t;


typedef struct brui_window_t {
  brpl_window_t pl; // Platform window
  br_shaders_t shaders;
  bruirs_t resizables;
  brsp_t sp;
  br_anims_t anims;
  br_text_renderer_t* text;

  struct {
    brpl_touch_point_t* arr;
    int* free_arr;

    int len, cap;
    int free_len;
    int free_next;

    double last_free_time;
    double last_touch_time;
  } touch_points;

  struct {
    br_vec2_t old_pos;
    br_vec2_t pos;
    br_vec2_t delta;
    bool dragging_left;
    bool dragging_right;
    bool click;
    bool active;
  } mouse;

  struct {
    bool ctrl_down;
    bool shift_down;
    bool alt_down;
    bool down[255];
  } key;

  struct {
    double old;
    double now;
    double frame;
  } time;

  struct {
    br_vec2_t padding;
    int font_size;
    float border_thick;
    float animation_speed;
  } def;

  br_theme_t theme;

  bool inited;
} brui_window_t;

// --------------------------------------------- BRUI_API ----------------------------------------------
BR_EXPORT bool brui_window_init(brui_window_t* ui_window);
BR_EXPORT brpl_event_t brui_event_next(brui_window_t* ui_window);
BR_EXPORT void brui_frame_start(brui_window_t* ui_window);
BR_EXPORT void brui_frame_end(brui_window_t* ui_window);

BR_EXPORT bool brui_buttonf(const char* fmt, ...);

#endif

#if defined(BRUI_IMPLEMENTATION)
#  if !defined(BR_INCLUDE_UNITY_BRUI_C)
#    define BR_INCLUDE_UNITY_BRUI_C
#    include "tools/unity/brui.c"
#  endif
#endif
