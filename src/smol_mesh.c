#include "br_plot.h"

#include <assert.h>
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "rlgl.h"

static size_t smol_mesh_get_vb_size(smol_mesh_t* mesh);
static void smol_mesh_upload(smol_mesh_t* mesh, bool dynamic);

smol_mesh_t* smol_mesh_malloc(size_t capacity, Shader s) {
  smol_mesh_t* ret = BR_MALLOC(sizeof(smol_mesh_t));
  *ret = (smol_mesh_t){
    // Maybe join this 3 mallocs into 1 malloc
    .verticies = BR_MALLOC(3*capacity*3*2*sizeof(float)),
    .normals = BR_MALLOC(3*capacity*3*2*sizeof(float)),
    .colors = BR_MALLOC(3*capacity*3*2*sizeof(float)),
    .capacity = capacity,
    .cur_len = 0,
    .active_shader = s
  };
  smol_mesh_upload(ret, true);
  return ret;
}

void smol_mesh_free(smol_mesh_t* mesh) {
  rlUnloadVertexArray(mesh->vaoId);
  rlUnloadVertexBuffer(mesh->vboIdVertex);
  rlUnloadVertexBuffer(mesh->vboIdNormal);
  rlUnloadVertexBuffer(mesh->vboIdColor);
  BR_FREE(mesh->verticies);
  BR_FREE(mesh->normals);
  BR_FREE(mesh->colors);
  BR_FREE(mesh);
}

void smol_mesh_update(smol_mesh_t* mesh) {
  int number_of_floats = (int)smol_mesh_get_vb_size(mesh);
  rlUpdateVertexBuffer(mesh->vboIdVertex, mesh->verticies, number_of_floats, 0);
  rlUpdateVertexBuffer(mesh->vboIdNormal, mesh->normals, number_of_floats, 0);
  rlUpdateVertexBuffer(mesh->vboIdColor, mesh->colors, number_of_floats, 0);
}

static float point_distance(Vector2 tg, Vector2 center, Vector2 to, Rectangle r) {
  Vector2 uv_center = { (center.x - r.x) / r.width, (center.y - r.y) / r.height};
  Vector2 uv_to = { (to.x - r.x) / r.width, (to.y - r.y) / r.height};
  float tg_norm = (float)sqrtf(tg.x * tg.x + tg.y * tg.y);
  float ret;
  if (tg_norm == tg_norm && tg_norm > 0.f) {
    Vector2 u = { tg.y / tg_norm, -tg.x / tg_norm };
    float b = -( u.x*uv_center.x + u.y*uv_center.y);
    ret = (u.x*uv_to.x + u.y*uv_to.y + b);
  } else {
    float b = -uv_center.x;
    ret = uv_to.x + b;
  }
  assert(ret == ret);
  assert(ret > -2. && ret < 2);
  return ret;
}

void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, Vector2 center, Vector2 tangent, Color color) {
  float sqrt_2 = sqrtf(2);
  size_t c = mesh->cur_len++;
  if (c + 1 >= mesh->capacity) {
    smol_mesh_update(mesh);
    smol_mesh_draw(mesh);
    c = mesh->cur_len++;
  }
  c*=18;
  for (size_t i = 0; i < 18*2; i += 3) {
    mesh->normals[c+i+0] = center.x;
    mesh->normals[c+i+1] = center.y;
    mesh->normals[c+i+2] = 0;
  }
  Vector3 vc = {color.r/255.f, color.g/255.f, color.b/255.f};
  for (size_t i = 0; i < 18*2; i += 3) {
    mesh->colors[c+i+0] = vc.x;
    mesh->colors[c+i+1] = vc.y;
    mesh->colors[c+i+2] = vc.z;
  }
  float ld = point_distance(tangent, center, (Vector2){rect.x, rect.y}, rect)/sqrt_2,
        rd = point_distance(tangent, center, (Vector2){rect.x + rect.width, rect.y}, rect)/sqrt_2,
        ru = point_distance(tangent, center, (Vector2){rect.x + rect.width, rect.y + rect.height}, rect)/sqrt_2,
        lu = point_distance(tangent, center, (Vector2){rect.x, rect.y + rect.height}, rect)/sqrt_2;
  //float m = maxf(maxf(ld, rd), maxf(ru, lu));
  //ld /= m; rd /= m; ru /= m; lu /= m;
  //assert(CheckCollisionPointRec(tangent, rect));
  mesh->verticies[c+0] = rect.x + rect.width;
  mesh->verticies[c+1] = rect.y;
  mesh->verticies[c+2] = rd;
  mesh->verticies[c+3] = center.x;
  mesh->verticies[c+4] = center.y;
  mesh->verticies[c+5] = 0.f;
  mesh->verticies[c+6] = rect.x;
  mesh->verticies[c+7] = rect.y;
  mesh->verticies[c+8] = ld;

  mesh->verticies[c+9] = rect.x + rect.width;
  mesh->verticies[c+10] = rect.y + rect.height;
  mesh->verticies[c+11] = ru;
  mesh->verticies[c+12] = center.x;
  mesh->verticies[c+13] = center.y;
  mesh->verticies[c+14] = 0.f;
  mesh->verticies[c+15] = rect.x + rect.width;
  mesh->verticies[c+16] = rect.y;
  mesh->verticies[c+17] = rd;

  c += 18;
  mesh->cur_len++;
  mesh->verticies[c+0] = rect.x;
  mesh->verticies[c+1] = rect.y + rect.height;
  mesh->verticies[c+2] = lu;
  mesh->verticies[c+3] = center.x;
  mesh->verticies[c+4] = center.y;
  mesh->verticies[c+5] = 0.f;
  mesh->verticies[c+6] = rect.x + rect.width;
  mesh->verticies[c+7] = rect.y + rect.height;
  mesh->verticies[c+8] = ru;

  mesh->verticies[c+9] = rect.x;
  mesh->verticies[c+10] = rect.y;
  mesh->verticies[c+11] = ld;
  mesh->verticies[c+12] = center.x;
  mesh->verticies[c+13] = center.y;
  mesh->verticies[c+14] = 0.f;
  mesh->verticies[c+15] = rect.x;
  mesh->verticies[c+16] = rect.y + rect.height;
  mesh->verticies[c+17] = lu;
}

void smol_mesh_gen_bb(smol_mesh_t* mesh, bb_t bb, Color color) {
  float xmi = bb.xmin, ymi = bb.ymin , xma = bb.xmax, yma = bb.ymax;
  Vector2 v[6] = {
    {xmi, ymi},
    {xma, ymi},
    {xma, yma},
    {xmi, yma},
    {xmi, ymi},
  };
  smol_mesh_gen_line_strip(mesh, v, 5, color);
}

bool smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2 const * points, size_t len, Color color) {
  // Todo: check if index v is inside gv->points
  size_t vn = 2*3*3;
  Vector3 cv = {color.r/255.f, color.g/255.f, color.b/255.f};
  for (size_t v = 0; v < ((len - 1)*vn); v += vn)
  {
    size_t c = mesh->cur_len++;
    if (c >= mesh->capacity) {
      smol_mesh_update(mesh);
      smol_mesh_draw(mesh);
      c = mesh->cur_len++;
    }
    c *= vn;
    Vector2 startPos = points[v/vn];
    Vector2 endPos = points[v/vn + 1];
    Vector2 delta = { endPos.x - startPos.x, endPos.y - startPos.y };
    float length = sqrtf(delta.x*delta.x + delta.y*delta.y);

    Vector2 strip[2] = {
      { startPos.x, startPos.y},
      { endPos.x, endPos.y},
    };
    //First triangle
    mesh->verticies[c+0] = strip[0].x;
    mesh->verticies[c+1] = strip[0].y;
    mesh->verticies[c+2] = -1;
    mesh->verticies[c+3] = strip[1].x;
    mesh->verticies[c+4] = strip[1].y;
    mesh->verticies[c+5] = -1;
    mesh->verticies[c+6] = strip[0].x;
    mesh->verticies[c+7] = strip[0].y;
    mesh->verticies[c+8] = 1;
    //Second triangle
    mesh->verticies[c+9]  = strip[0].x;
    mesh->verticies[c+10] = strip[0].y;
    mesh->verticies[c+11] = 1;
    mesh->verticies[c+12] = strip[1].x;
    mesh->verticies[c+13] = strip[1].y;
    mesh->verticies[c+14] = -1;
    mesh->verticies[c+15] = strip[1].x;
    mesh->verticies[c+16] = strip[1].y;
    mesh->verticies[c+17] = 1;

    for (size_t i = 0; i < vn; i += 3) {
      //Not a normal, this is dx, dy, length for first triangle
      mesh->normals[c+i+0] = delta.x;
      mesh->normals[c+i+1] = delta.y;
      mesh->normals[c+i+2] = length;
    }

    for (size_t i = 0; i < vn; i += 3) {
      mesh->colors[c+i+0] = cv.x;
      mesh->colors[c+i+1] = cv.y;
      mesh->colors[c+i+2] = cv.z;
    }
  }
  return true;
}

void smol_mesh_draw(smol_mesh_t* mesh) {
  // Bind shader program
  rlEnableShader(mesh->active_shader.id);

  rlEnableVertexArray(mesh->vaoId);
  // Bind mesh VBO data: vertex position (shader-location = 0)
  rlEnableVertexBuffer(mesh->vboIdVertex);
  rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_POSITION]);

  rlEnableVertexBuffer(mesh->vboIdNormal);
  rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_NORMAL]);

  rlEnableVertexBuffer(mesh->vboIdColor);
  rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_COLOR], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_COLOR]);

  // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
  rlDrawVertexArray(0, (int)mesh->cur_len*2*3);
  // Disable all possible vertex array objects (or VBOs)
  rlDisableVertexArray();
  rlDisableVertexBuffer();
  rlDisableVertexBufferElement();

  // Disable shader program
  rlDisableShader();
  mesh->points_drawn += mesh->cur_len;
  ++mesh->draw_calls;
  mesh->cur_len = 0;
}

static size_t smol_mesh_get_vb_size(smol_mesh_t* mesh) {
  return mesh->capacity * 3 * 2 * 3 * sizeof(float);
}

static void smol_mesh_upload(smol_mesh_t* mesh, bool dynamic) {

    mesh->vaoId = 0;        // Vertex Array Object
    mesh->vboIdVertex = 0;     // Vertex buffer: positions
    mesh->vboIdNormal = 0;     // Vertex buffer: texcoords
    int cap = (int)smol_mesh_get_vb_size(mesh);

    mesh->vaoId = rlLoadVertexArray();
    rlEnableVertexArray(mesh->vaoId);

    mesh->vboIdVertex = rlLoadVertexBuffer(mesh->verticies, cap, dynamic);
    rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);

    mesh->vboIdNormal = rlLoadVertexBuffer(mesh->normals, cap, dynamic);
    rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);

    mesh->vboIdColor = rlLoadVertexBuffer(mesh->colors, cap, dynamic);
    rlSetVertexAttribute((uint32_t)mesh->active_shader.locs[SHADER_LOC_VERTEX_COLOR], 3, RL_FLOAT, 0, 0, 0);

    rlDisableVertexArray();
}

