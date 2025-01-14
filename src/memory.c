#include "src/br_pp.h"

#include <stdlib.h>

#if defined(__linux__) && defined(BR_DEBUG)
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
#endif
