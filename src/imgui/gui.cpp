#include "src/br_plot.h"
#include "src/br_gui_internal.h"
#include "imgui_extensions.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"


GLFWwindow* ctx;

extern "C" void br_gui_init_specifics(br_plot_t* br) {
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

void graph_draw_min(br_plot_t* gv, float posx, float posy, float width, float height, float padding) {
  gv->uvScreen.x = (float)GetScreenWidth();
  gv->uvScreen.y = (float)GetScreenHeight();
  gv->graph_screen_rect.x = 50.f + posx + padding;
  gv->graph_screen_rect.y = posy + padding;
  gv->graph_screen_rect.width = width - 50.f - 2.f * padding;
  gv->graph_screen_rect.height = height - 30.f - 2.f * padding;
  update_variables(gv);
  BeginScissorMode((int)posx, (int)posy, (int)width, (int)height);
    DrawRectangleRec(gv->graph_screen_rect, BLACK);
    draw_grid_values(gv);
    graph_draw_grid(gv->gridShader.shader, gv->graph_screen_rect);
    points_groups_draw(&gv->groups, gv->lines_mesh, gv->quads_mesh, gv->graph_rect);
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
  if (ImGui::Begin("Test") && false == ImGui::IsWindowHidden()) {
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
  ImGui::SetNextWindowBgAlpha(0.7f);
  if (ImGui::Begin("Settings")) {
    ImGui::SliderFloat("Padding", &padding, 0.f, 100.f);
    ImGui::SliderFloat("Recoil", &gv->recoil, 0.f, 1.1f);
    ImGui::SliderFloat("Font scale", &context.font_scale, 0.5f, 2.5f);

    ImGui::Checkbox("Follow Arround", &gv->follow);
    ImGui::Checkbox("Debug Lines", &context.debug_bounds);
    ImGui::Checkbox("Jump Arround", &gv->jump_around);
    for (size_t i = 0; i < gv->groups.len; ++i) {
      sprintf(context.buff, "Plot %lu", i);
      ImGui::Checkbox(context.buff, &gv->groups.arr[i].is_selected);
      auto& c = gv->groups.arr[i].color;
      float colors[4] = { ((float)c.r)/255.f, ((float)c.g)/255.f, ((float)c.b)/255.f, ((float)c.a)/255.f};
      sprintf(context.buff, "PlotColor_%lu", i);
      ImGui::ColorPicker4(context.buff, colors);
      gv->groups.arr[i].color = { (unsigned char)(colors[0] * 255.f), (unsigned char)(colors[1] * 255.f), (unsigned char)(colors[2] * 255.f), (unsigned char)(colors[3] * 255.f) };
    }
  }
  ImGui::End();

  ImGui::SetNextWindowBgAlpha(0.7f);
  if (ImGui::Begin("Info") && false == ImGui::IsWindowHidden()) {
    //alloc_size, alloc_count, alloc_total_size, alloc_total_count, alloc_max_size, alloc_max_count, free_of_unknown_memory;
    if (ImGui::CollapsingHeader("Allocations")) {
      int s = sprintf(context.buff, "Allocations: %lu (%lu KB)", context.alloc_count, context.alloc_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Total Allocations: %lu (%lu KB)", context.alloc_total_count, context.alloc_total_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Max Allocations: %lu (%lu KB)", context.alloc_max_count, context.alloc_max_size >> 10); ImGui::TextUnformatted(context.buff, context.buff + s);
      s = sprintf(context.buff, "Unaccounted Allocations: >%lu", context.free_of_unknown_memory); ImGui::TextUnformatted(context.buff, context.buff + s);
    }
    if (ImGui::CollapsingHeader("Draw Debug")) {
      int s = sprintf(context.buff, "Draw Calls: %lu (%lu lines)", gv->lines_mesh->draw_calls, gv->lines_mesh->points_drawn); ImGui::TextUnformatted(context.buff, context.buff + s);
      for (size_t i = 0; i < gv->groups.len; ++i) {
        auto& a = gv->groups.arr[i];
        s = sprintf(context.buff, "Line #%d intervals(%lu):", a.group_id, a.resampling->intervals_count); ImGui::TextUnformatted(context.buff, context.buff + s);
        for (size_t j = 0; j < a.resampling->intervals_count; ++j) {
          auto& inter = a.resampling->intervals[j];
          s = sprintf(context.buff, "   %lu [%lu - %lu] ", inter.count, inter.from, inter.from + inter.count);
          help_resampling_dir_to_str(&context.buff[s], inter.dir);
          s = (int)strlen(context.buff);
          ImGui::TextUnformatted(context.buff, context.buff + s);
        }
      }
    }
    if (ImGui::CollapsingHeader("Queue")) {
      auto const& qc = gv->commands;
      int s = sprintf(context.buff, "Read  index: %lu", qc.read_index);                                                ImGui::TextUnformatted(context.buff, context.buff + s);
          s = sprintf(context.buff, "Write index: %lu", qc.write_index);                                               ImGui::TextUnformatted(context.buff, context.buff + s);
          s = sprintf(context.buff, "Length:      %lu", (qc.write_index - qc.read_index + qc.capacity) % qc.capacity); ImGui::TextUnformatted(context.buff, context.buff + s);
    }
  }
  ImGui::End();

  graph_frame_end(gv);
  ImGui::Render();
  int display_h = 0, display_w = 0;
  glfwGetFramebufferSize(ctx, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  EndDrawing();
  ClearBackground(BLACK);
}
