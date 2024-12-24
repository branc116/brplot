#pragma once
#include <stddef.h>
#include "external/Tracy/tracy/TracyC.h"

#if defined(DEBUG) || defined(RELEASE) || defined(BR_RELEASE)
#  error "Please don't define DEBUG or RELEASE or BR_RELEASE macros"
#endif
#if !defined(BR_DEBUG)
#  define BR_RELEASE
#endif

#if defined(__EMSCRIPTEN__)
#  include <emscripten.h>
#  define BR_API EMSCRIPTEN_KEEPALIVE
#else
#  define BR_API
#endif

#if defined(BR_DEBUG) && !defined(__linux__) && defined(UNIT_TEST)
   // IT don't work on windows....
#  undef UNIT_TEST
#endif

// This is the size of buffer used to transfer points from cpu to gpu.
#define PTOM_COUNT (1<<10)

#if defined(BR_MUSL)
#elif defined(__linux__)
#  define LOCK_T pthread_mutex_t
#endif

#if defined(LOCK_T)
#  define LOCK(x) LOCK_T x;
#else
#  define LOCK(x)
#endif

#if 1 || !defined(BR_DISABLE_LOG)
#  define LOG(...)
#  define LOGI(format, ...) fprintf(stderr, "[INFO][" __FILE__ ":%d]" format "\n", __LINE__, ##__VA_ARGS__)
#  define LOGW(format, ...) fprintf(stderr, "[WARNING][" __FILE__ ":%d]" format "\n", __LINE__, ##__VA_ARGS__)
#  define LOGE(format, ...) fprintf(stderr, "ERROR: [%s:%d]" format "\n", __FILE__, __LINE__,  ##__VA_ARGS__)
#  define LOGF(format, ...) (fprintf(stderr, "FATAL: [%s:%d]" format "\n", __FILE__, __LINE__,  ##__VA_ARGS__), exit(1))
#else
#  define LOG(...)
#  define LOGI(...)
#  define LOGE(text)
#  define LOGEF(format, ...)
#endif


#ifdef __cplusplus
extern "C" {
#endif

// TODO: Move malloc stuff to it's own header file, maybe...
void* br_malloc(size_t size);
void* br_calloc(size_t n, size_t size);
void* br_realloc(void *old, size_t newS);
void  br_free(void* p);
void* br_imgui_malloc(size_t size, void* user_data);
void  br_imgui_free(void* p, void* user_data);

#ifdef __cplusplus
}
#endif

#if defined(BR_DEBUG) && defined(__linux__)
#  define BR_MALLOC(size) malloc(size)
#  define BR_CALLOC calloc
#  define BR_REALLOC realloc
#  define BR_FREE free
#  define BR_IMGUI_MALLOC br_imgui_malloc
#  define BR_IMGUI_FREE br_imgui_free
#  include <assert.h>
//#  define BR_ASSERT(x) assert(x)
#  define BR_ASSERT(x) if (!(x)) *(volatile int*)0; assert(x)
#else
#  include <assert.h>
#  define BR_ASSERT(x) if (!(x)) { *(volatile int*)0; exit(0); } assert(x)
#  define BR_MALLOC malloc
#  define BR_CALLOC calloc
#  define BR_REALLOC realloc
#  define BR_FREE free
#  define BR_IMGUI_MALLOC br_imgui_malloc
#  define BR_IMGUI_FREE br_imgui_free
#endif

#if !defined(BR_HAS_HOTRELOAD)
#  if defined(BR_DEBUG) && defined(IMGUI) && defined(__linux__) && !defined(LIB)
#    define BR_HAS_HOTRELOAD 1
#  else
#    define BR_HAS_HOTRELOAD 0
#  endif
#endif

#if defined(BR_DEBUG) && defined(__linux__)
#  define BR_HAS_SHADER_RELOAD 1
#else
#  define BR_HAS_SHADER_RELOAD 0
#endif

#if defined(_MSC_VER)
#  define ssize_t long long int
#  define TEST_ONLY
#else
#  define TEST_ONLY __attribute__((__unused__))
#endif

#if defined(__cplusplus) &&  __cplusplus >= 201103L
#  define BR_THREAD_LOCAL       thread_local
#elif defined(_MSC_VER)
#  define BR_THREAD_LOCAL       __declspec(thread)
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#  define BR_THREAD_LOCAL       _Thread_local
#elif defined(__GNUC__) && __GNUC__ < 5
#  define BR_THREAD_LOCAL       __thread
#elif defined(__GNUC__)
#  define BR_THREAD_LOCAL       __thread
#endif

