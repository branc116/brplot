#pragma once
#include "src/br_da.h"
/* for size_t */
#include <stdlib.h>
#include <stdbool.h>

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

#if !defined(BR_FREAD)
#  define BR_FREAD fread
#endif

#if !defined(BR_FWRITE)
#  define BR_FWRITE fwrite
#endif

#define brfl_push(FL, VALUE) \
  (brfl__ret_handle = brfl_push_internal_get_handle((void**)&((FL).arr), &((FL).free_arr), &((FL).len), &((FL).cap), &((FL).free_len), &((FL).free_next), sizeof((FL).arr[0]), __FILE__, __LINE__), \
   (FL).arr[brfl__ret_handle] = (VALUE), \
   brfl__ret_handle)

#define brfl_push_end(FL, VALUE) \
  (brfl__ret_handle = brfl_push_end_internal_get_handle((void**)&((FL).arr), &((FL).free_arr), &((FL).len), &((FL).cap), &((FL).free_len), &((FL).free_next), sizeof((FL).arr[0]), __FILE__, __LINE__), \
   (FL).arr[brfl__ret_handle] = (VALUE), \
   brfl__ret_handle)

#define brfl_remove(FL, HANDLE) do { \
  int fn = (FL).free_next; \
  (FL).free_next = HANDLE; \
  (FL).free_arr[HANDLE] = fn == -1 ? -2 : fn; \
  --(FL).free_len; \
} while (0)

#define brfl_free(FL) do { \
  br_da_free(FL); \
  BR_FREE((FL).free_arr); \
  (FL).free_arr = NULL; \
  (FL).free_len = 0; \
  (FL).free_next = -1; \
} while (0)

#define brfl_foreach(INDEX, FL) for (int INDEX = brfl_next_taken((FL).free_arr, (FL).len, -1); INDEX < (FL).len; INDEX = brfl_next_taken((FL).free_arr, (FL).len, INDEX))
#define brfl_foreach_free(INDEX, FL) for (int INDEX = (FL).free_next, n = 0; n < (FL).len && INDEX > -1 && INDEX < (FL).len; ++n, INDEX = (FL).free_arr[INDEX])

#define brfl_write(FILE, FL, ERROR) do {                                                                              \
  ERROR = 0;                                                                                                          \
  size_t n_write = 0;                                                                                                 \
  if (1 != (n_write = BR_FWRITE(&(FL), sizeof(FL), 1, (FILE)))) {                                                     \
      BR_LOGE("Failed to write 1 free list to file, wrote %zu: %s", n_write, strerror(errno));                        \
      ERROR = 1;                                                                                                      \
      break;                                                                                                          \
  };                                                                                                                  \
  if ((FL).len == 0) break;                                                                                           \
  if ((FL).len != (int)(n_write = BR_FWRITE((FL).arr, sizeof((FL).arr[0]), (size_t)(FL).len, (FILE)))) {              \
    BR_LOGE("Failed to write %d free list elements to file, wrote %zu: %s", (FL).len, n_write, strerror(errno));      \
    ERROR = 1;                                                                                                        \
    break;                                                                                                            \
  }                                                                                                                   \
  if ((size_t)(FL).len != (n_write = BR_FWRITE((FL).free_arr, sizeof((FL).free_arr[0]), (size_t)(FL).len, (FILE)))) { \
    BR_LOGE("Failed to write %d free list free elements to file, wrote %zu: %s", (FL).len, n_write, strerror(errno)); \
    ERROR = 1;                                                                                                        \
    break;                                                                                                            \
  }                                                                                                                   \
} while (0)                                                                                                           \

#define brfl_read(FILE, FL, ERROR) do {                                                                              \
  ERROR = 0;                                                                                                         \
  size_t n_read = 0;                                                                                                 \
  if (1 != (n_read = BR_FREAD(&(FL), sizeof(FL), 1, FILE))) {                                                        \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Failed to read 1 free list from file, read %zu: %s", n_read, strerror(errno));                          \
    break;                                                                                                           \
  }                                                                                                                  \
  if ((FL).len > (FL).cap) {                                                                                         \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Free list len ( %d ) is bigger that cap ( %d )", (FL).len, (FL).cap);                                   \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  if ((FL).free_len > (FL).len) {                                                                                    \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Free list free len ( %d ) is bigger that len ( %d )", (FL).free_len, (FL).len);                         \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  if (0xFFFF < (FL).len) {                                                                                           \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Upgrade to premium version of list to support lists bigger that 0xFFFF. Requesting %d", (FL).len);      \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  if ((FL).len == 0) {                                                                                               \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  if ((FL).len < 0) {                                                                                                \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Free list len ( %d ) is negative...", (FL).len);                                                        \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  if ((FL).free_len < 0) {                                                                                           \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Free list free_len ( %d ) is negative...", (FL).free_len);                                              \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  (FL).cap = (FL).len;                                                                                               \
  {                                                                                                                  \
    size_t size = sizeof((FL).arr[0]) * (size_t)(FL).cap;                                                            \
    (FL).arr = BR_MALLOC(size);                                                                                      \
    if (NULL == (FL).arr) {                                                                                          \
      ERROR = 1;                                                                                                     \
      BR_LOGE("Failed to allocate free list arr ( %zu bytes, %d elements ): %s", size, (FL).cap, strerror(errno));   \
      memset(&(FL), 0, sizeof(FL));                                                                                  \
      break;                                                                                                         \
    }                                                                                                                \
  }                                                                                                                  \
  {                                                                                                                  \
    size_t size = sizeof((FL).free_arr[0]) * (size_t)(FL).cap;                                                       \
    (FL).free_arr = BR_MALLOC(size);                                                                                 \
    if (NULL == (FL).arr) {                                                                                          \
      ERROR = 1;                                                                                                     \
      BR_LOGE("Failed to allocate free list free arr ( %zu bytes, %d elements ): %s", size, (FL).cap, strerror(errno)); \
      BR_FREE((FL).arr);                                                                                             \
      memset(&(FL), 0, sizeof(FL));                                                                                  \
      break;                                                                                                         \
    }                                                                                                                \
  }                                                                                                                  \
  if ((size_t)(FL).cap != (n_read = BR_FREAD((FL).arr, sizeof((FL).arr[0]), (size_t)(FL).cap, (FILE)))) {            \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Failed to read %d free list elements, read %zu: %s", (FL).cap, n_read, strerror(errno));                \
    BR_FREE((FL).arr);                                                                                               \
    BR_FREE((FL).free_arr);                                                                                          \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  if ((size_t)(FL).cap != (n_read = BR_FREAD((FL).free_arr, sizeof((FL).free_arr[0]), (size_t)(FL).cap, (FILE)))) {  \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Failed to read %d free list free elements, read %zu: %s", (FL).cap, n_read, strerror(errno));           \
    BR_FREE((FL).arr);                                                                                               \
    BR_FREE((FL).free_arr);                                                                                          \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  for (int __i = 0; __i < (FL).len; ++__i) {                                                                         \
    if ((FL).free_arr[__i] == __i) {                                                                                 \
      ERROR = 1;                                                                                                     \
      BR_LOGE("Free arr element can't have itself as an value, something bad %d.", __i);                             \
      BR_FREE((FL).arr);                                                                                             \
      BR_FREE((FL).free_arr);                                                                                        \
      memset(&(FL), 0, sizeof(FL));                                                                                  \
      break;                                                                                                         \
    }                                                                                                                \
  }                                                                                                                  \
  if (false == brfl_is_valid((FL).free_arr, (FL).len, (FL).free_next, (FL).free_len)) {                              \
      ERROR = 1;                                                                                                     \
      BR_LOGE("Free arr is not valid");                                                                              \
      BR_FREE((FL).arr);                                                                                             \
      BR_FREE((FL).free_arr);                                                                                        \
      memset(&(FL), 0, sizeof(FL));                                                                                  \
      break;                                                                                                         \
  }                                                                                                                  \
} while(0)

#define brfl_is_free(FL, i) ((FL).free_arr[i] != -1)

extern BR_THREAD_LOCAL int brfl__ret_handle;
int brfl_push_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line);
int brfl_push_end_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line);
int brfl_next_taken(int const* free_arr, int len, int index);
bool brfl_is_valid(int const* free_arr, int len, int free_next, int free_len);

#if defined(BRFL_IMPLEMENTATION)
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
    BR_ASSERTF(ret >= 0, "[%s:%d] Free list is corupet, ret is %d", file, line, ret);
    free_next = free_arr[ret];
    free_arr[ret] = -1;
    *free_nextp = free_next == -2 ? -1 : free_next;
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
int brfl_push_end_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line) {
  (void)file; (void)line;
  int* free_arr = *free_arrp;
  int len = *lenp;
  int cap = *capp;
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

int brfl_next_taken(int const* free_arr, int len, int index) {
  for (++index; index < len; ++index) if (free_arr[index] == -1) return index;
  return index;
}

bool brfl_is_valid(int const* free_arr, int len, int free_next, int free_len) {
  struct { int* arr; size_t len, cap; } visited = { 0 };
  bool success = true;
  int i = 0;
  bool contains = false;

  if (free_next == -1)  {
    for (i = 0; i < len; ++i) if (free_arr[i] != -1) BR_ERROR("Invalid");
    if (len == free_len) goto done;
    else BR_ERROR("Invalid len: %d, free_len: %d", len, free_len);
  }
  if (free_next < 0)    BR_ERROR("invalid free_next: %d", free_next);
  if (free_next >= len) BR_ERROR("invalid free_next: %d", free_next);
  br_da_push(visited, free_next);
  for (i = 0; i < len; ++i) {
    int nf = free_arr[i];
    if (nf >= 0) {
      if (nf >= len) BR_ERROR("bad nf: %d", nf);
      br_da_contains(visited, nf, contains);
      if (contains) BR_ERROR("%d is freed twice", nf);
      br_da_push(visited, nf);
    } else if (nf < -2) {
      BR_ERROR("%d is not valid", nf);
    } else if (nf == -1) {
      --free_len;
    }
  }
  if (free_len != 0) BR_ERROR("free_len: %d", free_len);
  goto done;

error:
  success = false;

done:
//printf("len: %d, free_next: %d, free_len: %d, arr: ", len, free_next, free_len);
//for (int i = 0; i < len; ++i) printf("%d ", free_arr[i]);
//printf("\n");
  br_da_free(visited);
  return success;
}

#endif
