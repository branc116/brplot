#define BRUI_IMPLEMENTATION
#include <brui.h>

int main(void) {
  brui_window_t window = { 0 };
  while (false == window.pl.should_close) {
    while (brpl_event_frame_next != brui_event_next(&window).kind); // You can also handle some of the events 
    brui_frame_start(&window);
      if (brui_buttonf("Hello")) printf("Hello world\n");
    brui_frame_end(&window);
  }
}
