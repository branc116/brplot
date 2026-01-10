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

  float shadow_spred;
  float px_round;
  float shadow_intesity;
} br_theme_t;

void br_theme_dark(br_theme_t* theme);
void br_theme_light(br_theme_t* theme);

void br_theme_reset_ui(br_theme_t* theme);

