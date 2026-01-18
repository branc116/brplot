#pragma once
#include "src/br_pp.h"

#define DECLTYPE(VALUE) void

#define br_da_reserve_t(SIZE_T, ARR, N) do { \
  if ((ARR).cap == 0) {                                                                                             \
    BR_ASSERT((ARR).arr == NULL && "Cap is set to null, but arr is not null");                                      \
    (ARR).arr = (DECLTYPE((VALUE))*)BR_MALLOC(sizeof((ARR).arr[0]) * (N));                                          \
    if ((ARR).arr != NULL) {                                                                                        \
      (ARR).cap = (N);                                                                                              \
    }                                                                                                               \
  }                                                                                                                 \
  else if ((ARR).len + (N) < (ARR).cap);                                                                            \
  else {                                                                                                            \
    BR_ASSERT((ARR).arr != NULL);                                                                                   \
    SIZE_T new_cap =  (ARR).len + N;                                                                                \
    DECLTYPE((VALUE))* new_arr = (DECLTYPE((VALUE))*)BR_REALLOC((ARR).arr, (size_t)new_cap * sizeof((ARR).arr[0])); \
    if (new_arr) {                                                                                                  \
      (ARR).arr = new_arr;                                                                                          \
      (ARR).cap = new_cap;                                                                                          \
    } else {                                                                                                        \
      BR_LOGE("Failed to reserve %zu elements in the array.", (size_t)N);                                           \
    }                                                                                                               \
  }                                                                                                                 \
} while(0)                                                                                                          \

#define br_da_reserve(ARR, N) br_da_reserve_t(size_t, ARR, N)

#define br_da_push_t(SIZE_T, ARR, VALUE) do {                                                                       \
  if ((ARR).cap == 0) {                                                                                             \
    BR_ASSERT((ARR).arr == NULL && "Cap is set to null, but arr is not null");                                                                                   \
    (ARR).arr = (DECLTYPE((VALUE))*)BR_MALLOC(sizeof(*(ARR).arr));                                                  \
    if ((ARR).arr != NULL) {                                                                                        \
      (ARR).cap = 1;                                                                                                \
      (ARR).arr[(ARR).len++] = (VALUE);                                                                             \
    }                                                                                                               \
  }                                                                                                                 \
  else if ((ARR).len < (ARR).cap) (ARR).arr[(ARR).len++] = (VALUE);                                                 \
  else {                                                                                                            \
    BR_ASSERT((ARR).arr != NULL);                                                                                   \
    SIZE_T cap_diff = (ARR).cap;                                                                                    \
    bool is_ok = false;                                                                                             \
    while (!is_ok && cap_diff > 0) {                                                                                \
      SIZE_T new_cap =  (ARR).cap + cap_diff;                                                                       \
      DECLTYPE((VALUE))* new_arr = (DECLTYPE((VALUE))*)BR_REALLOC((ARR).arr, (size_t)new_cap * sizeof(*(ARR).arr)); \
      if (new_arr) {                                                                                                \
        (ARR).arr = new_arr;                                                                                        \
        (ARR).cap = new_cap;                                                                                        \
        (ARR).arr[(ARR).len++] = ((VALUE));                                                                         \
        is_ok = true;                                                                                               \
      } else {                                                                                                      \
        cap_diff >>= 1;                                                                                             \
      }                                                                                                             \
    }                                                                                                               \
    BR_ASSERT(is_ok);                                                                                               \
  }                                                                                                                 \
} while(0)                                                                                                          \

#define br_da_push(ARR, VALUE) br_da_push_t(size_t, ARR, VALUE)

#define br_da_push2(ARR, TYPE, ...) br_da_push_t(size_t, ARR, ((TYPE) { __VA_ARGS__ }))

#define br_da_free(ARR) do {                 \
  if ((ARR).arr != NULL) BR_FREE((ARR).arr); \
  (ARR).arr = NULL;                          \
  (ARR).len = (ARR).cap = 0;                 \
} while(0)

#define br_da_remove_n_at_t(T, ARR, N, I) do {                                    \
  BR_ASSERTF(((I) + (N) - 1) < (T)(ARR).len, "Index is out of bounds");           \
  T _i = (T)(I);                                                                  \
  for (; (_i + N) < (T)((ARR).len); ++_i) {                                       \
    (ARR).arr[_i] = (ARR).arr[_i + 1];                                            \
  }                                                                               \
  (ARR).len-=(N);                                                                 \
} while(0)

#define br_da_remove_n_at(ARR, N, I) br_da_remove_n_at_t(size_t, ARR, N, I)
#define br_da_remove_at(ARR, I) br_da_remove_n_at(ARR, 1, I)
#define br_da_remove_at_t(T, ARR, I) br_da_remove_n_at_t(T, ARR, 1, I)

#define br_da_remove(ARR, V) do {                                     \
  size_t _i = 0;                                                      \
  for (; _i < (size_t)(ARR).len; ) {                                  \
    if ((ARR).arr[_i] == (V)) (ARR).arr[_i] = (ARR).arr[--(ARR).len]; \
    else ++_i;                                                        \
  }                                                                   \
} while(0)

#define br_da_remove_feeld(ARR, FEELD, V) do {                        \
  size_t _i = 0;                                                      \
  for (; _i < (size_t)(ARR).len; ) {                                  \
    if ((ARR).arr[_i].FEELD == (V)) (ARR).arr[_i] = (ARR).arr[--(ARR).len]; \
    else ++_i;                                                        \
  }                                                                   \
} while(0)

#define br_da_find(ARR, FEELD, V, INDEX) do {                         \
  for (; (INDEX) < (ARR).len; ++(INDEX)) {                            \
    if ((ARR).arr[(INDEX)].FEELD == (V)) break;                       \
  }                                                                   \
} while(0)

#define br_da_contains_t(T, ARR, V, CONTAINS) do { \
  for (T _i = 0; _i < (ARR).len; ++_i) {           \
    if ((ARR).arr[_i] == V) {                      \
      CONTAINS = true;                             \
      break;                                       \
    }                                              \
  }                                                \
} while(0)

#define br_da_contains_feeld_t(T, ARR, FEELD, V, CONTAINS) do { \
  for (T _i = 0; _i < (ARR).len; ++_i) {           \
    if ((ARR).arr[_i].FEELD == V) {                      \
      CONTAINS = true;                             \
      break;                                       \
    }                                              \
  }                                                \
} while(0)

#define br_da_copy(DES, SRC) do { \
  BR_ASSERT(sizeof((DES).arr[0]) == sizeof((SRC).arr[0])); \
  if ((SRC).len == 0) { \
    (DES).arr = NULL; \
    (DES).len = (DES).cap = 0; \
  } else { \
    if ((DES).cap < (SRC).len) { \
      (DES).arr = BR_REALLOC((DES).arr, sizeof((SRC).arr[0]) * (size_t)(SRC).len); \
      BR_ASSERT((DES).arr); \
    } \
    memcpy(&(DES).arr[0], &(SRC).arr[0], sizeof((SRC).arr[0]) * (size_t)(SRC).len); \
    (DES).len = (SRC).len; \
    (DES).cap = (SRC).cap; \
  } \
} while (0)

#define br_da_top(ARR) br_da_get(ARR, (ARR).len - 1)

#define BR_DA_CHECK 0xdeadbeefdeadbeefULL
#define BR_MAX_LEN (1ULL<<31ULL)

#define br_da_write_header(FILE, ARR) do { \
  if (1 != BR_FWRITE(&(br_u64) { BR_DA_CHECK }, sizeof(br_u64), 1, FILE)) BR_ERRORE("Failed to write a array first check"); \
  if (1 != BR_FWRITE(&(ARR), sizeof((ARR)), 1, FILE))                     BR_ERRORE("Failed to write a array header"); \
  if (1 != BR_FWRITE(&(br_u64) { BR_DA_CHECK }, sizeof(br_u64), 1, FILE)) BR_ERRORE("Failed to write a array mid check"); \
} while (0)

#define br_da_write(FILE, ARR) do { \
  size_t BR_L = (size_t)(ARR).len; \
  br_da_write_header(FILE, ARR); \
  if (BR_L > 0) { \
    if (BR_L != BR_FWRITE((ARR).arr, sizeof((ARR).arr[0]), BR_L, FILE))   BR_ERRORE("Failed to write a data"); \
  } \
  if (1 != BR_FWRITE(&(br_u64) { BR_DA_CHECK }, sizeof(br_u64), 1, FILE)) BR_ERRORE("Failed to write a array final check"); \
} while(0)

#define br_da_read_check(FILE) do { \
  br_u64 tmp = 0; \
  if (1   != BR_FREAD(&tmp, sizeof(tmp), 1, FILE)) BR_ERRORE("Failed to read array first check"); \
  if (tmp != BR_DA_CHECK) { \
    fseek(FILE, SEEK_CUR, -64 - 8); \
    br_u8 tmp_buff[2*64 + 8]; \
    BR_FREAD(&tmp_buff, 2*64 + 8, 1, FILE); \
    for (int BR__i = 0; BR__i < 2*64 + 8; ++BR__i) { \
      if (BR__i == 64) printf("["); \
      printf("0x%02X", tmp_buff[BR__i]); \
      if (BR__i == 64 + 7) printf("] "); \
      else if ((BR__i + 1) % 8 == 0) printf("   "); \
      else printf(" "); \
      if ((BR__i + 1) % 16 == 0) printf("\n"); \
    } \
    BR_ERROR("First check don't match.."); \
  } \
} while(0)

#define br_da_read_head(FILE, ARR) do { \
  (ARR).arr = NULL; \
  br_da_read_check(FILE); \
  if (1ULL != BR_FREAD(&(ARR), sizeof((ARR)), 1, FILE)) BR_ERRORE("Failed to write a data len"); \
  if ((size_t)(ARR).len > BR_MAX_LEN)                   BR_ERROR("Array too big %zu (%llu is max)..", (size_t)(ARR).len, BR_MAX_LEN); \
  br_da_read_check(FILE); \
} while(0)

#define br_da_read(FILE, ARR) do { \
  br_da_read_head(FILE, ARR); \
  size_t BR_L = (size_t)(ARR).len; \
  \
  if (BR_L > 0) { \
    if (NULL == ((ARR).arr = BR_MALLOC(sizeof((ARR).arr[0]) * BR_L))) BR_ERRORE("Failed to allocate %zu bytes for array", BR_L); \
    (ARR).cap = (ARR).len; \
    if (BR_L != BR_FREAD((ARR).arr, sizeof((ARR).arr[0]), BR_L, FILE)) BR_ERRORE("Failed to read a data"); \
    br_da_read_check(FILE); \
  } else { \
    (ARR).arr = NULL; \
    (ARR).cap = 0; \
    br_da_read_check(FILE); \
  } \
} while(0)


#define br_da_contains(ARR, V, CONTAINS) br_da_contains_t(size_t, ARR, V, CONTAINS)

#if defined(BR_RELEASE)
#define br_da_get(ARR, INDEX) (ARR).arr[(INDEX)]
#define br_da_getp(ARR, INDEX) (&(ARR).arr[(INDEX)])
#define br_da_set(ARR, INDEX, VALUE) (ARR).arr[(INDEX)] = (VALUE)
#else
static inline void ___br_function_call_asset_id_ok(ssize_t arr_len, ssize_t acc_index, const char* file_location, int line_number) {
  if (arr_len <= acc_index || acc_index < 0) {
    LOGE("Out of bounds index to an array at %s:%d", file_location, line_number);
    LOGE("Index: %zd, Arr len: %zd", acc_index, arr_len);
  }
  BR_ASSERT(arr_len > acc_index && acc_index >= 0);
}
#define br_da_get(ARR, INDEX) (___br_function_call_asset_id_ok((ssize_t)((ARR).len), (ssize_t)(INDEX), __FILE__, __LINE__), \
  (ARR).arr[(INDEX)])
#define br_da_getp(ARR, INDEX) (___br_function_call_asset_id_ok((ssize_t)((ARR).len), (ssize_t)(INDEX), __FILE__, __LINE__), \
  &(ARR).arr[(INDEX)])
#define br_da_set(ARR, INDEX, VALUE) do { \
  ___br_function_call_asset_id_ok((ssize_t)((ARR).len), (ssize_t)(INDEX), __FILE__, __LINE__), \
  (ARR).arr[(INDEX)] = (VALUE); \
} while(0)
#endif
  
