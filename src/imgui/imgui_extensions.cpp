#include "imgui_extensions.h"
#include "imgui_internal.h"
#include "src/br_help.h"
#include "src/br_plot.h"
#include <algorithm>

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
  void ui_textbox(const char* label, br_str_t *str) {
    if (str->len >= str->cap) br_str_realloc(str, str->cap * 2);
    str->str[str->len] = 0;
    ImGui::InputText(label, str->str, str->cap, ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) {
      printf("Handled event: %d\n", data->EventFlag);
      br_str_t* str = (br_str_t*)data->UserData;
      if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
      {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        IM_ASSERT(data->Buf == str->str);
        if (data->BufTextLen + 1 >= (int)str->cap) br_str_realloc(str, (1 + maxui64(str->cap * 2, (size_t)data->BufTextLen)));
        str->len = (unsigned int)data->BufTextLen;
        data->Buf = str->str;
      } else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        //str->len = data->BufTextLen;
      }
      else {
        fprintf(stderr, "Unhandled event: %d\n", data->EventFlag);
      }
      //    ImGuiInputTextFlags_CallbackCompletion  = 1 << 6,   // Callback on pressing TAB (for completion handling)
      //    ImGuiInputTextFlags_CallbackHistory     = 1 << 7,   // Callback on pressing Up/Down arrows (for history handling)
      //    ImGuiInputTextFlags_CallbackAlways      = 1 << 8,   // Callback on each iteration. User code may query cursor position, modify text buffer.
      //    ImGuiInputTextFlags_CallbackCharFilter  = 1 << 9,   // Callback on character inputs to replace or discard them. Modify 'EventChar' to replace or discard, or return 1 in callback to discard.
      return 0;
    }, str);
  }
}
