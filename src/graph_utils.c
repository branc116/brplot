#include "src/br_plot.h"
#include "src/br_plotter.h"

#include <math.h>

// TODO: rename to br_plot_
void br_plotter_set_bottom_left(br_plot_t* plot, float left, float bottom) {
  // TODO 2D/3D
  BR_ASSERTF(plot->kind == br_plot_kind_2d, "Kind is %d", plot->kind);
  br_vec2_t bl = BR_VEC2(left, bottom);
  br_vec2d_t tr = BR_VEC2D(plot->dd.graph_rect.x + plot->dd.graph_rect.width, plot->dd.graph_rect.y);
  double newWidth = (tr.x - bl.x);
  double newHeight = (tr.y - bl.y);
  plot->dd.zoom.x = BR_EXTENTI_ASPECT(plot->cur_extent) * newWidth;
  plot->dd.offset.x -= (newWidth - plot->dd.graph_rect.width) / 2.f;
  plot->dd.zoom.y = newHeight;
  plot->dd.offset.y -= (newHeight - plot->dd.graph_rect.height) / 2.f;
}

void br_plotter_set_top_right(br_plot_t* plot, float right, float top) {
  // TODO 2D/3D
  BR_ASSERTF(plot->kind == br_plot_kind_2d, "Kind is %d", plot->kind);
  br_vec2_t tr = BR_VEC2(right, top);
  br_vec2d_t bl = BR_VEC2D(plot->dd.graph_rect.x, plot->dd.graph_rect.y - plot->dd.graph_rect.height);
  double newWidth = (tr.x - bl.x);
  double newHeight = (tr.y - bl.y);
  plot->dd.zoom.x = BR_EXTENTI_ASPECT(plot->cur_extent) * newWidth;
  plot->dd.offset.x += (newWidth - plot->dd.graph_rect.width) / 2.f;
  plot->dd.zoom.y = newHeight;
  plot->dd.offset.y += (newHeight - plot->dd.graph_rect.height) / 2.f;
}

void br_plots_focus_visible(br_plots_t plots, br_datas_t const groups) {
  for (int i = 0; i < plots.len; ++i) {
    if (plots.arr[i].kind != br_plot_kind_2d) continue;
    br_plot_focus_visible(&plots.arr[i], groups);
    br_plot_focus_visible(&plots.arr[i], groups);
  }
}

void br_plot_focus_visible(br_plot_t* plot, br_datas_t const groups) {
  // TODO 2D/3D
  BR_ASSERT(plot->kind == br_plot_kind_2d);
  if (plot->data_info.len == 0) return;

  br_bb_t bb = { 0 };
  int n = 0;
  for (int i = 1; i < plot->data_info.len; ++i) {
    br_plot_data_t di = plot->data_info.arr[i];
    if (false == BR_PLOT_DATA_IS_VISIBLE(di)) continue;
    br_data_t* d = br_data_get1(groups, di.group_id);
    if (n > 0) {
      if (d->len > 0) {
        br_bb_t this_bb = br_bb_add(d->dd.bounding_box, BR_VEC2((float)d->dd.rebase_x, (float)d->dd.rebase_y));
        bb = br_bb_union(bb, this_bb);
        ++n;
      }
    } else {
      if (d->len > 0) {
        bb = br_bb_add(d->dd.bounding_box, BR_VEC2((float)d->dd.rebase_x, (float)d->dd.rebase_y));
        ++n;
      }
    }
  }
  if (n == 0) return;

  float new_width = BR_BBW(bb);
  float new_height = BR_BBH(bb);
  bb.max_x += new_width  * .1f;
  bb.max_y += new_height * .1f;
  bb.min_x -= new_width  * .1f;
  bb.min_y -= new_height * .1f;
  new_width = BR_BBW(bb);
  new_height = BR_BBH(bb);
  br_vec2_t bl = bb.min;
  if (0) {
    float maxSize = fmaxf(new_width, new_height);
    plot->dd.zoom.x = BR_EXTENTI_ASPECT(plot->cur_extent) * maxSize; 
    plot->dd.zoom.y = new_height;
    plot->dd.offset.x = bl.x + maxSize / 2.f;
    plot->dd.offset.y = bl.y + maxSize / 2.f;
  } else {
    plot->dd.offset.x = bl.x + new_width / 2.f;
    plot->dd.offset.y = bl.y + new_height / 2.f;
    plot->dd.zoom.x = new_width;
    plot->dd.zoom.y = new_height;
  }
}

