#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_pp.h"
#include "src/br_smol_mesh.h"
#include "src/br_permastate.h"
#include "imgui_extensions.h"

#include "file_saver.cpp"
#ifndef RELEASE
#  include "external/imgui-docking/imgui_demo.cpp"
#endif
#include "imgui_extensions.cpp"
#include "ui_info.cpp"
#include "ui_settings.cpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#if !defined(PLATFORM_WEB)
#  include "backends/imgui_impl_opengl3_loader.h"
#endif
#include "external/glfw/include/GLFW/glfw3.h"
#include "tracy/TracyC.h"

#include <cstdlib>

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
"[Window][Plot #0]\n" \
"Pos=0,0\n" \
"Size=1005,720\n" \
"DockId=0x00000001,0\n" \
"\n" \
"[Docking][Data]\n" \
"DockSpace   ID=0x8B93E3BD Window=0xA787BDB4 Pos=0,0 Size=1280,720 Split=X\n" \
"  DockNode  ID=0x00000001 Parent=0x8B93E3BD SizeRef=1005,720\n" \
"  DockNode  ID=0x00000002 Parent=0x8B93E3BD SizeRef=273,720\n"

static void br_plot_screenshot_imgui(br_plot_t br, br_datas_t groups, char* path);

static int screenshot_file_save = 0;
static struct br_file_saver_s* fs = nullptr;

static ImVec4 clear_color = ImVec4(.0f, .0f, .0f, 1.00f);
static float padding = 50.f;

static GLFWwindow* ctx;

extern "C" void br_gui_init_specifics_gui(br_plotter_t* br) {
  if (false == br->loaded) br_plotter_add_plot_2d(br);

  ctx = glfwGetCurrentContext();
  ImGui::SetAllocatorFunctions(BR_IMGUI_MALLOC, BR_IMGUI_FREE, nullptr);
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();
#if defined(PLATFORM_WEB)
  const char* glsl_version = "#version 100";
  io.IniFilename = nullptr;
#elif defined(PLATFORM_DESKTOP)
  const char* glsl_version = "#version 330";
#endif
  ImGui_ImplGlfw_InitForOpenGL(ctx, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImGuiStyle& s = ImGui::GetStyle();
  s.Colors[ImGuiCol_WindowBg].w = 0.f;
#if !defined(RELEASE) && defined(__linux__)
  br_hotreload_start(&br->hot_state);
#endif
    ImGui::LoadIniSettingsFromMemory(DEFAULT_INI);
#if !defined(PLATFORM_WEB)
    ImGui::LoadIniSettingsFromDisk("imgui.ini");
#endif
}

extern "C" void br_gui_free_specifics(br_plotter_t* br) {
  (void)br;
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void graph_draw_min(br_datas_t groups, br_plot_t* plot, float posx, float posy, float width, float height, float padding) {
  TracyCFrameMarkStart("graph_draw_min");
  plot->resolution.x = (float)GetScreenWidth();
  plot->resolution.y = (float)GetScreenHeight();
  switch (plot->kind) {
    case br_plot_kind_2d: {
      plot->graph_screen_rect.x = 50.f + posx + padding;
      plot->graph_screen_rect.y = posy + padding;
      plot->graph_screen_rect.width = width - 50.f - 2.f * padding;
      plot->graph_screen_rect.height = height - 30.f - 2.f * padding;
    } break;
    case br_plot_kind_3d: {
      plot->graph_screen_rect.x = posx;
      plot->graph_screen_rect.y = posy;
      plot->graph_screen_rect.width = width;
      plot->graph_screen_rect.height = height;
    } break;
    default: assert(0);
  }
  br_plot_update_shader_values(plot);

  //DrawRectangleRec(plot->graph_screen_rect, BLACK);
  draw_grid_numbers(plot);
  smol_mesh_grid_draw(plot);
  br_datas_draw(groups, plot);
  TracyCFrameMarkEnd("graph_draw_min");
}

extern "C" void br_plotter_draw(br_plotter_t* gv) {
#if !defined(RELEASE) && defined(__linux__)
  if (gv->hot_state.is_init_called == false && gv->hot_state.func_init != nullptr) {
    pthread_mutex_lock(&gv->hot_state.lock);
      if (gv->hot_state.func_init != nullptr) {
        gv->hot_state.func_init(gv);
        gv->hot_state.is_init_called = true;
      }
    pthread_mutex_unlock(&gv->hot_state.lock);
  }
#endif
  BeginDrawing();
  TracyCFrameMarkStart("BeginDrawing");
  ImGui_ImplGlfw_NewFrame();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
  ImGui::DockSpaceOverViewport();
  br_plotter_update_variables(gv);
  Vector2 mouse_pos = GetMousePosition();
  ImGuiID prev = 0;
  bool switching = false;

  for (int i = 0; i < gv->plots.len; ++i) {
    ImGui::PushID(i);
    snprintf(context.buff, IM_ARRAYSIZE(context.buff), "Plot #%d", i);
    ImGui::SetNextWindowDockID(prev, ImGuiCond_FirstUseEver);
    if (switching && gv->active_plot_index == i) {
      ImGui::SetNextWindowFocus();
      gv->switch_to_active = false;
    }
    if ( ImGui::Begin(context.buff) && false == ImGui::IsWindowHidden() && ((false == switching) || (switching && gv->active_plot_index == i))){
      br_plot_update_variables(gv, &gv->plots.arr[i], gv->groups, mouse_pos); // Invalidates plots
      ImVec2 p = ImGui::GetWindowPos();
      ImVec2 size = ImGui::GetWindowSize();
      graph_draw_min(gv->groups,  &gv->plots.arr[i], p.x, p.y, size.x, size.y, padding);
      gv->active_plot_index = switching ? gv->active_plot_index : i;
    }
    switching = false;
    prev = ImGui::GetWindowDockID();
    ImGui::End();
    ImGui::PopID();
  }
#if !defined(RELEASE) && defined(__linux__)
  if (gv->hot_state.func_loop != nullptr) {
    pthread_mutex_lock(&gv->hot_state.lock);
      if (gv->hot_state.func_loop != nullptr) gv->hot_state.func_loop(gv);
    pthread_mutex_unlock(&gv->hot_state.lock);
  }
  ImGui::SetNextWindowBgAlpha(0.7f);
  ImGui::ShowDemoWindow();
#endif
  br::ui_settings(gv);
  br::ui_info(gv);
  if (screenshot_file_save == 1) {
    br_file_saver_state_t state = br_file_explorer(fs);
    switch (state) {
      case file_saver_state_exploring: break;
      case file_saver_state_accept: {
        br_str_t s = br_str_malloc(64);
        br_file_saver_get_path(fs, &s);
        br_plot_screenshot_imgui(*br_file_saver_get_plot_instance(fs), gv->groups, br_str_move_to_c_str(&s));
      } // FALLTHROUGH
      case file_saver_state_cancle: {
        br_file_saver_free(fs);
        screenshot_file_save = 0;
      } break;
    }
  }

  br_plotter_frame_end(gv);
  ImGui::Render();
  int display_h = 0, display_w = 0;
  glfwGetFramebufferSize(ctx, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  TracyCFrameMarkEnd("BeginDrawing");
  EndDrawing();
#if !defined(PLATFORM_WEB)
  ClearBackground(BLACK);
#endif
}

extern "C" void br_plot_screenshot(br_plot_t* plot, br_datas_t, char const*) {
  fs = br_file_saver_malloc("Save screenshot", std::getenv("HOME"), plot);
  screenshot_file_save = 1;
  return;
}

static void br_plot_screenshot_imgui(br_plot_t plot, br_datas_t groups, char* path) {
  float left_pad = 80.f;
  float bottom_pad = 80.f;
  Vector2 is = {1280, 720};
  RenderTexture2D target = LoadRenderTexture((int)is.x, (int)is.y); // TODO: make this values user defined.
  plot.graph_screen_rect = {left_pad, 0.f, is.x - left_pad, is.y - bottom_pad};
  plot.resolution = {is.x, is.y};
  br_plot_update_context(&plot, GetMousePosition());
  br_plot_update_shader_values(&plot);
  BeginTextureMode(target);
    smol_mesh_grid_draw(&plot);
    br_datas_draw(groups, &plot);
    draw_grid_numbers(&plot);
  EndTextureMode();
  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  ExportImage(img, path);
  UnloadImage(img);
  UnloadRenderTexture(target);
  BR_FREE(path);
}

