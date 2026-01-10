#include "src/br_pp.h"
#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__EMSCRIPTEN__)
#  define BR_THREAD_RET_TYPE void*
#  define BR_THREAD_FUNC
void br_thread_start(BR_THREAD_RET_TYPE (*function)(void*), void* args);
#else
#  define BR_THREAD_RET_TYPE DWORD
#  define BR_THREAD_FUNC WINAPI
unsigned long br_thread_start(BR_THREAD_RET_TYPE (WINAPI * function)(void*), void* args);
#endif
