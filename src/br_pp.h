#pragma once
#include <stddef.h>

#ifdef PLATFORM_WEB
#include <emscripten.h>
#define BR_API EMSCRIPTEN_KEEPALIVE
#else
#define BR_API
#endif

#ifndef RELEASE

#ifndef LINUX
#ifdef UNIT_TEST
#undef UNIT_TEST
// IT don't work on windows....
#endif
#endif

#endif


// This is the size of buffer used to transfer points from cpu to gpu.
#define PTOM_COUNT (1<<10)

//TODO: Do something with this...
#define GRAPH_LEFT_PAD 500

#ifdef LINUX
#include "pthread.h"
#define LOCK_T pthread_mutex_t
#endif

#ifdef LOCK_T
#define LOCK(x) LOCK_T x;
#else
#define LOCK(x)
#endif

#define LOG(...)
#define LOGI(...) fprintf(stderr, __VA_ARGS__)

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

#if !defined(RELEASE) && defined(LINUX)
#define BR_MALLOC(size) malloc(size)
#define BR_CALLOC calloc
#define BR_REALLOC realloc
#define BR_FREE free
#define BR_IMGUI_MALLOC br_imgui_malloc
#define BR_IMGUI_FREE br_imgui_free
#include "signal.h"
#define BR_ASSERT(x) if (!x) raise(SIGABRT)
#else
#include <stdlib.h>
#define BR_ASSERT(x) assert(x)
#define BR_MALLOC malloc
#define BR_CALLOC calloc
#define BR_REALLOC realloc
#define BR_FREE free
#define BR_IMGUI_MALLOC br_imgui_malloc
#define BR_IMGUI_FREE br_imgui_free
#endif
