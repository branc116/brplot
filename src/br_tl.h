#pragma once
// Thread local stuff - this is implemented somewhere in the desktop/platfrom.c
#include "src/br_math.h"

#include <stdbool.h>

typedef struct br_plotter_t br_plotter_t;

#ifdef __cplusplus
extern "C" {
#endif

br_vec2_t brtl_get_scroll(void);
br_vec2_t brtl_mouse_get_pos(void);
br_vec2_t brtl_mouse_get_delta(void);
float brtl_get_time(void);
bool brtl_mouse_is_down_l(void);
bool brtl_mouse_is_down_r(void);
bool brtl_mouse_is_pressed_l(void);
bool brtl_mouse_is_pressed_r(void);
bool brtl_key_is_down(int key);
bool brtl_key_is_pressed(int key);
bool brtl_key_ctrl(void);
bool brtl_key_alt(void);
bool brtl_key_shift(void);
br_plotter_t* brtl_get_plotter(void);

#ifdef __cplusplus
}
#endif
