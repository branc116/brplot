#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "raylib.h"
#include "GLFW/glfw3.h"
#include "plotter.h"

int main() {
  std::cout << "Hello World" << std::endl;
  InitWindow(1280, 720, "brplot");
  GLFWwindow* ctx = glfwGetCurrentContext();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
  //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
      style.WindowRounding = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  const char* glsl_version = "#version 130";
  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(ctx, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  bool show_demo_window = true;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  graph_values_t gv;
  graph_init(&gv, 500, 500);

  while (false == WindowShouldClose()) {
    BeginDrawing();
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    if (show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);

    if (ImGui::BeginChild("Test")) {
      ImVec2 p = ImGui::GetWindowPos();
      ImVec2 size = ImGui::GetWindowSize();
      graph_draw_min(&gv, p.x, p.y, size.x, size.y);
    }
    ImGui::EndChild();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(ctx, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    EndDrawing();
    glClear(GL_COLOR_BUFFER_BIT);
  }
  CloseWindow();
  
  return 0;
}
