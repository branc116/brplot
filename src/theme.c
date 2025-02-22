#include "src/br_theme.h"

BR_THREAD_LOCAL br_theme_t br_theme;

void br_theme_dark(void) {
  br_color_t bg = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  br_color_t txt = BR_COLOR(0xe0, 0xe0, 0xe0, 0xff);
  
  br_theme.colors.btn_inactive = br_color_lighter(bg, 0.9f);
  br_theme.colors.btn_hovered = br_color_greener(br_color_lighter(br_theme.colors.btn_inactive, 2.0f), 0.3f);
  br_theme.colors.btn_active = br_color_lighter(br_theme.colors.btn_hovered, 1.0f);
  br_theme.colors.btn_txt_inactive = txt;
  br_theme.colors.btn_txt_hovered = br_color_darker(txt, 0.2f);
  br_theme.colors.btn_txt_active = txt;

  br_theme.colors.plot_bg = br_color_lighter(bg, 0.2f);
  br_theme.colors.plot_menu_color = br_color_lighter(bg, .3f);

  br_theme.colors.grid_nums = txt;
  br_theme.colors.grid_nums_bg = br_theme.colors.plot_bg;
  br_theme.colors.grid_lines = br_color_greener(br_color_darker(txt, 0.7f), 0.02f);

  br_theme.colors.ui_edge_inactive = br_color_lighter(br_theme.colors.btn_inactive, 0.8f);
  br_theme.colors.ui_edge_active = br_theme.colors.btn_active;

  br_theme.colors.ui_edge_bg_inactive = br_theme.colors.plot_menu_color;
  br_theme.colors.ui_edge_bg_active = br_theme.colors.plot_menu_color;

  br_theme.colors.bg = bg;
  br_theme.colors.bg.a = 0x00;

  br_theme.colors.highlite_factor = 1.05f;
}

void br_theme_light(void) {
  br_color_t bg = BR_COLOR(0xc0, 0xd0, 0xc0, 0xff);
  br_color_t txt = BR_COLOR(0x0f, 0x0f, 0x0f, 0xff);
  
  br_theme.colors.btn_inactive = br_color_darker(bg, 0.3f);
  br_theme.colors.btn_hovered = br_color_greener(br_color_darker(br_theme.colors.btn_inactive, .4f), 0.3f);
  br_theme.colors.btn_active = br_color_darker(br_theme.colors.btn_hovered, .4f);
  br_theme.colors.btn_txt_inactive = txt;
  br_theme.colors.btn_txt_hovered = br_color_lighter(txt, 0.2f);
  br_theme.colors.btn_txt_active = txt;

  br_theme.colors.plot_bg = br_color_darker(bg, .05f);
  br_theme.colors.plot_menu_color = br_color_darker(bg, .1f);

  br_theme.colors.grid_nums = txt;
  br_theme.colors.grid_nums_bg = br_theme.colors.plot_bg;
  br_theme.colors.grid_lines = br_color_greener(br_color_darker(bg, 0.4f), 0.02f);

  br_theme.colors.bg = bg;
  br_theme.colors.bg.a = 0x00;

  br_theme.colors.highlite_factor = 0.95f;
}

void br_theme_reset_ui(void) {
  br_theme.ui.padding = BR_VEC2(4, 4);
  br_theme.ui.font_size = 26;
  br_theme.ui.border_thick = 2;
}

