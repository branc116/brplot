#include "src/br_pp.h"
#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_gl.h"
#include "src/br_tl.h"
#include "src/br_q.h"
#include "src/br_icons.h"
#include "src/br_ui.h"
#include "src/br_text_renderer.h"
#include "src/br_da.h"

#include <string.h>

#define GLAD_MALLOC BR_MALLOC
#define GLAD_FREE BR_FREE

#if defined(__GNUC__) || defined(__clang__)
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
#  if !defined(__MINGW32__)
#    pragma comment(lib, "user32.lib")
#    pragma comment(lib, "Gdi32.lib")
#    pragma comment(lib, "Shell32.lib")
#  endif
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
#  if !defined(BR_NO_X11)
#    define _GLFW_X11
#  endif
#  if defined(HEADLESS)
#    define _GLFW_NULL
#  endif
#  define BR_NO_WAYLAND
#endif
#if defined(__APPLE__)
#  define _GLFW_COCOA
#endif
#if defined(__TINYC__)
#  define _WIN32_WINNT_WINXP      0x0501
#endif


#if defined(_GLFW_X11) || defined(_GLFW_WAYLAND)
#  include "external/glfw/src/xkb_unicode.h"
#  include "external/glfw/src/posix_poll.h"
#endif

#if defined(_GLFW_X11)
#  include "external/X11/cursorfont.h"
#  include "external/X11/Xmd.h"
#  include "external/xkbcommon/xkbcommon.h"
#endif

#if !defined(__EMSCRIPTEN__)
// Common modules to all platforms
#  include "external/glfw/src/init.c"
#  include "external/glfw/src/platform.c"
#  include "external/glfw/src/context.c"
#  include "external/glfw/src/monitor.c"
#  include "external/glfw/src/window.c"
#  include "external/glfw/src/input.c"
#  if !defined(_GLFW_NULL) || !defined(BR_NO_WAYLAND) || !defined(BR_NO_X11)
#    include "external/glfw/src/egl_context.c"
#    include "external/glfw/src/osmesa_context.c"
#  endif
#endif

#include "external/glfw/src/x11_window.c"

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#  include "external/glfw/src/win32_init.c"
#  include "external/glfw/src/win32_module.c"
#  include "external/glfw/src/win32_monitor.c"
#  include "external/glfw/src/win32_window.c"
#  include "external/glfw/src/win32_time.c"
#  include "external/glfw/src/win32_thread.c"
#  include "external/glfw/src/wgl_context.c"
#endif

#if defined(_GLFW_X11)
#  include "external/glfw/src/x11_init.c"
#  include "external/glfw/src/x11_monitor.c"
#  include "external/glfw/src/glx_context.c"
#endif

#if defined(_GLFW_WAYLAND)
#  include "external/glfw/src/wl_init.c"
#  include "external/glfw/src/wl_monitor.c"
#  include "external/glfw/src/wl_window.c"
#endif

#if defined(_GLFW_NULL)
#  include "external/glfw/src/null_init.c"
#  include "external/glfw/src/null_monitor.c"
#  include "external/glfw/src/null_window.c"
#endif

#if defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__)
#  include "external/glfw/src/posix_module.c"
#  include "external/glfw/src/posix_thread.c"
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__)
#  include "external/glfw/src/posix_time.c"
#  include "external/glfw/src/posix_poll.c"
#  include "external/glfw/src/xkb_unicode.c"
#endif

#if defined(__APPLE__)
#  if defined(__OBJC__)
#    import <Cocoa/Cocoa.h>
#  else
#    include <ApplicationServices/ApplicationServices.h>
#    include <objc/objc.h>
#  endif
#  include <sys/syslimits.h>
#  include <mach-o/dyld.h>
#  include "external/glfw/src/cocoa_init.m"
#  include "external/glfw/src/cocoa_monitor.m"
#  include "external/glfw/src/cocoa_window.m"
#  include "external/glfw/src/cocoa_time.c"
#  include "external/glfw/src/nsgl_context.m"
#endif

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__MINGW32__) || defined(__CYGWIN__)
#  include <time.h>
#elif defined(_WIN32)
#  include <Windows.h>
#  include <synchapi.h>
#endif

#if BR_HAS_HOTRELOAD
#  include "src/desktop/hotreload.c"
#endif

static BR_THREAD_LOCAL br_plotter_t* stl_br = NULL;

static void log_glfw_errors(int level, const char* error);
static void br_glfw_on_scroll(struct GLFWwindow* window, double x, double y);
static void br_glfw_on_mouse_move(struct GLFWwindow* window, double x, double y);
static void br_glfw_on_mouse_button(struct GLFWwindow* window, int button, int action, int mods);
static void br_glfw_on_key(struct GLFWwindow* window, int key, int scancode, int action, int mods);
static void br_glfw_on_char(GLFWwindow* window, uint32_t codepoint);

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
#if !defined(HEADLESS)
    glfwMakeContextCurrent(br->win.glfw);
#endif
  }

  stl_br = br;
  brgl_load();
  brgl_disable_back_face_cull();
  brgl_enable_depth_test();
  brgl_enable(GL_BLEND);
  brgl_enable_clip_distance();
  brgl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  brgl_blend_equation(GL_FUNC_ADD);
  br_sizei_t size = brtl_window_size();
  brgl_viewport(0, 0, size.width, size.height);
  br->win.size = size;

  br->shaders = br_shaders_malloc();
  br->text = br_text_renderer_malloc(1024, 1024, br_font_data, &br->shaders.font);
  br->time.now = glfwGetTime();

  glfwSetScrollCallback(br->win.glfw, br_glfw_on_scroll);
  glfwSetScrollCallback(br->win.glfw, br_glfw_on_scroll);
  glfwSetCursorPosCallback(br->win.glfw, br_glfw_on_mouse_move);
  glfwSetMouseButtonCallback(br->win.glfw, br_glfw_on_mouse_button);
  glfwSetKeyCallback(br->win.glfw, br_glfw_on_key);
  glfwSetCharCallback(br->win.glfw, br_glfw_on_char);
#if BR_HAS_HOTRELOAD
  br_hotreload_start(&br->hot_state);
#endif
}

void br_plotter_deinit_specifics_platform(br_plotter_t* br) {
  br_text_renderer_free(br->text);
  br_shaders_free(br->shaders);
  glfwDestroyWindow(br->win.glfw);
  glfwTerminate();
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
  if (action == GLFW_REPEAT || action == GLFW_PRESS) {
    if (key >= BR_KEY_ESCAPE && key <= GLFW_KEY_MENU) {
      brtl_pressed_char_t c = { .key = (uint32_t)key, .is_special = true };
      br_da_push(stl_br->pressed_chars, c);
    }
  }
  if (key < 0 || key >= 512) {
    LOGW("Bad scancode %d, key %d", scancode, key);
    return;
  }
  if (action == GLFW_PRESS) {
    stl_br->key.down[key] = true;
  } else if (action == GLFW_RELEASE) {
    stl_br->key.down[key] = false;
  }
}

void br_glfw_on_char(GLFWwindow* window, uint32_t codepoint) {
  (void)window;
  brtl_pressed_char_t c = { .key = codepoint, .is_special = false };
  br_da_push(stl_br->pressed_chars, c);
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
#if BR_HAS_HOTRELOAD
  br_hotreload_tick(&br->hot_state);
#endif
}

void br_plotter_end_drawing(br_plotter_t* br) {
  br_shaders_draw_all(br->shaders);
  br_text_renderer_dump(br->text);
  handle_all_commands(br, br->commands);
#if BR_HAS_SHADER_RELOAD
  if (br->shaders_dirty) {
    br_shaders_refresh(br->shaders);
    br->shaders_dirty = false;
  }
#endif
  br->mouse.scroll = BR_VEC2(0, 0);
  br->mouse.pressed.right = br->mouse.pressed.left = false;
  br->pressed_chars.len = 0;
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

double brtl_time(void) {
  return  glfwGetTime();
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
  return stl_br->key.down[key - 0x20] || stl_br->key.down[key];
}

bool brtl_key_pressed(int key) {
  for (size_t i = 0; i < stl_br->pressed_chars.len; ++i) if (br_da_get(stl_br->pressed_chars, i).key == (uint32_t)key) return true;
  return false;
}

br_pressed_chars_t brtl_pressed_chars(void) {
  return stl_br->pressed_chars;
}

bool brtl_key_ctrl(void) {
  bool is_down = (stl_br->key.mod & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL;
  return is_down;
}

bool brtl_key_alt(void) {
  return (stl_br->key.mod & GLFW_MOD_ALT) == GLFW_MOD_ALT;
}

bool brtl_key_shift(void) {
  bool is_down = (stl_br->key.mod & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT;
  return is_down;
}

br_theme_t* brtl_theme(void) {
  return &stl_br->ui.theme;
}

br_sizei_t brtl_window_size(void) {
  br_sizei_t s;
  glfwGetWindowSize(stl_br->win.glfw, &s.width, &s.height);
  return s;
}

void brtl_window_size_set(int width, int height) {
  glfwSetWindowSize(stl_br->win.glfw, width, height);
}

br_extenti_t brtl_viewport(void) {
  return stl_br->win.viewport;
}

void brtl_viewport_set(br_extenti_t ex) {
  stl_br->win.viewport = ex;
}

br_strv_t brtl_clipboard(void) {
  const char* str = glfwGetClipboardString(stl_br->win.glfw);
  if (str == NULL) return BR_STRV(NULL, 0);
  return br_strv_from_c_str(str);
}

br_plotter_t* brtl_plotter(void) {
  return stl_br;
}

br_shaders_t* brtl_shaders(void) {
  return &stl_br->shaders;
}

br_datas_t* brtl_datas(void) {
  return &stl_br->groups;
}

br_text_renderer_t* brtl_text_renderer(void) {
  return stl_br->text;
}

static BR_THREAD_LOCAL brsp_t br_stl_string_pool;
brsp_t* brtl_brsp(void) {
  return &br_stl_string_pool;
}

bruirs_t* brtl_bruirs(void) {
  return &stl_br->resizables;
}

bruir_children_t brtl_bruirs_childern(int handle) {
  return brui_resizable_children_temp(handle);
}


bool* brtl_debug(void) {
  return &stl_br->ui.theme.ui.debug;
}

float* brtl_cull_min(void) {
  return &stl_br->ui.theme.ui.cull_min;
}

float* brtl_min_sampling(void) {
  return &stl_br->ui.theme.ui.min_sampling;
}
