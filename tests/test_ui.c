#define BRUI_IMPLEMENTATION
#include <brui.h>

brui_window_t win = { 0 };
int font_size = 20;
float something = 1.f;
bool settings = false;

int main(void) {
  brui_window_init(&win);
  while (false == win.pl.should_close) {
    while (brpl_event_frame_next != brui_event_next(&win).kind); // You can also handle some of the events
    brui_frame_start(&win);
      brui_text_size_set(font_size);
      brui_checkbox(BR_STRL("Settings"), &settings);
      br_shaders_draw_all(win.shaders);
      if (brui_collapsable(BR_STRL("Settings"), &settings)) {
        brui_slideri(BR_STRL("Font Size"), &font_size);
        if (brui_buttonf("FONT")) ++font_size;
        if (brui_buttonf("font")) --font_size;
        if (brui_buttonf("Light")) br_theme_light(&win.theme);
        if (brui_buttonf("Dark"))  br_theme_dark(&win.theme);
        brui_collapsable_end();
      }
      brui_push();
        brui_text_justify_set(br_dir_right_down);
        brui_text_ancor_set(br_dir_right_down);
        if (brui_buttonf("Hello")) printf("Hello world\n");
        brui_textf("Hello");
      brui_pop();
      brui_resizable_temp_push(BR_STRL("Hello"));
        if (brui_buttonf("Hello")) printf("Hello world\n");
        brui_slideri(BR_STRL("Font Size"), &font_size);
        brui_sliderf(BR_STRL("Something"), &something);
      brui_resizable_temp_pop();
    brui_frame_end(&win);
  }
}
// tcc -g -Iinclude -I. -o bin/test_ui tests/test_ui.c -lm
