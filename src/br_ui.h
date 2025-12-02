#pragma once
#include "src/br_math.h"
#include "src/br_str.h"
#include "src/br_text_renderer.h"
#include "src/br_string_pool.h"

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

typedef enum brui_action_kind_t {
  brui_action_none = 0,
  brui_action_sliding,
  brui_action_typing
} brui_action_kind_t;

typedef struct brui_state_t {
  br_bb_t bb;
  bool clicked;
  bool hovered;
} brui_state_t;

typedef struct brui_action_text_t {
  brsp_id_t id;
  int cursor_pos;
  int offset_ahandle;
  bool changed;
} brui_action_text_t;

typedef struct brui_action_t {
  brui_action_kind_t kind;
  union {
    struct {
      void* value;
      br_vec2_t drag_ancor_point;
    } slider;
    brui_action_text_t text;
  } args;
} brui_action_t;

typedef struct {
  br_vec2_t cur;
  br_bb_t limit;
  int start_z, z;
  br_vec2_t psum;
  float content_height;

  br_vec2_t padding;
  br_color_t background_color;

  int font_size;
  br_color_t font_color;
  br_text_renderer_ancor_t text_ancor;

  int cur_resizable;

  float vsplit_max_height;

  bool is_active;
  bool hide_border;
  bool hide_bg;
} brui_stack_el_t;

#define brui_resizable_tag_ancor_helper 10

#define brui_resizable_anim_fields(X) \
  X(br_extent_t, cur_extent) \
  X(float, hidden_factor) \

#define brui_resizable_fields(X) \
  X(brui_ancor_t, ancor) \
  X(int, z) \
  X(int, max_z) \
  X(int, parent) \
  X(int, tag) \
  X(brsp_id_t, title_id) \
  X(int, title_height_ah) \
  X(int, scroll_offset_percent_ah) \
  brui_resizable_anim_fields(X) \
  X(float, full_height) \
  X(br_extent_t, ancor_none_extent) \
  X(bool, is_hoverd) \
  X(bool, scroll_bar) \
  X(bool, title_enabled) \
  X(bool, was_draw_last_frame) \

typedef union {
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
  };
  struct {
#define X(type, name) type name;
    brui_resizable_fields(X)
#undef X
  };
} brui_resizable_t;

typedef struct brui_resizable_temp_state_t {
  int resizable_handle;
  bool was_drawn;
  bool is_deleted;
} brui_resizable_temp_state_t;

typedef struct brui_resizable_temp_t {
  size_t key;
  brui_resizable_temp_state_t value;
} brui_resizable_temp_t;

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
typedef struct {
  brui_stack_el_t* arr;
  size_t len, cap;

  bruirs_t* rs;
  brsp_t* sp;
  br_theme_t* theme;
  br_text_renderer_t* tr;
  br_shaders_t* shaders;
  br_anims_t* anims;

  float frame_time;
  float snap_cooldown;

  brui_action_t action;
  
  br_vec2_t mouse_pos;
  bool mouse_clicked;

  bool ctrl_down;
  bool select_next;
  bool log;
} brui_stack_t;

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

#define BRUI_SPLITR(VALUE) ((brui_split_t) { .kind = brui_split_relative, .relative = (VALUE) })
#define BRUI_SPLITA(VALUE) ((brui_split_t) { .kind = brui_split_absolute, .absolute = (VALUE) })

extern BR_THREAD_LOCAL char brui__scrach[2048];

void brui_construct(br_theme_t* theme, bruirs_t* rs, brsp_t* sp, br_text_renderer_t* tr, br_shaders_t* shaders, br_anims_t* anims);

void brui_begin(void);
void brui_end(void);

/* Call this to clear all temp memory */
void brui_finish(void);

br_size_t brui_text(br_strv_t strv);
void brui_text_at(br_strv_t strv, br_vec2_t at);
bool brui_text_input(brsp_id_t str_id);
void brui_new_lines(int n);
bool brui_button(br_strv_t text);
bool brui_checkbox(br_strv_t text, bool* checked);
void brui_img_hack(unsigned int texture_id);
void brui_img(unsigned int texture_id);
void brui_icon(float size, br_bb_t icon, br_color_t forground, br_color_t background);
bool brui_button_icon(br_sizei_t size, br_extent_t icon);
bool brui_triangle(br_vec2_t a, br_vec2_t b, br_vec2_t c, br_bb_t limit, br_color_t color, int z);
bool brui_rectangle(br_bb_t bb, br_bb_t limit, br_color_t color, int z);
bool brui_sliderf(br_strv_t text, float* val);
bool brui_sliderf2(br_strv_t text, float* value);
bool brui_sliderf3(br_strv_t text, float* value, int percision);
bool brui_slideri(br_strv_t text, int* value);
void brui_vsplit(int n);
void brui_vsplitvp(int n, ...);
void brui_vsplit_pop(void);
void brui_vsplit_end(void);
void brui_background(br_bb_t bb, br_color_t color);
void brui_border1(br_bb_t bb);
void brui_border2(br_bb_t bb, bool active);
bool brui_collapsable(br_strv_t name, bool* expanded);
void brui_collapsable_end(void);

void brui_push(void);
brui_state_t brui_pop(void);
void brui_push_simple(void);
void brui_pop_simple(void);
void brui_push_y(float y);

void brui_select_next(void);

int   brui_text_size(void);
void  brui_text_size_set(int size);
void  brui_text_align_set(br_text_renderer_ancor_t ancor);
void  brui_text_color_set(br_color_t color);
void  brui_ancor_set(brui_ancor_t ancor);
void  brui_z_set(int z);
void  brui_maxy_set(float value);
void  brui_scroll_move(float value);
float brui_min_y(void);
float brui_max_y(void);
void  brui_height_set(float value);
float brui_padding_x(void);
float brui_padding_y(void);
void  brui_padding_y_set(float value);
float brui_y(void);
float brui_width(void);
float brui_local_y(void);
bool  brui_active(void);
void  brui_debug(void);

void brui_mouse_clicked(bool is_mouse_clicked);
void brui_mouse_pos(br_vec2_t mouse_pos);
void brui_ctrl_down(bool is_down);
void brui_log(bool should_log);
void brui_frame_time(float frame_time);

brui_action_t* brui_action(void);
void           brui_action_stop(void);
brui_stack_t*  brui_stack(void);

void              brui_resizable_init(bruirs_t* rs, br_extent_t viewport);
void              brui_resizable_deinit(void);
int               brui_resizable_new(bruirs_t* rs, br_extent_t init_extent, int parent);
int               brui_resizable_new2(bruirs_t* rs, br_extent_t init_extent, int parent, brui_resizable_t template);
void              brui_resizable_delete(int handle);
void              brui_resizable_update(bruirs_t* rs, br_extent_t viewport);

void              brui_resizable_mouse_move(bruirs_t* resizables, br_vec2_t mouse_pos);
void              brui_resizable_mouse_pressl(bruirs_t* resizables, br_vec2_t mouse_pos, bool ctrl_down);
void              brui_resizable_mouse_pressr(bruirs_t* resizables, br_vec2_t mouse_pos);
void              brui_resizable_mouse_releasel(bruirs_t* resizables, br_vec2_t mouse_pos);
void              brui_resizable_mouse_releaser(bruirs_t* resizables, br_vec2_t mouse_pos);
void              brui_resizable_mouse_scroll(bruirs_t* resizables, br_vec2_t delta);
void              brui_resizable_page(bruirs_t* resizables, br_vec2_t delta);
void              brui_resizable_scroll_percent_set(bruirs_t* resizables, float percent);
br_vec2_t         brui_resizable_local(bruirs_t* resizables, int id, br_vec2_t global_pos);

void              bruir_resizable_refresh(bruirs_t* rs, int index);
brui_resizable_t* brui_resizable_push(int id);
void              brui_resizable_pop(void);
void              brui_resizable_show(int resizable_handle, bool show);
void              brui_resizable_maximize(int resizable_handle, bool maximize);
bool              brui_resizable_is_hidden(int resizable_handle);
br_vec2_t         brui_resizable_to_global(int resizable_handle, br_vec2_t pos);
void              brui_resizable_move_on_top(bruirs_t* rs, int r_handle);
int               brui_resizable_sibling_max_z(int id);
bruir_children_t  brui_resizable_children_temp(int resizable_handle);
int               brui_resizable_children_count(int resizable_handle);
int               brui_resizable_children_top_z(bruirs_t* rs, int resizable_handle);
br_extent_t       brui_resizable_cur_extent(int resizable_handle);

brui_resizable_temp_push_t brui_resizable_temp_push(br_strv_t id);
bool              brui_resizable_temp_pop(void);
void              brui_resizable_temp_delete(br_strv_t id);
void              brui_resizable_temp_delete_all(void);

void brui_resizable_save(FILE* file);
void brui_resizable_load(FILE* file);
