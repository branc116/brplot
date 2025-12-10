#include <brui.h>

// TODO: Export this
void brui_vsplit(int);
void brui_vsplit_pop();
void brui_vsplit_end();
int main(void) {
  brui_window_t window = { 0 };
  brui_window_init(&window);
  while (false == window.pl.should_close) {
    while (brpl_event_frame_next != brui_event_next(&window).kind); // You can also handle some of the events
    brui_frame_start(&window);
      const char* texts[] = { "Hello", "haaalo", "yooo", "hihi" };
      int n = sizeof(texts)/sizeof(texts[0]);
      brui_vsplit(n);
      for (int i = 0; i < n; ++i, brui_vsplit_pop()) {
        if (brui_buttonf(texts[i])) printf("Hello %s\n", texts[i]);
      }
      brui_vsplit_end();

    brui_frame_end(&window);
  }
}
