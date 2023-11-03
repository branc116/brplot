#include "plotter.h"
#include "stdio.h"
#include "raylib.h"
#include "../imgui/imgui.h"

#ifdef __cplusplus
extern "C" {
#endif
void hot_settings(graph_values_t* gv) {
  if (ImGui::Begin("Settings 2")) {
    if (ImGui::CollapsingHeader("Plots")) {
      for (size_t i = 0; i < gv->groups.len; ++i) {
        auto& c = gv->groups.arr[i].color;
        float colors[4] = { ((float)c.r)/255.f, ((float)c.g)/255.f, ((float)c.b)/255.f, ((float)c.a)/255.f};
        sprintf(context.buff, "Plot %lu", i);
        //ImGui::PushStyleColor(ImGuiCol_Header, colors);
        if (ImGui::TreeNode(context.buff)) {
          ImGui::Checkbox(context.buff, &gv->groups.arr[i].is_selected);
          sprintf(context.buff, "PlotColor_%lu", i);
          ImGui::SetNextItemWidth(60.f);
          ImGui::SameLine();
          ImGui::ColorPicker3(context.buff, colors, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview);
          gv->groups.arr[i].color = { (unsigned char)(colors[0] * 255.f), (unsigned char)(colors[1] * 255.f), (unsigned char)(colors[2] * 255.f), (unsigned char)(colors[3] * 255.f) };
          ImGui::TreePop();
        }
      }
    }
    if (ImGui::CollapsingHeader("Debug")) {
      ImGui::SliderFloat("Recoil", &gv->recoil, 0.f, 1.1f);
      ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);
      ImGui::Checkbox("Follow Arround", &gv->follow);
      ImGui::Checkbox("Debug Lines", &context.debug_bounds);
      ImGui::Checkbox("Jump Arround", &gv->jump_around);
    }
  }
  ImGui::End();
}

void br_hot(graph_values_t* gv) {
  hot_settings(gv);
}

#ifdef __cplusplus
}
#endif
