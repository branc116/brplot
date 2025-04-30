#pragma once
#include "src/br_math.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"

extern BR_THREAD_LOCAL int brui__n__;
#define brui_textf(...) (brui__n__ = sprintf(brui__scrach, __VA_ARGS__), brui_text(BR_STRV(brui__scrach, (uint32_t)brui__n__)))

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

#define brui_resizable_anim_fields(X) \
  X(float, scroll_offset_percent) \
  X(br_extent_t, cur_extent) \
  X(float, hidden_factor)

#define brui_resizable_fields(X) \
  X(brui_ancor_t, ancor) \
  X(int, z) \
  X(int, parent) \
  brui_resizable_anim_fields(X) \
  X(float, full_height) \
  X(bool, is_hoverd) \
  X(bool, scroll_bar)

typedef union {
  struct {
#define X(type, name) type name;
    brui_resizable_fields(X)
#undef X
  };
  struct {
    struct {
#define X(type, name) type name;
      brui_resizable_fields(X)
#undef X
    } current;
    struct {
#define X(type, name) type name;
      brui_resizable_anim_fields(X)
#undef X
    } target;
    int next_free;
    bool alloced;
  };
} brui_resizable_t;

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

#define BRUI_SPLITR(VALUE) ((brui_split_t) { .kind = brui_split_relative, .relative = (VALUE) })
#define BRUI_SPLITA(VALUE) ((brui_split_t) { .kind = brui_split_absolute, .absolute = (VALUE) })

extern BR_THREAD_LOCAL char brui__scrach[2048];

void brui_begin(void);
void brui_end(void);

br_size_t brui_text(br_strv_t strv);
void brui_new_lines(int n);
bool brui_button(br_strv_t text);
bool brui_checkbox(br_strv_t text, bool* checked);
void brui_img(unsigned int texture_id);
void brui_icon(float size, br_bb_t icon, br_color_t forground, br_color_t background);
bool brui_button_icon(br_sizei_t size, br_extent_t icon);
bool brui_sliderf(br_strv_t text, float* val);
bool brui_sliderf2(br_strv_t text, float* value);
bool brui_sliderf3(br_strv_t text, float* value, int percision);
bool brui_slideri(br_strv_t text, int* value);
void brui_vsplit(int n);
void brui_vsplitvp(int n, ...);
void brui_vsplit_pop(void);
void brui_vsplit_end(void);
void brui_background(br_bb_t bb, br_color_t color);
void brui_border(br_bb_t bb);
bool brui_collapsable(br_strv_t name, bool* expanded);
void brui_collapsable_end(void);

void brui_push(void);
void brui_pop(void);
void brui_push_simple(void);
void brui_pop_simple(void);
void brui_push_y(float y);

int   brui_text_size(void);
void  brui_text_size_set(int size);
void  brui_text_align_set(br_text_renderer_ancor_t ancor);
void  brui_text_color_set(br_color_t color);
void  brui_ancor_set(brui_ancor_t ancor);
void  brui_z_set(int z);
void  brui_maxy_set(float value);
float brui_min_y(void);
void  brui_height_set(float value);
float brui_padding_x(void);
void  brui_padding_y_set(float value);
float brui_y(void);
bool  brui_active(void);

void              brui_resizable_init(void);
void              brui_resizable_deinit(void);
int               brui_resizable_new(br_extent_t init_extent, int parent);
int               brui_resizable_new2(br_extent_t init_extent, int parent, brui_resizable_t template);
void              brui_resizable_delete(int handle);
void              brui_resizable_update(void);
void              bruir_resizable_refresh(int index);
brui_resizable_t* brui_resizable_get(int id);
void              brui_resizable_push(int id);
void              brui_resizable_pop(void);
int               brui_resizable_active(void);
void              brui_resizable_show(int resizable_handle, bool show);
bool              brui_resizable_is_hidden(int resizable_handle);

void brui_resizable_save(FILE* file);
void brui_resizable_load(FILE* file);
