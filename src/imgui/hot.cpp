#include "src/br_plot.h"
#include "src/br_gui_internal.h"
#include "src/imgui/imgui_extensions.h"
#include "src/br_plotter.h"
#include "raylib.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "rlgl.h"
#include "external/glad.h"
#include "raymath.h"
#include "src/br_da.h"

extern "C" void br_hot_init(br_plotter_t* gv) {
  fprintf(stderr, "First call\n");
}

bool focused = false;
float translate_speed = 1.0f;

extern "C" void br_hot_loop(br_plotter_t* br) {
  if (br->plots.len < 2) return;
  if (ImGui::Begin("TestMath")) {
    Matrix mvp = br->plots.arr[1].ddd.grid_shader->uvs.m_mvp_uv;
    Vector3 eye = br->plots.arr[1].ddd.eye;
    Vector2 v = { 3, 3 };
    Vector2 vt2 = Vector2TransformScale(v, mvp);
    ImGui::Text("(%f, %f) -> (%f, %f)", v.x, v.y, vt2.x, vt2.y);
    ImGui::Text("(%f, %f, %f)", eye.x, eye.y, eye.z);
  }
  ImGui::End();

#if 0
  //rlSetUniformMatrix(gv->grid3dShader.uMatProjection, gv->uvMatProjection);
  if (ImGui::Begin("3d View 2")) {
    ImVec2 v = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    if (CheckCollisionPointRec(GetMousePosition(), Rectangle { .x = v.x, .y = v.y, .width = size.x, .height = size.y })) {
      float mw = GetMouseWheelMove();
      
      if (IsKeyPressed(KEY_F)) {
        focused = !focused;
      }

      if (focused) {
        HideCursor();
        Vector2 delta = GetMouseDelta();
        float dt = GetFrameTime();
        float mouse_speed = 0.2f;
        Vector3 diff = Vector3Normalize(Vector3Subtract(gv->target, gv->eye));
        Vector3 right = Vector3CrossProduct(diff, gv->up);
        diff = Vector3RotateByAxisAngle(diff, gv->up, -mouse_speed * delta.x * dt);
        diff = Vector3RotateByAxisAngle(diff, right, -mouse_speed * delta.y * dt);
        if (fabsf(Vector3DotProduct(diff, gv->up)) < 0.99f) gv->target = Vector3Add(diff, gv->eye);
        Vector3 move = {0, 0, 0};
        if (IsKeyDown(KEY_W)) move.y += 1.f;
        if (IsKeyDown(KEY_S)) move.y -= 1.f;
        if (IsKeyDown(KEY_D)) move.x += 1.f;
        if (IsKeyDown(KEY_A)) move.x -= 1.f;
        if (IsKeyDown(KEY_E)) move.z += 1.f;
        if (IsKeyDown(KEY_Q)) move.z -= 1.f;
        if (IsKeyDown(KEY_LEFT_SHIFT)) translate_speed *= 1.1f;
        if (IsKeyDown(KEY_LEFT_CONTROL)) translate_speed *= .9f;

        gv->target = Vector3Add(gv->target, Vector3MultiplyValue(diff, translate_speed * move.y));
        gv->eye = Vector3Add(gv->eye, Vector3MultiplyValue(diff, translate_speed * move.y));
        gv->target = Vector3Add(gv->target, Vector3MultiplyValue(right, translate_speed * move.x));
        gv->eye = Vector3Add(gv->eye, Vector3MultiplyValue(right, translate_speed * move.x));
        gv->target = Vector3Add(gv->target, Vector3MultiplyValue(gv->up, translate_speed * move.z));
        gv->eye = Vector3Add(gv->eye, Vector3MultiplyValue(gv->up, translate_speed * move.z));
      }
      else ShowCursor();


      if (IsKeyDown(KEY_UP)) {
         Vector3 diff = Vector3MultiplyValue(gv->up, 0.01f);
         gv->target = Vector3Add(gv->target, diff);
         gv->eye = Vector3Add(gv->eye, diff);
      }

      if (IsKeyDown(KEY_DOWN)) {
         Vector3 diff = Vector3MultiplyValue(gv->up, -0.01f);
         gv->target = Vector3Add(gv->target, diff);
         gv->eye = Vector3Add(gv->eye, diff);
      }

      if (IsKeyDown(KEY_LEFT)) {
         Vector3 diff = Vector3MultiplyValue(Vector3CrossProduct(Vector3Subtract(gv->target, gv->eye), gv->up), 0.01f);
         gv->target = Vector3Add(gv->target, diff);
         gv->eye = Vector3Add(gv->eye, diff);
      }

      if (IsKeyDown(KEY_RIGHT)) {
         Vector3 diff = Vector3MultiplyValue(Vector3CrossProduct(Vector3Subtract(gv->target, gv->eye), gv->up), -0.01f);
         gv->target = Vector3Add(gv->target, diff);
         gv->eye = Vector3Add(gv->eye, diff);
      }

      if (IsKeyDown(KEY_J)) {
        Vector3 diff = Vector3Subtract(gv->target, gv->eye);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(diff, gv->up));
        gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, right, 0.01f), gv->eye);
      }

      if (IsKeyDown(KEY_K)) {
        Vector3 diff = Vector3Subtract(gv->target, gv->eye);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(diff, gv->up));
        gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, right, -0.01f), gv->eye);
      }
      
      if (IsKeyDown(KEY_L)) {
        Vector3 diff = Vector3Subtract(gv->target, gv->eye);
        gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, Vector3 { 0.f, 1.f, 0.f }, 0.01f), gv->eye);
      }

      if (IsKeyDown(KEY_H)) {
        Vector3 diff = Vector3Subtract(gv->target, gv->eye);
        gv->target = Vector3Add(Vector3RotateByAxisAngle(diff, Vector3 { 0.f, 1.f, 0.f }, -0.01f), gv->eye);
      }
    }
  }
  ImGui::End();
#endif
}

