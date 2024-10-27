
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


#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wpedantic"
#  pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#endif
int gladLoadGL( void* load);
void* glfwGetProcAddress(void*);

void dumby_func() {
  LOGE("Func not loaded...");
  BR_ASSERT(0);
}

void brgl_load(void) {
  gladLoadGL(glfwGetProcAddress);
#define X(type, name) name = (void*)glfwGetProcAddress(#name);
  TO_LOAD(X)
#undef X
#define X(type, name) name = name ? name : (void*)dumby_func;
  TO_LOAD(X)
#undef X
}

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif
