#pragma once
#include <stddef.h>

#ifdef PLATFORM_WEB
#  include <emscripten.h>
#  define BR_API EMSCRIPTEN_KEEPALIVE
#else
#  define BR_API
#endif

#if !defined(RELEASE) && !defined(__linux__) && defined(UNIT_TEST)
   // IT don't work on windows....
#  undef UNIT_TEST
#endif


// This is the size of buffer used to transfer points from cpu to gpu.
#define PTOM_COUNT (1<<10)

//TODO: Do something with this...
#define GRAPH_LEFT_PAD 500

#ifdef __linux__
#  define LOCK_T pthread_mutex_t
#endif

#if defined(LOCK_T)
#  define LOCK(x) LOCK_T x;
#else
#  define LOCK(x)
#endif

#if !defined(BR_DISABLE_LOG)
#  define LOG(...)
#  define LOGI(...) fprintf(stderr, __VA_ARGS__)
#  define LOGE(text) fprintf(stderr, "ERROR: [%s:%d]" text, __FILE__, __LINE__)
#  define LOGEF(format, ...) fprintf(stderr, "ERROR: [%s:%d]" format, __FILE__, __LINE__,  __VA_ARGS__)
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

#if !defined(RELEASE) && defined(__linux__)
#  define BR_MALLOC(size) malloc(size)
#  define BR_CALLOC calloc
#  define BR_REALLOC realloc
#  define BR_FREE free
#  define BR_IMGUI_MALLOC br_imgui_malloc
#  define BR_IMGUI_FREE br_imgui_free
#  include <assert.h>
#  define BR_ASSERT(x) assert(x)
#else
#  include <assert.h>
#  define BR_ASSERT(x) assert(x)
#  define BR_MALLOC malloc
#  define BR_CALLOC calloc
#  define BR_REALLOC realloc
#  define BR_FREE free
#  define BR_IMGUI_MALLOC br_imgui_malloc
#  define BR_IMGUI_FREE br_imgui_free
#endif

#if !defined(RELEASE) && defined(IMGUI) && defined(__linux__)
#  define BR_HAS_HOTRELOAD 1
#else
#  define BR_HAS_HOTRELOAD 0
#endif

#if !defined(RELEASE) && defined(__linux__)
#  define BR_HAS_SHADER_RELOAD 1
#else
#  define BR_HAS_SHADER_RELOAD 0
#endif

#if defined(_MSC_VER)
#  define ssize_t long long int
#endif
