#pragma once
#include "br_plot.h"
#include "assert.h"

#ifdef __cplusplus
#define DECLTYPE(VALUE) decltype(VALUE)
#else
#define DECLTYPE(VALUE) void
#endif
  

#define br_da_push_t(SIZE_T, ARR, VALUE) do {                          \
  if (ARR.cap == 0) {                                                  \
    ARR.arr = (DECLTYPE(VALUE)*)BR_MALLOC(sizeof(*ARR.arr));                             \
    if (ARR.arr != NULL) {                                             \
      ARR.cap = 1;                                                     \
      ARR.arr[ARR.len++] = (VALUE);                                    \
    }                                                                  \
  }                                                                    \
  else if (ARR.len < ARR.cap) ARR.arr[ARR.len++] = (VALUE);            \
  else {                                                               \
    SIZE_T cap_diff = ARR.cap;                                         \
    bool is_ok = false;                                                \
    while (!is_ok && cap_diff > 0) {                                   \
      SIZE_T new_cap =  ARR.cap + cap_diff;                            \
      DECLTYPE(VALUE)* new_arr = (DECLTYPE(VALUE)*)BR_REALLOC(ARR.arr, (size_t)new_cap * sizeof(*ARR.arr)); \
      if (new_arr) {                                                   \
        ARR.arr = new_arr;                                             \
        ARR.cap = new_cap;                                             \
        ARR.arr[ARR.len++] = (VALUE);                                  \
      } else {                                                         \
        cap_diff >>= 1;                                                \
      }                                                                \
    }                                                                  \
  }                                                                    \
} while(0)                                                             \

#define br_da_push(ARR, VALUE) br_da_push_t(size_t, ARR, VALUE)

#define br_da_free(ARR) do { \
  BR_FREE(ARR.arr);          \
  ARR.arr = NULL;            \
  ARR.len = ARR.cap = 0;     \
} while(0)

#define br_da_remove_n_at(ARR, N, I) do {                                             \
  assert("Index is out of bounds" && (I) > 0 && ((I) + (N) - 1) < (size_t)(ARR).len); \
  size_t i = (size_t)(I);                                                             \
  for (; (i + N) < (size_t)((ARR).len); ++i) {                                        \
    (ARR).arr[i] = (ARR).arr[i + 1];                                                  \
  }                                                                                   \
  --(ARR).len;                                                                        \
} while(0)

#define br_da_remove_at(ARR, I) br_da_remove_n_at(ARR, 1, I)

#define br_da_remove(ARR, V) do {            \
  size_t i = 0;                              \
  for (; i < (size_t)(ARR).len; ++i) {       \
    if ((ARR).arr[i] == (V)) {               \
      (ARR).arr[i] = (ARR).arr[--(ARR).len]; \
    }                                        \
  }                                          \
} while(0)

