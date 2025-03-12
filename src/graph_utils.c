#include "src/br_plot.h"
#include "src/br_plotter.h"

#include <math.h>
#include <assert.h>

// TODO: rename to br_plot_
BR_API void br_plotter_set_bottom_left(br_plot_t* plot, float left, float bottom) {
  // TODO 2D/3D
  assert(plot->kind == br_plot_kind_2d);
  br_vec2_t bl = BR_VEC2(left, bottom);
  br_vec2_t tr = BR_VEC2(plot->dd.graph_rect.x + plot->dd.graph_rect.width, plot->dd.graph_rect.y);
  float newWidth = (tr.x - bl.x);
  float newHeight = (tr.y - bl.y);
  plot->dd.zoom.x = BR_EXTENTI_ASPECT(plot->cur_extent) * newWidth;
  plot->dd.offset.x -= (newWidth - plot->dd.graph_rect.width) / 2.f;
  plot->dd.zoom.y = newHeight;
  plot->dd.offset.y -= (newHeight - plot->dd.graph_rect.height) / 2.f;
}

BR_API void br_plotter_set_top_right(br_plot_t* plot, float right, float top) {
  // TODO 2D/3D
  assert(plot->kind == br_plot_kind_2d);
  br_vec2_t tr = BR_VEC2(right, top);
  br_vec2_t bl = BR_VEC2(plot->dd.graph_rect.x, plot->dd.graph_rect.y - plot->dd.graph_rect.height);
  float newWidth = (tr.x - bl.x);
  float newHeight = (tr.y - bl.y);
  plot->dd.zoom.x = BR_EXTENTI_ASPECT(plot->cur_extent) * newWidth;
  plot->dd.offset.x += (newWidth - plot->dd.graph_rect.width) / 2.f;
  plot->dd.zoom.y = newHeight;
  plot->dd.offset.y += (newHeight - plot->dd.graph_rect.height) / 2.f;
}

BR_API void br_plots_focus_visible(br_plots_t plots, br_datas_t const groups) {
  for (int i = 0; i < plots.len; ++i) {
    if (plots.arr[i].kind != br_plot_kind_2d) continue;
    br_plot_focus_visible(&plots.arr[i], groups);
    br_plot_focus_visible(&plots.arr[i], groups);
  }
}

BR_API void br_plot_focus_visible(br_plot_t* plot, br_datas_t const groups) {
  // TODO 2D/3D
  BR_ASSERT(plot->kind == br_plot_kind_2d);
  if (groups.len == 0) return;
  size_t i = 0;
  while (i < groups.len && (groups.arr[i].kind != br_data_kind_2d || groups.arr[i].len == 0)) ++i;
  if (i >= groups.len) return;

  bb_t bb = groups.arr[i++].dd.bounding_box;
  for (; i < groups.len; ++i) {
    if (groups.arr[i].kind != br_data_kind_2d || groups.arr[i].len == 0) continue;
    bb_t cur_bb = groups.arr[i].dd.bounding_box;
    bb = (bb_t) {
      .xmin = fminf(bb.xmin, cur_bb.xmin),
      .ymin = fminf(bb.ymin, cur_bb.ymin),
      .xmax = fmaxf(bb.xmax, cur_bb.xmax),
      .ymax = fmaxf(bb.ymax, cur_bb.ymax),
    };
  }

  float newWidth = (bb.xmax - bb.xmin);
  float newHeight = (bb.ymax - bb.ymin);
  bb.xmax += newWidth * 0.1f;
  bb.ymax += newHeight * 0.1f;
  bb.xmin -= newWidth * 0.1f;
  bb.ymin -= newHeight * 0.1f;
  newWidth = (bb.xmax - bb.xmin);
  newHeight = (bb.ymax - bb.ymin);
  br_vec2_t bl = BR_VEC2(bb.xmin, bb.ymin);
  float maxSize = fmaxf(newWidth, newHeight);
  plot->dd.zoom.x = BR_EXTENTI_ASPECT(plot->cur_extent) * maxSize; 
  plot->dd.offset.x = bl.x + maxSize / 2.f;
  plot->dd.zoom.y = newHeight;
  plot->dd.offset.y = bl.y + maxSize / 2.f;
}

