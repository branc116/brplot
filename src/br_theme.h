#pragma once
#include "src/br_math.h"
#include "src/br_pp.h"

typedef struct {
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
  } colors;
} br_theme_t;

extern BR_THREAD_LOCAL br_theme_t br_theme;

void br_theme_dark(void);
void br_theme_light(void);

