#include "src/br_threads.h"

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
void br_thread_start(void *(*function)(void *), void* args) {
  pthread_t thread;
  pthread_create(&thread, &(pthread_attr_t){0}, function, args);
}
#elif defined(_WIN32) || defined(__CYGWIN__)
DWORD br_thread_start(DWORD (WINAPI * function)(void *), void* args) {
  DWORD thread_id;
  CreateThread(NULL, 0, function, args, 0, &thread_id);
  return thread_id;
}
#elif defined(__EMSCRIPTEN__)
void br_thread_start(void *(*function)(void *), void* args) {
  (void)function; (void)args;
  printf("Can't create thread on the web...\n");
}
#else
#  error "Unsupported Platform"
#endif

