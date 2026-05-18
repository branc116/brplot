#include "src/br_pp.h"
#include "src/br_gl.h"
#if !defined(BR_WANTS_GL)
#  define BR_WANTS_GL 1
#endif
#include "include/brplat.h"
#include "include/br_str_header.h"

unsigned int brgl_load_shader(const char* vs, const char* fs, int* ok) {
  GLuint vsid = brgl_compile_shader(vs, GL_VERTEX_SHADER);
  GLuint fsid = brgl_compile_shader(fs, GL_FRAGMENT_SHADER);
  GLuint program = brgl_glCreateProgram();
  GLint status = 1;

  brgl_glAttachShader(program, vsid);
  brgl_glAttachShader(program, fsid);
  brgl_glLinkProgram(program);
  brgl_glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int max_len = 0;
    brgl_glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);
    if (max_len > 0) {
      int len;
      char* scrach = br_scrach_get((size_t)max_len);
      brgl_glGetProgramInfoLog(program, max_len, &len, scrach);
      LOGE("SHADER: [ID %i] Link error: %s", program, scrach);
      br_scrach_free();
    }
    brgl_glDeleteProgram(program);
    *ok = 0;
  } else *ok = 1;
  brgl_glDeleteShader(vsid);
  brgl_glDeleteShader(fsid);
  return program;
}

unsigned int brgl_compile_shader(const char* code, GLenum type) {
  GLuint shader = 0;
  GLint success = 0;

  shader = brgl_glCreateShader(type);
  brgl_glShaderSource(shader, 1, &code, NULL);
  brgl_glCompileShader(shader);
  brgl_glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
      int max_len = 0;
      brgl_glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);
      if (max_len > 0) {
          int length = 0;
          char *log = br_scrach_get((size_t)max_len);
          brgl_glGetShaderInfoLog(shader, max_len, &length, log);
          LOGE("SHADER: [ID %i] type=%d Compile error: %s", shader, type, log);
          br_scrach_free();
      }
  }

  return shader;
}

void brgl_unload_shader(GLuint id) {
  brgl_glDeleteShader(id);
}

void brgl_enable_shader(GLuint id) {
  brgl_glUseProgram(id);
}

void brgl_blend_func(GLenum sfactor, GLenum dfactor) {
  brgl_glBlendFunc(sfactor, dfactor);
}

void brgl_blend_func_sep(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
  brgl_glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void brgl_blend_equation(GLenum mode) {
  brgl_glBlendEquation(mode);
}

void brgl_enable(GLenum mode) {
  brgl_glEnable(mode);
}

void brgl_disable(GLenum mode) {
  brgl_glDisable(mode);
}

void brgl_enable_back_face_cull(void) {
  brgl_enable(GL_CULL_FACE);
}

void brgl_disable_back_face_cull(void) {
  brgl_disable(GL_CULL_FACE);
}

void brgl_enable_depth_test(void) {
  brgl_glDepthFunc(GL_LEQUAL);
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

  br_extent_t last_draw_extent;
} br_framebuffers[BR_FRAMEBUFFERS] = { 0 };

void brgl_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  br_framebuffers[0].fb_id = 0;
  br_framebuffers[0].tx_id = 0;
  br_framebuffers[0].width = width;
  br_framebuffers[0].height = height;
  brgl_glViewport(x, y, width, height);
}

GLuint brgl_load_texture(const void* data, int width, int height, int format, bool mipmap) {
  (void)format;
  BR_ASSERT(format == BRGL_TEX_GRAY);
  GLuint id = 0;

  brgl_glActiveTexture(GL_TEXTURE0);
  brgl_glBindTexture(GL_TEXTURE_2D, 0);
  brgl_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  brgl_glGenTextures(1, &id);
  brgl_glBindTexture(GL_TEXTURE_2D, id);
  if (mipmap) brgl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  else        brgl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  brgl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  switch (format) {
    case BRGL_TEX_GRAY: {
      brgl_glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
 #if !defined(__EMSCRIPTEN__)
      GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
      brgl_glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
 #endif
    } break;
    case BRGL_TEX_RGBA: {
#define GL_RGBA8_EXT                      0x8058
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_RGBA16F 0x881A
      brgl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    } break;
    case BRGL_TEX_FLOAT_RGBA: {
      brgl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
    } break;
    default: BR_UNREACHABLE("Unknown texture format: %d", format);
  }

  if (mipmap) brgl_glGenerateMipmap(GL_TEXTURE_2D);

  brgl_glBindTexture(GL_TEXTURE_2D, 0);

  if (brgl_g_enable_multisampling) brgl_enable(GL_MULTISAMPLE);
  else                             brgl_disable(GL_MULTISAMPLE);

  return id;
}

void brgl_unload_texture(GLuint tex_id) {
  brgl_glDeleteTextures(1, &tex_id);
}

static bool brgl_fb_is_set(GLuint fb_id);

GLuint brgl_create_framebuffer(int width, int height) {
  GLuint br_id = 1;
  GLuint tx_id = 0;
  GLuint fb_id = 0;
  GLuint rb_id = 0;

  for (; brgl_fb_is_set(br_id); ++br_id);
  brgl_glGenFramebuffers(1, &fb_id);
  brgl_glBindFramebuffer(GL_FRAMEBUFFER, fb_id);

  brgl_glGenTextures(1, &tx_id);
  brgl_glBindTexture(GL_TEXTURE_2D, tx_id);
  brgl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  brgl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  brgl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  brgl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tx_id, 0);

  brgl_glGenRenderbuffers(1, &rb_id);
  brgl_glBindRenderbuffer(GL_RENDERBUFFER, rb_id);
  brgl_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  brgl_glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rb_id);

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

void brgl_enable_framebuffer(GLuint br_id, int x_offset, int y_offset, int new_width, int new_height) {
  GLuint fb_id = br_framebuffers[br_id].fb_id;
  int width = br_framebuffers[br_id].width;
  int height = br_framebuffers[br_id].height;
  brgl_glBindFramebuffer(fb_id ? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER, fb_id);
  if (fb_id != 0 && (width != new_width || height != new_height)) {
    GLuint tx_id = br_framebuffers[br_id].tx_id;
    GLuint rb_id = br_framebuffers[br_id].rb_id;
    brgl_glBindTexture(GL_TEXTURE_2D, tx_id);
    brgl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, new_width, new_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    brgl_glBindRenderbuffer(GL_RENDERBUFFER, rb_id);
    brgl_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, new_width, new_height);
  }
  br_framebuffers[br_id].width = new_width;
  br_framebuffers[br_id].height = new_height;
  brgl_viewport(x_offset, y_offset, new_width, new_height);
  brgl_glScissor(x_offset, y_offset, new_width, new_height);
}

br_extent_t brgl_framebuffer_last_draw_extent(GLuint fb_id) {
  return br_framebuffers[fb_id].last_draw_extent;
}

void brgl_framebuffer_last_draw_extent_set(GLuint fb_id, br_extent_t extent) {
  br_framebuffers[fb_id].last_draw_extent = extent;
}

void brgl_destroy_framebuffer(GLuint br_id) {
  brgl_glDeleteRenderbuffers(1, &br_framebuffers[br_id].rb_id);
  brgl_glDeleteTextures(1, &br_framebuffers[br_id].tx_id);
  brgl_glDeleteFramebuffers(1, &br_framebuffers[br_id].fb_id);
  memset(&br_framebuffers[br_id], 0, sizeof(br_framebuffers[0]));
}

GLuint brgl_load_vao(void) {
  GLuint vao = 0;
  brgl_glGenVertexArrays(1, &vao);
  return vao;
}

void brgl_enable_vao(GLuint vao) {
  brgl_glBindVertexArray(vao);
}

void brgl_draw_vao(GLint first, GLsizei count) {
  brgl_glDrawArrays(GL_TRIANGLES, first, count);
}

void brgl_disable_vao(void) {
  brgl_glBindVertexArray(0);
}

void brgl_unload_vao(GLuint id) {
  brgl_glDeleteVertexArrays(1, &id);
}

GLuint brgl_load_vbo(void const* data, GLsizeiptr len, GLboolean dynamic) {
  GLuint id = 0;

  brgl_glGenBuffers(1, &id);
  brgl_glBindBuffer(GL_ARRAY_BUFFER, id);
  brgl_glBufferData(GL_ARRAY_BUFFER, len, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

  return id;
}

void brgl_update_vbo(GLuint id, void const* data, GLsizeiptr size, GLintptr offset) {
  brgl_glBindBuffer(GL_ARRAY_BUFFER, id);
  brgl_glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

void brgl_enable_vbo(GLuint id) {
  brgl_glBindBuffer(GL_ARRAY_BUFFER, id);
}

void brgl_unload_vbo(GLuint id) {
  brgl_glDeleteBuffers(1, &id);
}

void brgl_enable_vattr(GLuint id) {
  brgl_glEnableVertexAttribArray(id);
}

void brgl_set_vattr(GLuint index, GLint compSize, GLenum type, GLboolean normalized, GLint stride, void const* pointer) {
  brgl_glVertexAttribPointer(index, compSize, type, normalized, stride, pointer);
}

void brgl_set_usamp(GLint uni, GLuint tex) {
  brgl_glActiveTexture(GL_TEXTURE0);
  brgl_glBindTexture(GL_TEXTURE_2D, tex);
  brgl_glUniform1i(uni, 0);
}

void brgl_set_umatrix(GLint uni, float* tex) {
  brgl_glUniformMatrix4fv(uni, 1, true, tex);
}

void brgl_set_u(GLint uni, float* value, int els, int n) {
    switch (els)
    {
        case 1: brgl_glUniform1fv(uni, n, value); break;
        case 2: brgl_glUniform2fv(uni, n, value); break;
        case 3: brgl_glUniform3fv(uni, n, value); break;
        case 4: brgl_glUniform4fv(uni, n, value); break;
        default: BR_ASSERT(0);
    }
}

GLint brgl_get_loca(GLuint shader_id, char const* name) {
  return brgl_glGetAttribLocation(shader_id, name);
}

GLint brgl_get_locu(GLuint shader_id, char const* name) {
  return brgl_glGetUniformLocation(shader_id, name);
}

void brgl_clear(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
  brgl_glClearColor(r, g, b, a);
  brgl_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void brgl_finish(void) {
  brgl_glFinish();
}

static bool brgl_fb_is_set(GLuint br_id) {
  return br_framebuffers[br_id].fb_id != 0;
}
