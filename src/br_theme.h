#pragma once
#include "src/br_math.h"

typedef struct br_theme_t {
  struct {
    br_color_t btn_inactive;
    br_color_t btn_hovered;
    br_color_t btn_active;
    br_color_t btn_txt_inactive;
    br_color_t btn_txt_hovered;
    br_color_t btn_txt_active;

    br_color_t grid_nums;
    br_color_t grid_nums_bg;
    br_color_t grid_lines;

    br_color_t plot_bg;
    br_color_t plot_menu_color;

    br_color_t ui_edge_inactive;
    br_color_t ui_edge_bg_inactive;
    br_color_t ui_edge_active;
    br_color_t ui_edge_bg_active;

    br_color_t bg;
    float highlite_factor;
  } colors;

  struct {
    br_vec2_t padding;
    int font_size;
    float border_thick;
    float animation_speed;
    float min_sampling;
    float cull_min;
    bool debug;
  } ui;
} br_theme_t;

void br_theme_dark(void);
void br_theme_light(void);

void br_theme_reset_ui(void);

