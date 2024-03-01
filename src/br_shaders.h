#pragma once
#ifndef DEBUG_MACROS
#include "raylib.h"
#include "rlgl.h"
#include "stdlib.h"
#endif

#define NOP2(N, M)

#define BR_ALL_SHADERS(X, X_VEC, X_BUF) \
  X(line_3d_simple, 1024,               \
      X_VEC(eye, 3)                     \
      X_VEC(m_mvp, 16),                 \
                                        \
      X_BUF(vertexPosition, 3)          \
      X_BUF(vertexNormal, 3)            \
      X_BUF(vertexColor, 3)             \
    )                                   \
  X(line_3d, 1024,                      \
      X_VEC(eye, 3)                     \
      X_VEC(m_mvp, 16),                 \
                                        \
      X_BUF(vertexPosition, 3)          \
      X_BUF(vertexNormal, 3)            \
      X_BUF(vertexColor, 3)             \
    )                                   \
  X(grid_3d, 1024,                      \
      X_VEC(eye, 3)                     \
      X_VEC(m_mvp, 16)                  \
      X_VEC(resolution, 4),             \
                                        \
      X_BUF(vertexPosition, 3)          \
      X_BUF(vertexNormal, 3)            \
      X_BUF(vertexColor, 3)             \
    )                                   \
  X(line, 1024,                         \
      X_VEC(offset, 2)                  \
      X_VEC(resolution, 4)              \
      X_VEC(zoom, 2)                    \
      X_VEC(screen, 2),                 \
                                        \
      X_BUF(vertexPosition, 3)          \
      X_BUF(vertexNormal, 3)            \
      X_BUF(vertexColor, 3)             \
    )                                   \
  X(grid, 2,                            \
      X_VEC(resolution, 4)              \
      X_VEC(zoom, 2)                    \
      X_VEC(offset, 2)                  \
      X_VEC(screen, 2),                 \
                                        \
      X_BUF(vertexPosition, 2)          \
    )

#define VEC_TYPE2 Vector2
#define VEC_TYPE3 Vector3
#define VEC_TYPE4 Vector4
#define VEC_TYPE16 Matrix
#define X_U(NAME, DIM) VEC_TYPE ## DIM NAME ## _uv;
#define X(NAME, CAP, U_VEC, BUFFS) \
  typedef struct {                 \
    U_VEC                          \
  } br_shader_ ## NAME ## _uvs_t;
BR_ALL_SHADERS(X, X_U, NOP2)
#undef X
#undef X_U
#undef VEC_TYPE16
#undef VEC_TYPE4
#undef VEC_TYPE3
#undef VEC_TYPE2

#define X_U(NAME, DIM) int NAME ## _u;
#define X_B(NAME, DIM)                \
  unsigned int NAME ## _vbo_id;       \
  int NAME ## _loc;                   \
  float* NAME ## _vbo;
#define X(NAME, CAP, U_VEC, BUFFS)    \
  typedef struct {                    \
    br_shader_ ## NAME ## _uvs_t uvs; \
    U_VEC                             \
    BUFFS                             \
    int len, cap;                     \
    unsigned int id;                  \
    unsigned int vao_id;              \
  } br_shader_ ## NAME ## _t;
BR_ALL_SHADERS(X, X_U, X_B)
#undef X
#undef X_B
#undef X_U

typedef struct {
#define X(NAME, CAP, UNIF, BUFF) br_shader_ ## NAME ## _t* NAME;
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
} br_shaders_t;

#define X(NAME, CAP, U_VEC, BUFF) void br_shader_ ## NAME ## _draw(br_shader_ ## NAME ## _t* shader);
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X

br_shaders_t br_shaders_malloc(void);
void br_shaders_free(br_shaders_t shaders);
void br_shaders_refresh(br_shaders_t shaders);

