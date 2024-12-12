#pragma once

#define BRGL_TEX_GRAY 1


#define GL_FALSE 0
#define GL_ONE 1
#define GL_TRIANGLES 0x0004
#define GL_SRC_ALPHA 0x0302
#define GL_DST_ALPHA 0x0304
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE0 0x84C0
#define GL_FLOAT 0x1406
#define GL_RED 0x1903
#define GL_RGB 0x1907
#define GL_MAX 0x8008
#define GL_R8 0x8229
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46

#define BR_KEY_TWO 50
#define BR_KEY_THREE 51
#define BR_KEY_C 67
#define BR_KEY_D 68
#define BR_KEY_F 70
#define BR_KEY_H 72
#define BR_KEY_J 74
#define BR_KEY_K 75
#define BR_KEY_R 82
#define BR_KEY_S 83
#define BR_KEY_T 84
#define BR_KEY_X 88
#define BR_KEY_Y 89

typedef int GLint;
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef char GLchar;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef unsigned int GLbitfield;

#if defined(_WIN64)
typedef signed   long long int GLsizeiptr;
typedef unsigned long long int khronos_usize_t;
typedef signed   long long int GLintptr;
typedef unsigned long long int khronos_uintptr_t;
#else
typedef signed   long  int     GLsizeiptr;
typedef unsigned long  int     khronos_usize_t;
typedef signed   long  int     GLintptr;
typedef unsigned long  int     khronos_uintptr_t;
#endif

void brgl_load(void);
unsigned int brgl_load_shader(const char* vs, const char* fs);
unsigned int brgl_compile_shader(const char* code, GLenum type);
void brgl_unload_shader(GLuint id);
void brgl_enable_shader(GLuint id);
void brgl_blend_equation(GLenum mode);
void brgl_blend_func(GLenum sfactor, GLenum dfactor);
void brgl_enable(GLenum mode);
void brgl_disable(GLenum mode);
void brgl_enable_back_face_cull(void);
void brgl_disable_back_face_cull(void);
void brgl_enable_depth_test(void);
void brgl_disable_depth_test(void);
void brgl_viewport(GLint x, GLint y, GLsizei width, GLsizei height);
GLuint brgl_load_texture(const void* data, int width, int height, int format);
void brgl_unload_texture(GLuint tex_id);

GLuint brgl_create_framebuffer(int width, int height);
GLuint brgl_framebuffer_to_texture(GLuint br_id);
void brgl_enable_framebuffer(GLuint fb_id, int new_width, int new_height);
void brgl_destroy_framebuffer(GLuint fb_id);

GLuint brgl_load_vao(void);
void brgl_enable_vao(GLuint vao);
void brgl_draw_vao(GLint first, GLsizei count);
void brgl_disable_vao(void);
void brgl_unload_vao(GLuint id);

GLuint brgl_load_vbo(void const* data, GLsizeiptr len, GLboolean dynamic);
void brgl_update_vbo(GLuint id, void const* data, GLsizeiptr size, GLintptr offset);
void brgl_enable_vbo(GLuint id);
void brgl_unload_vbo(GLuint id);

void brgl_enable_vattr(GLuint id);
void brgl_set_vattr(GLuint index, GLint compSize, GLenum type, GLboolean normalized, GLint stride, void const* pointer);
void brgl_set_usamp(GLint uni, GLuint tex);
void brgl_set_umatrix(GLint uni, float* tex);
void brgl_set_u(GLint uni, float* tex, int els, int n);
GLint brgl_get_loca(GLuint shader_id, char const* name);
GLint brgl_get_locu(GLuint shader_id, char const* name);

void brgl_clear_color(float r, float g, float b, float a);
void brgl_clear(void);
