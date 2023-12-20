typedef void (*GLFWglproc)(void);
void f(void) {}
GLFWglproc glfwGetProcAddress(const char* procname) {
  return f;
}
