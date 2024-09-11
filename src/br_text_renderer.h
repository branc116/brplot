#pragma once

typedef struct br_text_renderer_t br_text_renderer_t;
#if !defined(br_shader_font_t)
typedef struct br_shader_font_t br_shader_font_t;
#endif

#define BR_COLOR_PUN(x) *(br_color_t*)((void*)(&(x)))

typedef struct {
  unsigned char r, g, b, a;
} br_color_t;

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

typedef struct {
  float x0, y0; // LEFT-TOP
  float x1, y1; // RIGHT-BOT
} br_text_renderer_extent_t;


#if defined(__cplusplus)
extern "C" {
#endif

br_text_renderer_t* br_text_renderer_malloc(int bitmap_width, int bitmap_height, unsigned char const* font_data, br_shader_font_t** shader);
void br_text_renderer_free(br_text_renderer_t* r);
void br_text_renderer_dump(br_text_renderer_t* r);
br_text_renderer_extent_t br_text_renderer_push(br_text_renderer_t* r, float x, float y, int font_size, br_color_t color, const char* text);
br_text_renderer_extent_t br_text_renderer_push2(br_text_renderer_t* r, float x, float y, int font_size, br_color_t color, const char* text, br_text_renderer_ancor_t ancor);

#if defined(__cplusplus)
}
#endif
