
#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#include <pthread.h>
void br_thread_start(void *(*function)(void *), void* args) {
  pthread_t thread;
  pthread_create(&thread, &(pthread_attr_t){0}, function, args);
}
#elif defined(_WIN32) || defined(__CYGWIN__)
#define _WIN32_LEAN_AND_MEAN 1
#define NOGDI 1
#define NOUSER 1
#define LPMSG void*
#include <windows.h>
#include <processthreadsapi.h>

void br_thread_start(long unsigned int(*function)(void *), void* args) {
  CreateThread(NULL, 0, function, args, 0, NULL);
}
#elif defined(__EMSCRIPTEN__)
#include <stdio.h>
void br_thread_start(void *(*function)(void *), void* args) {
  printf("Can't create thread on the web...\n");
}
#else
#  error "Unsupported Platform"
#endif

