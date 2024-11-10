#include "src/imgui/imgui_extensions.h"
#include "src/br_plot.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

typedef struct br_file_saver_s {
  std::filesystem::path last_read = {};
  std::filesystem::path cwd = {};
  std::vector<std::filesystem::path> cur_paths = {};
  br_str_t name;

  br_str_t filter_string;
  std::chrono::duration<long, std::milli> filter_timeout = {};

  br_strv_t title;

  Rectangle file_list_rect;
  ImVec2 cur_button_sz = {100, 40};
  ImVec2 default_button_sz = {100, 40};
  br_plot_t const* plot;
} br_file_saver_t;

static void file_saver_change_cwd_to(br_file_saver_t& fs, std::filesystem::path&& p);

br_file_saver_t* br_file_saver_malloc(char const* title, char const * location, br_plot_t const* plot) {
  br_file_saver_t* br = new br_file_saver_t();
  br->title = br_strv_from_c_str(title);
  br->plot = plot;
  file_saver_change_cwd_to(*br, location);
  return br;
}

void br_file_saver_get_path(br_file_saver_t const* fs, br_str_t *path) {
  br_str_push_c_str(path, fs->cwd.string().c_str());
  if (path->str[path->len - 1] != '\\' || path->str[path->len - 1] != '/') br_str_push_char(path, '/');
  br_str_push_br_str(path, fs->name);
  br_str_push_c_str(path, ".png");
}

br_plot_t const* br_file_saver_get_plot_instance(br_file_saver_s const* fs) {
  return fs->plot;
}

void br_file_saver_free(br_file_saver_t* f) {
  br_str_free(f->name);
  br_str_free(f->filter_string);
  delete f;
}

static void file_saver_change_cwd_to(br_file_saver_t& fs, std::filesystem::path&& p) {
  fs.cwd = std::move(p);
  fs.last_read = fs.cwd;
  fs.cur_paths.clear();
  std::filesystem::directory_iterator di(fs.cwd);
  for (auto& d : di) {
    if (d.is_directory()) fs.cur_paths.push_back(std::move(d));
  }
  std::sort(fs.cur_paths.begin(), fs.cur_paths.end(), [](auto a, auto b) { return a < b; });
  fs.filter_string.len = 0;
}

static void file_saver_change_cwd_up(br_file_saver_t& fs) {
  file_saver_change_cwd_to(fs, fs.cwd.parent_path());
}

void br_file_explorer_filter(br_file_saver_t* fs) {
  auto io = ImGui::GetIO();
  if (CheckCollisionPointRec(GetMousePosition(), fs->file_list_rect)) {
    int kp = GetCharPressed();
    if (kp == 0 && fs->filter_timeout > std::chrono::milliseconds(0)) {
      fs->filter_timeout -= std::chrono::milliseconds((int)(io.DeltaTime * 1000.f));
    }
    else if (kp == 0) {
      fs->filter_timeout = std::chrono::milliseconds(0);
      fs->filter_string.len = 0;
    }
    else if (kp != 0 && fs->filter_timeout > std::chrono::milliseconds(0)) {
      fs->filter_timeout = std::chrono::milliseconds(3000);
      br_str_push_char(&fs->filter_string, (char)kp);
    } else {
      fs->filter_timeout = std::chrono::milliseconds(3000);
      fs->filter_string.len = 0;
      br_str_push_char(&fs->filter_string, (char)kp);
    }
  }
}

br_file_saver_state_t br_file_explorer(br_file_saver_t* fs) {
  ImGui::Begin("File Saver");
  ImGui::LabelText("##CurName", "%s/%.*s.png", fs->cwd.string().c_str(), fs->name.len, fs->name.str);
  br_file_explorer_filter(fs);
  if (fs->last_read != fs->cwd) {
    file_saver_change_cwd_to(*fs, std::move(fs->cwd));
  }
  if (ImGui::Button("..")) {
    file_saver_change_cwd_up(*fs);
  }
  float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
  ImGuiStyle& style = ImGui::GetStyle();
  auto window = ImGui::GetCurrentWindowRead();
  ImVec2 pos = window->DC.CursorPos;
  fs->file_list_rect.x = pos.x;
  fs->file_list_rect.y = pos.y;
  bool any = false;
  auto new_button_size = fs->default_button_sz;
  for (size_t i = 0; i < fs->cur_paths.size(); i++) {
    std::string_view f = std::string_view(fs->filter_string.str, fs->filter_string.len);
    if (fs->filter_string.len != 0 && std::string_view(fs->cur_paths[i].filename().string().c_str()).find(f) == std::string::npos) continue;
    any = true;
    ImGui::PushID((int)i);
    auto txt = fs->cur_paths[i].filename().string();
    new_button_size = br::max(new_button_size, ImGui::CalcTextSize(txt.c_str()) + ImVec2{ 10.f, 0.f });

    if (ImGui::Button(txt.c_str(), fs->cur_button_sz)) {
      file_saver_change_cwd_to(*fs, std::move(fs->cur_paths[i]));
    }
    float last_button_x2 = ImGui::GetItemRectMax().x;
    float next_button_x2 = last_button_x2 + style.ItemSpacing.x + fs->cur_button_sz.x;
    if (i + 1 < fs->cur_paths.size() && next_button_x2 < window_visible_x2) ImGui::SameLine();
    ImGui::PopID();
    pos = window->DC.CursorPos;
    fs->file_list_rect.width = fmaxf(fs->file_list_rect.width, pos.x - fs->file_list_rect.x);
    fs->file_list_rect.height = fmaxf(fs->file_list_rect.height, pos.y - fs->file_list_rect.y);
  }
  fs->cur_button_sz = new_button_size;
  if (any == false) fs->filter_string.len = 0;
  ImGui::NewLine();
  br::ui_textbox(".png", &fs->name);
  auto ret = file_saver_state_exploring;

  if (ImGui::Button("Save")) ret = file_saver_state_accept;
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) ret = file_saver_state_cancle;
  ImGui::End();
  return ret;
}

