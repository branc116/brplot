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

#define BR_WANTS_X11 1
#define brpl_x11_Bool int
#define brpl_x11_Status int
#define brpl_x11_None 0L
#define brpl_x11_XBufferOverflow -1
#define brpl_x11_XLookupChars    2
#define brpl_x11_XLookupBoth    4
#define brpl_x11_InputOutput    1
#define brpl_x11_Button1      1
#define brpl_x11_Button2      2
#define brpl_x11_Button3      3
#define brpl_x11_Button4      4
#define brpl_x11_Button5      5
#define brpl_x11_KeyPressMask      (1L<<0)
#define brpl_x11_KeyReleaseMask      (1L<<1)
#define brpl_x11_ButtonPressMask      (1L<<2)
#define brpl_x11_ButtonReleaseMask    (1L<<3)
#define brpl_x11_EnterWindowMask      (1L<<4)
#define brpl_x11_LeaveWindowMask      (1L<<5)
#define brpl_x11_PointerMotionMask    (1L<<6)
#define brpl_x11_PointerMotionHintMask    (1L<<7)
#define brpl_x11_Button1MotionMask    (1L<<8)
#define brpl_x11_Button2MotionMask    (1L<<9)
#define brpl_x11_Button3MotionMask    (1L<<10)
#define brpl_x11_Button4MotionMask    (1L<<11)
#define brpl_x11_Button5MotionMask    (1L<<12)
#define brpl_x11_ButtonMotionMask    (1L<<13)
#define brpl_x11_KeymapStateMask      (1L<<14)
#define brpl_x11_ExposureMask      (1L<<15)
#define brpl_x11_VisibilityChangeMask    (1L<<16)
#define brpl_x11_StructureNotifyMask    (1L<<17)
#define brpl_x11_ResizeRedirectMask    (1L<<18)
#define brpl_x11_SubstructureNotifyMask    (1L<<19)
#define brpl_x11_SubstructureRedirectMask  (1L<<20)
#define brpl_x11_FocusChangeMask      (1L<<21)
#define brpl_x11_PropertyChangeMask    (1L<<22)
#define brpl_x11_ColormapChangeMask    (1L<<23)
#define brpl_x11_OwnerGrabButtonMask    (1L<<24)

#define brpl_x11_CWBorderPixel           (1L<<3)
#define brpl_x11_CWEventMask    (1L<<11)
#define brpl_x11_CWColormap    (1L<<13)

#define brpl_x11_XNInputStyle "inputStyle"
#define brpl_x11_XNClientWindow "clientWindow"
#define brpl_x11_XNFocusWindow "focusWindow"

#define brpl_x11_XIMStatusNothing  0x0400L
#define brpl_x11_XIMPreeditNothing  0x0008L

typedef struct _XDisplay brpl_x11_Display;
typedef struct _XIM *brpl_x11_XIM;
typedef struct _XIC *brpl_x11_XIC;
typedef brpl_x11_Display* brpl_x11_Displayp;
typedef unsigned long brpl_x11_Time;
typedef unsigned long brpl_x11_XID;
typedef unsigned long brpl_x11_VisualID;
typedef unsigned long brpl_x11_Atom;
typedef int brpl_x11_XrmQuark, *brpl_x11_XrmQuarkList;
typedef char *brpl_x11_XPointer;
typedef brpl_x11_XID brpl_x11_Window;
typedef brpl_x11_XID brpl_x11_Pixmap;
typedef brpl_x11_XID brpl_x11_KeySym;
typedef brpl_x11_XID brpl_x11_Colormap;
typedef brpl_x11_XID brpl_x11_Cursor;

typedef void* brpl_x11_Visual;
typedef brpl_x11_Visual* brpl_x11_Visualp;
typedef struct {
  brpl_x11_Visual *visual;
  brpl_x11_VisualID visualid;
  int screen;
  int depth;
  int c_class;
  unsigned long red_mask;
  unsigned long green_mask;
  unsigned long blue_mask;
  int colormap_size;
  int bits_per_rgb;
} brpl_x11_XVisualInfo;

typedef struct {
    brpl_x11_Pixmap background_pixmap;  /* background or brpl_x11_None or ParentRelative */
    unsigned long background_pixel;  /* background pixel */
    brpl_x11_Pixmap border_pixmap;  /* border of the window */
    unsigned long border_pixel;  /* border pixel value */
    int bit_gravity;    /* one of bit gravity values */
    int win_gravity;    /* one of the window gravity values */
    int backing_store;    /* NotUseful, WhenMapped, Always */
    unsigned long backing_planes;/* planes to be preserved if possible */
    unsigned long backing_pixel;/* value to use in restoring planes */
    brpl_x11_Bool save_under;    /* should bits under be saved? (popups) */
    long event_mask;    /* set of events that should be saved */
    long do_not_propagate_mask;  /* set of events that should not propagate */
    brpl_x11_Bool override_redirect;  /* boolean value for override-redirect */
    brpl_x11_Colormap colormap;    /* color map to be associated with window */
    brpl_x11_Cursor cursor;    /* cursor to be displayed (or brpl_x11_None) */
} brpl_x11_XSetWindowAttributes;

typedef struct {
  long flags;  /* marks which fields in this structure are defined */
  int x, y;    /* obsolete for new window mgrs, but clients */
  int width, height;  /* should set so old wm's don't mess up */
  int min_width, min_height;
  int max_width, max_height;
  int width_inc, height_inc;
  struct {
    int x;  /* numerator */
    int y;  /* denominator */
  } min_aspect, max_aspect;
  int base_width, base_height;    /* added by ICCCM version 1 */
  int win_gravity;      /* added by ICCCM version 1 */
} brpl_x11_XSizeHints;

typedef struct {
  long flags;  /* marks which fields in this structure are defined */
  brpl_x11_Bool input;  /* does this application rely on the window manager to
      get keyboard input? */
  int initial_state;  /* see below */
  brpl_x11_Pixmap icon_pixmap;  /* pixmap to be used as icon */
  brpl_x11_Window icon_window;   /* window to be used as icon */
  int icon_x, icon_y;   /* initial position of icon */
  brpl_x11_Pixmap icon_mask;  /* icon mask bitmap */
  brpl_x11_XID window_group;  /* id of related window group */
  /* this structure may be extended in the future */
} brpl_x11_XWMHints;

typedef struct {
  int            type;       /* of event. Always GenericEvent */
  unsigned long  serial;     /* # of last request processed */
  brpl_x11_Bool           send_event; /* true if from SendEvent request */
  brpl_x11_Display        *display;   /* brpl_x11_Display the event was read from */
  union {
    int    extension; /* major opcode of extension that caused the event */
    brpl_x11_Window window;    /* "event" window it is reported relative to */
  };
  union {
    int    evtype;       /* actual event type. */
    brpl_x11_Window root;         /* root window that the event occurred on */
    brpl_x11_Atom   atom;
    brpl_x11_Atom   message_type;
  };
} brpl_x11_XGenericEvent;

typedef struct {
  int type;
  brpl_x11_Display *display;  /* brpl_x11_Display the event was read from */
  brpl_x11_XID resourceid;    /* resource id */
  unsigned long serial;  /* serial number of failed request */
  unsigned char error_code;  /* error code of failed request */
  unsigned char request_code;  /* Major op-code of failed request */
  unsigned char minor_code;  /* Minor op-code of failed request */
} brpl_x11_XErrorEvent;

typedef struct {
  char *res_name;
  char *res_class;
} brpl_x11_XClassHint;

typedef struct _XComposeStatus {
    brpl_x11_XPointer compose_ptr;  /* state table pointer */
    int chars_matched;    /* match state */
} brpl_x11_XComposeStatus;

typedef int (*brpl_x11_XErrorHandler)(brpl_x11_Display* d, brpl_x11_XErrorEvent* error);

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

#define BR_WANTS_XI 1
#define BR_HAS_XI 1

#define brpl_x11_KeyPress         2
#define brpl_x11_KeyRelease       3
#define brpl_x11_ButtonPress      4
#define brpl_x11_ButtonRelease    5
#define brpl_x11_MotionNotify     6
#define brpl_x11_EnterNotify      7
#define brpl_x11_LeaveNotify      8
#define brpl_x11_FocusIn          9
#define brpl_x11_FocusOut        10
#define brpl_x11_Expose          12
#define brpl_x11_ConfigureNotify 22
#define brpl_x11_PropertyNotify  28
#define brpl_x11_ClientMessage   33

#define brpl_x11_XI_TouchBegin  18 /* XI 2.2 */
#define brpl_x11_XI_TouchUpdate 19
#define brpl_x11_XI_TouchEnd    20

typedef struct {
    int                 deviceid;
    int                 mask_len;
    unsigned char*      mask;
} brpl_x11_XIEventMask;

typedef struct {
  brpl_x11_XGenericEvent generic;
  brpl_x11_Time   time;
  int    deviceid, sourceid, detail;
  brpl_x11_Window root, event, child;
  double root_x, root_y;
  double event_x, event_y;
  int    flags;
  struct {
    int mask_len;
    br_u64* mask;
  } btn;
} brpl_x11_XIDeviceEvent;

typedef struct {
  brpl_x11_XGenericEvent generic;
  brpl_x11_Window subwindow;  /* child window */
  brpl_x11_Time time;    /* milliseconds */
  int x, y;    /* pointer x, y coordinates in event window */
  // TODO: Maybe use x,y_root insted of x,y
  int x_root, y_root;  /* coordinates relative to root */
  unsigned int state;  /* key or button mask */
  unsigned int keycode;  /* detail */
  brpl_x11_Bool same_screen;  /* same screen flag */
} brpl_x11_XKeyEvent;
typedef brpl_x11_XKeyEvent brpl_x11_XKeyPressedEvent;

typedef struct {
  brpl_x11_XGenericEvent generic;
  brpl_x11_Window subwindow;  /* child window */
  brpl_x11_Time time;    /* milliseconds */
  int x, y;    /* pointer x, y coordinates in event window */
  int x_root, y_root;  /* coordinates relative to root */
  unsigned int state;  /* key or button mask */
  unsigned int button;  /* detail */
  brpl_x11_Bool same_screen;  /* same screen flag */
} brpl_x11_XButtonEvent;

typedef struct {
  brpl_x11_XGenericEvent generic;
  brpl_x11_Window subwindow;  /* child window */
  brpl_x11_Time time;    /* milliseconds */
  int x, y;    /* pointer x, y coordinates in event window */
  int x_root, y_root;  /* coordinates relative to root */
  unsigned int state;  /* key or button mask */
  char is_hint;    /* detail */
  brpl_x11_Bool same_screen;  /* same screen flag */
} brpl_x11_XMotionEvent;

typedef struct {
  brpl_x11_XGenericEvent generic;
  int x, y;
  int width, height;
  int border_width;
  brpl_x11_Window above;
  brpl_x11_Bool override_redirect;
} brpl_x11_XConfigureEvent;

typedef struct {
  brpl_x11_XGenericEvent generic;
  brpl_x11_Time time;
  int state;    /* NewValue, Deleted */
} brpl_x11_XPropertyEvent;

typedef struct {
  brpl_x11_XGenericEvent generic;
  int format;
  long msg;
} brpl_x11_XClientMessageEvent;

typedef struct {
  brpl_x11_XGenericEvent generic;
  unsigned int   cookie;
  void           *data;
} brpl_x11_XGenericEventCookie;

typedef union _XEvent {
  int type; /* must not be changed; first element */
  brpl_x11_XKeyEvent xkey;
  brpl_x11_XButtonEvent xbutton;
  brpl_x11_XMotionEvent xmotion;
  brpl_x11_XConfigureEvent xconfigure;
  brpl_x11_XPropertyEvent xproperty;
  brpl_x11_XClientMessageEvent xclient;
  brpl_x11_XGenericEventCookie xcookie;
  long pad[24];
} brpl_x11_XEvent;

#endif

#if defined(BR_HAS_GLX)
#  define BR_WANTS_GLX 1
   typedef struct __GLXcontext* GLXContext;
   typedef struct _GLXFBConfig _GLXFBConfig;
   typedef _GLXFBConfig *GLXFBConfig;
   typedef GLXFBConfig *GLXFBConfigs;
   typedef brpl_x11_XID GLXWindow;
   typedef const char* ccharp_t;
   typedef brpl_x11_XVisualInfo* XVisualInfop;
   typedef void* funcptr_t;
GLXContext (*glXCreateContextAttribsARB)(brpl_x11_Display* dpy, GLXFBConfig config, GLXContext share_context, brpl_x11_Bool direct, const int* attrib_list);
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
    case brpl_window_win32: loaded = brpl_win32_load(window); break;
#endif
    default: LOGE("Unknown kind: %d", window->kind); break;
  }
  return loaded && window->f.window_open && window->f.window_open(window);
}

void brpl_window_close(brpl_window_t* window) {
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
    case brpl_event_touch_begin: { LOGI("brpl_event_touch_begin: id=%d", ev.touch.id); } break;
    case brpl_event_touch_end: { LOGI("brpl_event_touch_end: id=%d", ev.touch.id); } break;
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
  brpl_x11_Window root;
  brpl_x11_XrmQuark context;
  brpl_x11_Window window_handle;
  void* glx_ctx;
  GLXWindow glx_window;

  brpl_x11_XIM im;
  brpl_x11_XIC ic;

  brpl_x11_Atom WM_DELETE_WINDOW;

  int xi_opcode;
} brpl_window_x11_t;

static void brpl_x11_frame_start(brpl_window_t* window) {
  (void)window;
}

static void brpl_x11_frame_end(brpl_window_t* window) {
  brpl_window_x11_t* w = window->win;
  glXSwapBuffers(w->display, w->glx_window);
}

static int brpl_x11_keysym(brpl_x11_XEvent event);
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
  brpl_x11_Display* d = w->display;

  if (0 == brpl_x11_XQLength(d)) {
    brpl_x11_XFlush(d);
    return (brpl_event_t) { .kind = brpl_event_next_frame, .time = brpl_time() };
  }

  brpl_x11_XEvent event;
  brpl_x11_XNextEvent(d, &event);

  if (w->xi_opcode == event.xcookie.generic.extension) {
    brpl_x11_XGetEventData(w->display, &event.xcookie);
    brpl_x11_XIDeviceEvent* de = (brpl_x11_XIDeviceEvent*)event.xcookie.data;
    brpl_event_t e = { .kind = brpl_event_nop };
    switch (event.xcookie.generic.evtype) {
      case brpl_x11_XI_TouchBegin: {
        e.kind = brpl_event_touch_begin;
        e.touch.id = de->detail;
        e.touch.pos = BR_VEC2((float)de->event_x, (float)de->event_y);
      } break;
      case brpl_x11_XI_TouchUpdate: {
        e.kind = brpl_event_touch_update;
        e.touch.id = de->detail;
        e.touch.pos = BR_VEC2((float)de->event_x, (float)de->event_y);
      } break;
      case brpl_x11_XI_TouchEnd: {
        e.kind = brpl_event_touch_end;
        e.touch.id = de->detail;
        e.touch.pos = BR_VEC2((float)de->event_x, (float)de->event_y);
      } break;
    }
    brpl_x11_XFreeEventData(w->display, &event.xcookie);
    return e;
  }
  switch (event.type) {
    case brpl_x11_MotionNotify: {
      brpl_x11_XMotionEvent m = event.xmotion;
      return (brpl_event_t) { .kind = brpl_event_mouse_move, .pos = BR_VEC2((float)m.x, (float)m.y) };
    } break;
    case brpl_x11_KeyPress: {
      const br_u32 keycode = event.xkey.keycode;
      int keysym = brpl_x11_keysym(event);
      int count;
      brpl_x11_Status status;
      char buffer[128];
      char* chars = buffer;

      count = brpl_x11_Xutf8LookupString(w->ic,
                                &event.xkey,
                                buffer, sizeof(buffer) - 1,
                                NULL, &status);

      if (status == brpl_x11_XBufferOverflow)
      {
          chars = BR_CALLOC((size_t)(count + 1), 1);
          count = brpl_x11_Xutf8LookupString(w->ic,
                                    &event.xkey,
                                    chars, count,
                                    NULL, &status);
      }

      if (status == brpl_x11_XLookupChars || status == brpl_x11_XLookupBoth)
      {
          utf8_chars.len = 0;
          utf8_chars.read_index = 0;

          br_strv_t s = BR_STRV(chars, (br_u32)count);
          BR_STRV_FOREACH_UTF8(s, value) br_da_push(utf8_chars, value);
      }

      if (chars != buffer) BR_FREE(chars);

      return (brpl_event_t) { .kind = brpl_event_key_press, .key = keysym, .keycode = (br_i32)keycode };
    } break;
    case brpl_x11_KeyRelease: {
      const br_u32 keycode = event.xkey.keycode;
      int keysym = brpl_x11_keysym(event);
      return (brpl_event_t) {
        .kind = brpl_event_key_release,
        .key = keysym,
        .keycode = (br_i32)keycode
      };
    } break;
    case brpl_x11_ButtonPress: {
      if (event.xbutton.button == brpl_x11_Button1)      return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = 0 };
      else if (event.xbutton.button == brpl_x11_Button2) return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = 1 };
      else if (event.xbutton.button == brpl_x11_Button3) return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = 3 };
      else if (event.xbutton.button == brpl_x11_Button4) return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(0, 1) };
      else if (event.xbutton.button == brpl_x11_Button5) return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(0, -1) };
      else if (event.xbutton.button == 6)       return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(1, 0) };
      else if (event.xbutton.button == 7)       return (brpl_event_t) { .kind = brpl_event_mouse_scroll, .vec = BR_VEC2(-1, 0) };
      else                                      return (brpl_event_t) { .kind = brpl_event_mouse_press, .mouse_key = (br_i32)event.xbutton.button - brpl_x11_Button1 - 4 };
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
    case brpl_x11_ButtonRelease: {
      if (event.xbutton.button <= brpl_x11_Button3) return (brpl_event_t) { .kind = brpl_event_mouse_release, .mouse_key = (br_i32)event.xbutton.button - brpl_x11_Button1 };
      else if (event.xbutton.button <= 7)  return (brpl_event_t) { .kind = brpl_event_nop };
      else                                 return (brpl_event_t) { .kind = brpl_event_mouse_release, .mouse_key = (br_i32)event.xbutton.button - brpl_x11_Button1 - 4 };
    } break;
    case brpl_x11_EnterNotify: {
      return (brpl_event_t) {
        .kind = brpl_event_window_focused,
      };
    } break;
    case brpl_x11_LeaveNotify: {
      return (brpl_event_t) {
        .kind = brpl_event_window_unfocused,
      };
    } break;
    case brpl_x11_FocusIn: {
      return (brpl_event_t) {
        .kind = brpl_event_window_focused,
      };
    } break;
    case brpl_x11_FocusOut: {
      return (brpl_event_t) {
        .kind = brpl_event_window_unfocused,
      };
    } break;
    case brpl_x11_ConfigureNotify: {
      brpl_x11_XConfigureEvent ce = event.xconfigure;
      return (brpl_event_t) {
        .kind = brpl_event_window_resize,
        .size = BR_SIZE((float)ce.width, (float)ce.height)
      };
    } break;
    case brpl_x11_PropertyNotify: {
      LOGI("Prop %lu changed: %d", event.xproperty.generic.atom, event.xproperty.state);
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
    case brpl_x11_Expose: {
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
    case brpl_x11_ClientMessage: {
      if ((brpl_x11_Atom)event.xclient.msg == w->WM_DELETE_WINDOW) {
        return (brpl_event_t) { .kind = brpl_event_close };
      } else {
        return (brpl_event_t) { .kind = brpl_event_nop };
      }
    } break;
    default: {
      LOGI("Unknown x11 event type: %d", event.type);
      return (brpl_event_t) { .kind = brpl_event_nop };
    } break;
  }
}

int brpl_x11_error_callback(brpl_x11_Display* d, brpl_x11_XErrorEvent* e) {
    char err_text[1024];
    brpl_x11_XGetErrorText(d, e->error_code, err_text, sizeof(err_text));
    LOGE("X Error: %s", err_text);
    return 0;
}

static bool brpl_x11_get_set_context(brpl_window_x11_t* x11, int* attrib_list, int major, int minor) {
  int out_ret = 0;
  GLXFBConfigs configs = glXChooseFBConfig(x11->display, x11->screen, attrib_list, &out_ret);
  if (out_ret == 0 || NULL == configs) return false;
  bool is_ok = false;
  brpl_x11_XVisualInfo* glx_visual = NULL;
#define GLX_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB     0x2092
#define GLX_CONTEXT_PROFILE_MASK_ARB      0x9126
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define GLX_CONTEXT_FLAGS_ARB             0x2094
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_FLAGS_ARB             0x2094
#define GLX_CONTEXT_DEBUG_BIT_ARB         0x00000001
  int context_attribs[] = {
      GLX_CONTEXT_MAJOR_VERSION_ARB, major,
      GLX_CONTEXT_MINOR_VERSION_ARB, minor,
      GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
      GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      // Important for RenderDoc:
      GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
      0
  };
  if (glXCreateContextAttribsARB == NULL) glXCreateContextAttribsARB = glXGetProcAddressARB("glXCreateContextAttribsARB");

  for (int i = 0; i < out_ret; ++i) {
    GLXFBConfig config = configs[i];

    glx_visual = glXGetVisualFromFBConfig(x11->display, config);
    if (NULL == glx_visual) continue;

    x11->glx_window = glXCreateWindow(x11->display, config, x11->window_handle, (int[]) { brpl_x11_None });
    if (0 == x11->glx_window) continue;

    x11->glx_ctx = glXCreateContextAttribsARB(x11->display, config, 0, true, context_attribs);
    if (0 == x11->glx_ctx) continue;

    is_ok = glXMakeCurrent(x11->display, x11->glx_window, x11->glx_ctx);
    if (is_ok) break;

    glXDestroyContext(x11->display, x11->glx_ctx);
    glXDestroyWindow(x11->display, x11->glx_window);
    brpl_x11_XFree(glx_visual);
  }
  brpl_x11_XFree(configs);
  brpl_x11_XFree(glx_visual);
  return is_ok;
}

static bool brpl_x11_open_window(brpl_window_t* window) {
  brpl_window_x11_t x11 = { 0 };
  brpl_x11_XSetErrorHandler(brpl_x11_error_callback);
  void* d = x11.display = brpl_x11_XOpenDisplay(NULL);
  if (NULL == x11.display) {
    LOGE("Failed to open x11 window. display is null.");
    return false;
  }
  x11.WM_DELETE_WINDOW = brpl_x11_XInternAtom(d, "WM_DELETE_WINDOW", false);
  x11.screen = brpl_x11_XDefaultScreen(x11.display);
  x11.root = brpl_x11_XRootWindow(x11.display, x11.screen);
  x11.context = brpl_x11_XrmUniqueQuark();

  brpl_x11_Visual* visual = brpl_x11_XDefaultVisual(x11.display, x11.screen);
  int depth = brpl_x11_XDefaultDepth(x11.display, x11.screen);
  brpl_x11_XSetWindowAttributes wa = { 0 };
  wa.event_mask = brpl_x11_StructureNotifyMask | brpl_x11_KeyPressMask | brpl_x11_KeyReleaseMask |
                  brpl_x11_PointerMotionMask | brpl_x11_ButtonPressMask | brpl_x11_ButtonReleaseMask |
                  brpl_x11_ExposureMask | brpl_x11_FocusChangeMask | brpl_x11_VisibilityChangeMask |
                  brpl_x11_EnterWindowMask | brpl_x11_LeaveWindowMask | brpl_x11_PropertyChangeMask;
  br_i32 xpos = 0, ypos = 0;
  br_u32 width = 800, height = 600;
  x11.window_handle = brpl_x11_XCreateWindow(x11.display,
                                    x11.root,
                                    xpos, ypos,
                                    width, height,
                                    0,      // Border width
                                    depth,  // Color depth
                                    brpl_x11_InputOutput,
                                    visual,
                                    brpl_x11_CWBorderPixel | brpl_x11_CWColormap | brpl_x11_CWEventMask,
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

  int attrib_list[] = {
    GLX_SAMPLE_BUFFERS, 1,
    GLX_X_RENDERABLE,	1,
    GLX_DOUBLEBUFFER, 1,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 0,
    GLX_DEPTH_SIZE, 24,
    GLX_STENCIL_SIZE, 8,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    0
  };
  int glmajor = window->opengl_version.major;
  if (glmajor == 0) glmajor = 1;
  int glminor = window->opengl_version.minor;
  if (false == brpl_x11_get_set_context(&x11, attrib_list, glmajor, glminor)) {
    attrib_list[1] = 0; // No MSAA
    if (false == brpl_x11_get_set_context(&x11, attrib_list, glmajor, glminor)) {
      attrib_list[0] = brpl_x11_None; // Anything
      if (false == brpl_x11_get_set_context(&x11, attrib_list, glmajor, glminor)) {
        return false;
      }
    }
  }

  const char* title = window->title;
  if (title == NULL) title = "Brpl";
  brpl_x11_Xutf8SetWMProperties(x11.display,
                       x11.window_handle,
                       title, title,
                       NULL, 0,
                       NULL, NULL, NULL);

  brpl_x11_XSetWMProtocols(x11.display, x11.window_handle, &x11.WM_DELETE_WINDOW, 1);
  brpl_x11_XMapWindow(d, x11.window_handle);
  brpl_x11_XFlush(d);

  x11.im = brpl_x11_XOpenIM(x11.display, 0, NULL, NULL);
  x11.ic = brpl_x11_XCreateIC(x11.im,
                     brpl_x11_XNInputStyle,
                     brpl_x11_XIMPreeditNothing | brpl_x11_XIMStatusNothing,
                     brpl_x11_XNClientWindow,
                     x11.window_handle,
                     brpl_x11_XNFocusWindow,
                     x11.window_handle,
                     NULL);

  int opcode = 0, event = 0, error = 0;
  brpl_x11_XQueryExtension(x11.display, "XInputExtension", &opcode, &event, &error);
  x11.xi_opcode = opcode;

  br_u64 mask = 1<<brpl_x11_XI_TouchBegin | 1<<brpl_x11_XI_TouchUpdate | 1<<brpl_x11_XI_TouchEnd;
  brpl_x11_XIEventMask evmask = {
    .deviceid = 0,
    .mask_len = sizeof(mask),
    .mask = (void*)&mask
  };
  int status = XISelectEvents(x11.display, x11.window_handle, &evmask, 1);
  brpl_x11_XFlush(x11.display);
  LOGI("brpl_x11_Status: %d", status);

  brpl_window_x11_t* win = BR_MALLOC(sizeof(brpl_window_x11_t));
  memcpy(win, &x11, sizeof(x11));
  window->win = win;
  return true;
}

static void brpl_x11_close_window(brpl_window_t* window) {
  brpl_window_x11_t* win = window->win;
  glXMakeCurrent(win->display, brpl_x11_None, NULL);
  glXDestroyWindow(win->display, win->glx_window);
  glXDestroyContext(win->display, win->glx_ctx);
  brpl_x11_XDestroyWindow(win->display, win->window_handle);
  brpl_x11_XFlush(win->display);
  brpl_x11_XCloseDisplay(win->display);
  BR_FREE(win);
}

static int brpl_x11_keysym(brpl_x11_XEvent event) {
  int keysym = brpl_x11_XLookupKeysym(&event.xkey, 0);
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

void brpl_additional_event_touch(brpl_window_t* window, int kind, float x, float y, int id) {
  brpl_event_kind_t ev_kind = brpl_event_none;
  switch (kind) {
    case 0: ev_kind = brpl_event_touch_begin; break;
    case 1: ev_kind = brpl_event_touch_update; break;
    case 2: ev_kind = brpl_event_touch_end; break;
    default: BR_LOGE("Bad event kind: %d", kind);
  }
  brpl_glfw_window_t* win = window->win;
  brpl_q_push(&win->q, (brpl_event_t) { .kind = ev_kind, .touch = { .pos = BR_VEC2(x, y), .id = id } });
}

void brpl_window_size_set(brpl_window_t* window, int width, int height) {
 brpl_glfw_window_t* win = window->win;
 glfwSetWindowSize(win->glfw, width, height);
 brpl_q_push(&win->q, (brpl_event_t) { .kind = brpl_event_window_resize, .size = BR_SIZE((float)width, (float)height) });
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
#pragma comment(lib, "user32.lib")

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
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = brpl_win32_event_callback;
  wc.hInstance = GetModuleHandleA(NULL);
  wc.lpszClassName = win->title;
  wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
  RegisterClassA(&wc);

  // TODO: Make WS_EX_NOREDIRECTIONBITMAP work with opengl.
  //       ATM This is imposible out of the box.
  //       One should most likely create Direct2D context. Draw
  //       OpenGL frame, take the framebuffer from opengl. Draw
  //       that framebuffer into the Direct2D context and then
  //       swap manauly with Direct2D DirectRendering bullshit.
  //DWORD ex_style = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP;
  DWORD ex_style = WS_EX_APPWINDOW;

  HWND hwnd = CreateWindowExA(ex_style, wc.lpszClassName, win->title,
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

static bool brpl_win32_load(brpl_window_t* window) {
  window->f.window_open = brpl_win32_open_window;
  window->f.frame_start = brpl_win32_frame_start;
  window->f.frame_end   = brpl_win32_frame_end;
  window->f.event_next  = brpl_win32_event_next;
  return true;
}

#endif // BR_HAS_WIN32

// -------------------------------
// END OF WIN32 IMPL
// -------------------------------
