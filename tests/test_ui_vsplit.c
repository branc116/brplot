#define BRUI_IMPLEMENTATION
#include <brui.h>

// TODO: Export this
void brui_vsplit(int);
bool brui_vsplit_pop(void);
int main(void) {
  brui_window_t window = { 0 };
  brui_window_init(&window);
  while (false == window.pl.should_close) {
    while (brpl_event_frame_next != brui_event_next(&window).kind); // You can also handle some of the events
    brui_frame_start(&window);
      const char* texts[] = { "Hello", "haaalo", "yooo", "hihi" };
      int n = 3, i = 0;
      brui_vsplit(n);
      do {
        if (brui_buttonf(texts[i])) printf("Hello %s\n", texts[i++]);
      } while (brui_vsplit_pop());

    brui_frame_end(&window);
  }
}
