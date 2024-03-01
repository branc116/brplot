#include "br_plot.h"
#include "math.h"
#include "assert.h"

BR_API void br_plotter_set_bottom_left(br_plot_instance_t* br, float left, float bottom) {
  assert(false);
#if 0
  Vector2 bl = {left, bottom};
  Vector2 tr = {br->graph_rect.x + br->graph_rect.width, br->graph_rect.y};
  float newWidth = (tr.x - bl.x);
  float newHeight = (tr.y - bl.y);
  br->uvZoom.x = br->graph_screen_rect.height / br->graph_screen_rect.width * newWidth;
  br->uvOffset.x -= (newWidth - br->graph_rect.width) / 2.f;
  br->uvZoom.y = newHeight;
  br->uvOffset.y -= (newHeight - br->graph_rect.height) / 2.f;
#endif
}

BR_API void br_plotter_set_top_right(br_plot_instance_t* br, float right, float top) {
  assert(false);
#if 0
  Vector2 tr = {right, top};
  Vector2 bl = {br->graph_rect.x, br->graph_rect.y - br->graph_rect.height};
  float newWidth = (tr.x - bl.x);
  float newHeight = (tr.y - bl.y);
  br->uvZoom.x = br->graph_screen_rect.height / br->graph_screen_rect.width * newWidth;
  br->uvOffset.x += (newWidth - br->graph_rect.width) / 2.f;
  br->uvZoom.y = newHeight;
  br->uvOffset.y += (newHeight - br->graph_rect.height) / 2.f;
#endif
}

BR_API void br_plotter_focus_visible(br_plot_instance_t* br) {
  assert(false);
#if 0
  if (br->groups.len == 0) return;
  size_t i = 0;
  while (i < br->groups.len && (br->groups.arr[i].len == 0 || false == br->groups.arr[i].is_selected)) ++i;
  if (i >= br->groups.len) return;

  bb_t bb = br->groups.arr[i++].bounding_box;
  for (; i < br->groups.len; ++i) {
    if (br->groups.arr[i].len == 0 || false == br->groups.arr[i].is_selected) continue;
    bb_t cur_bb = br->groups.arr[i].bounding_box;
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
  Vector2 bl = {bb.xmin, bb.ymin};
  float maxSize = fmaxf(newWidth, newHeight);
  br->uvZoom.x = br->graph_screen_rect.height / br->graph_screen_rect.width * maxSize; 
  br->uvOffset.x = bl.x + maxSize / 2.f;
  br->uvZoom.y = newHeight;
  br->uvOffset.y = bl.y + maxSize / 2.f;
#endif

//  graph_set_bottom_left(br, bb.xmin - (width * 0.1f), bb.ymin - (height * 0.1f));
//  graph_update_context(br);
//  graph_set_top_right  (br, bb.xmin + (width * 0.1f), bb.ymax + (height * 0.1f));
}

