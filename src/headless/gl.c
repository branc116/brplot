#include "src/br_gl.h"
#if !defined(_MSC_VER)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wreturn-type"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

void brgl_load(void) {}

GLuint glCreateProgram(void) {}
GLuint glCreateShader(GLenum type) {}
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const* string, const GLint * length) {}
void glAttachShader(GLuint program, GLuint shader) {}
void glLinkProgram(GLuint program) {}
void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {}
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {}
void glDeleteProgram(GLuint program) {}
void glDeleteShader(GLuint shader) {}
void glCompileShader(GLuint shader) {}
void glGetShaderiv(GLuint shader, GLenum pname, GLint* params) {}
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog) {}
void glBlendFunc(GLenum sfactor, GLenum dfactor) {}
void glBlendEquation(GLenum mode) {}
void glEnable(GLenum cap) {}
void glDisable(GLenum cap) {}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void * pixels) {}
void glBindTexture(GLenum target, GLuint texture) {}
void glDeleteTextures(GLsizei n, const GLuint * textures) {}
void glPixelStorei(GLenum pname, GLint param) {}
void glGenTextures(GLsizei n, GLuint* textures) {}
void glTexParameteriv(GLenum target, GLenum pname, const GLint* params) {}
void glGenBuffers(GLsizei n, GLuint* buffers) {}
void glBindBuffer(GLenum target, GLuint buffer) {}
void glBufferData(GLenum target, GLsizeiptr size, void const* data, GLenum usage) {}
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {}
void glUseProgram(GLuint program) {}
void glGenVertexArrays(GLsizei n, GLuint* arrays) {}
void glBindVertexArray(GLuint array) {}
void glDeleteVertexArrays(GLsizei n, GLuint const* arrays) {}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void const* data) {}
void glDeleteBuffers(GLsizei n, GLuint const* buffers) {}
GLint glGetAttribLocation(GLuint program, GLchar const* name) {}
GLint glGetUniformLocation(GLuint program, GLchar const* name) {}
void glActiveTexture(GLenum texture) {}
void glUniform1i(GLint location, GLint v0) {}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, GLfloat const* value) {}
void glUniform1fv(GLint location, GLsizei count, GLfloat const* value) {}
void glUniform2fv(GLint location, GLsizei count, GLfloat const* value) {}
void glUniform3fv(GLint location, GLsizei count, GLfloat const* value) {}
void glUniform4fv(GLint location, GLsizei count, GLfloat const* value) {}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {}
void glEnableVertexAttribArray(GLuint index) {}
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {}
void glClear(GLbitfield en) {}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {}

#if !defined(_MSC_VER)
#  pragma GCC diagnostic pop
#endif
