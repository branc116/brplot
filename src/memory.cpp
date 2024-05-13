#include "cstdlib"
#include "br_pp.h"

#if defined(__linux__) && !defined(RELEASE)
extern "C" void* br_malloc(size_t size) {
  void* ret = malloc(size);
  return ret;
}

extern "C" void* br_calloc(size_t n, size_t sz) {
  void* ret = calloc(n, sz);
  return ret;
}

extern "C" void* br_realloc(void *old, size_t newS) {
  void* newP = realloc(old, newS);
  return newP;
}

extern "C" void br_free(void* p) {
  free(p);
}

extern "C" void* br_imgui_malloc(size_t size, void*) {
  return br_malloc(size);
}

extern "C" void br_imgui_free(void* p, void*) {
  br_free(p);
}
#else
extern "C" void* br_imgui_malloc(size_t size, void*) {
  return malloc(size);
}

extern "C" void br_imgui_free(void* p, void*) {
  free(p);
}
#endif
