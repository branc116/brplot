#include "br_plot.h"
#include "br_plotter.h"
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
  x(KEY_S, KEY_PRESS, br_keybinding_screenshot) \
  x(KEY_THREE, KEY_PRESS, br_keybinding_switch_3d) \
  x(KEY_TWO, KEY_PRESS, br_keybinding_switch_2d)

#define X(key, kind, name) static inline void name(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t meta);
KEY_BINDINGS(X)
#undef X

void br_keybinding_handle_keys(br_plotter_t* br, br_plot_t* plot) {
  br_keybinding_ctrl_shift_t meta = {IsKeyDown(KEY_LEFT_CONTROL), IsKeyDown(KEY_LEFT_SHIFT)};
#define X(key, kind, name) if (((kind) == KEY_PRESS && IsKeyPressed(key)) || ((kind) == KEY_DOWN && IsKeyDown(key))) name(br, plot, meta);
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
    pi3->eye = (Vector3){ 0, 0, 100 };
    pi3->target = (Vector3){ 0, 0, 0 };
    pi3->up = (Vector3){ 0, 1, 0 };
  }
}

static inline void br_keybinding_clear(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot;
  if (cs.shift) br_datas_deinit(&br->groups);
  else          br_datas_empty(&br->groups);
}

static inline void br_keybinding_hide(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br, (void)plot, (void)cs;
  assert(0);
}

static inline void br_keybinding_test_points(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)plot;
  br_datas_add_test_points(&br->groups);
  br_dagen_push_expr_xy(&br->dagens, &br->groups, (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_y, .group_id = 5 } , (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_x, .group_id = 5 }, 12);
}

static inline void br_keybinding_follow(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br; (void)cs;
  plot->follow = !plot->follow;
  if (plot->follow) br_plot_focus_visible(plot, br->groups);
}

static inline void br_keybinding_debug(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br; (void)cs; (void)plot;
  context.debug_bounds = !context.debug_bounds;
}

static inline void br_keybinding_recoil_smol(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)br;
  if (plot->kind == br_plot_kind_2d) plot->dd.recoil -= 0.001f;
}
static inline void br_keybinding_recoil_big(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)br;
  if (plot->kind == br_plot_kind_2d) plot->dd.recoil += 0.001f;
}

static inline void br_keybinding_screenshot(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)cs; (void)br;
  br_plot_screenshot(plot, br->groups, "test.png"); // TODO set a sensible name
}

static inline void br_keybinding_switch_2d(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot, (void)cs;
  br_plotter_switch_2d(br);
}

static inline void br_keybinding_switch_3d(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot, (void)cs;
  br_plotter_switch_3d(br);
}

