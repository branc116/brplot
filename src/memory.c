#include "src/br_pp.h"

#include "stdlib.h"

#if defined(__linux__) && !defined(RELEASE)
void* br_malloc(size_t size) {
  void* ret = malloc(size);
  return ret;
}

void* br_calloc(size_t n, size_t sz) {
  void* ret = calloc(n, sz);
  return ret;
}

void* br_realloc(void *old, size_t newS) {
  void* newP = realloc(old, newS);
  return newP;
}

void br_free(void* p) {
  free(p);
}

void* br_imgui_malloc(size_t size, void*) {
  return br_malloc(size);
}

void br_imgui_free(void* p, void*) {
  br_free(p);
}
#else
void* br_imgui_malloc(size_t size, void* p) {
  (void)p;
  return malloc(size);
}

void br_imgui_free(void* p, void* up) {
  (void)up;
  free(p);
}
#endif
