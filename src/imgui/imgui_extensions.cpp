#include "imgui_extensions.h"
#include "imgui_internal.h"
#include "src/br_help.h"

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
      br_str_t* str1 = (br_str_t*)data->UserData;
      if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
      {
        // Resize string callback
        // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
        IM_ASSERT(data->Buf == str1->str);
        if (data->BufTextLen + 1 >= (int)str1->cap) br_str_realloc(str1, (1 + maxui64(str1->cap * 2, (size_t)data->BufTextLen)));
        str1->len = (unsigned int)data->BufTextLen;
        data->Buf = str1->str;
      } else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        //str->len = data->BufTextLen;
      } else {
        fprintf(stderr, "Unhandled event: %d\n", data->EventFlag);
      }
      //    ImGuiInputTextFlags_CallbackCompletion  = 1 << 6,   // Callback on pressing TAB (for completion handling)
      //    ImGuiInputTextFlags_CallbackHistory     = 1 << 7,   // Callback on pressing Up/Down arrows (for history handling)
      //    ImGuiInputTextFlags_CallbackAlways      = 1 << 8,   // Callback on each iteration. User code may query cursor position, modify text buffer.
      //    ImGuiInputTextFlags_CallbackCharFilter  = 1 << 9,   // Callback on character inputs to replace or discard them. Modify 'EventChar' to replace or discard, or return 1 in callback to discard.
      return 0;
    }, str);
  }

  void ShowMatrix(const char* name, Matrix m) {
    ImGui::PushID(name);
    ImGui::LabelText("##name", "%s", name);
    ImGui::LabelText("##0", "%5.4f %5.4f %.4f %.4f", m.m0, m.m1, m.m2, m.m3); 
    ImGui::LabelText("##1", "%.4f %.4f %.4f %.4f", m.m4, m.m5, m.m6, m.m7); 
    ImGui::LabelText("##2", "%.4f %.4f %.4f %.4f", m.m8, m.m9, m.m10, m.m11); 
    ImGui::LabelText("##3", "%.4f %.4f %.4f %.4f", m.m12, m.m13, m.m14, m.m15); 
    ImGui::PopID();
  }
}
