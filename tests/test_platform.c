#define BRPLAT_IMPLEMENTATION
#include <brplat.h>

int main(void) {
  float up_x = 0.f;
  float mouse_x = 0.f;
  brpl_window_t window = {
    .title = "Test",
    .viewport = {
      .width = 800, .height = 600
    }
  };

  brpl_window_open(&window);
  while (false == window.should_close) {
    brpl_event_t event = brpl_event_next(&window);
    switch (event.kind) {
      case brpl_event_mouse_move: mouse_x = event.pos.x; break;
      case brpl_event_frame_next: {
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
            glVertex3f(.10f, -.10f, 0.0);
          glEnd();
          glFlush();
        brpl_frame_end(&window);
      } break;
      default:; //LOGI("Unhandled event: %d", event.kind);
    }
  }
  return 0;
}

// tcc -I. -Iinclude -o bin/test_platform tests/test_platform.c && bin/test_platform
