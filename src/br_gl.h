#pragma once

#define BRGL_TEX_GRAY 1


/* Stencil */
#define GL_STENCIL_BITS            0x0D57
#define GL_STENCIL_TEST            0x0B90
#define GL_STENCIL_CLEAR_VALUE     0x0B91
#define GL_STENCIL_FUNC            0x0B92
#define GL_STENCIL_VALUE_MASK      0x0B93
#define GL_STENCIL_FAIL            0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF             0x0B97
#define GL_STENCIL_WRITEMASK       0x0B98
#define GL_STENCIL_INDEX           0x1901
#define GL_KEEP                    0x1E00
#define GL_REPLACE                 0x1E01
#define GL_INCR                    0x1E02
#define GL_DECR                    0x1E03

/* Blending */
#define GL_BLEND               0x0BE2
#define GL_BLEND_SRC           0x0BE1
#define GL_BLEND_DST           0x0BE0
#define GL_LEQUAL              0x0203
#define GL_SRC_COLOR           0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA           0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA           0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR           0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE  0x0308

#define GL_FALSE 0
#define GL_ONE 1
#define GL_TRIANGLES 0x0004
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_DST_ALPHA 0x0304
#define GL_DST_COLOR 0x0306
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE0 0x84C0
#define GL_FLOAT 0x1406
#define GL_RED 0x1903
#define GL_RGB 0x1907
#define GL_RGBA  0x1908
#define GL_CLIP_DISTANCE0 0x3000
#define GL_FUNC_ADD 0x8006
#define GL_MIN 0x8007
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

#define BR_KEY_SPACE 32
#define BR_KEY_OPEN_PAREN 40
#define BR_KEY_CLOSE_PAREN 41
#define BR_KEY_MUL 42
#define BR_KEY_PLUS 43
#define BR_KEY_COMMA 44
#define BR_KEY_MINUS 45
#define BR_KEY_DOT 46
#define BR_KEY_SLASH 47
#define BR_KEY_0 48
#define BR_KEY_1 49
#define BR_KEY_2 50
#define BR_KEY_3 51
#define BR_KEY_4 52
#define BR_KEY_5 53
#define BR_KEY_6 54
#define BR_KEY_7 55
#define BR_KEY_8 56
#define BR_KEY_9 57
#define BR_KEY_A 65
#define BR_KEY_Z 90
#define BR_KEY_a 97
#define BR_KEY_z 122
#define BR_KEY_HASH 35

#define BR_KEY_ESCAPE             256
#define BR_KEY_ENTER              257
#define BR_KEY_TAB                258
#define BR_KEY_BACKSPACE          259
#define BR_KEY_INSERT             260
#define BR_KEY_DELETE             261
#define BR_KEY_RIGHT              262
#define BR_KEY_LEFT               263
#define BR_KEY_DOWN               264
#define BR_KEY_UP                 265
#define BR_KEY_PAGE_UP            266
#define BR_KEY_PAGE_DOWN          267
#define BR_KEY_HOME               268
#define BR_KEY_END                269
#define BR_KEY_CAPS_LOCK          280
#define BR_KEY_SCROLL_LOCK        281
#define BR_KEY_NUM_LOCK           282
#define BR_KEY_PRINT_SCREEN       283
#define BR_KEY_PAUSE              284
#define BR_KEY_F1                 290
#define BR_KEY_F2                 291
#define BR_KEY_F3                 292
#define BR_KEY_F4                 293
#define BR_KEY_F5                 294
#define BR_KEY_F6                 295
#define BR_KEY_F7                 296
#define BR_KEY_F8                 297
#define BR_KEY_F9                 298
#define BR_KEY_F10                299
#define BR_KEY_F11                300
#define BR_KEY_F12                301
#define BR_KEY_F13                302
#define BR_KEY_F14                303
#define BR_KEY_F15                304
#define BR_KEY_F16                305
#define BR_KEY_F17                306
#define BR_KEY_F18                307
#define BR_KEY_F19                308
#define BR_KEY_F20                309
#define BR_KEY_F21                310
#define BR_KEY_F22                311
#define BR_KEY_F23                312
#define BR_KEY_F24                313
#define BR_KEY_F25                314
#define BR_KEY_KP_0               320
#define BR_KEY_KP_1               321
#define BR_KEY_KP_2               322
#define BR_KEY_KP_3               323
#define BR_KEY_KP_4               324
#define BR_KEY_KP_5               325
#define BR_KEY_KP_6               326
#define BR_KEY_KP_7               327
#define BR_KEY_KP_8               328
#define BR_KEY_KP_9               329
#define BR_KEY_KP_DECIMAL         330
#define BR_KEY_KP_DIVIDE          331
#define BR_KEY_KP_MULTIPLY        332
#define BR_KEY_KP_SUBTRACT        333
#define BR_KEY_KP_ADD             334
#define BR_KEY_KP_ENTER           335
#define BR_KEY_KP_EQUAL           336
#define BR_KEY_LEFT_SHIFT         340
#define BR_KEY_LEFT_CONTROL       341
#define BR_KEY_LEFT_ALT           342
#define BR_KEY_LEFT_SUPER         343
#define BR_KEY_RIGHT_SHIFT        344
#define BR_KEY_RIGHT_CONTROL      345
#define BR_KEY_RIGHT_ALT          346
#define BR_KEY_RIGHT_SUPER        347
#define BR_KEY_MENU               348

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
unsigned int brgl_load_shader(const char* vs, const char* fs, int* out_ok);
unsigned int brgl_compile_shader(const char* code, GLenum type);
void brgl_unload_shader(GLuint id);
void brgl_enable_shader(GLuint id);
void brgl_blend_equation(GLenum mode);
void brgl_blend_func(GLenum sfactor, GLenum dfactor);
void brgl_blend_func_sep(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
void brgl_enable(GLenum mode);
void brgl_disable(GLenum mode);
void brgl_enable_back_face_cull(void);
void brgl_disable_back_face_cull(void);
void brgl_enable_depth_test(void);
void brgl_disable_depth_test(void);
void brgl_enable_clip_distance(void);
void brgl_disable_clip_distance(void);
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

void brgl_clear(float r, float g, float b, float a);
