
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include "imgui/imgui.h"
#include "src/imgui/imgui_extensions.h"

#define file_saver2_t br_file_saver2_s

struct br_file_saver2_s {
  std::filesystem::path last_read;
  std::filesystem::path cwd;
  std::vector<std::filesystem::path> cur_paths;
  std::string name;

  std::string filter_string;
  std::chrono::duration<long, std::milli> filter_timeout;

  std::string title;
};

#include "src/br_plot.h"


void file_saver_change_cwd_to(file_saver2_t& fs, std::filesystem::path&& p) {
  fs.cwd = std::move(p);
  fs.last_read = fs.cwd;
  fs.cur_paths.clear();
  std::filesystem::directory_iterator di(fs.cwd);
  for (auto& d : di) {
    if (d.is_directory()) fs.cur_paths.push_back(std::move(d));
  }
  std::sort(fs.cur_paths.begin(), fs.cur_paths.end(), [](auto a, auto b) { return a < b; });
}

void file_saver_change_cwd_up(file_saver2_t& fs) {
  file_saver_change_cwd_to(fs, fs.cwd.parent_path());
}

file_saver_state_t hot_file_explorer(file_saver2_t* fs) {
  ImGui::Begin("File Saver");
  auto io = ImGui::GetIO();
  if (br::IsMouseOwerWindow()) {
    ImGui::LabelText("##CurName", "Mouse is not over"); 
    char kp = GetCharPressed();
    if (kp == 0 && fs->filter_timeout > std::chrono::milliseconds(0)) {
      fs->filter_timeout -= std::chrono::milliseconds((int)(io.DeltaTime * 1000.f));
    }
    else if (kp == 0) {
      fs->filter_timeout = std::chrono::milliseconds(0);
      fs->filter_string = "";
    }
    else if (kp != 0 && fs->filter_timeout > std::chrono::milliseconds(0)) {
      fs->filter_timeout = std::chrono::milliseconds(3000);
      fs->filter_string += (char)kp;
    } else {
      fs->filter_timeout = std::chrono::milliseconds(3000);
      fs->filter_string = (char)kp;
    }
  }
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
  for (size_t i = 0; i < fs->cur_paths.size(); i++) {
    if (fs->filter_string != "" && std::string_view(fs->cur_paths[i].filename().c_str()).find(fs->filter_string) == std::string::npos) continue;
    ImGui::PushID(i);
    if (ImGui::Button(fs->cur_paths[i].filename().c_str(), button_sz)) {
      file_saver_change_cwd_to(*fs, std::move(fs->cur_paths[i]));
    }
    float last_button_x2 = ImGui::GetItemRectMax().x;
    float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x;
    if (i + 1 < fs->cur_paths.size() && next_button_x2 < window_visible_x2) ImGui::SameLine();
    ImGui::PopID();
  }
  ImGui::NewLine();
  ImGui::InputText(".png", fs->name.data(), fs->name.capacity());
  std::filesystem::path full_path = fs->cwd / fs->name;
  if (std::filesystem::exists(full_path)) {
    ImGui::LabelText("##ERROR", "Warning: File `%s` already exists", full_path.c_str());
  }
  ImGui::End();
  return file_saver_state_exploring;
}

