#include "src/br_tl.h"
#include "src/br_ui.h"

br_vec2_t brtl_mouse_scroll(void) { return (br_vec2_t) { 0 }; }
br_vec2_t brtl_mouse_pos(void) { return (br_vec2_t) { 0 }; }
br_vec2_t brtl_mouse_delta(void) { return (br_vec2_t) { 0 }; }
bool brtl_mousel_down(void) { return false; }
bool brtl_mouser_down(void) { return false; }
bool brtl_mousel_pressed(void) { return false; }
bool brtl_mouser_pressed(void) { return false; }
double brtl_time(void) { return 0.0; }
float brtl_frame_time(void) { return 0.f; }
int brtl_fps(void) { return 60; }
bool brtl_key_down(int key) { (void)key; return false; }
bool brtl_key_pressed(int key) { (void)key; return false; }
br_pressed_chars_t brtl_pressed_chars(void) { return (br_pressed_chars_t){0}; }
void brtl_pressed_clear(void) {}
bool brtl_key_ctrl(void) { return false; }
bool brtl_key_alt(void) { return false; }
bool brtl_key_shift(void) { return false; }
br_theme_t* brtl_theme(void) { return NULL; }

br_sizei_t brtl_window_size(void) { return (br_sizei_t) { 0 }; }
void brtl_window_size_set(int width, int height) { (void)width; (void)height; }

br_extenti_t brtl_viewport(void) { return (br_extenti_t) { 0 }; }
void brtl_viewport_set(br_extenti_t ex) { (void)ex; }

br_strv_t brtl_clipboard(void) { return (br_strv_t){ 0 }; }

br_plotter_t* brtl_plotter(void) { return NULL; }
br_shaders_t* brtl_shaders(void) { return NULL; }
br_datas_t* brtl_datas(void) { return NULL; }
br_text_renderer_t* brtl_text_renderer(void) { return NULL; }

brsp_t sp = { 0 };
brsp_t* brtl_brsp(void) { return &sp; }

bruirs_t* brtl_bruirs(void) { return NULL; }

bool* brtl_debug(void) { return NULL; }

float* brtl_cull_min(void) { return NULL; }
float* brtl_min_sampling(void) { return NULL; }
