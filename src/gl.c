#include "src/br_gl.h"
#include "src/br_str.h"

#if defined(IMGUI) || defined(RAYLIB)
#  define BR_GL(ret_type, name) static ret_type (*name)
#elif defined(HEADLESS)
#  error "HEADLESS Gl not implemented yet."
#else
#  error "Gui is not selected add define flag e.g. -DIMGUI for imgui gui, or -DRAYLIB for raylib gui, or -DHEADLESS for headless gui."
#endif

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601

BR_GL(GLuint, glCreateProgram)(void);
BR_GL(GLuint, glCreateShader)(GLenum type);
BR_GL(void, glShaderSource)(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length);
BR_GL(void, glAttachShader)(GLuint program, GLuint shader);
BR_GL(void, glLinkProgram)(GLuint program);
BR_GL(void, glLinkProgram)(GLuint program);
BR_GL(void, glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
BR_GL(void, glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
BR_GL(void, glDeleteProgram)(GLuint program);
BR_GL(void, glDeleteShader)(GLuint shader);
BR_GL(void, glCompileShader)(GLuint shader);
BR_GL(void, glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
BR_GL(void, glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
BR_GL(void, glBlendFunc)(GLenum sfactor, GLenum dfactor);
BR_GL(void, glBlendEquation)(GLenum mode);
BR_GL(void, glEnable)(GLenum cap);
BR_GL(void, glDisable)(GLenum cap);
BR_GL(void, glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
BR_GL(void, glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels);
BR_GL(void, glBindTexture)(GLenum target, GLuint texture);
BR_GL(void, glDeleteTextures)(GLsizei n, const GLuint * textures);
BR_GL(void, glPixelStorei)(GLenum pname, GLint param);
BR_GL(void, glGenTextures)(GLsizei n, GLuint* textures);
BR_GL(void, glTexParameteriv)(GLenum target, GLenum pname, const GLint* params);
BR_GL(void, glGenBuffers)(GLsizei n, GLuint* buffers);
BR_GL(void, glBindBuffer)(GLenum target, GLuint buffer);
BR_GL(void, glBufferData)(GLenum target, GLsizeiptr size, void const* data, GLenum usage);
BR_GL(void, glVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
BR_GL(void, glUseProgram)(GLuint program);
BR_GL(void, glGenVertexArrays)(GLsizei n, GLuint* arrays);
BR_GL(void, glBindVertexArray)(GLuint array);
BR_GL(void, glBufferData)(GLenum target, GLsizeiptr size, void const* data, GLenum usage);
BR_GL(void, glDeleteVertexArrays)(GLsizei n, GLuint const* arrays);
BR_GL(void, glBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, void const* data);
BR_GL(void, glDeleteBuffers)(GLsizei n, GLuint const* buffers);
BR_GL(void, glDeleteShader)(GLuint shader);
BR_GL(GLint, glGetAttribLocation)(GLuint program, GLchar const* name);
BR_GL(GLint, glGetUniformLocation)(GLuint program, GLchar const* name);
BR_GL(void, glActiveTexture)(GLenum texture);
BR_GL(void, glUniform1i)(GLint location, GLint v0);
BR_GL(void, glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, GLfloat const* value);
BR_GL(void, glUniform1fv)(GLint location, GLsizei count, GLfloat const* value);
BR_GL(void, glUniform2fv)(GLint location, GLsizei count, GLfloat const* value);
BR_GL(void, glUniform3fv)(GLint location, GLsizei count, GLfloat const* value);
BR_GL(void, glUniform4fv)(GLint location, GLsizei count, GLfloat const* value);
BR_GL(void, glDrawArrays)(GLenum mode, GLint first, GLsizei count);
BR_GL(void, glEnableVertexAttribArray)(GLuint index);
BR_GL(void, glClearColor)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
BR_GL(void, glClear)(GLbitfield en);
BR_GL(void, glTexParameteri)(GLenum target, GLenum pname, GLint param);


#if defined(IMGUI) || defined(RAYLIB)
#  include "src/desktop/gl.c"
#elif defined(HEADLESS)
#  error "HEADLESS Gl not implemented yet."
#else
#  error "Gui is not selected add define flag e.g. -DIMGUI for imgui gui, or -DRAYLIB for raylib gui, or -DHEADLESS for headless gui."
#endif

unsigned int brgl_load_shader(const char* vs, const char* fs) {
  GLuint vsid = brgl_compile_shader(vs, GL_VERTEX_SHADER);
  GLuint fsid = brgl_compile_shader(fs, GL_FRAGMENT_SHADER);
  GLuint program = glCreateProgram();
  GLint status;

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
  } else {
    LOGI("Shader compile successfully");
  }
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
          LOGE("SHADER: [ID %i] Compile error: %s", shader, log);
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
  brgl_enable(GL_DEPTH_TEST);
}

void brgl_disable_depth_test(void) {
  brgl_disable(GL_DEPTH_TEST);
}

void brgl_viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  glViewport(x, y, width, height);
}

GLuint brgl_load_texture(const void* data, int width, int height, int format) {
  BR_ASSERT(format == BRGL_TEX_GRAY);
  GLuint id = 0;

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
  GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
  glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

  glBindTexture(GL_TEXTURE_2D, 0);

  return id;
}

void brgl_unload_texture(GLuint tex_id) {
  glDeleteTextures(1, &tex_id);
}

GLuint brgl_load_vao(void) {
  GLuint vao;
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

void brgl_clear_color(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    glClearColor(r, g, b, a);
}

void brgl_clear(void) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
