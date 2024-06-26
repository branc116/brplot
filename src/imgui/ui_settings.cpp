#include "imgui.h"
#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_da.h"
#include "src/imgui/imgui_extensions.h"

ImVec4 operator-(float f, ImVec4 v) {
  return ImVec4{f - v.x , f - v.y, f - v.z, f - v.w};
}

//extern float something;
//extern float something2;
//
//extern float stride_after;
//extern int max_stride;
//
//extern int raw_c;
//extern int not_raw_c;
extern "C" float line_3d_size;

namespace br {
  void ui_settings(br_plotter_t* br) {
    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Settings")) {
//      raw_c = 0;
//      not_raw_c = 0;
      if (ImGui::Button("Clear all")) {
        br_datas_deinit(&br->groups);
      }
      ImGui::SameLine();
      if (ImGui::Button("Empty all")) {
        br_datas_empty(&br->groups);
      }
      if (ImGui::CollapsingHeader("Plots")) {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyResizeDown;
        tab_bar_flags &= ~(ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyResizeDown);
        if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
          if (ImGui::TabItemButton("+2D", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip)) {
            br_plotter_add_plot_2d(br);
          }
          if (ImGui::TabItemButton("+3D", ImGuiTabItemFlags_Leading | ImGuiTabItemFlags_NoTooltip)) {
            br_plotter_add_plot_3d(br);
          }
          for (int i = 0; i < br->plots.len; ++i) {
            snprintf(context.buff, IM_ARRAYSIZE(context.buff), "Plot #%d", i);
            bool open = true;
            if (ImGui::BeginTabItem(context.buff, &open, ImGuiTabItemFlags_None)) {
              ImGui::PushID(i);
              for (size_t j = 0; j < br->groups.len; ++j) {
                bool contains = false;
                int group_id = br->groups.arr[j].group_id;
                br_da_contains_t(int, br->plots.arr[i].groups_to_show, group_id, contains);
                br_str_to_c_str1(br->groups.arr[j].name, context.buff);
                if (ImGui::Checkbox(context.buff, &contains)) {
                  if (contains) br_da_push_t(int, br->plots.arr[i].groups_to_show, group_id);
                  else br_da_remove(br->plots.arr[i].groups_to_show, group_id);
                }
              }
              ImGui::PopID();
              ImGui::EndTabItem();
            }
            if (false == open) {
              br_da_remove_at_t(int, br->plots, i);
              --i;
            }
          }
          ImGui::EndTabBar();
        }
      }
      if (ImGui::CollapsingHeader("Data Groups", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < br->groups.len; ++i) {
          auto& c = br->groups.arr[i].color;
          float colors[4] = { ((float)c.r)/255.f, ((float)c.g)/255.f, ((float)c.b)/255.f, ((float)c.a)/255.f};
          ImVec4 imcol = {colors[0], colors[1], colors[2], 1.f};
          ImVec4 imcol_inv = 1.f - imcol;
          imcol_inv.w = 1.f;
          sprintf(&context.buff[2], "Data Group %d", br->groups.arr[i].group_id);
          ImGui::PushStyleColor(ImGuiCol_Text, imcol);
          ImGui::PushStyleColor(ImGuiCol_HeaderActive, imcol_inv);
          ImGui::PushStyleColor(ImGuiCol_HeaderHovered, imcol_inv);
          if (ImGui::TreeNodeEx(&context.buff[2], ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PopStyleColor(3);
            sprintf(context.buff, "Clear##P%zu", i);
            if (ImGui::Button(context.buff)) {
              br_data_clear(&br->groups, &br->plots, br->groups.arr[i].group_id);
            }
            ImGui::SetNextItemWidth(60.f);
            sprintf(context.buff, "PlotColor_%zu", i);
            ImGui::ColorPicker3(context.buff, colors, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview);
            br->groups.arr[i].color = { (unsigned char)(colors[0] * 255.f), (unsigned char)(colors[1] * 255.f), (unsigned char)(colors[2] * 255.f), (unsigned char)(colors[3] * 255.f) };
            sprintf(context.buff, "%zu##%zu", br->groups.arr[i].len, i);
            ImGui::LabelText(context.buff, "Number of points");
            br::ui_textbox("Name", &br->groups.arr[i].name);
            ImGui::TreePop();
          } else ImGui::PopStyleColor(3);
          if (i != br->groups.len - 1) ImGui::Separator();
        }
      }
      if (ImGui::CollapsingHeader("Scene")) {
        br_plot_t* plot = &br->plots.arr[0];
        assert(plot->kind == br_plot_kind_2d);
        br::Input("Size", plot->dd.zoom);
        br::Input("Scene Center", plot->dd.offset);
        Vector2 tr{plot->dd.graph_rect.x + plot->dd.graph_rect.width, plot->dd.graph_rect.y};
        Vector2 bl{plot->dd.graph_rect.x, plot->dd.graph_rect.y - plot->dd.graph_rect.height};
        if (br::Input("Bottom Left", bl)) br_plotter_set_bottom_left(plot, bl.x, bl.y);
        if (br::Input("Top Right", tr)) br_plotter_set_top_right(plot, tr.x, tr.y);
        ImGui::InputFloat("3D line size", &line_3d_size);
        ImGui::Checkbox("Show Closest", &plot->dd.show_closest);
        ImGui::Checkbox("Show Closest X", &plot->dd.show_x_closest);
        ImGui::Checkbox("Show Closest Y", &plot->dd.show_y_closest);
      }
      if (ImGui::CollapsingHeader("Resampling")) {
//        ImGui::InputFloat("Max ratio bounding box/screen size", &something, 0.0f, 0.0f, "%f");
//        ImGui::SetItemTooltip("If you make this value larger, performace will improve, but visual quality will suffer.");
//        ImGui::InputFloat("Max ratio bounding box/screen size2", &something2, 0.0f, 0.0f, "%f");
//        ImGui::SetItemTooltip("If you make this value larger, performace will improve, but visual quality will suffer.");
//        ImGui::InputFloat("Stride After", &stride_after, 0.f, 0.f, "%f");
//        ImGui::SetItemTooltip("If you make this value larger, performace will improve, but visual quality will suffer."
//                          "Skip some points if the line is far away.");
//        ImGui::InputInt("Max Stride", &max_stride);
//        ImGui::SetItemTooltip("If you make this value larger, performace will improve, but visual quality will suffer."
//                          "Max points that can be skipped when line is far away.");
//        ImGui::LabelText("Raw", "raw: %d", raw_c);
//        ImGui::LabelText("Not Raw", "Not raw: %d", not_raw_c);
      }
      if (ImGui::CollapsingHeader("Debug")) {
        br_plot_t* plot = &br->plots.arr[0];
        assert(plot->kind == br_plot_kind_2d);
        ImGui::SliderFloat("Recoil", &plot->dd.recoil, 0.f, 1.1f);
        ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);
        ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);
        ImGui::Checkbox("Follow", &plot->follow);
        ImGui::Checkbox("Debug Lines", &context.debug_bounds);
        ImGui::Checkbox("Jump Arround", &plot->jump_around);
      }
    }
    ImGui::End();
  }
}
