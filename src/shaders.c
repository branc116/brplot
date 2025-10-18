#include "src/br_shaders.h"
// cpp -I. -DDEBUG_MACROS -E ./src/shaders.c | sed 's/^#/\/\//' | clang-format > tmp.c
#if !defined(DEBUG_MACROS)
#  include "src/br_pp.h"
#  include "src/br_gl.h"
#  include "src/br_filesystem.h"
#  include "src/br_memory.h"
#  include "src/br_da.h"
#  if defined(BR_RELEASE)
#    if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(_WIN32) || defined(__CYGWIN__)
#      include ".generated/shaders.h"
#    elif defined(__EMSCRIPTEN__)
#      include ".generated/shaders_web.h"
#    else
#      error "Shaders for this platform arn't defined"
#    endif
#  endif

#  include <stdio.h>
#  include <stdint.h>
#  include <stdlib.h>
#  include <stdbool.h>
#endif


#define X_BUF(NAME, LEN)                                                         \
  do {                                                                           \
    ret->NAME ## _vbo = BR_MALLOC((size_t)(LEN * cap * 3 * (int)sizeof(float))); \
    if (NULL == ret->NAME ## _vbo) {                                             \
      LOGE("Failed to allocate shader " #NAME " buffer, exitting...\n");         \
      exit(1);                                                                   \
    }                                                                            \
  } while (0);
#define X(NAME, CAP, U_VEC, BUFF)                                                   \
  inline static br_shader_ ## NAME ## _t* br_shader_ ## NAME ## _malloc(void) {     \
    const int cap = CAP;                                                            \
    br_shader_ ## NAME ## _t* ret = BR_CALLOC(1, sizeof(br_shader_ ## NAME ## _t)); \
    if (NULL == ret) {                                                              \
      LOGE("Failed to allocate shader " #NAME ", exitting...\n");                   \
      exit(1);                                                                      \
    }                                                                               \
    BUFF                                                                            \
    ret->cap = cap;                                                                 \
    return ret;                                                                     \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  shader->NAME ## _vbo_id = brgl_load_vbo(shader->NAME ## _vbo, LEN * shader->cap * 3 * (int)sizeof(float), true); \
  brgl_enable_vbo(shader->NAME ## _vbo_id); \
  brgl_set_vattr((uint32_t)shader->NAME ## _loc, LEN, GL_FLOAT, 0, 0, 0); \
  brgl_enable_vattr((uint32_t)shader->NAME ## _loc);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _upload(br_shader_ ## NAME ## _t* shader) { \
    shader->vao_id = brgl_load_vao(); \
    brgl_enable_vao(shader->vao_id); \
    brgl_enable_shader(shader->id); \
    BUFF \
    brgl_disable_vao(); \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  brgl_update_vbo((uint32_t)shader->NAME ## _vbo_id, shader->NAME ## _vbo, LEN * shader->cap * 3 * (int)sizeof(float), 0);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _update_vbs(br_shader_ ## NAME ## _t* shader) { \
    BUFF \
  }
BR_ALL_SHADERS(X, NOP2, X_BUF)
#undef X
#undef X_BUF

#define X_BUF(NAME, LEN) \
  brgl_unload_vbo((uint32_t)shader->NAME ## _vbo_id); \
  BR_FREE(shader->NAME ## _vbo);
#define X_VEC(NAME, LEN) brgl_unload_shader(shader->id);
#define X(NAME, CAP, U_VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _free(br_shader_ ## NAME ## _t* shader) { \
    brgl_unload_vao((uint32_t)shader->vao_id); \
    BUFF \
    U_VEC \
    BR_FREE(shader); \
  }
BR_ALL_SHADERS(X, X_VEC, X_BUF)
#undef X
#undef X_VEC
#undef X_BUF

#if defined(BR_RELEASE)
#  define READ_FILE(file_name) file_name
#  define FREE_FILE_CONTENT(file)
#  define FILE_CONTNET_TYPE const char*
#else
#  define READ_FILE(file_name) br_fs_read1(file_name).str
#  define FREE_FILE_CONTENT(file) BR_FREE(file)
#  define FILE_CONTNET_TYPE char*
#endif
#define X_BUF(NAME, LEN) shader->NAME ## _loc = brgl_get_loca(shader->id, #NAME);
#define X_VEC(NAME, LEN) shader->NAME ## _u = brgl_get_locu(shader->id, #NAME);
#define X(NAME, CAP, VEC, BUFF) \
  inline static void br_shader_ ## NAME ## _compile(br_shader_ ## NAME ## _t* shader) { \
    FILE_CONTNET_TYPE vs = READ_FILE(NAME ## _vs); \
    FILE_CONTNET_TYPE fs = READ_FILE(NAME ## _fs); \
    int ok = 0; \
    shader->id = brgl_load_shader(vs, fs, &ok); \
    if (ok == 0) { \
      LOGE("Failed to load shader: " #NAME); \
      exit(1); \
    } \
    FREE_FILE_CONTENT(fs); \
    FREE_FILE_CONTENT(vs); \
    BUFF \
    VEC \
  }
BR_ALL_SHADERS(X, X_VEC, X_BUF)
#undef X
#undef X_VEC
#undef X_BUF

#define SET_VEC_TEX(NAME) brgl_set_usamp(shader->NAME ## _u, shader->uvs.NAME ## _uv)
#define SET_VEC16(NAME) brgl_set_umatrix(shader->NAME ## _u, &shader->uvs.NAME ## _uv.m0)
#define SET_VEC4(NAME)  brgl_set_u(shader->NAME ## _u, &shader->uvs.NAME ## _uv.x, 4, 1)
#define SET_VEC3(NAME)  brgl_set_u(shader->NAME ## _u, &shader->uvs.NAME ## _uv.x, 3, 1)
#define SET_VEC2(NAME)  brgl_set_u(shader->NAME ## _u, &shader->uvs.NAME ## _uv.x, 2, 1)
#define SET_VEC1(NAME)  brgl_set_u(shader->NAME ## _u, &shader->uvs.NAME ## _uv, 1, 1)
#define X_VEC(NAME, DIM) SET_VEC ## DIM(NAME);
#define X(NAME, CAP, UNIF, NOP2) \
  inline static void br_shader_ ## NAME ## _update_us(br_shader_ ## NAME ## _t* shader) { \
    UNIF \
  }
BR_ALL_SHADERS(X, X_VEC, NOP2)
#undef X
#undef X_VEC

#define X(NAME, CAP, U_VEC, BUFF) \
  void br_shader_ ## NAME ## _draw(br_shader_ ## NAME ## _t* shader) { \
    brgl_enable_shader((uint32_t)shader->id); \
    br_shader_ ## NAME ## _update_us(shader); \
    br_shader_ ## NAME ## _update_vbs(shader); \
    brgl_enable_vao(shader->vao_id); \
    brgl_draw_vao(0, shader->len*3); \
    shader->len = 0; \
  }
BR_ALL_SHADERS(X, NOP2, NOP2)
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

void br_shaders_draw_all(br_shaders_t shaders) {
#define X(NAME, CAP, V, B) \
  if (shaders.NAME->len != 0) { \
    br_shader_ ## NAME ## _draw(shaders.NAME); \
    shaders.NAME->len = 0; \
  }
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
}

void br_shaders_free(br_shaders_t shaders) {
#define X(NAME, CAP, V, B) br_shader_ ## NAME ## _free(shaders.NAME);
  BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X
}

#define X_B(NAME, T) \
  el_size = (int)sizeof(el[0].NAME) / (int)sizeof(float); \
  for (int i = 0; i < el_size; ++i) { \
    for (int k = 0; k < 3; ++k) { \
      shader->NAME ## _vbo[el_size * (shader->len * 3 + k) + i] = ((float*)&(el[k].NAME))[i]; \
    } \
  }
#define X(NAME, CAP, V, B) \
void br_shader_ ## NAME ## _push_tri(br_shader_ ## NAME ## _t *shader, br_shader_ ## NAME ## _el_t el[3]) { \
  if (shader->len >= shader->cap) { \
    br_shader_ ## NAME ## _draw(shader); \
    shader->len = 0; \
  } \
  int el_size = 0; \
    B \
  ++shader->len; \
}
BR_ALL_SHADERS(X, NOP2, X_B)
#undef X
#undef X_B

#define X(NAME, CAP, V, B) \
void br_shader_ ## NAME ## _push_quad(br_shader_ ## NAME ## _t *shader, br_shader_ ## NAME ## _el_t el[4]) { \
  BR_PROFILE("Shader push quad" #NAME) { \
    br_shader_ ## NAME ## _push_tri(shader, (br_shader_ ## NAME ## _el_t[3]) { el[0], el[1], el[2] }); \
    br_shader_ ## NAME ## _push_tri(shader, (br_shader_ ## NAME ## _el_t[3]) { el[0], el[2], el[3] }); \
  } \
}
BR_ALL_SHADERS(X, NOP2, NOP2)
#undef X

#if BR_HAS_SHADER_RELOAD
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

static void* watch_shader_change(void* data) {
  struct { bool* is_dirty; bool const* should_close; }* args = data;
  uint32_t buff[128];
  int fd = inotify_init();
  const char* path = "src/shaders";
  if (-1 == inotify_add_watch(fd, path, IN_MODIFY | IN_CLOSE_WRITE)) {
    LOGE("Failed to start watching shader chages on path %s/%s: %s", getenv("PWD"), path, strerror(errno));
    return NULL;
  }
  LOGI("Sarting to watch shaders on path %s/%s", getenv("PWD"), path);
  while(!*args->should_close) {
    read(fd, buff, sizeof(buff));
    printf("DIRTY SHADERS\n");
    *args->is_dirty = true;
  }
  fprintf(stderr, "Stopping refresh shader thread\n");
  return NULL;
}

void br_start_refreshing_shaders(bool* is_dirty, bool const* should_close) {
  static BR_THREAD_LOCAL struct { bool* is_dirty; bool const* should_close; } args;
  args.is_dirty = is_dirty;
  args.should_close = should_close;
  pthread_t thread2;
  pthread_attr_t attrs2;
  pthread_attr_init(&attrs2);
  if (pthread_create(&thread2, &attrs2, watch_shader_change, &args)) {
    fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
  }
}

#elif defined(_WIN32) || defined(__CYGWIN__)
#include <stdio.h>

typedef struct br_plotter_t br_plotter_t;

void br_start_refreshing_shaders(bool* is_dirty, bool const* should_close) {
  (void)is_dirty; (void)should_close;
  LOGE("TODO: Refreshing shaders is not implemented on Windows....\n");
}
#elif defined(__EMSCRIPTEN__)
typedef struct br_plotter_t br_plotter_t;
void br_start_refreshing_shaders(bool* is_dirty, bool const* should_close) { (void)is_dirty; (void)should_close; }
#else
#  error "Unsupported Platform"
#endif
void br_shaders_refresh(br_shaders_t shaders) {
#  define X_BUF(NAME, LEN) shader->NAME ## _loc = brgl_get_loca(shader->id, #NAME);
#  define X_VEC(NAME, LEN) shader->NAME ## _u = brgl_get_locu(shader->id, #NAME);
#  define X(NAME, CAP, VEC, BUFF)                             \
     {                                                        \
       FILE_CONTNET_TYPE vs = READ_FILE(NAME ## _vs);         \
       FILE_CONTNET_TYPE fs = READ_FILE(NAME ## _fs);         \
       int ok = false;                                        \
       unsigned int new_shader_id = brgl_load_shader(vs, fs, &ok); \
       if (ok == false) {                                     \
         LOGE("Failed to load shader: " #NAME);               \
       }                                                      \
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
#endif // HAS_SHADER_RELOAD

