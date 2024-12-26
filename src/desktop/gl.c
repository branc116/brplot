#include "src/br_gl.h"

#include "external/glfw/include/GLFW/glfw3.h"

#define TO_LOAD(X) \
  X(GLuint, glCreateProgram) \
  X(GLuint, glCreateShader) \
  X(void, glShaderSource) \
  X(void, glAttachShader) \
  X(void, glLinkProgram) \
  X(void, glLinkProgram) \
  X(void, glGetProgramiv) \
  X(void, glGetProgramInfoLog) \
  X(void, glDeleteProgram) \
  X(void, glDeleteShader) \
  X(void, glCompileShader) \
  X(void, glGetShaderiv) \
  X(void, glGetShaderInfoLog) \
  X(void, glBlendFunc) \
  X(void, glBlendEquation) \
  X(void, glEnable) \
  X(void, glDisable) \
  X(void, glViewport) \
  X(void, glTexImage2D) \
  X(void, glBindTexture) \
  X(void, glDeleteTextures) \
  X(void, glPixelStorei) \
  X(void, glGenTextures) \
  X(void, glTexParameteriv) \
  X(void, glGenBuffers) \
  X(void, glBindBuffer) \
  X(void, glBufferData) \
  X(void, glVertexAttribPointer) \
  X(void, glUseProgram) \
  X(void, glGenVertexArrays) \
  X(void, glBindVertexArray) \
  X(void, glBufferData) \
  X(void, glDeleteVertexArrays) \
  X(void, glBufferSubData) \
  X(void, glDeleteBuffers) \
  X(void, glDeleteShader) \
  X(GLint, glGetAttribLocation) \
  X(GLint, glGetUniformLocation) \
  X(void, glActiveTexture) \
  X(void, glUniform1i) \
  X(void, glUniformMatrix4fv) \
  X(void, glUniform1fv) \
  X(void, glUniform2fv) \
  X(void, glUniform3fv) \
  X(void, glUniform4fv) \
  X(void, glDrawArrays) \
  X(void, glEnableVertexAttribArray) \
  X(void, glClearColor) \
  X(void, glClear) \
  X(void, glTexParameteri) \
  X(void, glFramebufferTexture) \
  X(void, glDrawBuffers) \
  X(void, glBindFramebuffer) \
  X(void, glGenFramebuffers) \
  X(void, glDebugMessageCallback) \
  X(void, glFramebufferTexture2D) \
  X(void, glGenRenderbuffers) \
  X(void, glFramebufferRenderbuffer) \
  X(void, glBindRenderbuffer) \
  X(void, glRenderbufferStorage) \
  X(void, glFramebufferParameteri) \
  X(void, glDeleteRenderbuffers) \
  X(void, glDeleteFramebuffers) \
  X(void, glDepthFunc) \
 

#if !defined(_MSC_VER)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wpedantic"
#  pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#  if defined(__clang__)
#    pragma GCC diagnostic ignored "-Wpointer-type-mismatch"
#  endif
#endif
int gladLoadGL( void* load);

void dumby_func() {
  LOGE("Func not loaded...");
  BR_ASSERT(0);
}

void glDebug(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
  (void)userParam;
  LOGE("GLERROR: %d %d %d %d %.*s", source, type, id, severity, length, message);
}

void brgl_load(void) {
#define X(type, name) name = (void*)glfwGetProcAddress(#name);
  TO_LOAD(X)
#undef X
#define X(type, name) name = name ? name : (void*)dumby_func;
  TO_LOAD(X)
#undef X
  glDebugMessageCallback(glDebug, NULL);
}

#if !defined(_MSC_VER)
#  pragma GCC diagnostic pop
#endif
