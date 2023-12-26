#include "src/br_plot.h"
#include "src/br_gui_internal.h"
#include "imgui_extensions.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"

GLFWwindow* ctx;
#define DEFAULT_INI "" \
"[Window][DockSpaceViewport_11111111]\n" \
"Size=1280,720\n" \
"\n" \
"[Window][Info]\n" \
"Pos=1007,0\n" \
"DockId=0x00000002,1\n" \
"\n" \
"[Window][Settings]\n" \
"Pos=1007,0\n" \
"DockId=0x00000002,0\n" \
"\n" \
"[Window][Plot]\n" \
"Pos=0,0\n" \
"Size=1005,720\n" \
"DockId=0x00000001,0\n" \
"\n" \
"[Docking][Data]\n" \
"DockSpace   ID=0x8B93E3BD Window=0xA787BDB4 Pos=0,0 Size=1280,720 Split=X\n" \
"  DockNode  ID=0x00000001 Parent=0x8B93E3BD SizeRef=1005,720\n" \
"  DockNode  ID=0x00000002 Parent=0x8B93E3BD SizeRef=273,720\n"


extern "C" void br_gui_init_specifics_gui(br_plot_t* br) {
  (void)br;
  ctx = glfwGetCurrentContext();
  ImGui::SetAllocatorFunctions(BR_IMGUI_MALLOC, BR_IMGUI_FREE, nullptr);
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
#ifdef PLATFORM_WEB
  const char* glsl_version = "#version 100";
  io.IniFilename = nullptr;
#elif PLATFORM_DESKTOP 
  const char* glsl_version = "#version 330";
#endif
  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(ctx, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImGuiStyle& s = ImGui::GetStyle();
  s.Colors[ImGuiCol_WindowBg].w = 0.f;
#ifndef RELEASE
#ifdef LINUX
  br_hotreload_start(&br->hot_state);
#endif
#endif
#ifdef PLATFORM_WEB
    ImGui::LoadIniSettingsFromMemory(DEFAULT_INI);
#endif
}
extern "C" void br_gui_free_specifics(br_plot_t* br) {
  (void)br;
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

static bool show_demo_window = true;
static ImVec4 clear_color = ImVec4(.0f, .0f, .0f, 1.00f);
static float padding = 50.f;

void graph_draw_min(br_plot_t* br, float posx, float posy, float width, float height, float padding) {
  br->uvScreen.x = (float)GetScreenWidth();
  br->uvScreen.y = (float)GetScreenHeight();
  br->graph_screen_rect.x = 50.f + posx + padding;
  br->graph_screen_rect.y = posy + padding;
  br->graph_screen_rect.width = width - 50.f - 2.f * padding;
  br->graph_screen_rect.height = height - 30.f - 2.f * padding;
  update_variables(br);
  BeginScissorMode((int)posx, (int)posy, (int)width, (int)height);
    DrawRectangleRec(br->graph_screen_rect, BLACK);
    draw_grid_values(br);
    graph_draw_grid(br->gridShader.shader, br->graph_screen_rect);
    points_groups_draw(&br->groups, {
      .line_mesh = br->lines_mesh,
      .quad_mesh = br->quads_mesh,
      .rect = br->graph_rect,
      .mouse_pos_graph = br->mouse_graph_pos,
      .show_x_closest = br->show_x_closest,
      .show_y_closest = br->show_y_closest,
      .show_closest = br->show_closest
    });
  EndScissorMode();
}

extern "C" void graph_draw(br_plot_t* gv) {
#ifndef RELEASE
#ifdef LINUX
  if (gv->hot_state.is_init_called == false && gv->hot_state.func_init != nullptr) {
    pthread_mutex_lock(&gv->hot_state.lock);
      if (gv->hot_state.func_init != nullptr) {
        gv->hot_state.func_init(gv);
        gv->hot_state.is_init_called = true;
      }
    pthread_mutex_unlock(&gv->hot_state.lock);
  }
#endif
#endif
  BeginDrawing();
  ImGui_ImplGlfw_NewFrame();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
  ImGui::DockSpaceOverViewport();
  if (ImGui::Begin("Plot") && false == ImGui::IsWindowHidden()) {
    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    graph_draw_min(gv, p.x, p.y, size.x, size.y, padding);
  }
  ImGui::End();
#ifndef RELEASE
#ifdef LINUX
  if (gv->hot_state.func_loop != nullptr) {
    pthread_mutex_lock(&gv->hot_state.lock);
      if (gv->hot_state.func_loop != nullptr) gv->hot_state.func_loop(gv);
    pthread_mutex_unlock(&gv->hot_state.lock);
  }
  if (show_demo_window) {
    ImGui::SetNextWindowBgAlpha(0.7f);
    ImGui::ShowDemoWindow(&show_demo_window);
  }
#endif
#endif
  br::ui_settings(gv);
  br::ui_info(gv);

  graph_frame_end(gv);
  ImGui::Render();
  int display_h = 0, display_w = 0;
  glfwGetFramebufferSize(ctx, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  EndDrawing();
#ifndef PLATFORM_WEB
  ClearBackground(BLACK);
#endif
}
