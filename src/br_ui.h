#pragma once
#include "src/br_math.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"

#define brui_textf(...) do { \
  int __n__ = sprintf(_scrach, __VA_ARGS__); \
  brui_text(BR_STRV(_scrach, (uint32_t)__n__)); \
} while (0)

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
} brui_ancor_t;

typedef struct {
  br_extenti_t cur_extent;
  brui_ancor_t ancor;
  int z;
  int parent;
  float scroll_offset;
  float scroll_offset_percent;
  float content_height;

  bool is_hoverd;
  bool scroll_bar;
  bool hidden;
} brui_resizable_t;

extern BR_THREAD_LOCAL char _scrach[2048];

void brui_begin(void);
void brui_end(void);

br_size_t brui_text(br_strv_t strv);
void brui_new_lines(int n);
bool brui_button(br_strv_t text);
bool brui_checkbox(br_strv_t text, bool* checked);
void brui_img(unsigned int texture_id);
bool brui_button_icon(br_sizei_t size, br_extent_t icon);
bool brui_sliderf(br_strv_t text, float* val);
bool brui_vsplit(int n, float height);
void brui_vsplit_pop(void);

void brui_push(br_extent_t max);
void brui_pop(void);

float brui_top_width(void);
int   brui_text_size(void);
void  brui_text_size_set(int size);
void  brui_text_align_set(br_text_renderer_ancor_t ancor);
void  brui_ancor_set(brui_ancor_t ancor);
void  brui_z_set(int z);

void brui_resizable_init(void);
int brui_resizable_new(br_extenti_t init_extent, int parent);
int brui_resizable_new2(br_extenti_t init_extent, int parent, brui_resizable_t template);
void brui_resizable_update(void);
void bruir_resizable_refresh(int index);
brui_resizable_t* brui_resizable_get(int id);
void brui_resizable_push(int id);
void brui_resizable_pop(void);
int  brui_resizable_active(void);

void brui_resizable_save(FILE* file);
void brui_resizable_load(FILE* file);
