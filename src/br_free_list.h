#if !defined(BR_ASSERT)
#  define BR_ASSERT(EXPR)
#endif

#if !defined(BR_ASSERTF)
#  define BR_ASSERTF(EXPR, FMT, ...)
#endif

#if !defined(BR_MALLOC)
#  define BR_MALLOC(SIZE) malloc(SIZE)
#endif

#if !defined(BR_REALLOC)
#  define BR_REALLOC(PTR, NEW_SIZE) realloc(PTR, NEW_SIZE)
#endif

#if !defined(BR_FREE)
#  define BR_FREE(PTR) free(PTR)
#endif

#if !defined(BR_LOGE)
#  define BR_LOGE(...)
#endif

#if !defined(BR_THREAD_LOCAL)
#  define BR_THREAD_LOCAL       _Thread_local
#endif

BR_THREAD_LOCAL extern int brfl__ret_handle;
#define brfl_push(FL, VALUE) \
  (brfl__ret_handle = brfl_push_internal_get_handle((void**)&((FL).arr), &((FL).free_arr), &((FL).len), &((FL).cap), &((FL).free_len), &((FL).free_next), sizeof((FL).arr[0]), __FILE__, __LINE__), \
   (FL).arr[brfl__ret_handle] = (VALUE), \
   brfl__ret_handle)

#define brfl_remove(FL, HANDLE) do { \
  int fn = (FL).free_next; \
  (FL).free_next = HANDLE; \
  (FL).free_arr[HANDLE] = fn; \
  --(FL).free_len; \
} while (0)

#define brfl_free(FL) do { \
  br_da_free(FL); \
  BR_FREE((FL).free_arr); \
  (FL).free_arr = NULL; \
  (FL).free_len = 0; \
  (FL).free_next = -1; \
} while (0)

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

BR_THREAD_LOCAL int brfl__ret_handle;
int brfl_push_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line) {
  (void)file; (void)line;
  int* free_arr = *free_arrp;
  int len = *lenp;
  int cap = *capp;
  int free_next = *free_nextp;
  if (cap == 0) {
    BR_ASSERTF(*arrp == NULL, "[%s:%d] Cap is set to null, but arr is not null", file, line);
    *arrp = BR_MALLOC(value_size);
    *free_arrp = BR_MALLOC(sizeof(free_arr[0]));
    *free_nextp = -1;
    if (*arrp != NULL && *free_arrp != NULL) {
      *capp = 1;
      **free_arrp = -1;
      *free_lenp = 1;
      *lenp = 1;
      return 0;
    } else {
      BR_LOGE("[%s:%d] Failed to allocate memory for free list arrays.\n"
           "        arr=%p, free_arr=%p, value_size=%zu", file, line, *arrp, (void*)*free_arrp, value_size);
      return -1;
    }
  } else if (free_next != -1) {
    int ret = free_next;
    BR_ASSERTF(ret < len, "[%s:%d] Free list is corupet, next free should be %d, but array only has %d elements.", file, line, ret, len);
    free_next = free_arr[ret];
    free_arr[ret] = -1;
    *free_nextp = free_next;
    ++(*free_lenp);
    return ret;
  } else if (len < cap) {
    free_arr[len] = -1;
    ++(*lenp);
    ++(*free_lenp);
    return len;
  } else {
    BR_ASSERTF(*arrp != NULL, "[%s:%d] Array capacity is %d, but array pointr is NULL", file, line, cap);
    int cap_diff = cap;
    bool is_ok = false;
    while (!is_ok && cap_diff > 0) {
      int new_cap = cap + cap_diff;
      void* new_arr = BR_REALLOC(*arrp, (size_t)new_cap * value_size);
      int* new_free_arr = BR_REALLOC(*free_arrp, (size_t)new_cap * sizeof(int));
      if (new_arr && new_free_arr) {
        *arrp = new_arr;
        *free_arrp = new_free_arr;
        *capp = new_cap;
        (*free_arrp)[len] = -1;
        ++(*lenp);
        is_ok = true;
      } else {
        if (new_arr) *arrp = new_arr;
        if (new_free_arr) *free_arrp = new_free_arr;
        cap_diff >>= 1;
      }
    }
    if (is_ok) {
      ++(*free_lenp);
      return *lenp - 1;
    }
    else {
      BR_ASSERTF(is_ok, "[%s:%d] Failed to reallocate array.\n"
                     "        Most likely out of memory.\n"
                     "        Cur capacity: %d, element size: %zu",
                     file, line, cap, value_size);
      return -1;
    }
  }
}
