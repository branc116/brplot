
#include "src/br_plot.h"
#include "src/imgui/imgui_extensions.h"
#include "stdio.h"
#include "raylib.h"
#include "cstdlib"
#include "imgui.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <chrono>

void hot_ui_settings(br_plot_t* gv) {

}

extern "C" void br_hot_init(br_plot_t* gv) {
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  fprintf(stderr, "First call\n");
}

extern "C" void br_hot_loop(br_plot_t* gv) {
}

