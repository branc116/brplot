#include "plotter.h"
#include "stdio.h"
#include "raylib.h"
#include "cstdlib"
#include "../imgui/imgui.h"
#include <filesystem>
#include <string>
#include <vector>

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

typedef enum {
  file_saver_state_exploring,
  file_saver_state_accept,
  file_saver_state_cancle
} file_saver_state_t;

typedef struct {
  std::filesystem::path last_read;
  std::filesystem::path cwd;
  std::vector<std::filesystem::path> cur_paths;
  std::string name;
} file_saver2_t;

void file_saver_change_cwd_to(file_saver2_t& fs, std::filesystem::path&& p) {
  fs.cwd = std::move(p);
  fs.last_read = fs.cwd;
  fs.cur_paths.clear();
  std::filesystem::directory_iterator di(fs.cwd);
  for (auto& d : di) {
    if (d.is_directory()) fs.cur_paths.push_back(std::move(d));
  }
}

void file_saver_change_cwd_up(file_saver2_t& fs) {
  file_saver_change_cwd_to(fs, fs.cwd.parent_path());
}

file_saver_state_t hot_file_explorer(file_saver2_t* fs) {
  ImGui::Begin("File Saver");
  ImGui::LabelText("##CurName", "%s/%s", fs->cwd.c_str(), fs->name.c_str()); 
  if (fs->last_read != fs->cwd) {
    file_saver_change_cwd_to(*fs, std::move(fs->cwd));
  }
  if (ImGui::Button("..")) {
    file_saver_change_cwd_up(*fs);
  }
  float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec2 button_sz(100, 40);
  for (size_t i = 0; i < fs->cur_paths.size(); i++)
  {
      ImGui::PushID(i);
      if (ImGui::Button(fs->cur_paths[i].filename().c_str(), button_sz)) {
        file_saver_change_cwd_to(*fs, std::move(fs->cur_paths[i]));
      }
      float last_button_x2 = ImGui::GetItemRectMax().x;
      float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x;
      if (i + 1 < fs->cur_paths.size() && next_button_x2 < window_visible_x2)
          ImGui::SameLine();
      ImGui::PopID();
  }
  ImGui::End();
  std::filesystem::path full_path = fs->cwd / fs->name;
  if (std::filesystem::exists(full_path)) {
    ImGui::LabelText("##ERROR", "Warning: File `%s` already exists", full_path.c_str());
  }
  return file_saver_state_exploring;
}

void hot_ui_settings(graph_values_t* gv) {
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

static file_saver2_t fs;

extern "C" void br_hot_init(graph_values_t* gv) {
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  fs.cwd = std::filesystem::path("/home");
  fs.name = "test.png";
  //io.Fonts->Clear();
  //io.Fonts->AddFontFromFileTTF("./fonts/PlayfairDisplayRegular-ywLOY.ttf", 20);
  fprintf(stderr, "First call\n");
}

extern "C" void br_hot_loop(graph_values_t* gv) {
  hot_ui_settings(gv);
  hot_file_explorer(&fs);
}

