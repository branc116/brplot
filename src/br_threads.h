#if defined(__cplusplus)
extern "C" {
#endif

void br_thread_start(void* (*function)(void*), void* args);

#if defined(__cplusplus)
}
#endif
