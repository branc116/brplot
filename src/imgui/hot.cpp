#include "src/br_plot.h"
#include "src/br_gui_internal.h"
#include "src/imgui/imgui_extensions.h"
#include "raylib.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "rlgl.h"
#include "external/glad.h"
#include "raymath.h"

void ShowMatrix(const char* name, Matrix m) {
  ImGui::PushID(name);
  ImGui::LabelText("##name", "%s", name);
  ImGui::LabelText("##0", "%5.4f %5.4f %.4f %.4f", m.m0, m.m1, m.m2, m.m3); 
  ImGui::LabelText("##1", "%.4f %.4f %.4f %.4f", m.m4, m.m5, m.m6, m.m7); 
  ImGui::LabelText("##2", "%.4f %.4f %.4f %.4f", m.m8, m.m9, m.m10, m.m11); 
  ImGui::LabelText("##3", "%.4f %.4f %.4f %.4f", m.m12, m.m13, m.m14, m.m15); 
  ImGui::PopID();
}

extern "C" void br_hot_init(br_plot_t* gv) {
  fprintf(stderr, "First call\n");
}

Vector3 Vector3MultiplyValue(Vector3 v, float s) {
  return Vector3 { v.x * s, v.y * s, v.z * s };
}

extern "C" void br_hot_loop(br_plot_t* gv) {
  ImGui::Begin("3d");
  br::Input("Eyes", gv->eye);
  br::Input("Target", gv->target);
  ImGui::InputFloat("FovY", &gv->fov_y);
  ImGui::InputFloat("Near", &gv->near_plane);
  ImGui::InputFloat("Far", &gv->far_plane);
  ShowMatrix("ViewMatrix", gv->uvMatView);
  ShowMatrix("Projection", gv->uvMatProjection);
  ShowMatrix("MVP", gv->uvMvp);
  ShowMatrix("Rl Project", rlGetMatrixProjection());
  ShowMatrix("Rl ModelView", rlGetMatrixTransform());
  if (IsKeyDown(KEY_UP)) {
     Vector3 diff = Vector3MultiplyValue(Vector3Subtract(gv->target, gv->eye), 0.01f);
     gv->target = Vector3Add(gv->target, diff);
     gv->eye = Vector3Add(gv->eye, diff);
  }

  if (IsKeyDown(KEY_DOWN)) {
     Vector3 diff = Vector3MultiplyValue(Vector3Subtract(gv->target, gv->eye), -0.01f);
     gv->target = Vector3Add(gv->target, diff);
     gv->eye = Vector3Add(gv->eye, diff);
  }

  //rlSetUniformMatrix(gv->grid3dShader.uMatProjection, gv->uvMatProjection);

  ImGui::End();
  ImGui::Begin("3d view");
  glUseProgram(gv->grid3dShader.shader.id);
  ImVec2 v = ImGui::GetWindowPos();
  ImVec2 size = ImGui::GetWindowSize();
  Vector2 screen = { (float)GetScreenWidth(), (float)GetScreenHeight() };
  Rectangle r = { .x = v.x / screen.x * 2 - 1, .y = ((screen.y - v.y)*2.f / screen.y) - 1.5f, .width = 2*size.x / screen.x, .height = 2*size.y / screen.y };
  Rectangle r1 = { .x = v.x, .y = GetScreenHeight() - v.y , .width = size.x, .height = -size.y };
  rlSetUniform(gv->grid3dShader.uResolution, &r1, RL_SHADER_UNIFORM_VEC4, 1);
  rlSetUniform(gv->grid3dShader.uScreen, &screen, RL_SHADER_UNIFORM_VEC2, 1);
  
  smol_mesh_gen_quad_simple(gv->graph_mesh_3d, r, RED);
  smol_mesh_update(gv->graph_mesh_3d);
  smol_mesh_draw(gv->graph_mesh_3d);
  ImGui::End();
}

