#pragma once
#include "src/br_str.h"
#include "src/br_math.h"

typedef struct br_text_renderer_t br_text_renderer_t;
#if !defined(br_shader_font_t)
typedef struct br_shader_font_t br_shader_font_t;
typedef struct br_shader_fontbg_t br_shader_fontbg_t;
#endif

typedef enum {
  br_text_renderer_ancor_x_left  = 1 << 1,
  br_text_renderer_ancor_x_mid   = 1 << 2,
  br_text_renderer_ancor_x_right = 1 << 3,
  br_text_renderer_ancor_y_up    = 1 << 4,
  br_text_renderer_ancor_y_mid   = 1 << 5,
  br_text_renderer_ancor_y_down  = 1 << 6,

  br_text_renderer_ancor_left_up    = br_text_renderer_ancor_x_left  | br_text_renderer_ancor_y_up,
  br_text_renderer_ancor_left_mid   = br_text_renderer_ancor_x_left  | br_text_renderer_ancor_y_mid,
  br_text_renderer_ancor_left_down  = br_text_renderer_ancor_x_left  | br_text_renderer_ancor_y_down,
  br_text_renderer_ancor_mid_up     = br_text_renderer_ancor_x_mid   | br_text_renderer_ancor_y_up,
  br_text_renderer_ancor_mid_mid    = br_text_renderer_ancor_x_mid   | br_text_renderer_ancor_y_mid,
  br_text_renderer_ancor_mid_down   = br_text_renderer_ancor_x_mid   | br_text_renderer_ancor_y_down,
  br_text_renderer_ancor_right_up   = br_text_renderer_ancor_x_right | br_text_renderer_ancor_y_up,
  br_text_renderer_ancor_right_mid  = br_text_renderer_ancor_x_right | br_text_renderer_ancor_y_mid,
  br_text_renderer_ancor_right_down = br_text_renderer_ancor_x_right | br_text_renderer_ancor_y_down,
} br_text_renderer_ancor_t;


#if defined(__cplusplus)
extern "C" {
#endif

br_text_renderer_t* br_text_renderer_malloc(int bitmap_width, int bitmap_height, unsigned char const* font_data, br_shader_font_t** shader);
bool br_text_renderer_load_font(br_text_renderer_t* r, br_strv_t path);
void br_text_renderer_free(br_text_renderer_t* r);
void br_text_renderer_dump(br_text_renderer_t* r);
uint32_t br_text_renderer_texture_id(br_text_renderer_t* r);

br_strv_t br_text_renderer_fit(br_text_renderer_t* r, br_size_t size, int font_size, br_strv_t text);
br_size_t br_text_renderer_measure(br_text_renderer_t* r, int font_size, br_strv_t str);

br_extent_t br_text_renderer_push0(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, const char* text);
br_extent_t br_text_renderer_push_strv0(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, br_strv_t text);
br_extent_t br_text_renderer_push(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, const char* text, br_bb_t limit);
br_extent_t br_text_renderer_push_strv(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, br_strv_t text, br_bb_t limit);
br_extent_t br_text_renderer_push2(br_text_renderer_t* r, br_vec3_t pos, int font_size, br_color_t color_fg, br_color_t color_bg, br_strv_t str, br_bb_t limit, br_text_renderer_ancor_t ancor);

void br_text_renderer_viewport_set(br_text_renderer_t* r, br_sizei_t viewport);

#if defined(__cplusplus)
}
#endif

