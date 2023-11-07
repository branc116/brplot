#include "plotter.h"
#include "stdio.h"
#include "raylib.h"
#include "../imgui/imgui.h"

ImVec4 operator-(float f, ImVec4 v) {
  return ImVec4{f - v.x , f - v.y, f - v.z, f - v.w};
}
namespace br {
  bool Input(const char* label, Vector2& a) {
    float* v = &a.x;
    return ImGui::InputFloat2(label, v);
  }

  bool Input(const char* label, Vector3& a) {
    float* v = &a.x;
    return ImGui::InputFloat3(label, v);
  }
}


#ifdef __cplusplus
extern "C" {
#endif

void hot_settings(graph_values_t* gv) {
  if (ImGui::Begin("Settings 2")) {
    if (ImGui::Button("Clear all")) {
      points_groups_deinit(&gv->groups);
    }
    ImGui::SameLine();
    if (ImGui::Button("Empty all")) {
      points_groups_empty(&gv->groups);
    }
    if (ImGui::Button("Toggle Hide all")) {
      for (size_t i = 0; i < gv->groups.len; ++i) {
        gv->groups.arr[i].is_selected = !gv->groups.arr[i].is_selected;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Hide all")) {
      for (size_t i = 0; i < gv->groups.len; ++i) {
        gv->groups.arr[i].is_selected = false;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Show all")) {
      for (size_t i = 0; i < gv->groups.len; ++i) {
        gv->groups.arr[i].is_selected = true;
      }
    }
    if (ImGui::CollapsingHeader("Plots", ImGuiTreeNodeFlags_DefaultOpen)) {
      for (size_t i = 0; i < gv->groups.len; ++i) {
        auto& c = gv->groups.arr[i].color;
        float colors[4] = { ((float)c.r)/255.f, ((float)c.g)/255.f, ((float)c.b)/255.f, ((float)c.a)/255.f};
        ImVec4 imcol = {colors[0], colors[1], colors[2], 1.f};
        ImVec4 imcol_inv = 1.f - imcol;
        imcol_inv.w = 1.f;
        sprintf(&context.buff[2], "Plot %lu", i);
        context.buff[0] = context.buff[1] = '#';
        ImGui::Checkbox(context.buff, &gv->groups.arr[i].is_selected);
        ImGui::PushStyleColor(ImGuiCol_Text, imcol);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, imcol_inv);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, imcol_inv);
        ImGui::SameLine();
        if (ImGui::TreeNodeEx(&context.buff[2], ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::PopStyleColor(3);
          sprintf(context.buff, "Clear##P%lu", i);
          if (ImGui::Button(context.buff)) {
            points_group_clear(&gv->groups, gv->groups.arr[i].group_id);
          }
          ImGui::SetNextItemWidth(60.f);
          sprintf(context.buff, "PlotColor_%lu", i);
          ImGui::ColorPicker3(context.buff, colors, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview);
          gv->groups.arr[i].color = { (unsigned char)(colors[0] * 255.f), (unsigned char)(colors[1] * 255.f), (unsigned char)(colors[2] * 255.f), (unsigned char)(colors[3] * 255.f) };
          sprintf(context.buff, "%lu##%lu", gv->groups.arr[i].len, i);
          ImGui::LabelText(context.buff, "Number of points");
          ImGui::TreePop();
        } else ImGui::PopStyleColor(3);
        if (i != gv->groups.len - 1) ImGui::Separator();
      }
    }
    if (ImGui::CollapsingHeader("Scene")) {
        br::Input("Size", gv->uvZoom);
        br::Input("Scene Center", gv->uvOffset);
        Vector2 tr{gv->graph_rect.x + gv->graph_rect.width, gv->graph_rect.y};
        Vector2 bl{gv->graph_rect.x, gv->graph_rect.y - gv->graph_rect.height};
        if (br::Input("Bottom Left", bl)) {
          float newWidth = (tr.x - bl.x);
          float newHeight = (tr.y - bl.y);
          gv->uvZoom.x = gv->graph_screen_rect.height / gv->graph_screen_rect.width * newWidth;
          gv->uvOffset.x -= (newWidth - gv->graph_rect.width) / 2.f; 
          gv->uvZoom.y = newHeight;
          gv->uvOffset.y -= (newHeight - gv->graph_rect.height) / 2.f;
        }
        if (br::Input("Top Right", tr)) {
          float newWidth = (tr.x - bl.x);
          float newHeight = (tr.y - bl.y);
          gv->uvZoom.x = gv->graph_screen_rect.height / gv->graph_screen_rect.width * newWidth;
          gv->uvOffset.x += (newWidth - gv->graph_rect.width) / 2.f;
          gv->uvZoom.y = newHeight;
          gv->uvOffset.y += (newHeight - gv->graph_rect.height) / 2.f;
        }
    }
    if (ImGui::CollapsingHeader("Debug")) {
      ImGui::SliderFloat("Recoil", &gv->recoil, 0.f, 1.1f);
      ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);
      ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);
      ImGui::Checkbox("Follow", &gv->follow);
      ImGui::Checkbox("Debug Lines", &context.debug_bounds);
      ImGui::Checkbox("Jump Arround", &gv->jump_around);
    }
  }
  ImGui::End();
}

void br_hot_init(graph_values_t* gv) {
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.Fonts->Clear();
    //io.Fonts->AddFontFromFileTTF("./fonts/PlayfairDisplayRegular-ywLOY.ttf", 20);
    fprintf(stderr, "First call\n");
}

void br_hot_loop(graph_values_t* gv) {
  hot_settings(gv);
}

#ifdef __cplusplus
}
#endif
