#include "stdio.h"

// This makes me mad.
// On some emscripten compilers this function is defined,
// and on some it's not. When it's not defined, compile failes.
// If it is defined and it's defined here also, then it failes in
// runtime, Even if it's marked as weak, linker will still pick
// this, implementation. Idk how to handle this..... 
// Maybe on different version of emcc diffrent flags are needed to compile this PoS.
/*
typedef void (*GLFWglproc)(void);
void f(void) {}
GLFWglproc __attribute__((weak)) glfwGetProcAddress(const char* procname) {
  fprintf(stderr, "Using glfwGetProcAddress mocked version, something went wrong with the compile process\n");
  fprintf(stderr, "This will not work, and will crash somewhere during the run time....\n");
  return f;
}
*/
