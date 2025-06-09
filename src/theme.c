#include "src/br_theme.h"
#include "src/br_tl.h"

void br_theme_dark(void) {
  br_color_t bg = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  br_color_t txt = BR_COLOR(0xe0, 0xe0, 0xe0, 0xff);
  
  BR_THEME.colors.btn_inactive = br_color_lighter(bg, 0.9f);
  BR_THEME.colors.btn_hovered = br_color_greener(br_color_lighter(BR_THEME.colors.btn_inactive, 2.0f), 0.3f);
  BR_THEME.colors.btn_active = br_color_lighter(BR_THEME.colors.btn_hovered, 1.0f);
  BR_THEME.colors.btn_txt_inactive = txt;
  BR_THEME.colors.btn_txt_hovered = br_color_darker(txt, 0.2f);
  BR_THEME.colors.btn_txt_active = txt;

  BR_THEME.colors.plot_bg = br_color_lighter(bg, 0.2f);
  BR_THEME.colors.plot_menu_color = br_color_lighter(bg, .3f);

  BR_THEME.colors.grid_nums = txt;
  BR_THEME.colors.grid_nums_bg = BR_THEME.colors.plot_bg;
  BR_THEME.colors.grid_lines = br_color_greener(br_color_darker(txt, 0.7f), 0.02f);

  BR_THEME.colors.ui_edge_inactive = br_color_lighter(BR_THEME.colors.btn_inactive, 0.8f);
  BR_THEME.colors.ui_edge_active = BR_THEME.colors.btn_active;

  BR_THEME.colors.ui_edge_bg_inactive = BR_THEME.colors.plot_menu_color;
  BR_THEME.colors.ui_edge_bg_active = BR_THEME.colors.plot_menu_color;

  BR_THEME.colors.bg = bg;

  BR_THEME.colors.highlite_factor = 1.05f;
}

void br_theme_light(void) {
  br_color_t bg = BR_COLOR(0xc0, 0xd0, 0xc0, 0xff);
  br_color_t txt = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  
  BR_THEME.colors.btn_inactive = br_color_darker(bg, 0.3f);
  BR_THEME.colors.btn_hovered = br_color_greener(br_color_darker(BR_THEME.colors.btn_inactive, .4f), 0.3f);
  BR_THEME.colors.btn_active = br_color_darker(BR_THEME.colors.btn_hovered, .4f);
  BR_THEME.colors.btn_txt_inactive = txt;
  BR_THEME.colors.btn_txt_hovered = br_color_lighter(txt, 0.2f);
  BR_THEME.colors.btn_txt_active = txt;

  BR_THEME.colors.plot_bg = br_color_darker(bg, .05f);
  BR_THEME.colors.plot_menu_color = br_color_darker(bg, .1f);

  BR_THEME.colors.grid_nums = txt;
  BR_THEME.colors.grid_nums_bg = BR_THEME.colors.plot_bg;
  BR_THEME.colors.grid_lines = br_color_greener(br_color_darker(bg, 0.4f), 0.02f);

  BR_THEME.colors.bg = bg;
  BR_THEME.colors.bg.a = 0x00;

  BR_THEME.colors.highlite_factor = 0.95f;
}

void br_theme_reset_ui(void) {
  BR_THEME.ui.min_sampling = 0.001f;
  BR_THEME.ui.cull_min = 2.f;
  BR_THEME.ui.padding = BR_VEC2(4, 2);
  BR_THEME.ui.font_size = 20;
  BR_THEME.ui.border_thick = 1;
  BR_THEME.ui.animation_speed = 10.f;
}

