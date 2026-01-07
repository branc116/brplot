#include "src/br_pp.h"
#include "src/br_gl.h"
#if !defined(BR_WANTS_GL)
#  define BR_WANTS_GL 1
#endif
#include "src/br_platform.h"
#include "include/br_str_header.h"

#include <string.h>
#include <stdio.h>

// TODO: Handle this some other way...
#if defined(BR_GL_DRAW_ALL_CB)

static BR_THREAD_LOCAL struct {
  br_shaders_t* shaders;
} brgl_state;

void brgl_construct(br_shaders_t* shaders) {
  brgl_state.shaders = shaders;
}

#endif

unsigned int brgl_load_shader(const char* vs, const char* fs, int* ok) {
  GLuint vsid = brgl_compile_shader(vs, GL_VERTEX_SHADER);
  GLuint fsid = brgl_compile_shader(fs, GL_FRAGMENT_SHADER);
  GLuint program = glCreateProgram();
  GLint status = 1;

  glAttachShader(program, vsid);
  glAttachShader(program, fsid);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int max_len = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);
    if (max_len > 0) {
      int len;
      char* scrach = br_scrach_get((size_t)max_len);
      glGetProgramInfoLog(program, max_len, &len, scrach);
      LOGE("SHADER: [ID %i] Link error: %s", program, scrach);
      br_scrach_free();
    }
    glDeleteProgram(program);
    *ok = 0;
  } else *ok = 1;
  glDeleteShader(vsid);
  glDeleteShader(fsid);
  return program;
}

unsigned int brgl_compile_shader(const char* code, GLenum type) {
  GLuint shader = 0;
  GLint success = 0;

  shader = glCreateShader(type);
  glShaderSource(shader, 1, &code, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
      int max_len = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);
      if (max_len > 0) {
          int length = 0;
          char *log = br_scrach_get((size_t)max_len);
          glGetShaderInfoLog(shader, max_len, &length, log);
          LOGE("SHADER: [ID %i] type=%d Compile error: %s", shader, type, log);
          br_scrach_free();
      }
  }

  return shader;
}

void brgl_unload_shader(GLuint id) {
  glDeleteShader(id);
}

void brgl_enable_shader(GLuint id) {
  glUseProgram(id);
}

void brgl_blend_func(GLenum sfactor, GLenum dfactor) {
  glBlendFunc(sfactor, dfactor);
}

void brgl_blend_func_sep(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
  glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void brgl_blend_equation(GLenum mode) {
  glBlendEquation(mode);
}

void brgl_enable(GLenum mode) {
  glEnable(mode);
}

void brgl_disable(GLenum mode) {
  glDisable(mode);
}

void brgl_enable_back_face_cull(void) {
  brgl_enable(GL_CULL_FACE);
}

void brgl_disable_back_face_cull(void) {
  brgl_disable(GL_CULL_FACE);
}

void brgl_enable_depth_test(void) {
  glDepthFunc(GL_LEQUAL);
  brgl_enable(GL_DEPTH_TEST);
}

void brgl_disable_depth_test(void) {
  brgl_disable(GL_DEPTH_TEST);
}

void brgl_enable_clip_distance(void) {
  brgl_enable(GL_CLIP_DISTANCE0);
  brgl_enable(GL_CLIP_DISTANCE0 + 1);
  brgl_enable(GL_CLIP_DISTANCE0 + 2);
  brgl_enable(GL_CLIP_DISTANCE0 + 3);
}

void brgl_disable_clip_distance(void) {
  brgl_disable(GL_CLIP_DISTANCE0);
  brgl_disable(GL_CLIP_DISTANCE0 + 1);
  brgl_disable(GL_CLIP_DISTANCE0 + 2);
  brgl_disable(GL_CLIP_DISTANCE0 + 3);
}

static BR_THREAD_LOCAL bool brgl_g_enable_multisampling = false;
#define GL_MULTISAMPLE 0x809D
void brgl_enable_multisampling(void) {
  brgl_g_enable_multisampling = true;
}

void brgl_disable_multisampling(void) {
  brgl_g_enable_multisampling = false;
}

#define BR_FRAMEBUFFERS 16
#define BR_FRAMEBUFFER_STACK 16
static BR_THREAD_LOCAL struct {
  GLuint fb_id, tx_id, rb_id;
  int width, height;
} br_framebuffers[BR_FRAMEBUFFERS] = { 0 };

void brgl_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  br_framebuffers[0].fb_id = 0;
  br_framebuffers[0].tx_id = 0;
  br_framebuffers[0].width = width;
  br_framebuffers[0].height = height;
  glViewport(x, y, width, height);
}

GLuint brgl_load_texture(const void* data, int width, int height, int format, bool mipmap) {
  (void)format;
  BR_ASSERT(format == BRGL_TEX_GRAY);
  GLuint id = 0;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  if (mipmap) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  else        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
#if !defined(__EMSCRIPTEN__)
  GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
  glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
#endif

  if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);

  if (brgl_g_enable_multisampling) brgl_enable(GL_MULTISAMPLE);
  else                             brgl_disable(GL_MULTISAMPLE);

  return id;
}

void brgl_unload_texture(GLuint tex_id) {
  glDeleteTextures(1, &tex_id);
}

static bool brgl_fb_is_set(GLuint fb_id);

GLuint brgl_create_framebuffer(int width, int height) {
  GLuint br_id = 1;
  GLuint tx_id = 0;
  GLuint fb_id = 0;
  GLuint rb_id = 0;

  for (; brgl_fb_is_set(br_id); ++br_id);
  glGenFramebuffers(1, &fb_id);
  glBindFramebuffer(GL_FRAMEBUFFER, fb_id);

  glGenTextures(1, &tx_id);
  glBindTexture(GL_TEXTURE_2D, tx_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tx_id, 0);

  glGenRenderbuffers(1, &rb_id);
  glBindRenderbuffer(GL_RENDERBUFFER, rb_id);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rb_id);

  br_framebuffers[br_id].tx_id = tx_id;
  br_framebuffers[br_id].fb_id = fb_id;
  br_framebuffers[br_id].rb_id = rb_id;
  br_framebuffers[br_id].width = width;
  br_framebuffers[br_id].height = height;
  return br_id;
}

GLuint brgl_framebuffer_to_texture(GLuint br_id) {
  return br_framebuffers[br_id].tx_id;
}

void brgl_enable_framebuffer(GLuint br_id, int new_width, int new_height) {
   // Draw everything that was not already
#if defined(BR_GL_DRAW_ALL_CB)
  BR_GL_DRAW_ALL_CB(*brgl_state.shaders);
#endif
  //br_shaders_draw_all(*brgl_state.shaders);

  GLuint fb_id = br_framebuffers[br_id].fb_id;
  int width = br_framebuffers[br_id].width;
  int height = br_framebuffers[br_id].height;
  glBindFramebuffer(fb_id ? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER, fb_id);
  if (fb_id != 0 && (width != new_width || height != new_height)) {
    GLuint tx_id = br_framebuffers[br_id].tx_id;
    GLuint rb_id = br_framebuffers[br_id].rb_id;
    glBindTexture(GL_TEXTURE_2D, tx_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, new_width, new_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindRenderbuffer(GL_RENDERBUFFER, rb_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, new_width, new_height);
  }
  br_framebuffers[br_id].width = new_width;
  br_framebuffers[br_id].height = new_height;
  brgl_viewport(0, 0, new_width, new_height);
}

void brgl_destroy_framebuffer(GLuint br_id) {
  glDeleteRenderbuffers(1, &br_framebuffers[br_id].rb_id);
  glDeleteTextures(1, &br_framebuffers[br_id].tx_id);
  glDeleteFramebuffers(1, &br_framebuffers[br_id].fb_id);
  memset(&br_framebuffers[br_id], 0, sizeof(br_framebuffers[0]));
}

GLuint brgl_load_vao(void) {
  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  return vao;
}

void brgl_enable_vao(GLuint vao) {
  glBindVertexArray(vao);
}

void brgl_draw_vao(GLint first, GLsizei count) {
  glDrawArrays(GL_TRIANGLES, first, count);
}

void brgl_disable_vao(void) {
  glBindVertexArray(0);
}

void brgl_unload_vao(GLuint id) {
  glDeleteVertexArrays(1, &id);
}

GLuint brgl_load_vbo(void const* data, GLsizeiptr len, GLboolean dynamic) {
  GLuint id = 0;

  glGenBuffers(1, &id);
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBufferData(GL_ARRAY_BUFFER, len, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

  return id;
}

void brgl_update_vbo(GLuint id, void const* data, GLsizeiptr size, GLintptr offset) {
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void brgl_enable_vbo(GLuint id) {
  glBindBuffer(GL_ARRAY_BUFFER, id);
}

void brgl_unload_vbo(GLuint id) {
  glDeleteBuffers(1, &id);
}

void brgl_enable_vattr(GLuint id) {
  glEnableVertexAttribArray(id);
}

void brgl_set_vattr(GLuint index, GLint compSize, GLenum type, GLboolean normalized, GLint stride, void const* pointer) {
  glVertexAttribPointer(index, compSize, type, normalized, stride, pointer);
}

void brgl_set_usamp(GLint uni, GLuint tex) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(uni, 0);
}

void brgl_set_umatrix(GLint uni, float* tex) {
  glUniformMatrix4fv(uni, 1, true, tex);
}

void brgl_set_u(GLint uni, float* value, int els, int n) {
    switch (els)
    {
        case 1: glUniform1fv(uni, n, value); break;
        case 2: glUniform2fv(uni, n, value); break;
        case 3: glUniform3fv(uni, n, value); break;
        case 4: glUniform4fv(uni, n, value); break;
        default: BR_ASSERT(0);
    }
}

GLint brgl_get_loca(GLuint shader_id, char const* name) {
  return glGetAttribLocation(shader_id, name);
}

GLint brgl_get_locu(GLuint shader_id, char const* name) {
  return glGetUniformLocation(shader_id, name);
}

void brgl_clear(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void brgl_finish(void) {
  glFinish();
}

static bool brgl_fb_is_set(GLuint br_id) {
  return br_framebuffers[br_id].fb_id != 0;
}
