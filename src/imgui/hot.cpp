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
  rlDisableBackfaceCulling();
}

Vector3 Vector3MultiplyValue(Vector3 v, float s) {
  return Vector3 { v.x * s, v.y * s, v.z * s };
}

void smol_mesh_gen_quad_3d_simple(smol_mesh_t* mesh, Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, Color color) {
  size_t c = mesh->cur_len++;
  if (c >= mesh->capacity) {
    smol_mesh_update(mesh);
    smol_mesh_draw(mesh);
    c = mesh->cur_len++;
  }
  c*=18;
  Vector3 vc = {color.r/255.f, color.g/255.f, color.b/255.f};
  for (size_t i = 0; i < 18; i += 3) {
    mesh->colors[c+i+0] = vc.x;
    mesh->colors[c+i+1] = vc.y;
    mesh->colors[c+i+2] = vc.z;
  }
  Vector3* points = (Vector3*)&mesh->verticies[c];
  points[0] = p1;
  points[1] = p2;
  points[2] = p3;

  points[3] = p3;
  points[4] = p4;
  points[5] = p1;
}

void smol_mesh_3d_gen_line_eye2(smol_mesh_3d_t* mesh, Vector3 p1, Vector3 p2, Vector3 eye, Color color) {
  Vector3 const cv = {color.r/255.f, color.g/255.f, color.b/255.f};
  Vector3 mid   = Vector3MultiplyValue(Vector3Add(p1, p2), 0.5f);
  Vector3 diff  = Vector3Normalize(Vector3Subtract(p2, p1));
  Vector3 norm = { diff.z, -diff.x, diff.y }; 
  norm = Vector3Perpendicular(diff);
  Vector3Angle(p1, p2);
  int n = 12;
  float dist1 = 0.01f * Vector3Distance(eye, p1);
  float dist2 = 0.01f * Vector3Distance(eye, p2);
  for (int k = 0; k <= n; ++k) {
    Vector3 next = Vector3Normalize(Vector3RotateByAxisAngle(norm, diff, (float)PI * 2 / (float)n));
    size_t i = mesh->cur_len++;
    if (i >= mesh->capacity) {
      smol_mesh_3d_update(mesh);
      smol_mesh_3d_draw(mesh);
      i = mesh->cur_len++;
    }
    Vector3* vecs = (Vector3*) &mesh->verticies[i*18];
    Vector3* colors = (Vector3*) &mesh->colors[i*18];
    Vector3* normals = (Vector3*) &mesh->normals[i*18];
    normals[0] = norm;
    normals[1] = next;
    normals[2] = norm;

    normals[3] = norm;
    normals[4] = next;
    normals[5] = next;

    vecs[0] = Vector3Add(p1, normals[0]);
    vecs[1] = Vector3Add(p1, normals[1]);
    vecs[2] = Vector3Add(p2, normals[2]);

    vecs[3] = Vector3Add(p2, normals[3]);
    vecs[4] = Vector3Add(p2, normals[4]);
    vecs[5] = Vector3Add(p1, normals[5]);

    for (int j = 0; j < 6; ++j) colors[j] = cv;
    norm = next;
  }
}

void smol_mesh_3d_gen_line_eye(smol_mesh_3d_t* mesh, Vector3 p1, Vector3 p2, Vector3 eye, Color color) {
  size_t i = mesh->cur_len++;
  if (i >= mesh->capacity) {
    smol_mesh_3d_update(mesh);
    smol_mesh_3d_draw(mesh);
    i = mesh->cur_len++;
  }
  Vector3* vecs = (Vector3*) &mesh->verticies[i*18];
  Vector3* colors = (Vector3*) &mesh->colors[i*18];
  Vector3* normals = (Vector3*) &mesh->normals[i*18];
  Vector3 const cv = {color.r/255.f, color.g/255.f, color.b/255.f};
  Vector3 mid = Vector3MultiplyValue(Vector3Add(p1, p2), 0.5f);
  Vector3 diff = Vector3Normalize(Vector3Subtract(p2, p1));
  float dist1 = 0.01f * Vector3Distance(eye, p1);
  float dist2 = 0.01f * Vector3Distance(eye, p2);
  dist1 = dist2 = (dist1 + dist2) * .5f;
  Vector3 right1 = Vector3CrossProduct(Vector3Normalize(Vector3Subtract(eye, p1)), diff);
  Vector3 right2 = Vector3CrossProduct(Vector3Normalize(Vector3Subtract(eye, p2)), diff);
  normals[0] = Vector3MultiplyValue(right1, -dist1);
  normals[1] = Vector3MultiplyValue(right1, dist1);
  normals[2] = Vector3MultiplyValue(right2, dist2);

  normals[3] = Vector3MultiplyValue(right2, dist2);
  normals[4] = Vector3MultiplyValue(right2, -dist2);
  normals[5] = Vector3MultiplyValue(right1, -dist1);

  vecs[0] = Vector3Add(p1, normals[0]);
  vecs[1] = Vector3Add(p1, normals[1]);
  vecs[2] = Vector3Add(p2, normals[2]);

  vecs[3] = Vector3Add(p2, normals[3]);
  vecs[4] = Vector3Add(p2, normals[4]);
  vecs[5] = Vector3Add(p1, normals[5]);

  for (int j = 0; j < 6; ++j) colors[j] = cv;
}

bool focused = false;
float translate_speed = 1.0f;

extern "C" void br_hot_loop(br_plot_t* gv) {
  ImGui::Begin("3d");
  br::Input("Eyes", gv->eye);
  br::Input("Target", gv->target);
  br::Input("Up", gv->up);
  if (ImGui::Button("Reset") || IsKeyPressed(KEY_R)) {
    gv->eye = Vector3 { 0, 0, 100 };
    gv->target = Vector3 { 0, 0, 0 };
    gv->up = Vector3 { 0, 1, 0 };
  }
  ImGui::InputFloat("FovY", &gv->fov_y);
  ImGui::InputFloat("Near", &gv->near_plane);
  ImGui::InputFloat("Far", &gv->far_plane);
  ShowMatrix("ViewMatrix", gv->uvMatView);
  ShowMatrix("Projection", gv->uvMatProjection);
  ShowMatrix("MVP", gv->uvMvp);
  ShowMatrix("Rl Project", rlGetMatrixProjection());
  ShowMatrix("Rl ModelView", rlGetMatrixTransform());
  ImGui::End();


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
    glUseProgram(gv->grid3dShader.shader.id);
    Vector2 screen = { (float)GetRenderWidth(), (float)GetRenderHeight() };
    float sz = 10000.f;
    Rectangle r = { .x = v.x / screen.x * 2, .y = 2.f -2.f*(v.y + size.y) / screen.y, .width = 2 * size.x / screen.x, .height = 2 * size.y / screen.y };
    Rectangle r1 = { .x = v.x, .y = GetScreenHeight() - v.y , .width = size.x, .height = -size.y };
    Rectangle r3 = { .x = -sz, .y = -sz, .width = sz * 2.f, .height = sz * 2.f };

    int loc = GetShaderLocation(gv->grid3dShader.shader, "eye");
    rlSetUniform(gv->grid3dShader.uResolution, &r1, RL_SHADER_UNIFORM_VEC4, 1);
    rlSetUniform(gv->grid3dShader.uScreen, &screen, RL_SHADER_UNIFORM_VEC2, 1);
    rlSetUniform(gv->grid3dShader.uOffset, &gv->eye, RL_SHADER_UNIFORM_VEC3, 1);
    rlSetUniform(loc, &gv->eye, RL_SHADER_UNIFORM_VEC3, 1);

    Matrix trans = MatrixTranslate(r.x, r.y, 0.f);
    gv->uvMatProjection = MatrixPerspective(gv->fov_y, screen.x / screen.y, gv->near_plane, gv->far_plane);
    gv->uvMatView = MatrixLookAt(gv->eye, gv->target, gv->up);
    Matrix end = MatrixIdentity();
    end = MatrixMultiply(end, gv->uvMatView);
    end = MatrixMultiply(end, gv->uvMatProjection);
    end = MatrixMultiply(end, trans);
    gv->uvMvp = end;
    rlSetUniformMatrix(gv->grid3dShader.uMvp, gv->uvMvp);
    
    smol_mesh_gen_quad_simple(gv->graph_mesh_3d, r3, Color {0, 0, 1, 0});
    smol_mesh_gen_quad_3d_simple(gv->graph_mesh_3d, Vector3 { -sz, 0, -sz }, Vector3 { sz, 0, -sz }, Vector3 { sz, 0, sz }, Vector3 { -sz, 0, sz }, Color {0, 1, 0, 0});
    smol_mesh_update(gv->graph_mesh_3d);
    smol_mesh_draw(gv->graph_mesh_3d);

    glUseProgram(gv->lines3dShader.shader.id);
    loc = GetShaderLocation(gv->lines3dShader.shader, "eye");
    rlSetUniformMatrix(gv->lines3dShader.uMvp, gv->uvMvp);
    rlSetUniform(loc, &gv->eye, RL_SHADER_UNIFORM_VEC3, 1);

    rlEnableDepthTest();
    smol_mesh_3d_gen_line_eye(gv->lines_mesh_3d, Vector3 { 0, 0, 0 }, Vector3 { 10, 10, 0 }, gv->eye, GREEN);
    smol_mesh_3d_gen_line_eye(gv->lines_mesh_3d, Vector3 { 0, 0, 0 }, Vector3 { 10, 10, 10 }, gv->eye, GREEN);
    smol_mesh_3d_gen_line_eye(gv->lines_mesh_3d, Vector3 { 0, 0, 0 }, Vector3 { 10, 0, 0 }, gv->eye, BLUE);
    smol_mesh_3d_update(gv->lines_mesh_3d);
    smol_mesh_3d_draw(gv->lines_mesh_3d);
    rlDisableDepthTest();
  }
  ImGui::End();
}

