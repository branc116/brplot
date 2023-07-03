#include "plotter.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "rlgl.h"

static smol_mesh_t temp_smol_mesh = {
  .verticies = NULL,
  .normals = NULL,
  .length = PTOM_COUNT,
};

void smol_mesh_upload(smol_mesh_t* mesh, bool dynamic) {

    mesh->vaoId = 0;        // Vertex Array Object
    mesh->vboIdVertex = 0;     // Vertex buffer: positions
    mesh->vboIdNormal = 0;     // Vertex buffer: texcoords

    mesh->vaoId = rlLoadVertexArray();
    rlEnableVertexArray(mesh->vaoId);

    mesh->vboIdVertex = rlLoadVertexBuffer(mesh->verticies, mesh->vertex_count*3*sizeof(float), dynamic);
    rlSetVertexAttribute(0, 3, RL_FLOAT, 0, 0, 0);

    // Enable vertex attributes: normals (shader-location = 2)
    mesh->vboIdNormal = rlLoadVertexBuffer(mesh->normals, mesh->vertex_count*3*sizeof(float), dynamic);
    rlSetVertexAttribute(2, 3, RL_FLOAT, 0, 0, 0);

    rlDisableVertexArray();
}

void smol_mesh_init_temp(void) {
  temp_smol_mesh = (smol_mesh_t){
    .verticies = malloc(3*PTOM_COUNT*3*2*sizeof(float)),
    .normals = malloc(3*PTOM_COUNT*3*2*sizeof(float)),
    .length = PTOM_COUNT,
  };
  smol_mesh_gen_line_strip(&temp_smol_mesh, NULL, 0, 0);
  smol_mesh_upload(&temp_smol_mesh, true);
}

smol_mesh_t* smol_mesh_get_temp(void) {
  return &temp_smol_mesh;
}

void smol_mesh_init(smol_mesh_t* mesh) {
  mesh->verticies = temp_smol_mesh.verticies;
  mesh->normals = temp_smol_mesh.normals;
  mesh->length = PTOM_COUNT;
}

void smol_mesh_unload(smol_mesh_t* mesh) {
  rlUnloadVertexArray(mesh->vaoId);
  rlUnloadVertexBuffer(mesh->vboIdVertex);
  rlUnloadVertexBuffer(mesh->vboIdNormal);
}

void smol_mesh_update(smol_mesh_t* mesh) {
  Mesh m = { 0 };
  m.vaoId = mesh->vaoId;
  m.vboId = mesh->vboId;
  // length * (2 triengle per line) * (3 verticies per triangle) * (3 floats per vertex)
  int number_of_floats = mesh->length*2*3*3;
  UpdateMeshBuffer(m, 0, mesh->verticies, number_of_floats * sizeof(float), 0);
  UpdateMeshBuffer(m, 1, mesh->normals, number_of_floats * sizeof(float), 0);
}

static void merge_points(float* a, float* b) {
  float nv = (*a + *b)/2;
  *a = *b = nv;
}

bool smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2* points, int len, int offset) {
    int l = points != NULL ? len - offset - 1 : 0;
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
      Vector2 startPos = points[offset + v/(2*3*3)];
      Vector2 endPos = points[offset + v/(2*3*3) + 1];
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

void smol_mesh_draw_line_strip(smol_mesh_t* mesh, Shader shader) {
  // Bind shader program
  rlEnableShader(shader.id);

  rlEnableVertexArray(mesh->vaoId);
  // Bind mesh VBO data: vertex position (shader-location = 0)
  rlEnableVertexBuffer(mesh->vboIdVertex);
  rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION]);

  rlEnableVertexBuffer(mesh->vboIdNormal);
  rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_NORMAL], 3, RL_FLOAT, 0, 0, 0);
  rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_NORMAL]);

  // Bind mesh VBO data: vertex colors (shader-location = 3, if available)
  rlDrawVertexArray(0, mesh->vertex_count);
  // Disable all possible vertex array objects (or VBOs)
  rlDisableVertexArray();
  rlDisableVertexBuffer();
  rlDisableVertexBufferElement();

  // Disable shader program
  rlDisableShader();
}

