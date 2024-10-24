#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_help.h"
#include "src/br_gl.h"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#include <time.h>
#endif

#include "GLFW/glfw3.h"

static void log_glfw_errors(int level, const char* error);
static void br_help_sleep(double seconds);

void br_plotter_init_specifics_platform(br_plotter_t* br) {
  br->glfw_window =  glfwGetCurrentContext();
  if (br->glfw_window) {
    LOGI("GLFW window alrady initialized: %p", (void*)br->glfw_window);
    return;
  }

  glfwSetErrorCallback(log_glfw_errors);
  int result = glfwInit();
  if (result == GLFW_FALSE) LOGF("GLFW: Failed to initialize GLFW");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
  glfwWindowHint(GLFW_FLOATING, 1);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // OSX Requires forward compatibility
#else
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE); // Forward Compatibility Hint: Only 3.3 and above!
#endif
  br->glfw_window = glfwCreateWindow(br->width, br->height, "brplot", NULL, NULL);
  glfwMakeContextCurrent(br->glfw_window);
  glfwSwapInterval(1);
  brgl_load();
 
  br->shaders = br_shaders_malloc();
  br->text = br_text_renderer_malloc(1024, 1024, br_font_data, &br->shaders.font);
}

static void log_glfw_errors(int level, const char* error) {
  LOGE("[%d] %s", level, error);
}

void br_plotter_begin_drawing(br_plotter_t* br) {
  br->time.old = br->time.now;
  br->time.now = glfwGetTime();   // Elapsed time since glfwInit()
  br->time.frame = br->time.now - br->time.old;
}

void br_plotter_end_drawing(br_plotter_t* br) {
  glfwSwapBuffers(br->glfw_window);
  double target_frame_time = 1/60.0;
  double frame_time = glfwGetTime() - br->time.now;
  br_help_sleep(target_frame_time - frame_time);
}

static void br_help_sleep(double seconds) {
  if (seconds <= 0.0) return;
  #if defined(_WIN32)
      Sleep((unsigned long)(sleepSeconds*1000.0));
  #endif
  #if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
      struct timespec req = { 0 };
      time_t sec = (long)seconds;
      long nsec = ((long)(seconds - (double)sec))*1000000000L;
      req.tv_sec = sec;
      req.tv_nsec = nsec;

      // NOTE: Use nanosleep() on Unix platforms... usleep() it's deprecated.
      while (nanosleep(&req, &req) == -1) continue;
  #endif
  #if defined(__APPLE__)
      usleep(sleepSeconds*1000000.0);
  #endif
}
