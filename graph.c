#include "graph.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"

// This is the size of one group of points that will be drawn with one draw call.
// TODO: make this dynamic.
#define PTOM_COUNT (1<<15)

static void test_points(graph_values_t* gv);
static void refresh_shaders_if_dirty(graph_values_t* gv);
static void update_resolution(graph_values_t* gv);
static void init_smol_mesh(smol_mesh_t* mesh);

//TODO: think about renaming this to snake case.
//I used PascalCase to name this functions because they are the same class of functions raylib provieds.
//But this are not raylib functions. I don't know...
//TODO: Maybe move this functions to something like Raylib extensions file.
//      Or Smol mesh file...
static bool GenMeshLineStrip(smol_mesh_t* mesh, point_group_t* gv, int offset);
static void UploadSmolMesh(smol_mesh_t* mesh, bool dynamic);
static void DrawLineStripMesh(smol_mesh_t* mesh, Shader shader, Color color);
static void UpdateSmolMesh(smol_mesh_t* mesh);
static void UnloadSmolMesh(smol_mesh_t* mesh);

static int DrawButton(bool* is_pressed, float x, float y, float font_size, char* buff, const char* str, ...);
static void DrawLeftPanel(graph_values_t* gv, char *buff, float font_scale);

// Neded for copying stuff to gpu.
static float tmp_v_buff[3*PTOM_COUNT*3*2];
static float tmp_norm_buff[3*PTOM_COUNT*3*2];
static float tmp_v_buff2[2*PTOM_COUNT*3*2];

static smol_mesh_t temp_smol_mesh = {
  .verticies = tmp_v_buff,
  .normals = tmp_norm_buff,
  .tex_cords = tmp_v_buff2,
  .length = PTOM_COUNT,
};

void init_graph(graph_values_t* gv) {
  (void)gv;
  GenMeshLineStrip(&temp_smol_mesh, NULL, 0);
  UploadSmolMesh(&temp_smol_mesh, true);
}

void DrawGraph(graph_values_t* gv) {
  char buff[128];
  float font_scale = 1.4;
  update_resolution(gv);
  refresh_shaders_if_dirty(gv);
  Vector2 mp = GetMousePosition();
  bool is_inside = CheckCollisionPointRec(mp, gv->graph_rect);

  if (is_inside) {
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      Vector2 delt = GetMouseDelta();
      gv->uvOffset.x -= gv->uvZoom.x*delt.x/gv->graph_rect.height;
      gv->uvOffset.y += gv->uvZoom.y*delt.y/gv->graph_rect.height;
    }

    float mw = GetMouseWheelMove();
    if (IsKeyDown(KEY_X)) {
      gv->uvZoom.x *= (1 + mw/10);
    } else if (IsKeyDown(KEY_Y)) {
      gv->uvZoom.y *= (1 + mw/10);
    } else {
      gv->uvZoom.x *= (1 + mw/10);
      gv->uvZoom.y *= (1 + mw/10);
    }

    if (IsKeyPressed(KEY_R)) {
      gv->uvZoom.x = gv->uvZoom.y = 1;
      gv->uvOffset.x = gv->uvOffset.y = 0;
    }
    if (IsKeyPressed(KEY_C)) {
      for (int i = 0; i < gv->groups_len; ++i) {
        point_group_t* g = &gv->groups[i];
        for (int j = 0; j < g->smol_meshes_len; ++j) {
          UnloadSmolMesh(&g->meshes[j]);
        }
        memset(gv->groups, 0, sizeof(gv->groups));
      }
      gv->groups_len = 0;
    }
    if (IsKeyPressed(KEY_T)) {
      for (int i = 0; i < 100; ++i) {
        test_points(gv);
      }
    }
  }
  DrawLeftPanel(gv, buff, font_scale);
  for (int i = 0; i < 2; ++i) {
    SetShaderValue(gv->shaders[i], gv->uResolution[i], &gv->graph_rect, SHADER_UNIFORM_VEC4);
    SetShaderValue(gv->shaders[i], gv->uZoom[i], &gv->uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uOffset[i], &gv->uvOffset, SHADER_UNIFORM_VEC2);
    SetShaderValue(gv->shaders[i], gv->uScreen[i], &gv->uvScreen, SHADER_UNIFORM_VEC2);
  }
  DrawFPS(0, 0);
  BeginShaderMode(gv->gridShader);
    DrawRectangleRec(gv->graph_rect, RED);
  EndShaderMode();
  {
    for (int j = 0; j < gv->groups_len; ++j) {
      point_group_t * g = &gv->groups[j];
      if ( g->len / PTOM_COUNT > g->smol_meshes_len ) {
        int indx = g->smol_meshes_len++;
        smol_mesh_t* sm = &g->meshes[indx];
        init_smol_mesh(sm);
        assert(GenMeshLineStrip(sm, g, indx*sm->length));
        UploadSmolMesh(sm, false);
      }
    }
    for (int j = 0; j < gv->groups_len; ++j) {
      point_group_t* g = &gv->groups[j];
      if (g->is_selected) {
        if (GenMeshLineStrip(&temp_smol_mesh, g, g->smol_meshes_len*(temp_smol_mesh.length))) {
          UpdateSmolMesh(&temp_smol_mesh);
          DrawLineStripMesh(&temp_smol_mesh, gv->linesShader, gv->group_colors[j]);
        }
      }
    }
    for (int j = 0; j < gv->groups_len; ++j) {
      point_group_t * g = &gv->groups[j];
      if (g->is_selected) {
        for (int k = 0; k < g->smol_meshes_len; ++k) {
          DrawLineStripMesh(&g->meshes[k], gv->linesShader, gv->group_colors[j]);
        }
      }
    }
  }
  if (is_inside) {
    float pad = 5.;
    float fs = 10 * font_scale;
    Vector2 s = { 100, fs + 2 * pad};
    Vector2 mp_in_graph = { mp.x - gv->graph_rect.x, mp.y - gv->graph_rect.y };
    sprintf(buff, "(%.1e, %.1e)", -(gv->graph_rect.width  - 2.*mp_in_graph.x)/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x,
                                   (gv->graph_rect.height - 2.*mp_in_graph.y)/gv->graph_rect.height*gv->uvZoom.y/2. + gv->uvOffset.y);
    s.x = MeasureText(buff, fs) + 2 * pad;
    DrawRectangleV(mp, s, RAYWHITE);
    DrawText(buff, mp.x + pad, mp.y + pad, fs, BLACK);
  }
}

point_group_t* init_point_group_t(point_group_t* g, int cap, int group_id, Vector2* points) {
    g->cap = cap;
    g->len = 0;
    g->group_id = group_id;
    g->is_selected = true;
    g->points = points;
    g->smol_meshes_len = 0;
    return g;
}

point_group_t* push_point_group(graph_values_t *gv, int group) {
  if (gv->groups_len == 0) {
    return init_point_group_t(&gv->groups[gv->groups_len++], POINTS_CAP, group, gv->points);
  }
  int max_size = 0;
  point_group_t* max_group = &gv->groups[0];
  for (int i = 0; i < gv->groups_len; ++i) {
    if (gv->groups[i].group_id == group) {
      return &gv->groups[i];
    }
    int size = gv->groups[i].cap;
    if (size > max_size) {
      max_group = &gv->groups[i];
      max_size = size;
    }
  }
  int l = max_group->cap;
  Vector2* ns2 = max_group->points + (l - (l / 2));
  max_group->cap = l - (l / 2);
  return init_point_group_t(&gv->groups[gv->groups_len++], l / 2, group, ns2);
}

void push_point(point_group_t* g, Vector2 v) {
  if (g->len + 1 > g->cap) {
    fprintf(stderr, "Trying to add point to a group thats full");
    exit(-1);
  }
  g->points[g->len++] = v;
}


static void refresh_shaders_if_dirty(graph_values_t* gv) {
  if (gv->shaders_dirty) {
    gv->shaders_dirty = false;
    Shader new_line = LoadShader("./shaders/line.vs", "./shaders/line.fs");
    if (new_line.locs != NULL) {
      UnloadShader(gv->linesShader);
      gv->linesShader = new_line;
    }
    Shader new_grid = LoadShader(NULL, "./shaders/grid.fs");
    if (new_grid.locs != NULL) {
      UnloadShader(gv->gridShader);
      gv->gridShader = new_grid;
    }
    for (int i = 0; i < 2; ++i) {
      gv->uResolution[i] = GetShaderLocation(gv->shaders[i], "resolution");
      gv->uZoom[i] = GetShaderLocation(gv->shaders[i], "zoom");
      gv->uOffset[i] = GetShaderLocation(gv->shaders[i], "offset");
      gv->uScreen[i] = GetShaderLocation(gv->shaders[i], "screen");
    }
  }
}

static void test_points(graph_values_t* gv) {
  for (int harm = 1; harm <= 4; ++harm) {
    for(int i = 0; i < 1025; ++i) {
      int group = harm;
      point_group_t* g = push_point_group(gv, group);
      float x = g->len*.1;
      float y = (float)x*0.01;
      Vector2 p = {x, harm*sin(y/(1<<harm)) };
      push_point(g, p);
    }
  }
}

static int DrawButton(bool* is_pressed, float x, float y, float font_size, char* buff, const char* str, ...) {
  Vector2 mp = GetMousePosition();
  int c = 0;
  va_list args;
  va_start(args, str);
  vsprintf(buff, str, args);
  va_end(args);
  float pad = 5.;
  Vector2 size = { MeasureText(buff, font_size) +  2 * pad, font_size + 2 * pad };
  Rectangle box = { x, y, size.x, size.y };
  bool is_in = CheckCollisionPointRec(mp, box);
  if (is_in) {
    bool is_p = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    c = is_p ? 2 : 1;
    if (is_p && is_pressed) {
      *is_pressed = !*is_pressed;
    }
  }
  if (is_pressed && *is_pressed) {
    DrawRectangleRec(box, BLUE);
  } else if (is_in) {
    DrawRectangleRec(box, RED);
  }
  DrawText(buff, x + pad, y + pad, font_size, WHITE);
  return c;
}

static void DrawLeftPanel(graph_values_t* gv, char *buff, float font_scale) {
  DrawButton(NULL, gv->graph_rect.x - 30, gv->graph_rect.y - 30, font_scale * 10,
      buff, "(%f, %f)", -gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x, gv->uvZoom.y/2 + gv->uvOffset.y);
  DrawButton(NULL, gv->graph_rect.x + gv->graph_rect.width - 120, gv->graph_rect.y - 30, font_scale * 10,
      buff, "(%f, %f)", gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x, gv->uvZoom.y/2 + gv->uvOffset.y);

  DrawButton(NULL, gv->graph_rect.x - 30, gv->graph_rect.y + 20 + gv->graph_rect.height, font_scale * 10,
      buff, "(%f, %f)", -gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x, -gv->uvZoom.y/2 + gv->uvOffset.y);
  DrawButton(NULL, gv->graph_rect.x + gv->graph_rect.width - 120, gv->graph_rect.y + 20 + gv->graph_rect.height, font_scale * 10,
      buff, "(%f, %f)", gv->graph_rect.width/gv->graph_rect.height*gv->uvZoom.x/2. + gv->uvOffset.x, -gv->uvZoom.y/2 + gv->uvOffset.y);

  int i = 0;
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "offset: (%f, %f)", gv->uvOffset.x, gv->uvOffset.y);
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "zoom: (%f, %f)", gv->uvZoom.x, gv->uvZoom.y);
  DrawButton(NULL, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "Line groups: %d/%d", gv->groups_len, GROUP_CAP);
  for(int j = 0; j < gv->groups_len; ++j) {
    DrawButton(&gv->groups[j].is_selected, 30, gv->graph_rect.y + 33*(i++), font_scale * 15, buff, "Group #%d: %d/%d", gv->groups[j].group_id, gv->groups[j].len, gv->groups[j].cap);
  }
}

static void update_resolution(graph_values_t* gv) {
  gv->uvScreen.x = GetScreenWidth();
  gv->uvScreen.y = GetScreenHeight();
  int w = gv->uvScreen.x - GRAPH_LEFT_PAD - 60, h = gv->uvScreen.y - 120;
  gv->graph_rect.x = GRAPH_LEFT_PAD;
  gv->graph_rect.y = 60;
  gv->graph_rect.width = w;
  gv->graph_rect.height = h;
}

static void init_smol_mesh(smol_mesh_t* mesh) {
  mesh->verticies = tmp_v_buff;
  mesh->normals = tmp_norm_buff;
  mesh->tex_cords = tmp_v_buff2;
  mesh->length = PTOM_COUNT;
}

static void UnloadSmolMesh(smol_mesh_t* mesh) {
  Mesh m = { 0 };
  m.vaoId = mesh->vaoId;
  m.vboId = mesh->vboId;
  UnloadMesh(m);
}

static void UpdateSmolMesh(smol_mesh_t* mesh) {
  Mesh m = { 0 };
  m.vaoId = mesh->vaoId;
  m.vboId = mesh->vboId;
  // length * (2 triengle per line) * (3 verticies per triangle) * (3 floats per vertex)
  int number_of_floats = mesh->length*2*3*3;
  UpdateMeshBuffer(m, 0, mesh->verticies, number_of_floats * sizeof(float), 0);
  UpdateMeshBuffer(m, 2, mesh->normals, number_of_floats * sizeof(float), 0);
}

static void UploadSmolMesh(smol_mesh_t* mesh, bool dynamic) {
  Mesh m = { 0 };
  m.vaoId = mesh->vaoId;
  m.vboId = mesh->vboId;
  m.vertices = mesh->verticies;
  m.normals = mesh->normals;
  m.texcoords = mesh->tex_cords;
  m.vertexCount = mesh->vertex_count;
  m.triangleCount = mesh->triangle_count;

  UploadMesh(&m, dynamic);
  mesh->vboId = m.vboId;
  mesh->vaoId = m.vaoId;
}

static void merge_points(float* a, float* b) {
  float nv = (*a + *b)/2;
  *a = *b = nv;
}

static bool GenMeshLineStrip(smol_mesh_t* mesh, point_group_t* g, int offset)
{
    int l = g != NULL ? g->len - offset - 1 : 0;
    int count = PTOM_COUNT < l ? PTOM_COUNT : l;
    count = count < 0 ? 0 : count;

    // Clear rest of normals so that end of buffer can be detected inside of the shader.
    for (int i = count; i < mesh->length; ++i) {
      mesh->normals[i] = 0;
    }

    mesh->vertex_count = mesh->length*2*3;
    mesh->triangle_count = mesh->length*2;

    if (count <= 0) return false;

    // Todo: check if index v is inside gv->points
    for (int v = 0; v < (count*2*3*3); v += 2*3*3)
    {
      Vector2 startPos = g->points[offset + v/(2*3*3)];
      Vector2 endPos = g->points[offset + v/(2*3*3) + 1];
      Vector2 delta = { endPos.x - startPos.x, endPos.y - startPos.y };
      float length = sqrtf(delta.x*delta.x + delta.y*delta.y);

      Vector2 strip[2] = {
          { startPos.x, startPos.y},
          { endPos.x, endPos.y},
      };
      //First triangle
      mesh->verticies[v+0] = strip[0].x;
      mesh->verticies[v+1] = strip[0].y;
      mesh->verticies[v+2] = -1;
      mesh->verticies[v+3] = strip[1].x;
      mesh->verticies[v+4] = strip[1].y;
      mesh->verticies[v+5] = -1;
      mesh->verticies[v+6] = strip[0].x;
      mesh->verticies[v+7] = strip[0].y;
      mesh->verticies[v+8] = 1;
      //Second triangle
      mesh->verticies[v+9]  = strip[0].x;
      mesh->verticies[v+10] = strip[0].y;
      mesh->verticies[v+11] = 1;
      mesh->verticies[v+12] = strip[1].x;
      mesh->verticies[v+13] = strip[1].y;
      mesh->verticies[v+14] = -1;
      mesh->verticies[v+15] = strip[1].x;
      mesh->verticies[v+16] = strip[1].y;
      mesh->verticies[v+17] = 1;

      for (int i = 0; i < 18; i += 3) {
      //Not a normal, this is dx, dy, length for first triangle 
        mesh->normals[v+i+0] = delta.x;
        mesh->normals[v+i+1] = delta.y;
        mesh->normals[v+i+2] = length;
      }
  }
  for (int i = 1; i < count; ++i) {
    int vend = (i - 1) * 18, vstart = i * 18;
    merge_points(&mesh->normals[vend+3], &mesh->normals[vstart+0]);
    merge_points(&mesh->normals[vend+4], &mesh->normals[vstart+1]);
    mesh->normals[vend+12] = mesh->normals[vend+3];
    mesh->normals[vend+13] = mesh->normals[vend+4];
    merge_points(&mesh->normals[vend+15], &mesh->normals[vstart+6]);
    merge_points(&mesh->normals[vend+16], &mesh->normals[vstart+7]);
    mesh->normals[vstart+9] = mesh->normals[vstart+6];
    mesh->normals[vstart+10] = mesh->normals[vstart+7];
  }
  return true;
}

// Copy paste from Raylib source. More or less. Delted stuff that were not needed..
static void DrawLineStripMesh(smol_mesh_t* mesh, Shader shader, Color color)
{
    // Bind shader program
    rlEnableShader(shader.id);

    // Try binding vertex array objects (VAO) or use VBOs if not possible
    // WARNING: UploadMesh() enables all vertex attributes available in mesh and sets default attribute values
    // for shader expected vertex attributes that are not provided by the mesh (i.e. colors)
    // This could be a dangerous approach because different meshes with different shaders can enable/disable some attributes
    if (!rlEnableVertexArray(mesh->vaoId))
    {
        // Bind mesh VBO data: vertex position (shader-location = 0)
        rlEnableVertexBuffer(mesh->vboId[0]);
        rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION]);
    }

    // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
    if (shader.locs[SHADER_LOC_VERTEX_COLOR] != -1)
    {
        if (mesh->vboId[3] != 0)
        {
            rlEnableVertexBuffer(mesh->vboId[3]);
            rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_COLOR], 4, RL_UNSIGNED_BYTE, 1, 0, 0);
            rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_COLOR]);
        }
        else
        {
            // Set default value for defined vertex attribute in shader but not provided by mesh
            // WARNING: It could result in GPU undefined behaviour
            float value[4] = { color.r, color.g, color.b, color.a };
            rlSetVertexAttributeDefault(shader.locs[SHADER_LOC_VERTEX_COLOR], value, SHADER_ATTRIB_VEC4, 4);
            rlDisableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_COLOR]);
        }
    }

    // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
    if (shader.locs[SHADER_LOC_VERTEX_NORMAL] != -1)
    {
        if (mesh->vboId[2] != 0)
        {
            rlEnableVertexBuffer(mesh->vboId[2]);
            rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);
            rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_NORMAL]);
        }
    }

    rlDrawVertexArray(0, mesh->vertex_count);
    // Disable all possible vertex array objects (or VBOs)
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();

    // Disable shader program
    rlDisableShader();
}

