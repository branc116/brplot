#pragma once
#include "misc/tests.h"
#include "br_plot.h"
#include "assert.h"

#define br_da_push(ARR, VALUE) do {                                    \
  if (ARR.cap == 0) {                                                  \
    ARR.arr = BR_MALLOC(sizeof(*ARR.arr));                             \
    if (ARR.arr != NULL) {                                             \
      ARR.cap = 1;                                                     \
      ARR.arr[ARR.len++] = (VALUE);                                    \
    }                                                                  \
  }                                                                    \
  else if (ARR.len < ARR.cap) ARR.arr[ARR.len++] = (VALUE);            \
  else {                                                               \
    size_t cap_diff = ARR.cap;                                         \
    bool is_ok = false;                                                \
    while (!is_ok && cap_diff > 0) {                                   \
      size_t new_cap =  ARR.cap + cap_diff;                            \
      void* new_arr = BR_REALLOC(ARR.arr, new_cap * sizeof(*ARR.arr)); \
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

typedef struct {
  int* arr;
  size_t len, cap;
} test_arr;

TEST_CASE(da) {
  test_arr a = {0};
  br_da_push(a, 1);
  br_da_push(a, 2);
  br_da_push(a, 3);
  br_da_remove(a, 2);
  br_da_remove_at(a, 2);
  br_da_remove_n_at(a, 1, 0);
  br_da_free(a);
}

