#pragma once
#ifndef DEBUG_MACROS
#  include "src/br_math.h"

#  include <stdlib.h>
#endif

#if defined(BR_SHADER_TOOL) || defined(BR_DEBUG)
#  define grid_fs           "src/shaders/grid.fs"
#  define grid_vs           "src/shaders/grid.vs"
#  define line_fs           "src/shaders/line.fs"
#  define line_vs           "src/shaders/line.vs"
#  define quad_fs           "src/shaders/quad.fs"
#  define quad_vs           "src/shaders/quad.vs"
#  define grid_3d_fs        "src/shaders/grid_3d.fs"
#  define grid_3d_vs        "src/shaders/grid_3d.vs"
#  define line_3d_fs        "src/shaders/line_3d.fs"
#  define line_3d_vs        "src/shaders/line_3d.vs"
#  define line_3d_simple_fs "src/shaders/line_3d_simple.fs"
#  define line_3d_simple_vs "src/shaders/line_3d_simple.vs"
#  define font_fs           "src/shaders/font.fs"
#  define font_vs           "src/shaders/font.vs"
#  define img_fs            "src/shaders/img.fs"
#  define img_vs            "src/shaders/img.vs"
#  define icon_fs           "src/shaders/icon.fs"
#  define icon_vs           "src/shaders/icon.vs"
#endif

#define NOP2(N, M)

#define BR_ALL_SHADERS(X, X_VEC, X_BUF) \
  X(grid, 128,                          \
      X_VEC(bg_color, 4)                \
      X_VEC(lines_color, 4),            \
                                        \
      X_BUF(vert, 3)                    \
    )                                   \
  X(grid_3d, 12,                        \
      X_VEC(eye, 3)                     \
      X_VEC(target, 3)                  \
      X_VEC(look_dir, 3)                \
      X_VEC(m_mvp, 16),                 \
                                        \
      X_BUF(vertexPosition, 3)          \
      X_BUF(vertexColor, 3)             \
      X_BUF(z, 1)                       \
    )                                   \
  X(line_3d_simple, 1024,               \
      X_VEC(m_mvp, 16)                  \
      X_VEC(color, 3),                  \
                                        \
      X_BUF(vertexPosition, 3)          \
      X_BUF(vertexNormal, 3)            \
    )                                   \
  X(line_3d, 16*1024,                   \
      X_VEC(eye, 3)                     \
      X_VEC(m_mvp, 16)                  \
      X_VEC(color, 3),                  \
                                        \
      X_BUF(vertexPosition, 3)          \
      X_BUF(vertexNormal, 3)            \
    )                                   \
  X(line, 2048,                         \
      X_VEC(color, 3),                  \
                                        \
      X_BUF(pos_delta, 4)               \
    )                                   \
  X(icon, 1024,                         \
      X_VEC(atlas, _TEX),               \
                                        \
      X_BUF(pos, 4)                     \
      X_BUF(fg, 4)                      \
      X_BUF(bg, 4)                      \
      X_BUF(clip_dists, 4)              \
      X_BUF(z, 1)                       \
    )                                   \
  X(font, 1024,                         \
      X_VEC(atlas, _TEX),               \
                                        \
      X_BUF(pos, 4)                     \
      X_BUF(fg, 4)                      \
      X_BUF(bg, 4)                      \
      X_BUF(clip_dists, 4)              \
      X_BUF(z, 1)                       \
    )                                   \
  X(img, 2,                             \
      X_VEC(image, _TEX),               \
                                        \
      X_BUF(pos, 4)                     \
      X_BUF(z, 1)                       \
    )                                   \

#define VEC_TYPE1 float
#define VEC_TYPE2 br_vec2_t
#define VEC_TYPE3 br_vec3_t
#define VEC_TYPE4 br_vec4_t
#define VEC_TYPE16 br_mat_t
#define VEC_TYPE_TEX unsigned int
#define X_U(NAME, DIM) VEC_TYPE ## DIM NAME ## _uv;
#define X(NAME, CAP, U_VEC, BUFFS) \
  typedef struct {                 \
    U_VEC                          \
  } br_shader_ ## NAME ## _uvs_t;
BR_ALL_SHADERS(X, X_U, NOP2)
#undef X
#undef X_U

#define X_U(NAME, DIM) VEC_TYPE ## DIM NAME;
#define X(NAME, CAP, U_VEC, BUFFS) \
  typedef struct {                 \
    BUFFS                          \
  } br_shader_ ## NAME ## _el_t;
BR_ALL_SHADERS(X, NOP2, X_U)
#undef X
#undef X_U

#undef VEC_TYPE_TEX
#undef VEC_TYPE16
#undef VEC_TYPE4
#undef VEC_TYPE3
#undef VEC_TYPE2
#undef VEC_TYPE1

#define X_U(NAME, DIM) int NAME ## _u;
#define X_B(NAME, DIM)                \
  unsigned int NAME ## _vbo_id;       \
  int NAME ## _loc;                   \
  float* NAME ## _vbo;
#define X(NAME, CAP, U_VEC, BUFFS)    \
  typedef struct br_shader_ ## NAME ## _t { \
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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_shaders_t {
#define X(NAME, CAP, UNIF, BUFF) br_shader_ ## NAME ## _t* NAME;
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
} br_shaders_t;

#define X(NAME, CAP, U_VEC, BUFF) void br_shader_ ## NAME ## _draw(br_shader_ ## NAME ## _t* shader);
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X

br_shaders_t br_shaders_malloc(void);
void br_shaders_draw_all(br_shaders_t shaders);
void br_shaders_free(br_shaders_t shaders);
void br_shaders_refresh(br_shaders_t shaders);

#define X(NAME, CAP, U_VEC, BUFF) void br_shader_ ## NAME ## _push_tri(br_shader_ ## NAME ## _t* shader, br_shader_ ## NAME ## _el_t el[3]);
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X

#define X(NAME, CAP, U_VEC, BUFF) void br_shader_ ## NAME ## _push_quad(br_shader_ ## NAME ## _t* shader, br_shader_ ## NAME ## _el_t el[4]);
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X

#if BR_HAS_SHADER_RELOAD
void br_start_refreshing_shaders(bool* is_dirty, bool const* should_close);
#endif

#ifdef __cplusplus
}
#endif
