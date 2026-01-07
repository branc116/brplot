#include "src/br_gl.h"


//br_extent_t brui_resizable_cur_extent(int handle) { (void)handle; return BR_EXTENT(0,0,0,0); };
//void brui_resizable_delete(int handle) { (void)handle; }
void brgl_destroy_framebuffer(GLuint br_id) { (void)br_id; }
GLuint brgl_create_framebuffer(int width, int height) { (void)width; (void)height; return 0; }
void brgl_draw_vao(GLint first, GLsizei count) { (void)first; (void)count; }
void brgl_enable_vao(GLuint vao) { (void)vao; }
void brgl_enable_shader(GLuint id) { (void)id; }
void brgl_set_usamp(GLint uni, GLuint tex) { (void)uni; (void)tex; }
void brgl_set_u(GLint uni, float* value, int els, int n) { (void)uni; (void)value; (void)els; (void)n; }
void brgl_set_umatrix(GLint uni, float* tex) { (void)uni; (void)tex; }
GLint brgl_get_locu(GLuint shader_id, char const* name) { (void)shader_id; (void)name; return 1; }
GLint brgl_get_loca(GLuint shader_id, char const* name) { (void)shader_id; (void)name; return 1; }
unsigned int brgl_load_shader(const char* vs, const char* fs, int* out_ok) { (void)vs; (void)fs; *out_ok = true; return 1; }
void brgl_unload_shader(GLuint id) { (void)id; }
void brgl_unload_vbo(GLuint id) { (void)id; }
void brgl_unload_vao(GLuint id) { (void)id; }
void brgl_update_vbo(GLuint id, void const* data, GLsizeiptr size, GLintptr offset) { (void)id; (void)data; (void)size; (void)offset; }
void brgl_disable_vao(void) {}
void brgl_enable_vattr(GLuint id) { (void)id; }
void brgl_set_vattr(GLuint index, GLint compSize, GLenum type, GLboolean normalized, GLint stride, void const* pointer) { (void)index; (void)compSize; (void)type; (void)normalized; (void)stride; (void)pointer; }
void brgl_enable_vbo(GLuint id) { (void)id; }
GLuint brgl_load_vbo(void const* data, GLsizeiptr len, GLboolean dynamic) { (void)data; (void)len; (void)dynamic; return 1; }
GLuint brgl_load_vao(void) { return 1; }
void brgl_disable_back_face_cull(void) {}
void brgl_enable_depth_test(void) {}
void brgl_enable(GLenum mode) { (void)mode; }
void brgl_enable_clip_distance(void) {}
void brgl_blend_func(GLenum sfactor, GLenum dfactor) { (void)sfactor; (void)dfactor; }
void brgl_blend_equation(GLenum mode) { (void)mode; }
void brgl_viewport(GLint x, GLint y, GLsizei width, GLsizei height) { (void)x; (void)y; (void)width; (void)height; }
void brgl_enable_framebuffer(GLuint fb_id, int new_width, int new_height) { (void)fb_id; (void)new_width; (void)new_height; }
GLuint brgl_load_texture(const void* data, int width, int height, int format, bool mipmap) { (void)data; (void)width; (void)height; (void)format; (void)mipmap; return 1; }
void brgl_unload_texture(GLuint tex_id) { (void)tex_id; }
GLuint brgl_framebuffer_to_texture(GLuint br_id) { (void)br_id; return 1; }
void brgl_clear(float r, float g, float b, float a) { (void)r; (void)g; (void)b; (void)a; }
void brgl_construct(br_shaders_t* shaders) { (void)shaders; }
void brgl_disable_multisampling(void) {}
void brgl_enable_multisampling(void) {}
