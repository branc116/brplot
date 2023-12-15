#include "imgui_extensions.h"
#include "imgui_internal.h"

namespace ImGui {
  bool IsWindowHidden() {
    return ImGui::GetCurrentWindowRead()->Hidden;
  }
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

  Vector2 ToRaylib(ImVec2 vec) {
    return Vector2{vec.x, vec.y};
  }

  bool IsMouseOwerWindow() {
    auto pos = ImGui::GetWindowPos();
    auto size = ImGui::GetWindowSize();
    Rectangle r{pos.x, pos.y, size.x, size.y};
    return CheckCollisionPointRec(ToRaylib(ImGui::GetMousePos()), r);
  }
}
