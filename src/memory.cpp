#include "br_plot.h"
#include "br_help.h"
#include "cstdlib"
#include "cassert"
#include <map>

static std::map<size_t, size_t> alloc_sizes;

static inline void __attribute__((constructor(101))) run_tests(void) {
  alloc_sizes = {};
}

static void br_malloc_stats(size_t mem, size_t size) {
  context.alloc_size += size;
  ++context.alloc_count;
  context.alloc_total_size += size;
  ++context.alloc_total_count;
  context.alloc_max_size = maxui64(context.alloc_max_size, context.alloc_size);
  context.alloc_max_count = maxui64(context.alloc_max_count, context.alloc_count);
  alloc_sizes.insert(std::pair<size_t, size_t>(mem, size));
}

static void br_free_stats(size_t mem) {
  if (mem == 0) return;
  auto f = alloc_sizes.find(mem);
  if (f == alloc_sizes.end()) {
    ++context.free_of_unknown_memory;
    return;
  }
  size_t size = f->second;
  alloc_sizes.erase(mem);
  context.alloc_size -= size;
  --context.alloc_count;
}

// size_t alloc_size, alloc_count, alloc_total_size, alloc_total_count, alloc_max_size, alloc_max_count;
extern "C" void* br_malloc(size_t size) {
  void* ret = malloc(size);
  br_malloc_stats((size_t)ret, size);
  return ret;
}

extern "C" void* br_calloc(size_t n, size_t sz) {
  void* ret = calloc(n, sz);
  br_malloc_stats((size_t)ret, n*sz);
  return ret;
}

extern "C" void* br_realloc(void *old, size_t newS) {
  br_free_stats((size_t)old);
  void* newP = realloc(old, newS);
  br_malloc_stats((size_t)newP, newS);
  return newP;
}

extern "C" void br_free(void* p) {
  br_free_stats((size_t)p);
  free(p);
}

extern "C" void* br_imgui_malloc(size_t size, void*) {
  return br_malloc(size);
}

extern "C" void br_imgui_free(void* p, void*) {
  br_free(p);
}
