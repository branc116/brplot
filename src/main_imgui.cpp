#include <iostream>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_extensions.h"
#include "raylib.h"
#include "GLFW/glfw3.h"
#include "plotter.h"

extern context_t context;

int main() {
  int display_w = 1280, display_h = 720;
#ifdef RELEASE
  SetTraceLogLevel(LOG_ERROR);
#endif
  SetWindowState(FLAG_MSAA_4X_HINT);
  InitWindow(display_w, display_h, "brplot");
  SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  SetExitKey(KEY_NULL);

  graph_values_t* gv = (graph_values_t*)malloc(sizeof(graph_values_t));
  graph_init(gv, 1, 1);
#ifndef RELEASE
  start_refreshing_shaders(gv);
#endif
  read_input_main(gv);

  GLFWwindow* ctx = glfwGetCurrentContext();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
  const char* glsl_version = "#version 330";
  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(ctx, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  bool show_demo_window = true;
  ImVec4 clear_color = ImVec4(.0f, .0f, .0f, 1.00f);
  ImGuiStyle& s = ImGui::GetStyle();
  s.Colors[ImGuiCol_WindowBg].w = 0.f;
  float padding = 10.f;

  while (false == WindowShouldClose()) {
    BeginDrawing();
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
    if (ImGui::Begin("Test")) {
      ImVec2 p = ImGui::GetWindowPos();
      ImVec2 size = ImGui::GetWindowSize();
      graph_draw_min(gv, p.x, p.y, size.x, size.y, padding);
    }
    ImGui::End();
#ifndef RELEASE
    if (show_demo_window) {
      ImGui::SetNextWindowBgAlpha(0.7f);
      ImGui::ShowDemoWindow(&show_demo_window);
    }
#endif
    if (ImGui::Begin("Settings")) {
      ImGui::SliderFloat("Padding", &padding, 0.f, 100.f);
    }
    ImGui::End();

    ImGui::Render();
    glfwGetFramebufferSize(ctx, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    EndDrawing();
    glClear(GL_COLOR_BUFFER_BIT);
  }

  //Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  CloseWindow();
  read_input_stop();
  graph_free(gv);
  free(gv);
  return 0;
}
