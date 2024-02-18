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


static Vector3 translate;
extern "C" void br_hot_init(br_plot_t* gv) {
  fprintf(stderr, "First call\n");
  translate = {0,0,1};
}

Vector3 Vector3MultiplyValue(Vector3 v, float s) {
  return Vector3 { v.x * s, v.y * s, v.z * s };
}

extern "C" void br_hot_loop(br_plot_t* gv) {
  ImGui::Begin("3d");
  br::Input("Eyes", gv->eye);
  br::Input("Target", gv->target);
  br::Input("Up", gv->up);
  br::Input("Trans", translate);
  if (ImGui::Button("Reset") || IsKeyPressed(KEY_R)) {
    gv->eye = Vector3 { 0, 0, -1 };
    gv->target = Vector3 { 0, 0, 0 };
    gv->up = Vector3 { 0, 1, 0 };
    translate = Vector3 { 0, 0, 0 };
  }
  ImGui::InputFloat("FovY", &gv->fov_y);
  ImGui::InputFloat("Near", &gv->near_plane);
  ImGui::InputFloat("Far", &gv->far_plane);
  ShowMatrix("ViewMatrix", gv->uvMatView);
  ShowMatrix("Projection", gv->uvMatProjection);
  ShowMatrix("MVP", gv->uvMvp);
  ShowMatrix("Rl Project", rlGetMatrixProjection());
  ShowMatrix("Rl ModelView", rlGetMatrixTransform());
  if (IsKeyDown(KEY_UP)) {
     Vector3 diff = Vector3MultiplyValue(gv->up, 0.01f);
     translate = Vector3Add(translate, diff);
     //gv->target = Vector3Add(gv->target, diff);
     //gv->eye = Vector3Add(gv->eye, diff);
  }

  if (IsKeyDown(KEY_DOWN)) {
     Vector3 diff = Vector3MultiplyValue(gv->up, -0.01f);
     translate = Vector3Add(translate, diff);
     //gv->target = Vector3Add(gv->target, diff);
     //gv->eye = Vector3Add(gv->eye, diff);
  }

  if (IsKeyDown(KEY_LEFT)) {
     Vector3 diff = Vector3MultiplyValue(Vector3CrossProduct(Vector3Subtract(gv->target, gv->eye), gv->up), 0.01f);
     translate = Vector3Add(translate, diff);
     //gv->target = Vector3Add(gv->target, diff);
     //gv->eye = Vector3Add(gv->eye, diff);
  }

  if (IsKeyDown(KEY_RIGHT)) {
     Vector3 diff = Vector3MultiplyValue(Vector3CrossProduct(Vector3Subtract(gv->target, gv->eye), gv->up), -0.01f);
     translate = Vector3Add(translate, diff);
     //gv->target = Vector3Add(gv->target, diff);
     //gv->eye = Vector3Add(gv->eye, diff);
  }

  if (IsKeyDown(KEY_J)) {
    Vector3 diff = Vector3Subtract(gv->target, gv->eye);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(diff, gv->up));
    gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, right, 0.01f), gv->eye);
    gv->up = Vector3RotateByAxisAngle(gv->up, right, 0.01f);
  }

  if (IsKeyDown(KEY_K)) {
    Vector3 diff = Vector3Subtract(gv->target, gv->eye);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(diff, gv->up));
    gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, right, -0.01f), gv->eye);
    gv->up = Vector3RotateByAxisAngle(gv->up, right, -0.01f);
  }
  
  if (IsKeyDown(KEY_L)) {
    Vector3 diff = Vector3Subtract(gv->target, gv->eye);
    gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, Vector3 { 0.f, 1.f, 0.f }, 0.01f), gv->eye);
  }

  if (IsKeyDown(KEY_H)) {
    Vector3 diff = Vector3Subtract(gv->target, gv->eye);
    gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, Vector3 { 0.f, 1.f, 0.f }, -0.01f), gv->eye);
  }

  //rlSetUniformMatrix(gv->grid3dShader.uMatProjection, gv->uvMatProjection);

  ImGui::End();
  ImGui::Begin("3d view");
    glUseProgram(gv->grid3dShader.shader.id);
    ImVec2 v = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    Vector2 screen = { (float)GetRenderWidth(), (float)GetRenderHeight() };
    Rectangle r = { .x = v.x / screen.x * 2 - 1, .y = 1.f - 2.f*(v.y + size.y) / screen.y, .width = 2 * size.x / screen.x, .height = 2 * size.y / screen.y };
    Rectangle r1 = { .x = v.x, .y = GetScreenHeight() - v.y , .width = size.x, .height = -size.y };

    int loc = GetShaderLocation(gv->grid3dShader.shader, "eye");
    rlSetUniform(gv->grid3dShader.uResolution, &r1, RL_SHADER_UNIFORM_VEC4, 1);
    rlSetUniform(gv->grid3dShader.uScreen, &screen, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(gv->grid3dShader.uOffset, &translate, RL_SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(loc, &gv->eye, RL_SHADER_UNIFORM_VEC3, 1);

    gv->uvMatProjection = MatrixPerspective(gv->fov_y, 1.0f, gv->near_plane, gv->far_plane);
    gv->uvMatView = MatrixLookAt(gv->eye, gv->target, gv->up);
    Matrix transMatrix = MatrixTranslate(translate.x, translate.y, translate.z);
    gv->uvMvp = MatrixMultiply(gv->uvMatView, gv->uvMatProjection);
    rlSetUniformMatrix(gv->grid3dShader.uMvp, gv->uvMvp);
    
    smol_mesh_gen_quad_simple(gv->graph_mesh_3d, r, RED);
    smol_mesh_update(gv->graph_mesh_3d);
    smol_mesh_draw(gv->graph_mesh_3d);
  ImGui::End();
}

