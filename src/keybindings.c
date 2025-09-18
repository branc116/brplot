#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_tl.h"
#include "src/br_gl.h"

#include <stdbool.h>

typedef struct {
  bool ctrl, shift;
} br_keybinding_ctrl_shift_t;

#define BR_KEY_STATE_DOWN 1
#define BR_KEY_STATE_PRESS 2

#define KEY_BINDINGS(x) \
  x('r', BR_KEY_STATE_PRESS, br_keybinding_gui_reset) \
  x('c', BR_KEY_STATE_PRESS, br_keybinding_clear) \
  x('t', BR_KEY_STATE_PRESS, br_keybinding_test_points) \
  x('f', BR_KEY_STATE_PRESS, br_keybinding_follow) \
  x('d', BR_KEY_STATE_PRESS, br_keybinding_debug) \
  x('s', BR_KEY_STATE_PRESS, br_keybinding_screenshot) \
  x('3', BR_KEY_STATE_PRESS, br_keybinding_switch_3d) \
  x('2', BR_KEY_STATE_PRESS, br_keybinding_switch_2d)

#define X(key, kind, name) static inline void name(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t meta);
KEY_BINDINGS(X)
#undef X

void br_keybinding_handle_keys(br_plotter_t* br, br_plot_t* plot) {
  br_keybinding_ctrl_shift_t meta = { brtl_key_ctrl(), brtl_key_shift() };
#define X(key, kind, name) if (((kind) == BR_KEY_STATE_PRESS && brtl_key_pressed(key)) || ((kind) == BR_KEY_STATE_DOWN && brtl_key_down(key))) name(br, plot, meta);
  KEY_BINDINGS(X)
#undef X
}

static inline void br_keybinding_gui_reset(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br;
  if (plot->kind == br_plot_kind_2d) {
    if (!cs.ctrl)  plot->dd.zoom.x = plot->dd.zoom.y = 1;
    if (!cs.shift) plot->dd.offset.x = plot->dd.offset.y = 0;
  } else {
    br_plot_3d_t* pi3 = &plot->ddd;
    pi3->eye = BR_VEC3(0, 0, 100);
    pi3->target = BR_VEC3(0, 0, 0);
    pi3->up = BR_VEC3(0, 1, 0);
  }
}

static inline void br_keybinding_clear(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot;
  if (cs.shift) br_plotter_datas_deinit(br);
  else          br_datas_empty(&br->groups);
}

static inline void br_keybinding_test_points(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs;
  (void)plot;
  br_datas_add_test_points(&br->groups);
}

static inline void br_keybinding_follow(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br; (void)cs;
  if (cs.ctrl == false) {
    plot->follow = !plot->follow;
    if (plot->follow) br_plot_focus_visible(plot, br->groups);
  } else {
    br_plot_focus_visible(plot, br->groups);
  }
}

static inline void br_keybinding_debug(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br; (void)cs; (void)plot;
  bool old_d = *brtl_debug();
  *brtl_debug() = !old_d;
}

static inline void br_keybinding_screenshot(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs;
  br_plot_screenshot(br->text, plot, &br->shaders, br->groups, "test.png"); // TODO set a sensible name
}

static inline void br_keybinding_switch_2d(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot, (void)cs;
  br_plotter_switch_2d(br);
}

static inline void br_keybinding_switch_3d(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot, (void)cs;
  br_plotter_switch_3d(br);
}

