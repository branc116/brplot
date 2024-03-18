#pragma once

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

#ifdef WINDOWS
typedef int64_t ssize_t;
#endif
