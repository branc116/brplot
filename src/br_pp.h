#pragma once

// --------------------------------------------- DEFINES ---------------------------------------------

#if !defined(BR_DISABLE_LOG)
#  define LOG(...)
#  define LOGI(format, ...) fprintf(stderr, "[INFO][" __FILE__ ":%d] " format "\n", __LINE__, ##__VA_ARGS__)
#  define LOGW(format, ...) fprintf(stderr, "[WARNING][" __FILE__ ":%d] " format "\n", __LINE__, ##__VA_ARGS__)
#  define LOGE(format, ...) fprintf(stderr, "[ERROR][" __FILE__ ":%d] " format "\n", __LINE__, ##__VA_ARGS__)
#else
#  define LOG(...)
#  define LOGI(...)
#  define LOGW(...)
#  define LOGE(...)
#endif

#define LOGF(format, ...) do { \
  fprintf(stderr, "[FATAL][" __FILE__ ":%d] " format "\n", __LINE__, ##__VA_ARGS__); \
  abort(); \
} while(0)

#define BR_LOG_GL_ERROR(ERROR) do { \
  if (0 != (ERROR)) { \
    LOGF("GL Error: %d", (ERROR)); \
  } \
} while(0)

#define BR_MALLOC(size) malloc(size)
#define BR_CALLOC calloc
#define BR_REALLOC realloc
#define BR_FREE free
#define BR_ASSERT(x) do { \
  if (!(x)) { \
     LOGE("ASSERT FAILED: `" #x "`"); \
     BR_BREAKPOINT(); \
     LOGF("Exiting"); \
  } \
} while (0)
#define BR_ASSERTF(x, fmt, ...) do { \
   if (!(x)) { \
     LOGE("ASSERT FAILED: `" #x "`" fmt, ##__VA_ARGS__); \
     BR_BREAKPOINT(); \
     LOGF("Exiting"); \
   } \
} while (0)

#if defined(BRPLOT_IMPLEMENTATION)
#  define BR_LIB
#endif

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

#if defined(BR_MUSL)
#elif defined(__linux__)
#  define LOCK_T pthread_mutex_t
#endif

#if defined(LOCK_T)
#  define LOCK(x) LOCK_T x;
#else
#  define LOCK(x)
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#if defined(BR_DEBUG)
#  if defined(_MSC_VER)
#    define BR_BREAKPOINT() __debugbreak()
#  else
#    define BR_BREAKPOINT() __builtin_trap()
#  endif
#else
# define BR_BREAKPOINT()
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

#if !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS // Windows bullshit
#endif
#if !defined(_GNU_SOURCE)
#  define _GNU_SOURCE // Linux bullshit
#endif

#include "external/Tracy/tracy/TracyC.h"

