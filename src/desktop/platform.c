#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_help.h"
#include "src/br_gl.h"
#include "src/br_tl.h"

#include <string.h>


#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#  include <time.h>
#endif

#include "GLFW/glfw3.h"

static _Thread_local br_plotter_t* stl_br = NULL;

static void log_glfw_errors(int level, const char* error);
static void br_help_sleep(double seconds);
static void br_glfw_on_scroll(struct GLFWwindow* window, double x, double y);
static void br_glfw_on_mouse_move(struct GLFWwindow* window, double x, double y);
static void br_glfw_on_mouse_button(struct GLFWwindow* window, int button, int action, int mods);
static void br_glfw_on_key(struct GLFWwindow* window, int key, int scancode, int action, int mods);

void br_plotter_init_specifics_platform(br_plotter_t* br) {
  br->win.glfw =  glfwGetCurrentContext();
  if (br->win.glfw) {
    LOGI("GLFW window alrady initialized: %p", (void*)br->win.glfw);
  } else {

    glfwSetErrorCallback(log_glfw_errors);
    int result = glfwInit();
    if (result == GLFW_FALSE) LOGF("GLFW: Failed to initialize GLFW");
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
    br->win.glfw = glfwCreateWindow(br->width, br->height, "brplot", NULL, NULL);
    glfwMakeContextCurrent(br->win.glfw);
    glfwSwapInterval(1);
  }
  stl_br = br;
  brgl_load();
  brgl_disable_back_face_cull();
  brgl_enable_depth_test();
  brgl_enable(GL_BLEND);
  brgl_blend_func(GL_SRC_ALPHA, GL_DST_ALPHA);
  brgl_blend_equation(GL_MAX);
 
  br->shaders = br_shaders_malloc();
  br->text = br_text_renderer_malloc(1024, 1024, br_font_data, &br->shaders.font);
  glfwGetWindowSize(br->win.glfw, &br->win.size.width, &br->win.size.height);
  br->time.now = glfwGetTime();

  glfwSetScrollCallback(br->win.glfw, br_glfw_on_scroll);
  glfwSetScrollCallback(br->win.glfw, br_glfw_on_scroll);
  glfwSetCursorPosCallback(br->win.glfw, br_glfw_on_mouse_move);
  glfwSetMouseButtonCallback(br->win.glfw, br_glfw_on_mouse_button);
  glfwSetKeyCallback(br->win.glfw, br_glfw_on_key);
}

static void br_glfw_on_scroll(GLFWwindow* window, double x, double y) {
  (void)window;
  stl_br->mouse.scroll = BR_VEC2(x, y);
}

static void br_glfw_on_mouse_move(struct GLFWwindow * window, double x, double y) {
  (void)window;
  stl_br->mouse.pos = BR_VEC2(x, y);
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
  (void)window;
  LOGI("ctrl: %d | %d", mods & GLFW_MOD_CONTROL, mods);
  stl_br->key.mod = mods;
  if (key == GLFW_KEY_LEFT_ALT) return;
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
  br->time.frame = br->time.now - br->time.old;

  glfwPollEvents();
  br->mouse.delta = BR_VEC2_SUB(br->mouse.pos, br->mouse.old_pos);
  br->mouse.old_pos = br->mouse.pos;

  brgl_clear_color(0,0,0,0);
  brgl_clear();
}

void br_plotter_end_drawing(br_plotter_t* br) {
  glfwSwapBuffers(br->win.glfw);

  br->mouse.scroll = BR_VEC2(0, 0);
  br->mouse.pressed.right = br->mouse.pressed.left = false;
  memset(&br->key.pressed[0], 0, sizeof(br->key.pressed));

  double target_frame_time = 1/60.0;
  double frame_time = glfwGetTime() - br->time.now;
  br_help_sleep(target_frame_time - frame_time);
  br->should_close = glfwWindowShouldClose(br->win.glfw);
  glfwSetWindowShouldClose(br->win.glfw, GLFW_FALSE);
}

br_vec2_t brtl_get_scroll(void) {
  return stl_br->mouse.scroll;
}

br_vec2_t brtl_mouse_get_pos(void) {
  return stl_br->mouse.pos;
}

br_vec2_t brtl_mouse_get_delta(void) {
  return stl_br->mouse.delta;
}

float brtl_get_time(void) {
  return (float)stl_br->time.now;
}

bool brtl_mouse_is_down_l(void) {
  return stl_br->mouse.down.left;
}

bool brtl_mouse_is_down_r(void) {
  return stl_br->mouse.down.right;
}

bool brtl_mouse_is_pressed_l(void) {
  return stl_br->mouse.pressed.left;
}

bool brtl_mouse_is_pressed_r(void) {
  return stl_br->mouse.pressed.right;
}

bool brtl_key_is_down(int key) {
  int code = glfwGetKeyScancode(key);
  if (code < 0 || code >= 64) {
    LOGW("Key %d, scancode %d is not valid", key, code);
  }
  return stl_br->key.down[code];
}

bool brtl_key_is_pressed(int key) {
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

br_plotter_t* brtl_get_plotter(void) {
  return stl_br;
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
