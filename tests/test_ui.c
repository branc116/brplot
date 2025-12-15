#include <brui.h>

int main(void) {
  brui_window_t window = { 0 };
  brui_window_init(&window);
  while (false == window.pl.should_close) {
    while (brpl_event_frame_next != brui_event_next(&window).kind); // You can also handle some of the events 
    brui_frame_start(&window);
      if (brui_buttonf("Hello")) printf("Hello world\n");
      brui_resizable_temp_push(BR_STRL("Helllo"));
        if (brui_buttonf("Hello")) printf("Hello world\n");
      brui_resizable_temp_pop();
    brui_frame_end(&window);
  }
}
