#define BRUI_IMPLEMENTATION
#include <brui.h>

int font_size = 20;

int main(void) {
  brui_window_t win = {0};
  brui_window_init(&win);
  GLuint triangle_framebuffer = brgl_create_framebuffer(800, 600);
  while (!win.pl.should_close) {
    while (brui_event_next(&win).kind != brpl_event_frame_next);

    brgl_enable_framebuffer(triangle_framebuffer, 800, 600);
      glClearColor(0.0f, 0.9f, 0.3f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.0f, 0.0f, 1.f);
        glVertex3f(0.0, 1.0, 0.0);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-1.0, -1.0, 0.0);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(1.0, -1.0, 0.0);
      glEnd();
      glFlush();
    brgl_enable_framebuffer(0, win.pl.viewport.width, win.pl.viewport.height);

    brui_frame_start(&win);

      brui_resizable_temp_push(BR_STRL("Framebuffer"));
        brui_framebuffer(triangle_framebuffer);
      brui_resizable_temp_pop();

      brui_resizable_temp_push(BR_STRL("Image"));
        brui_text_size_set(font_size);
        if (brui_buttonf("FONT %d", font_size)) font_size += 1;
        // Draw font/texture atlas into current resizable...
        brui_texture(brtr_texture_id());
      brui_resizable_temp_pop();

    brui_frame_end(&win);
  }
}

// tcc -g -I. -Iinclude -o bin/test_img tests/test_img.c -lm
