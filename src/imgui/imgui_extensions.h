#pragma once
#include "src/br_plot.h"

#include "raylib.h"
#include "imgui.h"

#include <algorithm>

#if defined(__GNUC__)
#  define BR_UNUSED_FN __attribute__ ((__unused__))
#elif defined(_MSC_VER)
#  define BR_UNUSED_FN
#endif


namespace ImGui {
  bool IsWindowHidden();
}

namespace br {
  bool Input(const char* label, Vector2& a);
  bool Input(const char* label, Vector3& a);
  Vector2 ToRaylib(ImVec2 vec);
  bool IsMouseOwerWindow();
  void ui_settings(br_plotter_t* br);
  void ui_info(br_plotter_t* br);
  void ui_textbox(const char* label, br_str_t* str);
  void ShowMatrix(const char* name, Matrix m);
  BR_UNUSED_FN inline ImVec2 max(ImVec2 a, ImVec2 b) { return ImVec2 { std::max(a.x, b.x), std::max(a.y, b.y) }; }
}

struct br_file_saver_s;

typedef enum {
  file_saver_state_exploring,
  file_saver_state_accept,
  file_saver_state_cancle
} br_file_saver_state_t;

br_file_saver_state_t br_file_explorer(struct br_file_saver_s* fs);
struct br_file_saver_s* br_file_saver_malloc(char const* title, char const* location, br_plot_t const* plot);
void br_file_saver_get_path(struct br_file_saver_s const* fs, br_str_t* path);
br_plot_t const* br_file_saver_get_plot_instance(struct br_file_saver_s const* fs);
void br_file_saver_free(struct br_file_saver_s* fs);
