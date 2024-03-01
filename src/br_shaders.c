#include "br_shaders.h"
// cpp -I. -DDEBUG_MACROS -E ./src/br_shaders.c | sed 's/^#/\/\//' | clang-format
#ifndef DEBUG_MACROS
#include "br_plot.h"
#endif

#ifdef RELEASE
#  ifdef PLATFORM_DESKTOP
#    include "src/misc/shaders.h"
#  elif PLATFORM_WEB
#    include "src/misc/shaders_web.h"
#  else
#    error "Shaders for this platform arn't defined"
#  endif
#else
#  include "src/misc/shaders_dbg.h"
#endif

static const char* grid_VS = SHADER_GRID_VS;
static const char* grid_FS = SHADER_GRID_FS;
static const char* line_FS = SHADER_LINE_FS;
static const char* line_VS = SHADER_LINE_VS;
//static const char* quad_FS = SHADER_QUAD_FS;
//static const char* quad_VS = SHADER_QUAD_VS;

static const char* grid_3d_FS = SHADER_GRID_3D_FS;
static const char* grid_3d_VS = SHADER_GRID_3D_VS;
static const char* line_3d_FS = SHADER_LINE_3D_FS;
static const char* line_3d_VS = SHADER_LINE_3D_VS;
static const char* line_3d_simple_FS = SHADER_LINE_3D_SIMPLE_FS;
static const char* line_3d_simple_VS = SHADER_LINE_3D_SIMPLE_VS;

#define X_BUF(NAME, LEN) \
  ret->NAME ## _vbo = BR_MALLOC((size_t)(LEN * cap * 3 * (int)sizeof(float)));
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static br_shader_ ## NAME ## _t* br_shader_ ## NAME ## _malloc(void) { \
    const int cap = CAP; \
    br_shader_ ## NAME ## _t* ret = BR_CALLOC(1, sizeof(br_shader_ ## NAME ## _t)); \
    BUFF \
    ret->cap = cap; \
    return ret; \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  shader->NAME ## _vbo_id = rlLoadVertexBuffer(shader->NAME ## _vbo, LEN * shader->cap * 3 * (int)sizeof(float), true); \
  rlSetVertexAttribute((uint32_t)shader->NAME ## _loc, 3, RL_FLOAT, 0, 0, 0);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _upload(br_shader_ ## NAME ## _t* shader) { \
    shader->vao_id = rlLoadVertexArray(); \
    rlEnableVertexArray(shader->vao_id); \
    rlEnableShader(shader->id); \
    BUFF \
    rlDisableVertexArray(); \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  rlUpdateVertexBuffer((uint32_t)shader->NAME ## _vbo_id, shader->NAME ## _vbo, LEN * shader->cap * 3 * (int)sizeof(float), 0);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _update_vbs(br_shader_ ## NAME ## _t* shader) { \
    BUFF \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF


#define X_BUF(NAME, LEN) \
  rlUnloadVertexBuffer((uint32_t)shader->NAME ## _vbo_id); \
  BR_FREE(shader->NAME ## _vbo);
#define X_VEC(NAME, LEN) rlUnloadShaderProgram(shader->id);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _free(br_shader_ ## NAME ## _t* shader) { \
    rlUnloadVertexArray((uint32_t)shader->vao_id); \
    BUFF \
    U_VEC \
    BR_FREE(shader); \
  }
BR_ALL_SHADERS(X, X_VEC, X_BUF)
#undef X
#undef X_VEC
#undef X_BUF

#ifdef RELEASE
#  define READ_FILE(file_name) file_name
#  define FREE_FILE_CONTENT(file)
#else
#  define READ_FILE(file_name) LoadFileText(file_name)
#  define FREE_FILE_CONTENT(file) BR_FREE(file)
#endif
#define X_BUF(NAME, LEN) shader->NAME ## _loc = rlGetLocationAttrib(shader->id, #NAME);
#define X_VEC(NAME, LEN) shader->NAME ## _u = rlGetLocationUniform(shader->id, #NAME);
#define X(NAME, CAP, VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _compile(br_shader_ ## NAME ## _t* shader) { \
    char* vs = READ_FILE(NAME ## _VS); \
    char* fs = READ_FILE(NAME ## _FS); \
    shader->id = rlLoadShaderCode(vs, fs); \
    FREE_FILE_CONTENT(fs); \
    FREE_FILE_CONTENT(vs); \
    BUFF \
    VEC \
  }
BR_ALL_SHADERS(X, X_VEC, X_BUF)
#undef X
#undef X_VEC
#undef X_BUF

#define SET_VEC16(NAME) rlSetUniformMatrix(shader->NAME ## _u, shader->uvs.NAME ## _uv)
#define SET_VEC4(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_VEC4, 1)
#define SET_VEC3(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_VEC3, 1)
#define SET_VEC2(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_VEC2, 1)
#define SET_VEC1(NAME)  rlSetUniform(shader->NAME ## _u, &shader->uvs.NAME ## _uv, RL_SHADER_UNIFORM_FLOAT, 1)
#define X_VEC(NAME, DIM) SET_VEC ## DIM(NAME);
#define X(NAME, CAP, UNIF, NOP2) \
  inline static void br_shader_ ## NAME ## _update_us(br_shader_ ## NAME ## _t* shader) { \
    UNIF \
  }
BR_ALL_SHADERS(X, X_VEC, NOP2)
#undef X
#undef X_VEC

#define X_BUF(NAME, LEN) \
  rlEnableVertexBuffer(shader->NAME ## _vbo_id); \
  rlSetVertexAttribute((uint32_t)shader->NAME ## _loc, LEN, RL_FLOAT, 0, 0, 0); \
  rlEnableVertexAttribute((uint32_t)shader->NAME ## _loc);
#define X(NAME, CAP, U_VEC, BUFF) \
  void br_shader_ ## NAME ## _draw(br_shader_ ## NAME ## _t* shader) { \
    rlEnableShader((uint32_t)shader->id); \
    br_shader_ ## NAME ## _update_us(shader); \
    br_shader_ ## NAME ## _update_vbs(shader); \
    rlEnableVertexArray(shader->vao_id); \
    BUFF \
    rlDrawVertexArray(0, shader->len*3); \
    rlDisableVertexArray(); \
    rlDisableVertexBuffer(); \
    rlDisableVertexBufferElement(); \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

br_shaders_t br_shaders_malloc(void) {
  br_shaders_t ret;
#define X(NAME, CAP, V, B) ret.NAME = br_shader_ ## NAME ## _malloc();
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
#define X(NAME, CAP, V, B) br_shader_ ## NAME ## _compile(ret.NAME);
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
#define X(NAME, CAP, V, B) br_shader_ ## NAME ## _upload(ret.NAME);
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
  return ret;
}

void br_shaders_free(br_shaders_t shaders) {
#define X(NAME, CAP, V, B) br_shader_ ## NAME ## _free(shaders.NAME);
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
}

#ifndef RELEASE
void br_shaders_refresh(br_shaders_t shaders) {
#  define X_BUF(NAME, LEN) shader->NAME ## _loc = rlGetLocationAttrib(shader->id, #NAME);
#  define X_VEC(NAME, LEN) shader->NAME ## _u = rlGetLocationUniform(shader->id, #NAME);
#  define X(NAME, CAP, VEC, BUFF)                             \
     {                                                        \
       char* vs = READ_FILE(NAME ## _VS);                     \
       char* fs = READ_FILE(NAME ## _FS);                     \
       unsigned int new_shader_id = rlLoadShaderCode(vs, fs); \
       if (new_shader_id > 0) {                               \
         br_shader_ ## NAME ## _t* shader = shaders.NAME;     \
         shader->id = new_shader_id;                          \
         BUFF                                                 \
         VEC                                                  \
       }                                                      \
       FREE_FILE_CONTENT(fs);                                 \
       FREE_FILE_CONTENT(vs);                                 \
     }
   BR_ALL_SHADERS(X, X_VEC, X_BUF)
#  undef X
#  undef X_VEC
#  undef X_BUF
}
#endif

