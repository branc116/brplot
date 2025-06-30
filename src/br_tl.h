#pragma once
// Thread local stuff - this is implemented somewhere in the desktop/platfrom.c
#include "src/br_math.h"
#include "src/br_str.h"

#include <stdbool.h>
#define BR_THEME (*brtl_theme())

typedef struct br_plotter_t br_plotter_t;
typedef struct br_shaders_t br_shaders_t;
typedef struct br_datas_t br_datas_t;
typedef struct br_text_renderer_t br_text_renderer_t;
typedef struct br_theme_t br_theme_t;
typedef struct bruirs_t bruirs_t;
typedef struct bruir_children_t bruir_children_t;
typedef struct brsp_t brsp_t;
typedef struct {
  uint32_t* arr;
  size_t len, cap;
} br_pressed_chars_t;

#ifdef __cplusplus
extern "C" {
#endif

br_vec2_t brtl_mouse_scroll(void);
br_vec2_t brtl_mouse_pos(void);
br_vec2_t brtl_mouse_delta(void);
bool brtl_mousel_down(void);
bool brtl_mouser_down(void);
bool brtl_mousel_pressed(void);
bool brtl_mouser_pressed(void);
double brtl_time(void);
float brtl_frame_time(void);
int brtl_fps(void);
bool brtl_key_down(int key);
bool brtl_key_pressed(int key);
br_pressed_chars_t brtl_pressed_chars(void);
bool brtl_key_ctrl(void);
bool brtl_key_alt(void);
bool brtl_key_shift(void);
br_theme_t* brtl_theme(void);

br_sizei_t brtl_window_size(void);
void brtl_window_size_set(int width, int height);

br_extenti_t brtl_viewport(void);
void brtl_viewport_set(br_extenti_t ex);

br_strv_t brtl_clipboard(void);

br_plotter_t* brtl_plotter(void);
br_shaders_t* brtl_shaders(void);
br_datas_t* brtl_datas(void);
br_text_renderer_t* brtl_text_renderer(void);

brsp_t* brtl_brsp(void);

bruirs_t* brtl_bruirs(void);
bruir_children_t brtl_bruirs_childern(int handle);

bool* brtl_debug(void);

float* brtl_cull_min(void);
float* brtl_min_sampling(void);
#ifdef __cplusplus
}
#endif
