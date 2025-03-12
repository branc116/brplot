#if defined(__cplusplus)
extern "C" {
#endif

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) || defined(__EMSCRIPTEN__)
#  define BR_THREAD_RET_TYPE void*
#else
#  define BR_THREAD_RET_TYPE long unsigned int
#endif

void br_thread_start(BR_THREAD_RET_TYPE (*function)(void*), void* args);

#if defined(__cplusplus)
}
#endif
