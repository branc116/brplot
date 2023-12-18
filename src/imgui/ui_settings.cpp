#include "src/imgui/imgui_extensions.h"

ImVec4 operator-(float f, ImVec4 v) {
  return ImVec4{f - v.x , f - v.y, f - v.z, f - v.w};
}

namespace br {
  void ui_settings(br_plot_t* br) {
    ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Settings 2")) {
    if (ImGui::Button("Clear all")) {
      points_groups_deinit(&br->groups);
    }
    ImGui::SameLine();
    if (ImGui::Button("Empty all")) {
      points_groups_empty(&br->groups);
    }
    if (ImGui::Button("Toggle Hide all")) {
      for (size_t i = 0; i < br->groups.len; ++i) {
        br->groups.arr[i].is_selected = !br->groups.arr[i].is_selected;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Hide all")) {
      for (size_t i = 0; i < br->groups.len; ++i) {
        br->groups.arr[i].is_selected = false;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Show all")) {
      for (size_t i = 0; i < br->groups.len; ++i) {
        br->groups.arr[i].is_selected = true;
      }
    }
    if (ImGui::CollapsingHeader("Plots", ImGuiTreeNodeFlags_DefaultOpen)) {
      for (size_t i = 0; i < br->groups.len; ++i) {
        auto& c = br->groups.arr[i].color;
        float colors[4] = { ((float)c.r)/255.f, ((float)c.g)/255.f, ((float)c.b)/255.f, ((float)c.a)/255.f};
        ImVec4 imcol = {colors[0], colors[1], colors[2], 1.f};
        ImVec4 imcol_inv = 1.f - imcol;
        imcol_inv.w = 1.f;
        sprintf(&context.buff[2], "Plot %lu", i);
        context.buff[0] = context.buff[1] = '#';
        ImGui::Checkbox(context.buff, &br->groups.arr[i].is_selected);
        ImGui::PushStyleColor(ImGuiCol_Text, imcol);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, imcol_inv);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, imcol_inv);
        ImGui::SameLine();
        if (ImGui::TreeNodeEx(&context.buff[2], ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PopStyleColor(3);
          sprintf(context.buff, "Clear##P%lu", i);
          if (ImGui::Button(context.buff)) {
            points_group_clear(&br->groups, br->groups.arr[i].group_id);
          }
          ImGui::SetNextItemWidth(60.f);
          sprintf(context.buff, "PlotColor_%lu", i);
          ImGui::ColorPicker3(context.buff, colors, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview);
          br->groups.arr[i].color = { (unsigned char)(colors[0] * 255.f), (unsigned char)(colors[1] * 255.f), (unsigned char)(colors[2] * 255.f), (unsigned char)(colors[3] * 255.f) };
          sprintf(context.buff, "%lu##%lu", br->groups.arr[i].len, i);
          ImGui::LabelText(context.buff, "Number of points");
          br::ui_textbox("Name", &br->groups.arr[i].name);
          ImGui::TreePop();
        } else ImGui::PopStyleColor(3);
        if (i != br->groups.len - 1) ImGui::Separator();
      }
    }
    if (ImGui::CollapsingHeader("Scene")) {
        br::Input("Size", br->uvZoom);
        br::Input("Scene Center", br->uvOffset);
        Vector2 tr{br->graph_rect.x + br->graph_rect.width, br->graph_rect.y};
        Vector2 bl{br->graph_rect.x, br->graph_rect.y - br->graph_rect.height};
        if (br::Input("Bottom Left", bl)) {
          float newWidth = (tr.x - bl.x);
          float newHeight = (tr.y - bl.y);
          br->uvZoom.x = br->graph_screen_rect.height / br->graph_screen_rect.width * newWidth;
          br->uvOffset.x -= (newWidth - br->graph_rect.width) / 2.f; 
          br->uvZoom.y = newHeight;
          br->uvOffset.y -= (newHeight - br->graph_rect.height) / 2.f;
        }
        if (br::Input("Top Right", tr)) {
          float newWidth = (tr.x - bl.x);
          float newHeight = (tr.y - bl.y);
          br->uvZoom.x = br->graph_screen_rect.height / br->graph_screen_rect.width * newWidth;
          br->uvOffset.x += (newWidth - br->graph_rect.width) / 2.f;
          br->uvZoom.y = newHeight;
          br->uvOffset.y += (newHeight - br->graph_rect.height) / 2.f;
        }
    }
    if (ImGui::CollapsingHeader("Debug")) {
      ImGui::SliderFloat("Recoil", &br->recoil, 0.f, 1.1f);
      ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);
      ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);
      ImGui::Checkbox("Follow", &br->follow);
      ImGui::Checkbox("Debug Lines", &context.debug_bounds);
      ImGui::Checkbox("Jump Arround", &br->jump_around);
    }
  }
  ImGui::End();
  }
}