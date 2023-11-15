#include "imgui_extensions.h"
#include "imgui_internal.h"

namespace ImGui {
  bool IsWindowHidden() {
    return ImGui::GetCurrentWindowRead()->Hidden;
  }
}
