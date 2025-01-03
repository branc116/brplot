#pragma once
// Thread local stuff - this is implemented somewhere in the desktop/platfrom.c
#include "src/br_math.h"

#include <stdbool.h>

typedef struct br_plotter_t br_plotter_t;
typedef struct br_shaders_t br_shaders_t;
typedef struct br_text_renderer_t br_text_renderer_t;

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
float brtl_time(void);
int brtl_fps(void);
bool brtl_key_down(int key);
bool brtl_key_pressed(int key);
bool brtl_key_ctrl(void);
bool brtl_key_alt(void);
bool brtl_key_shift(void);

br_sizei_t brtl_window_size(void);
void brtl_window_size_set(int width, int height);
void brtl_window_close(void);

br_extenti_t brtl_viewport(void);
void brtl_viewport_set(br_extenti_t ex);

br_plotter_t* brtl_plotter(void);
br_shaders_t* brtl_shaders(void);
br_text_renderer_t* brtl_text_renderer(void);

#ifdef __cplusplus
}
#endif
