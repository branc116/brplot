#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_help.h"
#include "src/br_gl.h"
#include "src/br_tl.h"
#include "src/br_q.h"

#include <string.h>

#define GLAD_MALLOC BR_MALLOC
#define GLAD_FREE BR_FREE

#if !defined(_MSC_VER)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wreturn-type"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  pragma GCC diagnostic ignored "-Wpedantic"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

#include "external/glfw/include/GLFW/glfw3.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#  define _GLFW_WIN32
#endif

#if defined(__linux__)
#  if !defined(BR_NO_X11)
#    define _GLFW_X11
#  endif
#  if !defined(BR_NO_WAYLAND)
#    define _GLFW_WAYLAND
#  endif
#  if defined(HEADLESS)
#    define _GLFW_NULL
#  endif
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#  define _GLFW_X11
#endif
#if defined(__APPLE__)
#  define _GLFW_COCOA
#endif
#if defined(__TINYC__)
#  define _WIN32_WINNT_WINXP      0x0501
#endif


#if !defined(__EMSCRIPTEN__)
// Common modules to all platforms
#  include "external/glfw/src/init.c"
#  include "external/glfw/src/platform.c"
#  include "external/glfw/src/context.c"
#  include "external/glfw/src/monitor.c"
#  include "external/glfw/src/window.c"
#  include "external/glfw/src/input.c"
#endif


#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#  include "external/glfw/src/win32_init.c"
#  include "external/glfw/src/win32_module.c"
#  include "external/glfw/src/win32_monitor.c"
#  include "external/glfw/src/win32_window.c"
#  include "external/glfw/src/win32_time.c"
#  include "external/glfw/src/win32_thread.c"
#  include "external/glfw/src/wgl_context.c"

#  include "external/glfw/src/egl_context.c"
#  include "external/glfw/src/osmesa_context.c"
#  if defined(_GLFW_NULL)
#    include "external/glfw/src/null_init.c"
#    include "external/glfw/src/null_monitor.c"
#    include "external/glfw/src/null_window.c"
#  endif
#endif

#if defined(__linux__)
#  include "external/glfw/src/posix_module.c"
#  include "external/glfw/src/posix_thread.c"
#  include "external/glfw/src/posix_time.c"
#  include "external/glfw/src/posix_poll.c"
#  include "external/glfw/src/xkb_unicode.c"

#  include "external/glfw/src/egl_context.c"
#  include "external/glfw/src/osmesa_context.c"

#  if defined(_GLFW_WAYLAND)
#    include "external/glfw/src/wl_init.c"
#    include "external/glfw/src/wl_monitor.c"
#    include "external/glfw/src/wl_window.c"
#  endif

#  if defined(_GLFW_X11)
#    include "external/glfw/src/x11_init.c"
#    include "external/glfw/src/x11_monitor.c"
#    include "external/glfw/src/x11_window.c"
#    include "external/glfw/src/glx_context.c"
#  endif

#  if defined(_GLFW_NULL)
#    include "external/glfw/src/null_init.c"
#    include "external/glfw/src/null_monitor.c"
#    include "external/glfw/src/null_window.c"
#  endif
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__)
#  include "external/glfw/src/posix_module.c"
#  include "external/glfw/src/posix_thread.c"
#  include "external/glfw/src/posix_time.c"
#  include "external/glfw/src/posix_poll.c"
#  include "external/glfw/src/xkb_unicode.c"

#  include "external/glfw/src/x11_init.c"
#  include "external/glfw/src/x11_monitor.c"
#  include "external/glfw/src/x11_window.c"
#  include "external/glfw/src/glx_context.c"

#  include "external/glfw/src/egl_context.c"
#  include "external/glfw/src/osmesa_context.c"
#endif

#if defined(__APPLE__)
#  include <sys/syslimits.h>
#  include <mach-o/dyld.h>
#  include "external/glfw/src/posix_module.c"
#  include "external/glfw/src/posix_thread.c"
#  include "external/glfw/src/cocoa_init.m"
#  include "external/glfw/src/cocoa_monitor.m"
#  include "external/glfw/src/cocoa_window.m"
#  include "external/glfw/src/cocoa_time.c"
#  include "external/glfw/src/nsgl_context.m"

#  include "external/glfw/src/egl_context.c"
#  include "external/glfw/src/osmesa_context.c"
#endif

#if !defined(_MSC_VER)
#  pragma GCC diagnostic pop
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__MINGW32__) || defined(__CYGWIN__)
#  include <time.h>
#elif defined(_WIN32)
#  include "Windows.h"
#  include "synchapi.h"
#endif

static BR_THREAD_LOCAL br_plotter_t* stl_br = NULL;

static void log_glfw_errors(int level, const char* error);
static void br_help_sleep(double seconds);
static void br_glfw_on_scroll(struct GLFWwindow* window, double x, double y);
static void br_glfw_on_mouse_move(struct GLFWwindow* window, double x, double y);
static void br_glfw_on_mouse_button(struct GLFWwindow* window, int button, int action, int mods);
static void br_glfw_on_key(struct GLFWwindow* window, int key, int scancode, int action, int mods);

void br_plotter_init_specifics_platform(br_plotter_t* br, int width, int height) {
  br->win.glfw = glfwGetCurrentContext();
  if (br->win.glfw) {
    LOGI("GLFW window alrady initialized: %p", (void*)br->win.glfw);
  } else {
    glfwSetErrorCallback(log_glfw_errors);
    int result = glfwInit();
    if (result == GLFW_FALSE) LOGF("GLFW: Failed to initialize GLFW");
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // OSX Requires forward compatibility
#else
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE); // Forward Compatibility Hint: Only 3.3 and above!
#endif
    glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
    br->win.glfw = glfwCreateWindow(width, height, "brplot", NULL, NULL);
    glfwMakeContextCurrent(br->win.glfw);
    //glfwSwapInterval(1);
  }
  stl_br = br;
  brgl_load();
  brgl_disable_back_face_cull();
  brgl_enable_depth_test();
  brgl_enable(GL_BLEND);
  brgl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  brgl_blend_equation(GL_FUNC_ADD);
  br_sizei_t size = brtl_window_size();
  brgl_viewport(0, 0, size.width, size.height);
  br->win.size = size;

  br->shaders = br_shaders_malloc();
  br->shaders.font->uvs.sub_pix_aa_map_uv =  BR_VEC3(-1, 0, 1);
  br->shaders.font->uvs.sub_pix_aa_scale_uv = 0.2f;
  br->text = br_text_renderer_malloc(1024, 1024, br_font_data, &br->shaders.font);
  br->time.now = glfwGetTime();

  glfwSetScrollCallback(br->win.glfw, br_glfw_on_scroll);
  glfwSetScrollCallback(br->win.glfw, br_glfw_on_scroll);
  glfwSetCursorPosCallback(br->win.glfw, br_glfw_on_mouse_move);
  glfwSetMouseButtonCallback(br->win.glfw, br_glfw_on_mouse_button);
  glfwSetKeyCallback(br->win.glfw, br_glfw_on_key);
}

static void br_glfw_on_scroll(GLFWwindow* window, double x, double y) {
  (void)window;
  stl_br->mouse.scroll = BR_VEC2((float)x, (float)y);
}

static void br_glfw_on_mouse_move(struct GLFWwindow * window, double x, double y) {
  (void)window;
  stl_br->mouse.pos = BR_VEC2((float)x, (float)y);
}

static void br_glfw_on_mouse_button(struct GLFWwindow* window, int button, int action, int mods) {
  (void)window;
  stl_br->key.mod = mods;
  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT: {
                                   stl_br->mouse.down.left = action == GLFW_PRESS ? true : false;
                                   stl_br->mouse.pressed.left = action == GLFW_RELEASE ? true : false;
                                 } break;
    case GLFW_MOUSE_BUTTON_RIGHT: {
                                   stl_br->mouse.down.right = action == GLFW_PRESS ? true : false;
                                   stl_br->mouse.pressed.right = action == GLFW_RELEASE ? true : false;
                                 } break;
    default: LOGI("Unknown button %d", button);
  }
}

static void br_glfw_on_key(struct GLFWwindow* window, int key, int scancode, int action, int mods) {
  (void)window; (void) mods;
  if (key == GLFW_KEY_LEFT_ALT) return;
  if (key == GLFW_KEY_LEFT_ALT) {
    if (action == GLFW_PRESS) {
      stl_br->key.mod |= GLFW_MOD_ALT;
    } else if (action == GLFW_RELEASE) {
      stl_br->key.mod &= ~(GLFW_MOD_ALT);
    }
  }
  if (key == GLFW_KEY_LEFT_CONTROL) {
    if (action == GLFW_PRESS) {
      stl_br->key.mod |= GLFW_MOD_CONTROL;
    } else if (action == GLFW_RELEASE) {
      stl_br->key.mod &= ~(GLFW_MOD_CONTROL);
    }
  }
  if (key == GLFW_KEY_LEFT_SHIFT) {
    if (action == GLFW_PRESS) {
      stl_br->key.mod |= GLFW_MOD_SHIFT;
    } else if (action == GLFW_RELEASE) {
      stl_br->key.mod &= ~(GLFW_MOD_SHIFT);
    }
  }
  if (scancode < 0 || scancode >= 64) {
    LOGW("Bad scancode %d, key %d", scancode, key);
    return;
  }
  if (action == GLFW_PRESS) {
    stl_br->key.pressed[scancode] = true;
    stl_br->key.down[scancode] = true;
  } else if (action == GLFW_RELEASE) {
    stl_br->key.down[scancode] = false;
  }
}

static void log_glfw_errors(int level, const char* error) {
  LOGE("[%d] %s", level, error);
}

void br_plotter_begin_drawing(br_plotter_t* br) {
  br->time.old = br->time.now;
  br->time.now = glfwGetTime();
  br->time.frame = (br->time.now - br->time.old);

  glfwPollEvents();
  br->mouse.delta = br_vec2_sub(br->mouse.pos, br->mouse.old_pos);
  br->mouse.old_pos = br->mouse.pos;
  glfwGetWindowSize(br->win.glfw, &br->win.size.width, &br->win.size.height);
}

void br_plotter_end_drawing(br_plotter_t* br) {
  br_shaders_draw_all(br->shaders);
  br_text_renderer_dump(br->text);
  glfwSetWindowSize(br->win.glfw, br->win.size.width, br->win.size.height);
  handle_all_commands(br, br->commands);
#if BR_HAS_SHADER_RELOAD
  if (br->shaders_dirty) {
    br_shaders_refresh(br->shaders);
    br->shaders_dirty = false;
  }
#endif
  br->mouse.scroll = BR_VEC2(0, 0);
  br->mouse.pressed.right = br->mouse.pressed.left = false;
  memset(&br->key.pressed[0], 0, sizeof(br->key.pressed));
  glfwSwapBuffers(br->win.glfw);

  br->should_close = br->should_close || glfwWindowShouldClose(br->win.glfw);
  glfwSetWindowShouldClose(br->win.glfw, GLFW_FALSE);
}

br_vec2_t brtl_mouse_scroll(void) {
  return stl_br->mouse.scroll;
}

br_vec2_t brtl_mouse_pos(void) {
  return stl_br->mouse.pos;
}

br_vec2_t brtl_mouse_delta(void) {
  return stl_br->mouse.delta;
}

float brtl_time(void) {
  return  (float)glfwGetTime();
}

float brtl_frame_time(void) {
  return (float)stl_br->time.frame;
}

int brtl_fps(void) {
  return (int)(1.f/(float)stl_br->time.frame);
}

bool brtl_mousel_down(void) {
  return stl_br->mouse.down.left;
}

bool brtl_mouser_down(void) {
  return stl_br->mouse.down.right;
}

bool brtl_mousel_pressed(void) {
  return stl_br->mouse.pressed.left;
}

bool brtl_mouser_pressed(void) {
  return stl_br->mouse.pressed.right;
}

bool brtl_key_down(int key) {
  int code = glfwGetKeyScancode(key);
  if (code < 0 || code >= 64) {
    LOGW("Key %d, scancode %d is not valid", key, code);
  }
  return stl_br->key.down[code];
}

bool brtl_key_pressed(int key) {
  int code = glfwGetKeyScancode(key);
  if (code < 0 || code >= 64) {
    LOGW("Key %d, scancode %d is not valid", key, code);
  }
  return stl_br->key.pressed[code];
}

bool brtl_key_ctrl(void) {
  bool is_down = (stl_br->key.mod & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL;
  //LOGI("CTRL: %d", is_down);
  return is_down;
}

bool brtl_key_alt(void) {
  return (stl_br->key.mod & GLFW_MOD_ALT) == GLFW_MOD_ALT;
}

bool brtl_key_shift(void) {
  return (stl_br->key.mod & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT;
}

br_sizei_t brtl_window_size(void) {
  br_sizei_t s;
  glfwGetWindowSize(stl_br->win.glfw, &s.width, &s.height);
  return s;
}

void brtl_window_size_set(int width, int height) {
  glfwSetWindowSize(stl_br->win.glfw, width, height);
}

void brtl_window_close(void) {
  glfwDestroyWindow(stl_br->win.glfw);
  glfwTerminate();
}

br_extenti_t brtl_viewport(void) {
  return stl_br->win.viewport;
}

void brtl_viewport_set(br_extenti_t ex) {
  stl_br->win.viewport = ex;
}

br_plotter_t* brtl_plotter(void) {
  return stl_br;
}

br_shaders_t* brtl_shaders(void) {
  return &stl_br->shaders;
}
br_text_renderer_t* brtl_text_renderer(void) {
  return stl_br->text;
}

static void br_help_sleep(double seconds) {
  if (seconds <= 0.0) return;
#if defined(_WIN32) || defined(__MINGW32__) || defined(__CYGWIN__)
  Sleep((unsigned long)(seconds * 1000.0));
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
  struct timespec req = { 0 };
  time_t sec = (long)seconds;
  long nsec = ((long)(seconds - (double)sec))*1000000000L;
  req.tv_sec = sec;
  req.tv_nsec = nsec;

  // NOTE: Use nanosleep() on Unix platforms... usleep() it's deprecated.
  while (nanosleep(&req, &req) == -1) continue;
#endif
#if defined(__APPLE__)
  usleep(seconds*1000000.0);
#endif
}
