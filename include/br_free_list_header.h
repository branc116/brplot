#if !defined(BR_INCLUDE_BR_FREE_LIST_HEADER_H)
#define BR_INCLUDE_BR_FREE_LIST_HEADER_H
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

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wpedantic"
#endif

#define brfl_push(FL, VALUE) brfl_push_internal((void**)&((FL).arr), &((FL).free_arr), &((FL).len), &((FL).cap), &((FL).free_len), &((FL).free_next), sizeof((FL).arr[0]), &(VALUE), __FILE__, __LINE__);
#define brfl_push_end(FL, VALUE) brfl_push_end_internal((void**)&((FL).arr), &((FL).free_arr), &((FL).len), &((FL).cap), &((FL).free_len), &((FL).free_next), sizeof((FL).arr[0]), &(VALUE), __FILE__, __LINE__);

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#define brfl_remove(FL, HANDLE) do { \
  int fn = (FL).free_next; \
  (FL).free_next = HANDLE; \
  (FL).free_arr[HANDLE] = fn == -1 ? -2 : fn; \
  --(FL).free_len; \
} while (0)

#define brfl_free(FL) do { \
  br_da_free(FL); \
  if ((FL).free_arr) BR_FREE((FL).free_arr); \
  (FL).free_arr = NULL; \
  (FL).free_len = 0; \
  (FL).free_next = -1; \
} while (0)

#define brfl_foreach(INDEX, FL) for (int INDEX = brfl_next_taken((FL).free_arr, (FL).len, -1); INDEX < (FL).len; INDEX = brfl_next_taken((FL).free_arr, (FL).len, INDEX))
#define brfl_foreachv(VALUE, FL) for (int _i = brfl_next_taken((FL).free_arr, (FL).len, -1); \
    (_i < (FL).len ? (VALUE = (FL).arr[_i], _i < (FL).len) : false); \
    _i = brfl_next_taken((FL).free_arr, (FL).len, _i))
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
    if (NULL == (FL).free_arr) {                                                                                     \
      ERROR = 1;                                                                                                     \
      BR_LOGE("Failed to allocate free list free arr ( %zu bytes, %d elements ): %s", size, (FL).cap, strerror(errno)); \
      brfl_free(FL);                                                                                                 \
      memset(&(FL), 0, sizeof(FL));                                                                                  \
      break;                                                                                                         \
    }                                                                                                                \
  }                                                                                                                  \
  if ((size_t)(FL).cap != (n_read = BR_FREAD((FL).arr, sizeof((FL).arr[0]), (size_t)(FL).cap, (FILE)))) {            \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Failed to read %d free list elements, read %zu: %s", (FL).cap, n_read, strerror(errno));                \
    brfl_free(FL);                                                                                                   \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  if ((size_t)(FL).cap != (n_read = BR_FREAD((FL).free_arr, sizeof((FL).free_arr[0]), (size_t)(FL).cap, (FILE)))) {  \
    ERROR = 1;                                                                                                       \
    BR_LOGE("Failed to read %d free list free elements, read %zu: %s", (FL).cap, n_read, strerror(errno));           \
    brfl_free(FL);                                                                                                   \
    memset(&(FL), 0, sizeof(FL));                                                                                    \
    break;                                                                                                           \
  }                                                                                                                  \
  for (int __i = 0; __i < (FL).len; ++__i) {                                                                         \
    if ((FL).free_arr[__i] == __i) {                                                                                 \
      ERROR = 1;                                                                                                     \
      BR_LOGE("Free arr element can't have itself as an value, something bad %d.", __i);                             \
      brfl_free(FL);                                                                                                 \
      memset(&(FL), 0, sizeof(FL));                                                                                  \
      break;                                                                                                         \
    }                                                                                                                \
  }                                                                                                                  \
  if (false == brfl_is_valid((FL).free_arr, (FL).len, (FL).free_next, (FL).free_len)) {                              \
      ERROR = 1;                                                                                                     \
      BR_LOGE("Free arr is not valid");                                                                              \
      brfl_free(FL);                                                                                                 \
      memset(&(FL), 0, sizeof(FL));                                                                                  \
      break;                                                                                                         \
  }                                                                                                                  \
} while(0)

#define brfl_is_free(FL, i) ((FL).free_arr[i] != -1)

extern BR_THREAD_LOCAL int brfl__ret_handle;
int brfl_push_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line);
int brfl_push_end_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line);
int brfl_push_internal(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, void* value, const char* file, int line);
int brfl_push_end_internal(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, void* value, const char* file, int line);
int brfl_next_taken(int const* free_arr, int len, int index);
bool brfl_is_valid(int const* free_arr, int len, int free_next, int free_len);

#endif // !defined(BR_INCLUDE_BR_FREE_LIST_HEADER_H)
