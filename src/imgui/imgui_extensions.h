#pragma once
#include "raylib.h"
#include "imgui.h"
#include "src/br_plot.h"

namespace ImGui {
  bool IsWindowHidden();
}

namespace br {
  bool Input(const char* label, Vector2& a);
  bool Input(const char* label, Vector3& a);
  Vector2 ToRaylib(ImVec2 vec);
  bool IsMouseOwerWindow();
  void ui_settings(br_plot_t* br);
  void ui_info(br_plot_t* br);
  void ui_textbox(const char* label, br_str_t* str);
}
