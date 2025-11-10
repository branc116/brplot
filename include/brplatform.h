#if !defined(BR_INCLUDE_BR_PLATFORM_H)
#define BR_INCLUDE_BR_PLATFORM_H
#include "src/br_pp.h"
#include "src/br_math.h"

typedef void* voidp;

#if !defined(BR_WANTS_GL)
#  define BR_WANTS_GL 1
#endif

#if BR_WANTS_GL
#  include "src/br_gl.h"
#endif

#if defined(_WIN32)
   void* brpl_load_gl(void* module, const char* func_name);
#else
#  define brpl_load_gl brpl_load_symbol
#endif

#include ".generated/gl.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct brpl_touch_point_t {
  br_vec2_t pos;
  int id;
} brpl_touch_point_t;

typedef enum brpl_event_kind_t {
  brpl_event_none = 0, // Nothing happend
  brpl_event_key_press = 1,
  brpl_event_key_release = 2,
  brpl_event_input = 3,
  brpl_event_mouse_move = 4,
  brpl_event_mouse_scroll = 5,
  brpl_event_mouse_press = 6,
  brpl_event_mouse_release = 7,
  brpl_event_window_resize = 8,
  brpl_event_window_shown = 9,
  brpl_event_window_hidden = 10,
  brpl_event_window_focused = 11,
  brpl_event_window_unfocused = 12,
  brpl_event_close = 13,
  brpl_event_next_frame = 14,
  brpl_event_scale = 15, // DPI
  brpl_event_touch_begin = 16,
  brpl_event_touch_update = 17,
  brpl_event_touch_end = 18,
  brpl_event_nop, // Something happend but it's nothing
  brpl_event_unknown, // Something happend but I don't know what..
} brpl_event_kind_t;

typedef struct brpl_event_t {
  brpl_event_kind_t kind;

  union {
    br_vec2_t pos;
    br_vec2_t vec;
    br_size_t size;
    double time;
    br_u32 utf8_char;
    int mouse_key;
    struct {
      int key;
      int keycode;
    };
    brpl_touch_point_t touch;
  };
} brpl_event_t;

typedef struct brpl_q_t {
  int read_index, write_index;
  brpl_event_t events[1024];
} brpl_q_t;

typedef enum brpl_window_kind_t {
  brpl_window_x11,
  brpl_window_win32,
  brpl_window_glfw,
} brpl_window_kind_t;

typedef struct brpl_window_t brpl_window_t;
typedef struct brpl_window_t {
  struct {
    void (*frame_start)(brpl_window_t* window);
    void (*frame_end)(brpl_window_t* window);
    brpl_event_t (*event_next)(brpl_window_t* window);
    bool (*window_open)(brpl_window_t* window);
    void (*window_close)(brpl_window_t* window);
  } f;
  // Data that specific windowing backend is using
  void* win;
  uint64_t timer_frequency;

  const char* title;       // INPUT
  brpl_window_kind_t kind; // INPUT
  br_extenti_t viewport;   // INPUT

  br_vec2_t scale; // Think DPI
  bool active;
  bool should_close;
} brpl_window_t;

bool brpl_window_open(brpl_window_t* window);
void brpl_window_close(brpl_window_t* window);

void brpl_frame_start(brpl_window_t* window);
void brpl_frame_end  (brpl_window_t* window);

brpl_event_t brpl_event_next(brpl_window_t* window);

uint64_t brpl_timestamp(void);
double   brpl_time(void);

void* brpl_load_library(const char* path);
void* brpl_load_symbol(void* library, const char* name);

void brpl_q_push(brpl_q_t* q, brpl_event_t event);
brpl_event_t brpl_q_pop(brpl_q_t* q);

// Only for web and with glfw...
void brpl_additional_event_touch(brpl_window_t* window, int kind, float x, float y, int id);
void brpl_window_size_set(brpl_window_t* window, int height, int width);

#define BR_KEY_SPACE              32
#define BR_KEY_APOSTROPHE         39  /* ' */
#define BR_KEY_COMMA              44  /* , */
#define BR_KEY_MINUS              45  /* - */
#define BR_KEY_PERIOD             46  /* . */
#define BR_KEY_SLASH              47  /* / */
#define BR_KEY_0                  48
#define BR_KEY_1                  49
#define BR_KEY_2                  50
#define BR_KEY_3                  51
#define BR_KEY_4                  52
#define BR_KEY_5                  53
#define BR_KEY_6                  54
#define BR_KEY_7                  55
#define BR_KEY_8                  56
#define BR_KEY_9                  57
#define BR_KEY_SEMICOLON          59  /* ; */
#define BR_KEY_EQUAL              61  /* = */
#define BR_KEY_A                  65
#define BR_KEY_B                  66
#define BR_KEY_C                  67
#define BR_KEY_D                  68
#define BR_KEY_E                  69
#define BR_KEY_F                  70
#define BR_KEY_G                  71
#define BR_KEY_H                  72
#define BR_KEY_I                  73
#define BR_KEY_J                  74
#define BR_KEY_K                  75
#define BR_KEY_L                  76
#define BR_KEY_M                  77
#define BR_KEY_N                  78
#define BR_KEY_O                  79
#define BR_KEY_P                  80
#define BR_KEY_Q                  81
#define BR_KEY_R                  82
#define BR_KEY_S                  83
#define BR_KEY_T                  84
#define BR_KEY_U                  85
#define BR_KEY_V                  86
#define BR_KEY_W                  87
#define BR_KEY_X                  88
#define BR_KEY_Y                  89
#define BR_KEY_Z                  90
#define BR_KEY_LEFT_BRACKET       91  /* [ */
#define BR_KEY_BACKSLASH          92  /* \ */
#define BR_KEY_RIGHT_BRACKET      93  /* ] */
#define BR_KEY_GRAVE_ACCENT       96  /* ` */
#define BR_KEY_WORLD_1            161 /* non-US #1 */
#define BR_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define BR_KEY_ESCAPE             256
#define BR_KEY_ENTER              257
#define BR_KEY_TAB                258
#define BR_KEY_BACKSPACE          259
#define BR_KEY_INSERT             260
#define BR_KEY_DELETE             261
#define BR_KEY_RIGHT              262
#define BR_KEY_LEFT               263
#define BR_KEY_DOWN               264
#define BR_KEY_UP                 265
#define BR_KEY_PAGE_UP            266
#define BR_KEY_PAGE_DOWN          267
#define BR_KEY_HOME               268
#define BR_KEY_END                269
#define BR_KEY_CAPS_LOCK          280
#define BR_KEY_SCROLL_LOCK        281
#define BR_KEY_NUM_LOCK           282
#define BR_KEY_PRINT_SCREEN       283
#define BR_KEY_PAUSE              284
#define BR_KEY_F1                 290
#define BR_KEY_F2                 291
#define BR_KEY_F3                 292
#define BR_KEY_F4                 293
#define BR_KEY_F5                 294
#define BR_KEY_F6                 295
#define BR_KEY_F7                 296
#define BR_KEY_F8                 297
#define BR_KEY_F9                 298
#define BR_KEY_F10                299
#define BR_KEY_F11                300
#define BR_KEY_F12                301
#define BR_KEY_F13                302
#define BR_KEY_F14                303
#define BR_KEY_F15                304
#define BR_KEY_F16                305
#define BR_KEY_F17                306
#define BR_KEY_F18                307
#define BR_KEY_F19                308
#define BR_KEY_F20                309
#define BR_KEY_F21                310
#define BR_KEY_F22                311
#define BR_KEY_F23                312
#define BR_KEY_F24                313
#define BR_KEY_F25                314
#define BR_KEY_KP_0               320
#define BR_KEY_KP_1               321
#define BR_KEY_KP_2               322
#define BR_KEY_KP_3               323
#define BR_KEY_KP_4               324
#define BR_KEY_KP_5               325
#define BR_KEY_KP_6               326
#define BR_KEY_KP_7               327
#define BR_KEY_KP_8               328
#define BR_KEY_KP_9               329
#define BR_KEY_KP_DECIMAL         330
#define BR_KEY_KP_DIVIDE          331
#define BR_KEY_KP_MULTIPLY        332
#define BR_KEY_KP_SUBTRACT        333
#define BR_KEY_KP_ADD             334
#define BR_KEY_KP_ENTER           335
#define BR_KEY_KP_EQUAL           336
#define BR_KEY_LEFT_SHIFT         340
#define BR_KEY_LEFT_CONTROL       341
#define BR_KEY_LEFT_ALT           342
#define BR_KEY_LEFT_SUPER         343
#define BR_KEY_RIGHT_SHIFT        344
#define BR_KEY_RIGHT_CONTROL      345
#define BR_KEY_RIGHT_ALT          346
#define BR_KEY_RIGHT_SUPER        347
#define BR_KEY_MENU               348

#endif

#if defined(BRPL_IMPLMENTATION)
#undef BRPL_IMPLMENTATION
#  include "src/br_pp.h"
#  include "tools/unity/brplatform.c"
#endif
