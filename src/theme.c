#include "src/br_theme.h"

void br_theme_dark(br_theme_t* t) {
  br_color_t bg = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  br_color_t txt = BR_COLOR(0xe0, 0xe0, 0xe0, 0xff);
  
  t->colors.btn_inactive = br_color_lighter(bg, 0.9f);
  t->colors.btn_hovered = br_color_greener(br_color_lighter(t->colors.btn_inactive, 2.0f), 0.3f);
  t->colors.btn_active = br_color_lighter(t->colors.btn_hovered, 1.0f);
  t->colors.btn_txt_inactive = txt;
  t->colors.btn_txt_hovered = br_color_darker(txt, 0.2f);
  t->colors.btn_txt_active = txt;

  t->colors.plot_bg = br_color_lighter(bg, 0.2f);
  t->colors.plot_menu_color = br_color_lighter(bg, .3f);

  t->colors.grid_nums = txt;
  t->colors.grid_nums_bg = t->colors.plot_bg;
  t->colors.grid_lines = br_color_greener(br_color_darker(txt, 0.7f), 0.02f);

  t->colors.ui_edge_inactive = br_color_lighter(t->colors.btn_inactive, 0.8f);
  t->colors.ui_edge_active = t->colors.btn_active;

  t->colors.ui_edge_bg_inactive = t->colors.plot_menu_color;
  t->colors.ui_edge_bg_active =   t->colors.plot_menu_color;

  t->colors.bg = bg;

  t->colors.highlite_factor = 1.5f;
}

void br_theme_light(br_theme_t* t) {
  br_color_t bg = BR_COLOR(0xc0, 0xd0, 0xc0, 0xff);
  br_color_t txt = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  
  t->colors.btn_inactive = br_color_darker(bg, 0.3f);
  t->colors.btn_hovered = br_color_greener(br_color_darker(t->colors.btn_inactive, .4f), 0.3f);
  t->colors.btn_active = br_color_darker(t->colors.btn_hovered, .4f);
  t->colors.btn_txt_inactive = txt;
  t->colors.btn_txt_hovered = br_color_lighter(txt, 0.2f);
  t->colors.btn_txt_active = txt;

  t->colors.plot_bg = br_color_darker(bg, .05f);
  t->colors.plot_menu_color = br_color_darker(bg, .1f);

  t->colors.grid_nums = txt;
  t->colors.grid_nums_bg = t->colors.plot_bg;
  t->colors.grid_lines = br_color_greener(br_color_darker(bg, 0.4f), 0.02f);

  t->colors.bg = bg;
  t->colors.bg.a = 0x00;

  t->colors.highlite_factor = -0.21f;
}

void br_theme_reset_ui(br_theme_t* t) {
  t->ui.min_sampling = 0.001f;
  t->ui.cull_min = 2.f;
  t->ui.padding = BR_VEC2(4, 2);
  t->ui.font_size = 20;
  t->ui.border_thick = 1;
  t->ui.animation_speed = 10.f;
  t->ui.default_grid_line_thickenss = 1.5f;
}

