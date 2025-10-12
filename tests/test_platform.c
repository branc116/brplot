#include "src/br_pp.h"
#define BR_WANTS_GL 1
#include "src/br_platform.h"
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"

#include <stdlib.h>

int main(void) {
  float up_x = 0.f;
  int mouse_x = 0.f;
  brpl_window_t window = {
    .title = "Test",
    .kind = brpl_window_glfw,
    .viewport = {
      .width = 800, .height = 600
    }
  };

  BR_ASSERT(brpl_window_open(&window));
  while (false == window.should_close) {
    brpl_event_t event = brpl_event_next(&window);
    switch (event.kind) {
      case brpl_event_mouse_move: {
        mouse_x = event.pos.x;
      } break;
      case brpl_event_window_resize: {
        window.viewport.size = BR_SIZE_TOI(event.size);
      } break;
      case brpl_event_window_unfocused: {
        LOGI("Unfocused"); break;
      } break;
      case brpl_event_close: {
        window.should_close = true;
      } break;
      case brpl_event_next_frame: {
        up_x = 2 * (mouse_x / (float)window.viewport.width) - 1;
        brpl_frame_start(&window);
          glViewport(0, 0, window.viewport.width, window.viewport.height);
          glClearColor(0.0f, 0.9f, 0.3f, 1.0f);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          glColor3f(1.0f, 0.0f, 0.0f);
          glBegin(GL_TRIANGLES);
            glVertex3f(up_x, 1.0, 0.0);
            glVertex3f(-1.0, -1.0, 0.0);
            glVertex3f(1.0, -1.0, 0.0);

            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(0.0, .1f, 0.0);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex3f(-.1f, -.1f, 0.0);
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3f(.10, -.10, 0.0);
          glEnd();
          glFlush();
        brpl_frame_end(&window);
      } break;
      default:; //LOGI("Unhandled event: %d", event.kind);
    }
  }
  return 0;
}

void br_on_fatal_error(void) {
}
// cc tests/test_platform.c src/platform2.c -I.
