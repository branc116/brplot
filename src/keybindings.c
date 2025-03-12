#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_tl.h"
#include "src/br_gl.h"

#include <assert.h>
#include <stdbool.h>

typedef struct {
  bool ctrl, shift;
} br_keybinding_ctrl_shift_t;

#define BR_KEY_DOWN 1
#define BR_KEY_PRESS 2

#define KEY_BINDINGS(x) \
  x(BR_KEY_R, BR_KEY_PRESS, br_keybinding_gui_reset) \
  x(BR_KEY_C, BR_KEY_PRESS, br_keybinding_clear) \
  x(BR_KEY_H, BR_KEY_PRESS, br_keybinding_hide) \
  x(BR_KEY_T, BR_KEY_PRESS, br_keybinding_test_points) \
  x(BR_KEY_F, BR_KEY_PRESS, br_keybinding_follow) \
  x(BR_KEY_D, BR_KEY_PRESS, br_keybinding_debug) \
  x(BR_KEY_J, BR_KEY_DOWN,  br_keybinding_recoil_smol) \
  x(BR_KEY_K, BR_KEY_DOWN,  br_keybinding_recoil_big) \
  x(BR_KEY_S, BR_KEY_PRESS, br_keybinding_screenshot) \
  x(BR_KEY_THREE, BR_KEY_PRESS, br_keybinding_switch_3d) \
  x(BR_KEY_TWO, BR_KEY_PRESS, br_keybinding_switch_2d)

#define X(key, kind, name) static inline void name(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t meta);
KEY_BINDINGS(X)
#undef X

void br_keybinding_handle_keys(br_plotter_t* br, br_plot_t* plot) {
  br_keybinding_ctrl_shift_t meta = { brtl_key_ctrl(), brtl_key_shift() };
#define X(key, kind, name) if (((kind) == BR_KEY_PRESS && brtl_key_pressed(key)) || ((kind) == BR_KEY_DOWN && brtl_key_down(key))) name(br, plot, meta);
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

static inline void br_keybinding_hide(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)br, (void)plot, (void)cs;
  assert(0);
}

static inline void br_keybinding_test_points(br_plotter_t* br, br_plot_t* plot, br_keybinding_ctrl_shift_t cs) {
  (void)plot;
  br_datas_add_test_points(&br->groups);
  //br_dagen_push_expr_xy(&br->dagens, &br->groups, (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_y, .group_id = 5 } , (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_x, .group_id = 5 }, 12);

  if (cs.ctrl) {
    // TODO: This is bad and this will leak...
//    br_dagen_expr_t* ops = malloc(4 * sizeof(*ops));
//    ops[0] = (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_x, .group_id = 5 };
//    ops[1] = (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_y, .group_id = 0 };
//    ops[2] = (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_y, .group_id = 5 };
//    ops[3] = (br_dagen_expr_t) { .kind = br_dagen_expr_kind_reference_y, .group_id = 0 };
//
//    br_dagen_push_expr_xy(&br->dagens, &br->groups,
//        (br_dagen_expr_t) { .kind = br_dagen_expr_kind_add, .operands = { .op1 = &ops[0], .op2 = &ops[1] } },
//        ops[2], 14);
  }
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

