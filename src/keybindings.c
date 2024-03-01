#include "br_plot.h"
#include "stdbool.h"
#include "raylib.h"
#include "assert.h"

typedef struct {
  bool ctrl, shift;
} br_keybinding_ctrl_shift_t;

#define KEY_DOWN 1
#define KEY_PRESS 2

#define KEY_BINDINGS(x) \
  x(KEY_R, KEY_PRESS, br_keybinding_gui_reset) \
  x(KEY_C, KEY_PRESS, br_keybinding_clear) \
  x(KEY_H, KEY_PRESS, br_keybinding_hide) \
  x(KEY_T, KEY_PRESS, br_keybinding_test_points) \
  x(KEY_F, KEY_PRESS, br_keybinding_follow) \
  x(KEY_D, KEY_PRESS, br_keybinding_debug) \
  x(KEY_J, KEY_DOWN,  br_keybinding_recoil_smol) \
  x(KEY_K, KEY_DOWN,  br_keybinding_recoil_big) \
  x(KEY_S, KEY_PRESS, br_keybinding_screenshot)

#define X(key, kind, name) static inline void name(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t meta);
KEY_BINDINGS(X)
#undef X

void br_keybinding_handle_keys(br_plotter_t* br, br_plot_instance_t* plot) {
  br_keybinding_ctrl_shift_t meta = {IsKeyDown(KEY_LEFT_CONTROL), IsKeyDown(KEY_LEFT_SHIFT)};
#define X(key, kind, name) if (((kind) == KEY_PRESS && IsKeyPressed(key)) || ((kind) == KEY_DOWN && IsKeyDown(key))) name(br, plot, meta);
  KEY_BINDINGS(X)
#undef X
}

static inline void br_keybinding_gui_reset(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br;
  if (plot->kind == br_plot_instance_kind_2d) {
    if (!cs.ctrl)  plot->dd.zoom.x = plot->dd.zoom.y = 1;
    if (!cs.shift) plot->dd.offset.x = plot->dd.offset.y = 0;
  } else {
    assert(false);
  }
}

static inline void br_keybinding_clear(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot;
  if (cs.shift) points_groups_deinit(&br->groups);
  else          points_groups_empty(&br->groups);
}

static inline void br_keybinding_hide(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot;
  if (cs.shift) for (size_t i = 0; i < br->groups.len; ++i) br->groups.arr[i].is_selected = !br->groups.arr[i].is_selected;
  else          for (size_t i = 0; i < br->groups.len; ++i) br->groups.arr[i].is_selected = false;
}

static inline void br_keybinding_test_points(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)plot;
  points_groups_add_test_points(&br->groups);
}

static inline void br_keybinding_follow(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br; (void)cs;
  plot->follow = !plot->follow;
  if (plot->follow) br_plotter_focus_visible(plot, br->groups);
}

static inline void br_keybinding_debug(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br; (void)cs; (void)plot;
  context.debug_bounds = !context.debug_bounds;
}

static inline void br_keybinding_recoil_smol(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)br;
  if (plot->kind == br_plot_instance_kind_2d) plot->dd.recoil -= 0.001f;
}
static inline void br_keybinding_recoil_big(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)br;
  if (plot->kind == br_plot_instance_kind_2d) plot->dd.recoil += 0.001f;
}

static inline void br_keybinding_screenshot(br_plotter_t* br, br_plot_instance_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)br;
  br_plot_instance_screenshot(plot, br->groups, "test.png"); // TODO set a sensible name
}

