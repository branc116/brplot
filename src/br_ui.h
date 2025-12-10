#pragma once
#include "include/brui.h"

#define brui_resizable_tag_ancor_helper 10

#define BRUI_SPLITR(VALUE) ((brui_split_t) { .kind = brui_split_relative, .relative = (VALUE) })
#define BRUI_SPLITA(VALUE) ((brui_split_t) { .kind = brui_split_absolute, .absolute = (VALUE) })

void brui_begin(void);
void brui_end(void);

/* Call this to clear all temp memory */
void brui_finish(void);

br_size_t brui_text(br_strv_t strv);
br_size_t brui_textf(const char* str, ...);
void brui_text_at(br_strv_t strv, br_vec2_t at);
bool brui_text_input(brsp_id_t str_id);
void brui_new_lines(int n);
bool brui_button(br_strv_t text);
bool brui_checkbox(br_strv_t text, bool* checked);
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
brui_pop_t brui_pop(void);
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

void brui_log(bool should_log);

brui_action_t* brui_action(void);
void           brui_action_stop(void);

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
