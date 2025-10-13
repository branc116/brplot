#include <stdint.h>
#include "src/br_pp.h"

#define BR_WANTS_GL 1

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
#define BR_WANTS_X11 1
#include "external/X11/Xlib.h"
#include "external/X11/Xresource.h"
#include "external/X11/Xutil.h"
typedef Display* Displayp;
#endif

#if defined(BR_HAS_GLX)
#define BR_WANTS_GLX 1
typedef struct __GLXcontext* GLXContext;
typedef struct _GLXFBConfig _GLXFBConfig;
typedef _GLXFBConfig *GLXFBConfig;
typedef GLXFBConfig *GLXFBConfigs;
typedef XID GLXWindow;
typedef const char* ccharp_t;
typedef XVisualInfo* XVisualInfop;
typedef void* funcptr_t;
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
#elif defined(_WIN32)
#  include <Windows.h>
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
static bool brpl_x11_open_window(brpl_window_t* window);
#endif
#if BR_HAS_GLFW
static bool brpl_glfw_load(brpl_window_t* window);
static bool brpl_glfw_open_window(brpl_window_t* window);
#endif

static BR_THREAD_LOCAL struct {
  uint64_t start;
  double frequency;
  bool monotonic;
} brpl__time;

bool brpl_window_open(brpl_window_t* window) {
  brpl_time_init();
  switch (window->kind) {
#if BR_HAS_X11
    case brpl_window_x11: {
      if (false == brpl_x11_load(window)) return false;
      return brpl_x11_open_window(window);
    } break;
#endif
#if BR_HAS_GLFW
    case brpl_window_glfw: {
      if (false == brpl_glfw_load(window)) return false;
      return brpl_glfw_open_window(window);
    } break;
#endif
    default: {
      LOGF("Unknown kind: %d", window->kind);
      return false;
    } break;
  }
}
bool brpl_window_close(brpl_window_t* window) {
  BR_TODO("brpl_window_close");
}

void brpl_frame_start(brpl_window_t* window) {
  window->f.frame_start(window);
}

void brpl_frame_end(brpl_window_t* window) {
  window->f.frame_end(window);
}

brpl_event_t brpl_event_next(brpl_window_t* window) {
  brpl_event_t ev = window->f.event_next(window);
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
  return ev;
}

uint64_t brpl_timestamp(void) {
#if defined(_WIN32)
  uint64_t value;
  QueryPerformanceCounter(&value);
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
  return dlopen(path, RTLD_LAZY | RTLD_LOCAL);
#endif
}

void* brpl_load_symbol(void* library, const char* name) {
#if defined(_WIN32)
  return (void*)GetProcAddress((HMODULE) library, name);
#else
  return (void*)dlsym(library, name);
#endif
}

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

#include <assert.h>                                               
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

#define GLX_USE_GL		1
#define GLX_BUFFER_SIZE		2
#define GLX_LEVEL		3
#define GLX_RGBA		4
#define GLX_DOUBLEBUFFER	5
#define GLX_STEREO		6
#define GLX_AUX_BUFFERS		7
#define GLX_RED_SIZE		8
#define GLX_GREEN_SIZE		9
#define GLX_BLUE_SIZE		10
#define GLX_ALPHA_SIZE		11
#define GLX_DEPTH_SIZE		12
#define GLX_STENCIL_SIZE	13
#define GLX_ACCUM_RED_SIZE	14
#define GLX_ACCUM_GREEN_SIZE	15
#define GLX_ACCUM_BLUE_SIZE	16
#define GLX_ACCUM_ALPHA_SIZE	17

typedef struct brpl_window_x11_t {
  void* display;
  int screen;
  int root;
  int context;
  int parent;
  int window_handle;
  void* glx_ctx;
  int glx_window;

  XIM im;
  XIC ic;

  Atom NET_WM_NAME;
  Atom UTF8_STRING;
  Atom NET_WM_ICON_NAME;
} brpl_window_x11_t;

static bool brpl_x11_open_window(brpl_window_t* window) {
  brpl_window_x11_t x11 = { 0 };
  void* d = x11.display = XOpenDisplay(NULL);
  if (NULL == x11.display) {
    LOGE("Failed to open x11 window. display is null.");
    return false;
  }
  x11.NET_WM_NAME = XInternAtom(d, "_NET_WM_NAME", false);
  x11.UTF8_STRING = XInternAtom(d, "UTF8_STRING", false);
  x11.NET_WM_ICON_NAME = XInternAtom(d, "_NET_WM_ICON_NAME", false);
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
  int xpos = 0, ypos = 0, width = 800, height = 600;
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
  BR_ASSERT(x11.window_handle);

  int major, minor;
  if (!glXQueryVersion(d, &major, &minor)) return false;
  LOGI("glx.version= %d.%d", major, minor);

  int native_count = 0;
  int valid_count = 0;
  int last_good = 0;
  GLXFBConfigs nativeConfigs = glXGetFBConfigs(d, x11.screen, &native_count);
  for (int i = 0; i < native_count; ++i) {
    GLXFBConfig n = nativeConfigs[i];
    int value = 0;

#define GLX_DRAWABLE_TYPE 0x8010
#define GLX_RENDER_TYPE 0x8011
#define GLX_RGBA_BIT			0x00000001
#define GLX_WINDOW_BIT			0x00000001
    glXGetFBConfigAttrib(d, n, GLX_RENDER_TYPE, &value);
    if (!(value & GLX_RGBA_BIT)) continue;

    glXGetFBConfigAttrib(d, n, GLX_DRAWABLE_TYPE, &value);
    if (!(value & GLX_WINDOW_BIT)) continue;

    glXGetFBConfigAttrib(d, n, GLX_DOUBLEBUFFER, &value);
    if (!value) continue;
    LOGI("DB: %d", value);

    last_good = i;
    valid_count += 1;
  }
  BR_ASSERT(native_count);
  LOGI("%d -> %d (%d)", native_count, valid_count, last_good);

#define GLX_RGBA_TYPE 0x8014
  x11.glx_ctx = glXCreateNewContext(d, nativeConfigs[last_good], GLX_RGBA_TYPE, NULL, True);
  BR_ASSERT(x11.glx_ctx);

  x11.glx_window = glXCreateWindow(x11.display, nativeConfigs[last_good], x11.window_handle, NULL);
  XFree(nativeConfigs);
  bool isOk = glXMakeCurrent(x11.display, x11.glx_window, x11.glx_ctx);
  BR_ASSERT(isOk);

  const char* title = window->title;
  if (title == NULL) title = "Brpl";
  size_t title_len = strlen(title);
  Xutf8SetWMProperties(x11.display,
                       x11.window_handle,
                       title, title,
                       NULL, 0,
                       NULL, NULL, NULL);

  XChangeProperty(x11.display,  x11.window_handle,
                  x11.NET_WM_NAME, x11.UTF8_STRING, 8,
                  PropModeReplace,
                  (unsigned char*) title, title_len);

  XChangeProperty(x11.display,  x11.window_handle,
                  x11.NET_WM_ICON_NAME, x11.UTF8_STRING, 8,
                  PropModeReplace,
                  (unsigned char*) title, title_len);
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

#if 0
  if (window->x11.ic)
  {
      XWindowAttributes attribs;
      XGetWindowAttributes(_glfw.x11.display, window->x11.handle, &attribs);

      unsigned long filter = 0;
      if (XGetICValues(window->x11.ic, XNFilterEvents, &filter, NULL) == NULL)
      {
          XSelectInput(_glfw.x11.display,
                       window->x11.handle,
                       attribs.your_event_mask | filter);
      }
  }
#endif

  brpl_window_x11_t* win = BR_MALLOC(sizeof(brpl_window_x11_t));
  memcpy(win, &x11, sizeof(x11));
  window->win = win;
  return true;
}

static void brpl_x11_frame_start(brpl_window_t* window) {
  (void)window;
}

static void brpl_x11_frame_end(brpl_window_t* window) {
  brpl_window_x11_t* w = window->win;
  glXSwapBuffers(w->display, w->glx_window);
}

static int brpl_x11_keysym(XEvent event) {
  int keysym = XLookupKeysym(&event.xkey, 0);
  if (keysym >= 'a' && keysym <= 'z') {
    LOGI("Big");
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

static brpl_event_t brpl_x11_event_next(brpl_window_t* window) {
  // TODO: Move this struct to a window_x11_t struct;
  static BR_THREAD_LOCAL struct {
    int* arr;
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
  switch (event.type) {
    case MotionNotify: {
      XMotionEvent m = event.xmotion;
      return (brpl_event_t) { .kind = brpl_event_mouse_move, .pos = BR_VEC2(m.x, m.y) };
    } break;
    case KeyPress: {
      const int keycode = event.xkey.keycode;
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
          chars = BR_CALLOC(count + 1, 1);
          count = Xutf8LookupString(w->ic,
                                    &event.xkey,
                                    chars, count,
                                    NULL, &status);
      }

      if (status == XLookupChars || status == XLookupBoth)
      {
          utf8_chars.len = 0;
          utf8_chars.read_index = 0;

          br_strv_t s = BR_STRV(chars, count);
          BR_STRV_FOREACH_UTF8(s, value) br_da_push(utf8_chars, value);
      }

      if (chars != buffer) BR_FREE(chars);

      return (brpl_event_t) { .kind = brpl_event_key_press, .key = keysym, .keycode = keycode };
    } break;
    case KeyRelease: {
      const int keycode = event.xkey.keycode;
      int keysym = brpl_x11_keysym(event);
      LOGI("Key Release: %d hint: %d", event.xkey.state, event.xkey.keycode);
      return (brpl_event_t) {
        .kind = brpl_event_key_release,
        .key = keysym,
        .keycode = keycode
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
      else                                      return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = event.xbutton.button - Button1 - 4 };
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
    case ButtonRelease: {
      if (event.xbutton.button <= Button3) return (brpl_event_t) { .kind = brpl_event_mouse_release, .mouse_key = event.xbutton.button - Button1 };
      else if (event.xbutton.button <= 7)  return (brpl_event_t) { .kind = brpl_event_nop };
      else                                 return (brpl_event_t) { .kind = brpl_event_mouse_release, .mouse_key = event.xbutton.button - Button1 - 4 };
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
        .size = BR_SIZE(ce.width, ce.height)
      };
    } break;
    case PropertyNotify: {
      LOGI("Prop %lu changed: %d", event.xproperty.atom, event.xproperty.state);
    } break;
    case Expose: {
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
    default: {
      LOGI("Unknown x11 event type: %d", event.type); 
    } break;
  }

  return (brpl_event_t) { .kind = brpl_event_next_frame, .time = brpl_time() };
}

static bool brpl_x11_load(brpl_window_t* win) {
  bool ok = br_x11_load() && br_gl_load() && br_glx_load();
  if (ok) {
    win->f.frame_start = brpl_x11_frame_start;
    win->f.frame_end =   brpl_x11_frame_end;
    win->f.event_next =  brpl_x11_event_next;
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

// /usr/include/GLFW/glfw3.h
static bool brpl_glfw_load(brpl_window_t* win) {
  bool ok = br_glfw_load();
  win->f.frame_start = brpl_glfw_frame_start;
  win->f.frame_end   = brpl_glfw_frame_end;
  win->f.event_next  = brpl_glfw_event_next;
  return ok;
}

static void brpl_glfw_error_callback(int error_code, const char* description) {
  LOGE("GLFW error %d: %s", error_code, description);
}
static void brpl_glfw_windowsizefun(GLFWwindow* window, int width, int height) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_window_resize, .size = BR_SIZE(width, height) });
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
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  LOGI("mousebutton but=%d, action=%d", button, action);
  int ev = action == 0 ? brpl_event_mouse_release : brpl_event_mouse_press;
  int key  = button == 0 ? 0 : 3;
  brpl_q_push(&win->q, (brpl_event_t) { .kind = ev, .mouse_key = key });
}
void brpl_glfw_cursorposfun(GLFWwindow* window, double xpos, double ypos) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_mouse_move, .pos = BR_VEC2(xpos, ypos) });
}
void brpl_glfw_scrollfun(GLFWwindow* window, double xoffset, double yoffset) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(xoffset, yoffset) });
}
void brpl_glfw_keyfun(GLFWwindow* window, int key, int scancode, int action, int mods) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_event_kind_t event = action == 0 ? brpl_event_key_release : brpl_event_key_press;
  brpl_q_push(&win->q, (brpl_event_t) { .kind = event, .key = key, .keycode = scancode });
}
void brpl_glfw_charfun(GLFWwindow* window, unsigned int codepoint) {
  brpl_glfw_window_t* win = glfwGetWindowUserPointer(window);
  brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_input, .utf8_char = codepoint });
}

static bool brpl_glfw_open_window(brpl_window_t* window) {
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
  brpl_glfw_window_t* win = BR_CALLOC(sizeof(brpl_glfw_window_t), 1);
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
  return true;
}
#endif
// -------------------------------
// END OF GLFW IMPL
// -------------------------------
