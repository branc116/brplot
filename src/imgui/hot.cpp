#include "src/br_plot.h"
#include "src/br_gui_internal.h"
#include "src/br_plotter.h"
#include "src/br_str.h"
#include "src/resampling2.hpp"

#define RAYMATH_STATIC_INLINE
#include "external/glad.h"
#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

extern "C" void br_hot_init(br_plotter_t* gv) {
  fprintf(stderr, "First call\n");
}

void draw_node(const resampling2_nodes_3d_t* const nodes, br_data_t* d, int index, int depth) {
  resampling2_nodes_3d_t node = nodes[index];
  ImGui::Indent((float)depth * 1.5f);
  ImGui::Text("%zd %zd %.3f %.3f %.3f %.3f-%.3f %.3f-%.3f %.3f-%3f", node.base.child1, node.base.child2,
      node.curvature.x, node.curvature.y, node.curvature.z,
      d->ddd.xs[node.base.min_index_x], d->ddd.xs[node.base.max_index_x],
      d->ddd.ys[node.base.min_index_y], d->ddd.ys[node.base.max_index_y],
      d->ddd.zs[node.min_index_z], d->ddd.zs[node.max_index_z]);
  if (node.base.child1 != 0) draw_node(nodes, d, node.base.child1, depth + 1);
  if (node.base.child2 != 0) draw_node(nodes, d, node.base.child2, depth + 1);
  ImGui::Indent((float)-depth * 1.5f);
}

extern "C" void br_hot_loop(br_plotter_t* br) {
  char buff[128];
  if (ImGui::Begin("TestMath")) {
    for (size_t i = 0; i < br->groups.len; ++i) {
      br_str_to_c_str1(br->groups.arr[i].name, buff);
      if (ImGui::CollapsingHeader(buff)) {
        resampling2_t* r = br->groups.arr[i].resampling;
        ImGui::Text("count: %zd, type: %s", r->common.len, r->kind == br_data_kind_2d ? "2D" : "3D");
        if (r->kind != br_data_kind_3d) continue;
        draw_node(r->ddd.arr, &br->groups.arr[i], 0, 0);
      }
    }
  }
  ImGui::End();
}

