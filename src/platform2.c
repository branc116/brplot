#include "src/br_pp.h"
#include "src/br_threads.h"

#include <stdint.h>
#include <assert.h>

#if !defined(BR_WANTS_GL)
#  define BR_WANTS_GL 1
#endif

#if defined(BR_HAS_GLFW)
#  define BR_WANTS_GLFW 1
   typedef struct GLFWwindow GLFWwindow;
   typedef GLFWwindow* GLFWwindowp;
   typedef void (* GLFWerrorfun)(int error_code, const char* description);
   typedef void (* GLFWwindowsizefun)(GLFWwindow* window, int width, int height);
   typedef void (* GLFWwindowclosefun)(GLFWwindow* window);
   typedef void (* GLFWwindowfocusfun)(GLFWwindow* window, int focused);
   typedef void (* GLFWwindowcontentscalefun)(GLFWwindow* window, float xscale, float yscale);
   typedef void (* GLFWmousebuttonfun)(GLFWwindow* window, int button, int action, int mods);
   typedef void (* GLFWcursorposfun)(GLFWwindow* window, double xpos, double ypos);
   typedef void (* GLFWscrollfun)(GLFWwindow* window, double xoffset, double yoffset);
   typedef void (* GLFWkeyfun)(GLFWwindow* window, int key, int scancode, int action, int mods);
   typedef void (* GLFWcharfun)(GLFWwindow* window, unsigned int codepoint);
#endif

#if defined(BR_HAS_X11)
#  define BR_WANTS_X11 1
#  include "external/X11/Xlib.h"
#  include "external/X11/Xresource.h"
#  include "external/X11/Xutil.h"
   typedef Display* Displayp;

const char* br_glx_library_names[] = {
  "libGLX.so.0",
  "libGL.so",
  "libGL-1.so",
  "libGL.so.1",
};

const char* br_x11_library_names[] = {
  "libX11.so",
  "libX11-6.so",
  "libX11.so.6",
};

const char* br_xi_library_names[] = {
  "libXi.so",
};

#  define BR_WANTS_XI 1
#  define BR_HAS_XI 1
typedef struct
{
    int                 deviceid;
    int                 mask_len;
    unsigned char*      mask;
} XIEventMask;

/* Event types */
#define XI_DeviceChanged                 1
#define XI_KeyPress                      2
#define XI_KeyRelease                    3
#define XI_ButtonPress                   4
#define XI_ButtonRelease                 5
#define XI_Motion                        6
#define XI_Enter                         7
#define XI_Leave                         8
#define XI_FocusIn                       9
#define XI_FocusOut                      10
#define XI_HierarchyChanged              11
#define XI_PropertyEvent                 12
#define XI_RawKeyPress                   13
#define XI_RawKeyRelease                 14
#define XI_RawButtonPress                15
#define XI_RawButtonRelease              16
#define XI_RawMotion                     17
#define XI_TouchBegin                    18 /* XI 2.2 */
#define XI_TouchUpdate                   19
#define XI_TouchEnd                      20
#define XI_TouchOwnership                21
#define XI_RawTouchBegin                 22
#define XI_RawTouchUpdate                23
#define XI_RawTouchEnd                   24
#define XI_BarrierHit                    25 /* XI 2.3 */
#define XI_BarrierLeave                  26
#define XI_GesturePinchBegin             27 /* XI 2.4 */
#define XI_GesturePinchUpdate            28
#define XI_GesturePinchEnd               29
#define XI_GestureSwipeBegin             30
#define XI_GestureSwipeUpdate            31
#define XI_GestureSwipeEnd               32
#define XI_LASTEVENT                     XI_GestureSwipeEnd
#define XISetMask(ptr, event)   (((unsigned char*)(ptr))[(event)>>3] |=  (1 << ((event) & 7)))
#endif

#if defined(BR_HAS_GLX)
#  define BR_WANTS_GLX 1
   typedef struct __GLXcontext* GLXContext;
   typedef struct _GLXFBConfig _GLXFBConfig;
   typedef _GLXFBConfig *GLXFBConfig;
   typedef GLXFBConfig *GLXFBConfigs;
   typedef XID GLXWindow;
   typedef const char* ccharp_t;
   typedef XVisualInfo* XVisualInfop;
   typedef void* funcptr_t;
#endif

#if defined(BR_HAS_WIN32)
#  define BR_HAS_WGL 1
#  define BR_WANTS_WGL 1
#  define BR_HAS_GDI 1
#  define BR_WANTS_GDI 1
#  include <windows.h>
const char* br_wgl_library_names[] = {
  "opengl32.dll"
};
const char* br_gdi_library_names[] = {
  "gdi32.dll"
};
#endif

#include "src/br_platform.h"
#include "src/br_gl.h"
#include "src/br_memory.h"
#include "src/br_str.h"
#include "src/br_da.h"

const char* br_gl_library_names[] = {
#if defined(_WIN32)
  "opengl32.dll",
  "OpenGL.dll",
#else
  "libGL.so",
  "libGL-1.so",
  "libGL.so.1",
#endif
};

const char* br_glfw_library_names[] = {
#if defined(_WIN32)
  "glfw.dll",
  "libglfw.dll",
  "glfw3.dll",
  "libglfw3.dll",
#elif defined(__APPLE__)
  "libglfw.dylib",
  "libglfw3.dylib",
#else
  "libglfw.so",
  "libglfw3.so",
  "libglfw.so.3",
#endif
};

#include ".generated/gl.c"

#if defined(__APPLE__)
#  include <mach/mach_time.h>
#  include <dlfcn.h>
#elif defined(_WIN32)
#  include <windows.h>
#else
#  include <unistd.h>
#  include <time.h>
#  include <sys/time.h>
#  include <dlfcn.h>
#endif


#include <string.h>
#include <stdlib.h>

static void brpl_time_init(void);
#if BR_HAS_X11
static bool brpl_x11_load(brpl_window_t* window);
#endif
#if BR_HAS_GLFW
static bool brpl_glfw_load(brpl_window_t* window);
#endif
#if BR_HAS_WIN32
static bool brpl_win32_load(brpl_window_t* window);
#endif

static BR_THREAD_LOCAL struct {
  uint64_t start;
  double frequency;
  bool monotonic;
} brpl__time;

bool brpl_window_open(brpl_window_t* window) {
  brpl_time_init();
  bool loaded = false;
  switch (window->kind) {
#if BR_HAS_X11
    case brpl_window_x11: loaded = brpl_x11_load(window); break;
#endif
#if BR_HAS_GLFW
    case brpl_window_glfw: loaded = brpl_glfw_load(window); break;
#endif
#if BR_HAS_WIN32
    case brpl_window_win32: loaded = brpl_glfw_load(window); break;
#endif
    default: LOGE("Unknown kind: %d", window->kind); break;
  }
  return loaded && window->f.window_open && window->f.window_open(window);
}

bool brpl_window_close(brpl_window_t* window) {
  window->f.window_close(window);
}

void brpl_frame_start(brpl_window_t* window) {
  window->f.frame_start(window);
}

void brpl_frame_end(brpl_window_t* window) {
  window->f.frame_end(window);
}

brpl_event_t brpl_event_next(brpl_window_t* window) {
  brpl_event_t ev = window->f.event_next(window);
#if 0
  switch (ev.kind) {
    case brpl_event_none: { LOGI("brpl_event_none"); } break;
    case brpl_event_key_press: { LOGI("brpl_event_key_press"); } break;
    case brpl_event_key_release: { LOGI("brpl_event_key_release"); } break;
    case brpl_event_input: { LOGI("brpl_event_input"); } break;
    case brpl_event_mouse_move: { LOGI("brpl_event_mouse_move"); } break;
    case brpl_event_mouse_scroll: { LOGI("brpl_event_mouse_scroll"); } break;
    case brpl_event_mouse_press: { LOGI("brpl_event_mouse_press"); } break;
    case brpl_event_mouse_release: { LOGI("brpl_event_mouse_release"); } break;
    case brpl_event_window_resize: { LOGI("brpl_event_window_resize"); } break;
    case brpl_event_window_shown: { LOGI("brpl_event_window_shown"); } break;
    case brpl_event_window_hidden: { LOGI("brpl_event_window_hidden"); } break;
    case brpl_event_window_focused: { LOGI("brpl_event_window_focused"); } break;
    case brpl_event_window_unfocused: { LOGI("brpl_event_window_unfocused"); } break;
    case brpl_event_close: { LOGI("brpl_event_close"); } break;
    case brpl_event_next_frame: { LOGI("brpl_event_next_frame: time=%f", ev.time); } break;
    case brpl_event_scale: { LOGI("brpl_event_scale"); } break;
    case brpl_event_nop: { LOGI("brpl_event_nop"); } break;
    case brpl_event_unknown: { LOGI("brpl_event_unknown"); } break;
  }
#endif
  return ev;
}

uint64_t brpl_timestamp(void) {
#if defined(_WIN32)
  br_u64 value;
  QueryPerformanceCounter((LARGE_INTEGER*)&value);
  return value;
#elif defined(__APPLE__)
  return mach_absolute_time();
#else
  struct timespec ts;
#if defined(_POSIX_MONOTONIC_CLOCK)
  clock_gettime(brpl__time.monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, &ts);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
#endif
  return (uint64_t)((double)ts.tv_sec * brpl__time.frequency) + (uint64_t) ts.tv_nsec;
#endif
}

double brpl_time(void) {
  return (double)(brpl_timestamp() - brpl__time.start) / brpl__time.frequency;
}

static void brpl_time_init(void) {
#if defined(_WIN32)
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  brpl__time.frequency = (double)freq.QuadPart;
#elif defined(__APPLE__)
 #define GLFW_BUILD_COCOA_TIMER
  mach_timebase_info_data_t info;
  mach_timebase_info(&info);
  brpl__time.frequency = (info.denom * 1e9) / info.numer;
#else
#if defined(_POSIX_MONOTONIC_CLOCK)
    if (clock_gettime(CLOCK_MONOTONIC, &(struct timespec) {0}) == 0) brpl__time.monotonic = true;
#endif
  brpl__time.frequency = 1000000000.0;
#endif
  brpl__time.start = brpl_timestamp();
}

void* brpl_load_library(const char* path) {
#if defined(_WIN32)
  return LoadLibraryA(path);
#else
  return dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
#endif
}

void* brpl_load_symbol(void* library, const char* name) {
#if defined(_WIN32)
  return (void*)GetProcAddress((HMODULE) library, name);
#else
  return (void*)dlsym(library, name);
#endif
}

#if defined(_WIN32)
void* brpl_load_gl(void* module, const char* func_name) {
  void* ret = brpl_load_symbol(module, func_name);
  if (NULL == ret) {
    void* (*wglGetProcAddress_l)(const char* name) = brpl_load_symbol(module, "wglGetProcAddress");
    if (NULL == wglGetProcAddress_l) {
      LOGF("Failed to load GetProcAddress");
    }
    ret = wglGetProcAddress_l(func_name);
  }
  return ret;
}
#endif

void brpl_q_push(brpl_q_t* q, brpl_event_t event) {
  int index = q->write_index;
  q->events[index] = event;
  static_assert(sizeof(q->events) / sizeof(q->events[0]) == 1024, "events queue must be 1024 or changed the bit patter on the nextline");
  int next_index = (index + 1) & 0x2FF;
  BR_ASSERT(next_index != q->read_index);
  q->write_index = next_index;
}

brpl_event_t brpl_q_pop(brpl_q_t* q) {
  if (q->read_index == q->write_index) return (brpl_event_t) { .kind = brpl_event_none };
  int index = q->read_index;
  brpl_event_t ret = q->events[index];
  static_assert(sizeof(q->events) / sizeof(q->events[0]) == 1024, "events queue must be 1024 or changed the bit patter on the next line");
  q->read_index = (index + 1) & 0x2FF;
  return ret;
}

// -------------------------------
// X11 IMPL
// -------------------------------
#if BR_HAS_X11

typedef struct brpl_window_x11_t {
  void* display;
  int screen;
  Window root;
  int context;
  int parent;
  Window window_handle;
  void* glx_ctx;
  GLXWindow glx_window;

  XIM im;
  XIC ic;

  Atom WM_DELETE_WINDOW;
} brpl_window_x11_t;

static void brpl_x11_frame_start(brpl_window_t* window) {
  (void)window;
}

static void brpl_x11_frame_end(brpl_window_t* window) {
  brpl_window_x11_t* w = window->win;
  glXSwapBuffers(w->display, w->glx_window);
}

static int brpl_x11_keysym(XEvent event);
static brpl_event_t brpl_x11_event_next(brpl_window_t* window) {
  // TODO: Move this struct to a window_x11_t struct;
  static BR_THREAD_LOCAL struct {
    br_u32* arr;
    size_t len, cap;

    size_t read_index;
  } utf8_chars = { 0 };

  if (utf8_chars.read_index < utf8_chars.len) {
    return (brpl_event_t) { .kind = brpl_event_input, .utf8_char = utf8_chars.arr[utf8_chars.read_index++] };
  }

  brpl_window_x11_t* w = window->win;
  Display* d = w->display;

  if (0 == XQLength(d)) {
    XFlush(d);
    return (brpl_event_t) { .kind = brpl_event_next_frame, .time = brpl_time() };
  }

  XEvent event;
  XNextEvent(d, &event);
  LOGI("Ex: %d", event.xgeneric.extension);
  switch (event.type) {
    case MotionNotify: {
      XMotionEvent m = event.xmotion;
      return (brpl_event_t) { .kind = brpl_event_mouse_move, .pos = BR_VEC2((float)m.x, (float)m.y) };
    } break;
    case KeyPress: {
      const br_u32 keycode = event.xkey.keycode;
      int keysym = brpl_x11_keysym(event);
      int count;
      Status status;
      char buffer[128];
      char* chars = buffer;

      count = Xutf8LookupString(w->ic,
                                &event.xkey,
                                buffer, sizeof(buffer) - 1,
                                NULL, &status);

      if (status == XBufferOverflow)
      {
          chars = BR_CALLOC((size_t)(count + 1), 1);
          count = Xutf8LookupString(w->ic,
                                    &event.xkey,
                                    chars, count,
                                    NULL, &status);
      }

      if (status == XLookupChars || status == XLookupBoth)
      {
          utf8_chars.len = 0;
          utf8_chars.read_index = 0;

          br_strv_t s = BR_STRV(chars, (br_u32)count);
          BR_STRV_FOREACH_UTF8(s, value) br_da_push(utf8_chars, value);
      }

      if (chars != buffer) BR_FREE(chars);

      return (brpl_event_t) { .kind = brpl_event_key_press, .key = keysym, .keycode = (br_i32)keycode };
    } break;
    case KeyRelease: {
      const br_u32 keycode = event.xkey.keycode;
      int keysym = brpl_x11_keysym(event);
      return (brpl_event_t) {
        .kind = brpl_event_key_release,
        .key = keysym,
        .keycode = (br_i32)keycode
      };
    } break;
    case ButtonPress: {
      if (event.xbutton.button == Button1)      return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = 0 };
      else if (event.xbutton.button == Button2) return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = 1 };
      else if (event.xbutton.button == Button3) return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = 3 };
      else if (event.xbutton.button == Button4) return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(0, 1) };
      else if (event.xbutton.button == Button5) return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(0, -1) };
      else if (event.xbutton.button == 6)       return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(1, 0) };
      else if (event.xbutton.button == 7)       return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(-1, 0) };
      else                                      return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = (br_i32)event.xbutton.button - Button1 - 4 };
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
    case ButtonRelease: {
      if (event.xbutton.button <= Button3) return (brpl_event_t) { .kind = brpl_event_mouse_release, .mouse_key = (br_i32)event.xbutton.button - Button1 };
      else if (event.xbutton.button <= 7)  return (brpl_event_t) { .kind = brpl_event_nop };
      else                                 return (brpl_event_t) { .kind = brpl_event_mouse_release, .mouse_key = (br_i32)event.xbutton.button - Button1 - 4 };
    } break;
    case EnterNotify: {
      return (brpl_event_t) {
        .kind = brpl_event_window_focused,
      };
    } break;
    case LeaveNotify: {
      return (brpl_event_t) {
        .kind = brpl_event_window_unfocused,
      };
    } break;
    case FocusIn: {
      return (brpl_event_t) {
        .kind = brpl_event_window_focused,
      };
    } break;
    case FocusOut: {
      return (brpl_event_t) {
        .kind = brpl_event_window_unfocused,
      };
    } break;
    case ConfigureNotify: {
      XConfigureEvent ce = event.xconfigure;
      return (brpl_event_t) {
        .kind = brpl_event_window_resize,
        .size = BR_SIZE((float)ce.width, (float)ce.height)
      };
    } break;
    case PropertyNotify: {
      LOGI("Prop %lu changed: %d", event.xproperty.atom, event.xproperty.state);
    } break;
    case Expose: {
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
    case ClientMessage: {
      if (event.xclient.data.l[0] == w->WM_DELETE_WINDOW) {
        return (brpl_event_t) { .kind = brpl_event_close };
      } else {
        return (brpl_event_t) { .kind = brpl_event_nop };
      }
    } break;
    default: {
      LOGI("Unknown x11 event type: %d", event.type);
    } break;
  }

  return (brpl_event_t) { .kind = brpl_event_next_frame, .time = brpl_time() };
}

int brpl_x11_error_callback(Display* d, XErrorEvent* e) {
    char err_text[1024];
    XGetErrorText(d, e->error_code, err_text, sizeof(err_text));
    LOGE("X Error: %s", err_text);
    return 0;
}

static bool brpl_x11_open_window(brpl_window_t* window) {
  brpl_window_x11_t x11 = { 0 };
  XSetErrorHandler(brpl_x11_error_callback);
  void* d = x11.display = XOpenDisplay(NULL);
  if (NULL == x11.display) {
    LOGE("Failed to open x11 window. display is null.");
    return false;
  }
  x11.WM_DELETE_WINDOW = XInternAtom(d, "WM_DELETE_WINDOW", false);
  x11.screen = DefaultScreen(x11.display);
  x11.root = RootWindow(x11.display, x11.screen);
  x11.context = XrmUniqueQuark();

  Visual* visual = DefaultVisual(x11.display, x11.screen);
  int depth = DefaultDepth(x11.display, x11.screen);
  XSetWindowAttributes wa = { 0 };
  wa.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                  PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                  ExposureMask | FocusChangeMask | VisibilityChangeMask |
                  EnterWindowMask | LeaveWindowMask | PropertyChangeMask;
  br_i32 xpos = 0, ypos = 0;
  br_u32 width = 800, height = 600;
  x11.window_handle = XCreateWindow(x11.display,
                                    x11.root,
                                    xpos, ypos,
                                    width, height,
                                    0,      // Border width
                                    depth,  // Color depth
                                    InputOutput,
                                    visual,
                                    CWBorderPixel | CWColormap | CWEventMask,
                                    &wa);
  if (0 == x11.window_handle) return false;

  int major, minor;
  if (!glXQueryVersion(d, &major, &minor)) return false;

#define GLX_DOUBLEBUFFER	5
#define GLX_RED_SIZE		8
#define GLX_GREEN_SIZE		9
#define GLX_BLUE_SIZE		10
#define GLX_ALPHA_SIZE		11
#define GLX_DEPTH_SIZE		12
#define GLX_STENCIL_SIZE	13
#define GLX_RENDER_TYPE			0x8011
#define GLX_DRAWABLE_TYPE		0x8010
#define GLX_RGBA_BIT			0x00000001
#define GLX_WINDOW_BIT			0x00000001
#define GLX_X_RENDERABLE		0x8012
#define GLX_SAMPLE_BUFFERS              0x186a0 /*100000*/
#define GLX_SAMPLES         0x186a1 /*100001*/
#define GLX_RGBA_TYPE 0x8014
#define GLX_X_VISUAL_TYPE		0x22
#define GLX_TRUE_COLOR			0x8002

  const int attrib_list[] = {
    GLX_X_RENDERABLE,	1,
    GLX_DOUBLEBUFFER, 1,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 24,
    GLX_STENCIL_SIZE, 8,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_SAMPLE_BUFFERS, 1,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_SAMPLES, 4,
    0
  };

  int out_ret = 0;
  GLXFBConfigs config = glXChooseFBConfig(d, x11.screen, attrib_list, &out_ret);
  if (out_ret == 0 || NULL == config) return false;
  XVisualInfo* glx_visual = glXGetVisualFromFBConfig(d, config[0]);
  if (NULL == glx_visual) return false;

  x11.glx_ctx = glXCreateContext(d, glx_visual, NULL, true);
  if (0 == x11.glx_ctx) return false;

  x11.glx_window = glXCreateWindow(x11.display, config[0], x11.window_handle, NULL);
  bool isOk = glXMakeCurrent(x11.display, x11.glx_window, x11.glx_ctx);
  if (false == isOk) return false;

  const char* title = window->title;
  if (title == NULL) title = "Brpl";
  int title_len = (int)strlen(title);
  Xutf8SetWMProperties(x11.display,
                       x11.window_handle,
                       title, title,
                       NULL, 0,
                       NULL, NULL, NULL);

  XSetWMProtocols(x11.display, x11.window_handle, &x11.WM_DELETE_WINDOW, 1);
  XMapWindow(d, x11.window_handle);
  XFlush(d);

  x11.im = XOpenIM(x11.display, 0, NULL, NULL);
  x11.ic = XCreateIC(x11.im,
                     XNInputStyle,
                     XIMPreeditNothing | XIMStatusNothing,
                     XNClientWindow,
                     x11.window_handle,
                     XNFocusWindow,
                     x11.window_handle,
                     NULL);

  int error = 0;
  major = 0, minor = 0;
  XQueryExtension(x11.display, "XInputExtension", &major, &minor, &error);
  LOGI("XINPUT: maj: %d, min: %d, error: %d", major, minor, error);

  XIEventMask evmask;
  unsigned char mask[(XI_LASTEVENT + 7)/8] = {0};
#define XIAllDevices                            0
  evmask.deviceid = XIAllDevices;
  evmask.mask_len = sizeof(mask);
  evmask.mask = mask;
  XISetMask(mask, XI_TouchBegin);
  XISetMask(mask, XI_TouchUpdate);
  XISetMask(mask, XI_TouchEnd);
  XISelectEvents(x11.display, x11.window_handle, &evmask, 1);

  brpl_window_x11_t* win = BR_MALLOC(sizeof(brpl_window_x11_t));
  memcpy(win, &x11, sizeof(x11));
  window->win = win;
  return true;
}

static void brpl_x11_close_window(brpl_window_t* window) {
  brpl_window_x11_t* win = window->win;
  glXMakeCurrent(win->display, None, NULL);
  glXDestroyWindow(win->display, win->glx_window);
  glXDestroyContext(win->display, win->glx_ctx);
  XDestroyWindow(win->display, win->window_handle);
  XFlush(win->display);
  XCloseDisplay(win->display);
  BR_FREE(win);
}

static int brpl_x11_keysym(XEvent event) {
  int keysym = XLookupKeysym(&event.xkey, 0);
  if (keysym >= 'a' && keysym <= 'z') {
    return keysym - 0x20;
  }
  switch (keysym) {
    case 0xFF08: return BR_KEY_BACKSPACE;
    case 0xFF09: return BR_KEY_TAB;
    case 0xFF0D: return BR_KEY_ENTER;
    case 0xFF50: return BR_KEY_HOME;
    case 0xFF51: return BR_KEY_LEFT;
    case 0xFF52: return BR_KEY_UP;
    case 0xFF53: return BR_KEY_RIGHT;
    case 0xFF54: return BR_KEY_DOWN;
    case 0xFF55: return BR_KEY_PAGE_UP;
    case 0xFF56: return BR_KEY_PAGE_DOWN;
    case 0xFF57: return BR_KEY_END;
    case 0xFF1B: return BR_KEY_ESCAPE;
    case 0xFFE1: return BR_KEY_LEFT_SHIFT;
    case 0xFFE3: return BR_KEY_LEFT_CONTROL;
    case 0xFFE7: return BR_KEY_LEFT_ALT;
    case 0xFFFF: return BR_KEY_DELETE;
    default: return keysym;
  }
}

static bool brpl_x11_load(brpl_window_t* win) {
  bool ok = br_x11_load() && br_gl_load() && br_glx_load() && br_xi_load();
  if (ok) {
    win->f.frame_start = brpl_x11_frame_start;
    win->f.frame_end =   brpl_x11_frame_end;
    win->f.event_next =  brpl_x11_event_next;
    win->f.window_open =  brpl_x11_open_window;
    win->f.window_close =  brpl_x11_close_window;
  }
  return ok;
}

#endif

// -------------------------------
// END OF X11 IMPL
// -------------------------------

// -------------------------------
// GLFW IMPL
// -------------------------------

#if BR_HAS_GLFW
typedef struct brpl_glfw_window_t {
  GLFWwindow* glfw;
  brpl_q_t q;
} brpl_glfw_window_t;

static void brpl_glfw_frame_start(brpl_window_t* win) {
  (void)win;
  glfwPollEvents();
}

static void brpl_glfw_frame_end(brpl_window_t* win) {
  brpl_glfw_window_t* gw = win->win;
  glfwSwapBuffers(gw->glfw);
}

static brpl_event_t brpl_glfw_event_next(brpl_window_t* win) {
  brpl_glfw_window_t* gw = win->win;
  brpl_event_t e = brpl_q_pop(&gw->q);
  if (e.kind == brpl_event_none) return (brpl_event_t) { .kind = brpl_event_next_frame, .time = brpl_time() };
  else return e;
}

static void brpl_glfw_error_callback(int error_code, const char* description) {
  LOGE("GLFW error %d: %s", error_code, description);
}
static void brpl_glfw_windowsizefun(GLFWwindow* window, int width, int height) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_window_resize, .size = BR_SIZE((float)width, (float)height) });
}
static void brpl_glfw_windowclosefun(GLFWwindow* window) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_close });
}
static void brpl_glfw_windowfocusfun(GLFWwindow* window, int focused) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = focused ? brpl_event_window_focused : brpl_event_window_unfocused });
}
static void brpl_glfw_windowcontentscalefun(GLFWwindow* window, float xscale, float yscale) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_scale, .size = BR_SIZE(xscale, yscale) });
}
static void brpl_glfw_mousebuttonfun(GLFWwindow* window, int button, int action, int mods) {
  (void)mods;
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  LOGI("mousebutton but=%d, action=%d", button, action);
  int ev = action == 0 ? brpl_event_mouse_release : brpl_event_mouse_press;
  int key  = button == 0 ? 0 : 3;
  brpl_q_push(&win->q, (brpl_event_t) { .kind = ev, .mouse_key = key });
}
static void brpl_glfw_cursorposfun(GLFWwindow* window, double xpos, double ypos) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_mouse_move, .pos = BR_VEC2((float)xpos, (float)ypos) });
}
static void brpl_glfw_scrollfun(GLFWwindow* window, double xoffset, double yoffset) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2((float)xoffset, (float)yoffset) });
}
static void brpl_glfw_keyfun(GLFWwindow* window, int key, int scancode, int action, int mods) {
  (void)mods;
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_event_kind_t event = action == 0 ? brpl_event_key_release : brpl_event_key_press;
  brpl_q_push(&win->q, (brpl_event_t) { .kind = event, .key = key, .keycode = scancode });
}
static void brpl_glfw_charfun(GLFWwindow* window, unsigned int codepoint) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_input, .utf8_char = codepoint });
}

static bool brpl_glfw_window_open(brpl_window_t* window) {
  bool ok = glfwInit();
  if (false == ok) {
    LOGE("Failed to initialize the glfw");
    return false;
  }
  glfwSetErrorCallback(brpl_glfw_error_callback);
  glfwDefaultWindowHints();
#if !defined(__EMSCRIPTEN__)
#define GLFW_CONTEXT_VERSION_MAJOR  0x00022002
#define GLFW_CONTEXT_VERSION_MINOR  0x00022003
#define GLFW_OPENGL_FORWARD_COMPAT  0x00022006
  glfwInitHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
  glfwInitHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwInitHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

  GLFWwindow* glfw_window = glfwCreateWindow(window->viewport.width, window->viewport.height, window->title, NULL, NULL);
  if (NULL == glfw_window) {
    LOGE("Failed to create glfw window");
    return false;
  }
  brpl_glfw_window_t* win = BR_CALLOC(1, sizeof(brpl_glfw_window_t));
  win->glfw = glfw_window;
  window->win = win;
  glfwSetWindowUserPointer(glfw_window, win);
  glfwSetWindowSizeCallback(glfw_window, brpl_glfw_windowsizefun);
  glfwSetWindowCloseCallback(glfw_window, brpl_glfw_windowclosefun);
  glfwSetWindowFocusCallback(glfw_window, brpl_glfw_windowfocusfun);
  glfwSetWindowContentScaleCallback(glfw_window, brpl_glfw_windowcontentscalefun);
  glfwSetMouseButtonCallback(glfw_window, brpl_glfw_mousebuttonfun);
  glfwSetCursorPosCallback(glfw_window, brpl_glfw_cursorposfun);
  glfwSetScrollCallback(glfw_window, brpl_glfw_scrollfun);
  glfwSetKeyCallback(glfw_window, brpl_glfw_keyfun);
  glfwSetCharCallback(glfw_window, brpl_glfw_charfun);
  glfwMakeContextCurrent(glfw_window);
  if (false == br_gl_load()) {
    LOGE("Failed to load gl.");
    return false;
  }
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_window_focused });
  return true;
}

void brpl_glfw_window_close(brpl_window_t* window) {
  BR_TODO("brpl_glfw_window_close");
}

// /usr/include/GLFW/glfw3.h
static bool brpl_glfw_load(brpl_window_t* win) {
  bool ok = br_glfw_load();
  win->f.frame_start = brpl_glfw_frame_start;
  win->f.frame_end   = brpl_glfw_frame_end;
  win->f.event_next  = brpl_glfw_event_next;
  win->f.window_open = brpl_glfw_window_open;
  win->f.window_close = brpl_glfw_window_close;
  return ok;
}

#endif // BR_HAS_GLFW

// -------------------------------
// END OF GLFW IMPL
// -------------------------------

// -------------------------------
// WIN32 IMPL
// -------------------------------
#if BR_HAS_WIN32

typedef struct brpl_win32_window_t {
  brpl_q_t q;
  HANDLE event_window_create_done;
  HWND hwnd;
  HDC hdc;
} brpl_win32_window_t;

static BR_THREAD_LOCAL brpl_q_t* brpl_win32_q;

static void brpl_win32_frame_start(brpl_window_t* win) {
}

static void brpl_win32_frame_end(brpl_window_t* win) {
  br_SwapBuffers(((brpl_win32_window_t*)win->win)->hdc);
}

static brpl_event_t brpl_win32_event_next(brpl_window_t* win) {
  brpl_win32_window_t* gw = win->win;
  brpl_event_t e = brpl_q_pop(&gw->q);
  if (e.kind == brpl_event_none) return (brpl_event_t) { .kind = brpl_event_next_frame, .time = brpl_time() };
  else if (e.kind == brpl_event_next_frame) return (brpl_event_t) { .kind = brpl_event_next_frame, .time = brpl_time() };
  else return e;
}

static bool brpl_win32_load(brpl_window_t* window) {
  window->f.frame_start = brpl_win32_frame_start;
  window->f.frame_end   = brpl_win32_frame_end;
  window->f.event_next  = brpl_win32_event_next;
  return true;
}

LRESULT CALLBACK brpl_win32_event_callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  brpl_event_t ev;
  bool release = 0;
  bool is_down = false;
  switch (uMsg) {
    case WM_SIZE: {
      int ww = LOWORD(lParam);
      int wh = HIWORD(lParam);
      brpl_q_push(brpl_win32_q, (brpl_event_t) { .kind = brpl_event_window_resize, .size = BR_SIZE((float)ww, (float)wh) });
    } break;
    case WM_CLOSE: {
      brpl_q_push(brpl_win32_q, (brpl_event_t) { .kind = brpl_event_close });
    } break;
    case WM_ERASEBKGND: break;
    case WM_PAINT:      break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP: {
      release = 1;
    } BR_FALLTHROUGH;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN: {
      if (release) ev.kind = brpl_event_mouse_release;
      else         ev.kind = brpl_event_mouse_press;
      switch (uMsg) {
        case WM_LBUTTONUP: case WM_LBUTTONDOWN: ev.mouse_key = 0; break;
        case WM_MBUTTONUP: case WM_MBUTTONDOWN: ev.mouse_key = 1; break;
        case WM_RBUTTONUP: case WM_RBUTTONDOWN: ev.mouse_key = 3; break;
      }
      brpl_q_push(brpl_win32_q, ev);
    } break;
    case WM_MOUSEMOVE: {
      br_vec2_t pos = BR_VEC2(LOWORD(lParam), HIWORD(lParam));
      brpl_q_push(brpl_win32_q, (brpl_event_t) { .kind = brpl_event_mouse_move, .pos = pos });
    } break;
    case WM_MOUSEWHEEL: {
      float wd = ((short)HIWORD(wParam)) / 120.f;
      brpl_q_push(brpl_win32_q, (brpl_event_t) { .kind = brpl_event_mouse_scroll, . vec = BR_VEC2(0, wd) });
    } break;
#if defined(WM_MOUSEHWHEEL)
    case WM_MOUSEHWHEEL: {
      float wd = ((short)HIWORD(wParam)) / 120.f;
      brpl_q_push(brpl_win32_q, (brpl_event_t) { .kind = brpl_event_mouse_scroll, . vec = BR_VEC2(wd, 0) });
    } break;
#endif
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN: {
      is_down = true;
    } BR_FALLTHROUGH;
    case WM_SYSKEYUP:
    case WM_KEYUP: {
      if (is_down) ev.kind = brpl_event_key_press;
      else         ev.kind = brpl_event_key_release;

      switch (wParam) {
        case 8:          ev.key = BR_KEY_BACKSPACE;    break;
        case 9:          ev.key = BR_KEY_TAB;          break;
        case VK_CONTROL: ev.key = BR_KEY_LEFT_CONTROL; break;
        case VK_SHIFT:   ev.key = BR_KEY_LEFT_SHIFT;   break;
        case VK_MENU:    ev.key = BR_KEY_LEFT_ALT;     break;
        case 13:         ev.key = BR_KEY_ENTER;        break;
        case 27:         ev.key = BR_KEY_ESCAPE;       break;
        case 33:         ev.key = BR_KEY_PAGE_UP;      break;
        case 34:         ev.key = BR_KEY_PAGE_DOWN;    break;
        case 35:         ev.key = BR_KEY_END;          break;
        case 36:         ev.key = BR_KEY_HOME;         break;
        case 37:         ev.key = BR_KEY_LEFT;         break;
        case 38:         ev.key = BR_KEY_UP;           break;
        case 39:         ev.key = BR_KEY_RIGHT;        break;
        case 46:         ev.key = BR_KEY_DELETE;       break;
        default:         ev.key = (int)wParam;         break;
      }
      ev.keycode = ev.key;
      brpl_q_push(brpl_win32_q, ev);
      return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    } break;
    case WM_CHAR: {
      br_u32 c = (br_u32)wParam;
      brpl_q_push(brpl_win32_q, (brpl_event_t) { .kind = brpl_event_input, .utf8_char = c });
    } break;
    default: {
      return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    } break;
  }
  return 0;
}

static unsigned long brpl_win32_window_event_loop(void* arg) {
  brpl_window_t* win = arg;
  brpl_win32_window_t* win32 = win->win;
  bool success = true;
  brpl_win32_q = &win32->q;

  WNDCLASS wc = {0};
  wc.style = 0; //CS_OWNDC;
  wc.lpfnWndProc = brpl_win32_event_callback;
  wc.hInstance = GetModuleHandleA(NULL);
  wc.lpszClassName = win->title;
  wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
  RegisterClassA(&wc);

  HWND hwnd = CreateWindowExA( WS_EX_APPWINDOW, wc.lpszClassName, win->title,
    WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SIZEBOX,
    CW_USEDEFAULT, CW_USEDEFAULT, win->viewport.width, win->viewport.height,
    NULL, NULL, wc.hInstance, NULL
  );

  if (0 == hwnd) {
    brpl_q_push(&win32->q, (brpl_event_t) { .kind = brpl_event_close });
    BR_ERROR("Failed to create a window");
  }

  HDC hdc = GetDC(hwnd);
  win32->hwnd = hwnd;
  win32->hdc = hdc;
  SetEvent(win32->event_window_create_done);

  MSG msg;
  while (false == win->should_close && GetMessageA(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
  }
  brpl_q_push(&win32->q, (brpl_event_t) { .kind = brpl_event_close });
  goto done;

error:
  SetEvent(win32->event_window_create_done);  // Notify main thread

done:
  if (hwnd) ReleaseDC(hwnd, hdc);
  return success == true;
}

static bool brpl_win32_open_window(brpl_window_t* window) {
  bool wgl_loaded = br_gdi_load() && br_wgl_load();
  if (false == wgl_loaded) {
    LOGE("Failed to load wgl");
    return false;
  }
  brpl_win32_window_t* win32 = BR_CALLOC(1, sizeof(brpl_win32_window_t));
  window->win = win32;
  win32->event_window_create_done = CreateEventA(NULL, FALSE, FALSE, NULL);

  br_thread_start(brpl_win32_window_event_loop, window);

  WaitForSingleObject(win32->event_window_create_done, INFINITE);
  CloseHandle(win32->event_window_create_done);

  PIXELFORMATDESCRIPTOR pfd = { 0 };
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 24;
  pfd.cStencilBits = 8;
  pfd.iLayerType = PFD_MAIN_PLANE;
  int format = br_ChoosePixelFormat(win32->hdc, &pfd);
  br_SetPixelFormat(win32->hdc, format, &pfd);
  HGLRC hRC = br_wglCreateContext(win32->hdc);
  br_wglMakeCurrent(win32->hdc, hRC);

  bool gl_loaded = br_gl_load();
  brpl_q_push(&win32->q, (brpl_event_t) { .kind = brpl_event_window_focused });
  return 0 != win32->hwnd && gl_loaded;
}

#endif // BR_HAS_WIN32

// -------------------------------
// END OF WIN32 IMPL
// -------------------------------
