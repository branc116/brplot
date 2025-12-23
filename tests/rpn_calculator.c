#define BRUI_IMPLEMENTATION
#include <brui.h>


// #define X(NAME, MIN_ARGS, INSERT_AFTER)
#define RPN_FUNCS(X) \
  X(clear, 1, true)  \
  X(dup,   1, false) \
  X(pop,   1, false) \
  X(push,  0, true)  \
  X(sin,   1, false) \
  X(cos,   1, false) \
  X(tg,    1, false) \
  X(ctg,   1, false) \
  X(atan,  1, false) \
  X(atan2, 2, false) \
  X(log,   1, false) \
  X(log2,  1, false) \
  X(log10, 1, false) \

typedef struct stack_el {
  brsp_id_t value;
} stack_el_t;

typedef struct {
  stack_el_t* arr;
  int len, cap;
} stack_t;

brui_window_t win = { 0 };
static stack_t stack;
bool insert_mode = false;

#define X(NAME, MIN_ARGS, INSERT_AFTER) static void rpn_##NAME##_unsafe(void);
  RPN_FUNCS(X)
#undef X

#define X(NAME, MIN_ARGS, INSERT_AFTER) static void rpn_##NAME(void) { \
  if (stack.len >= MIN_ARGS) { \
    rpn_##NAME##_unsafe(); \
    insert_mode = INSERT_AFTER; \
  } \
}
  RPN_FUNCS(X)
#undef X



static float rpn_get1(void) {
  br_strv_t op = brsp_get(win.sp, stack.arr[stack.len - 1].value); 
  return br_strv_to_float(op);
}

static void rpn_get2(float* out1, float* out2) {
  br_strv_t op1 = brsp_get(win.sp, stack.arr[stack.len - 1].value); 
  br_strv_t op2 = brsp_get(win.sp, stack.arr[stack.len - 2].value); 
  *out1 = br_strv_to_float(op1);
  *out2 = br_strv_to_float(op2);
}

static float rpn_pop1(void) {
  float ret = rpn_get1();
  rpn_pop();
  return ret;
}

static void rpn_set(float value) {
  br_strv_t s = br_scrach_printf("%f", value);
  brsp_set(&win.sp, stack.arr[stack.len - 1].value, s);
  insert_mode = false;
}

static void rpn_push_value(float value) {
  rpn_push();
  rpn_set(value);
}

static void calc_button_append(br_strv_t sv) {
  if (brui_button(sv)) {
    if (false == insert_mode) {
      rpn_push();
      insert_mode = true;
    }
    brsp_insert_strv_at_end(&win.sp, stack.arr[stack.len - 1].value, sv);
  }
}

static void draw_display(void) {
  int ts = brui_text_size();
  brui_push(); // Current state
    for (int i = 0; i < 5; ++i) {
      int index = stack.len - i - 1;
      if (i == 0 && insert_mode) brui_push();
        brui_vsplitvp(2, BRUI_SPLITA(ts + 2), BRUI_SPLITR(1));
          brui_textf("%d: ", i + 1);
        brui_vsplit_pop();
          if (index >= 0) {
            stack_el_t cur = stack.arr[index];
            brui_text_input(cur.value);
          }
        brui_vsplit_end();
      if (i == 0 && insert_mode) brui_pop();
    }
  brui_pop();
}

typedef struct {
  int cur;
  int cols;
  struct {
    brui_split_t* arr;
    int len, cap;
  } splits;
} grid_t;

grid_t grid_begin(int cols) {
  BR_ASSERTF(cols > 0, "There should be more than 0 collumns in the grid.. Requested is: %d", cols);
  grid_t grid = { .cur = 0, .cols = cols };

  br_da_push(grid.splits, BRUI_SPLITA(10));
  for (int i = 0; i < cols; ++i) {
    br_da_push(grid.splits, BRUI_SPLITR(1));
  }
  br_da_push(grid.splits, BRUI_SPLITA(10));

  return grid;
}

void grid_next(grid_t* g) {
  if (g->cur == 0) {
    brui_vsplitarr(g->splits.len, g->splits.arr);
    brui_vsplit_pop(); // PADDING
    brui_push();
    g->cur = 1;
  } else if (g->cur < g->cols) {
    brui_pop();
    brui_vsplit_pop();
    brui_push();
    g->cur += 1;
  } else {
    brui_pop();
    brui_vsplit_pop(); // PADDING
    brui_vsplit_end();
    brui_vsplitarr(g->splits.len, g->splits.arr);
    brui_vsplit_pop(); // PADDING
    brui_push();
    g->cur = 1;
  }
}

void grid_end(grid_t* g) {
  if (g->cur > 0) {
    brui_pop();
    while (g->cur <= g->cols) {
      brui_vsplit_pop();
      g->cur += 1;
    }

    brui_vsplit_end();
  }
  br_da_free(g->splits);
}

static void draw_functions(void) {
  brui_push();
    brui_text_size_set(20);
    grid_t g = grid_begin(5);
#define X(NAME, MIN_ARGS, INSERT_AFTER) grid_next(&g); \
    if (brui_button(BR_STRL(#NAME))) { \
      rpn_##NAME(); \
    }
    RPN_FUNCS(X)
#undef X

    grid_end(&g);
  brui_pop();
}


static void draw_buttons(void) {
  int ts = 92;
  brui_push(); // Buttons
    brui_text_size_set(ts);
    brui_text_align_set(br_text_renderer_ancor_x_mid);
    brui_vsplitvp(6, BRUI_SPLITR(1), BRUI_SPLITA(ts), BRUI_SPLITA(ts), BRUI_SPLITA(ts), BRUI_SPLITA(ts), BRUI_SPLITR(1));
    brui_vsplit_pop();
      calc_button_append(BR_STRL("1"));
      calc_button_append(BR_STRL("4"));
      calc_button_append(BR_STRL("7"));
      calc_button_append(BR_STRL("0"));
    brui_vsplit_pop();
      calc_button_append(BR_STRL("2"));
      calc_button_append(BR_STRL("5"));
      calc_button_append(BR_STRL("8"));
      calc_button_append(BR_STRL("."));
    brui_vsplit_pop();
      calc_button_append(BR_STRL("3"));
      calc_button_append(BR_STRL("6"));
      calc_button_append(BR_STRL("9"));
      if (brui_button(BR_STRL("="))) {
        if (insert_mode) insert_mode = false;
        else rpn_dup();
      }
    brui_vsplit_pop();
      if (brui_button(BR_STRL("+"))) {
        if (stack.len > 1) rpn_push_value(rpn_pop1() + rpn_pop1());
        else               rpn_dup();
      }
      if (brui_button(BR_STRL("-"))) {
        if (stack.len > 1) {
          float op1 = rpn_pop1(), op2 = rpn_pop1();
          rpn_push_value(op2 - op1);
        } else rpn_set(-rpn_get1());
      }
      if (brui_button(BR_STRL("*"))) {
        if (stack.len > 1) rpn_push_value(rpn_pop1() * rpn_pop1());
      }
      if (brui_button(BR_STRL("/"))) {
        if (stack.len > 1) {
          float op1 = rpn_pop1(), op2 = rpn_pop1();
          rpn_push_value(op2 / op1);
        }
      }
    brui_vsplit_pop();
    brui_vsplit_end();
  brui_pop();
}

BR_EXPORT void rpn_init(void) {
  brui_window_init(&win);
}

BR_EXPORT void rpn_one_frame(void) {
  while (brui_event_next(&win).kind != brpl_event_frame_next);
  brui_frame_start(&win);
    brui_text_size_set(32);
    draw_display();
    draw_functions();
    draw_buttons();
  brui_frame_end(&win);
}

#if !defined(__EMSCRIPTEN__)
int main(void) {
  rpn_init();
  while (false == win.pl.should_close) {
    rpn_one_frame();
  }

  return 0;
}
#else
BR_EXPORT void rpn_wasm_resize(int width, int height) {
  brpl_window_size_set(&win.pl, width, height);
}

BR_EXPORT void rpn_wasm_touch_event(int kind, float x, float y, int id) {
  brpl_additional_event_touch(&win.pl, kind, x, y, id);
}
#endif

static void rpn_clear_unsafe(void) {
  brsp_set(&win.sp, stack.arr[stack.len - 1].value, BR_STRL(""));
}

static void rpn_dup_unsafe(void) {
  brsp_id_t new_id = brsp_copy(&win.sp, stack.arr[stack.len - 1].value);
  br_da_push(stack, (stack_el_t) { .value = new_id });
}

static void rpn_pop_unsafe(void) {
  brsp_remove(&win.sp, stack.arr[stack.len - 1].value);
  --stack.len;
}

static void rpn_push_unsafe(void) {
  stack_el_t se = { 0 };
  se.value = brsp_new(&win.sp);
  br_da_push(stack, se);
}

static void rpn_sin_unsafe(void) {
  rpn_set(sinf(rpn_get1()));
}
static void rpn_cos_unsafe(void) {
  rpn_set(cosf(rpn_get1()));
}
static void rpn_tg_unsafe(void) {
  rpn_set(tanf(rpn_get1()));
}
static void rpn_ctg_unsafe(void) {
  rpn_set(tanf(rpn_get1()));
}
static void rpn_atan_unsafe(void) {
  rpn_set(atanf(rpn_get1()));
}
static void rpn_atan2_unsafe(void) {
  float x = rpn_pop1(), y = rpn_pop1();
  rpn_push_value(atan2f(y, x));
}

static void rpn_log_unsafe(void) {
  rpn_set(logf(rpn_get1()));
}

static void rpn_log2_unsafe(void) {
  rpn_set(log2f(rpn_get1()));
}

static void rpn_log10_unsafe(void) {
  rpn_set(log10f(rpn_get1()));
}


// tcc tests/rpn_calculator.c -I. -Iinclude -o bin/rpn_calculator -lm && bin/calculator
// emcc tests/rpn_calculator.c -DBR_LIB -DGRAPHICS_API_OPENGL_ES3=1 -ggdb -DBR_LIB -I. -Iinclude -o bin/rpn_calculator.js -sWASM_BIGINT -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sGL_ENABLE_GET_PROC_ADDRESS -sCHECK_NULL_WRITES=0 -sDISABLE_EXCEPTION_THROWING=1 -sFILESYSTEM=0 -sDYNAMIC_EXECUTION=0 -sMODULARIZE=1 -sEXPORT_ES6=1 -lm
            
                                          
                                          
            
// TODOs:
// * br_text_renderer_ancor_x_right does not work..
// * Add scrollable componen
// * Text input cursor on the exact mouse position.
//
