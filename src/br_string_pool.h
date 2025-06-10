#pragma once
#include "src/br_pp.h"
#include "src/br_str.h"

typedef int brsp_id_t;

typedef struct brsp_node_t {
  int start_index;
  int len, cap;

  int prev_in_memory, next_in_memory;
} brsp_node_t;

typedef struct brsp_t {
  brsp_node_t* arr;
  int* free_arr;
  int len, cap;
  int free_len, free_next;

  br_str_t pool;
  int first_in_memory, last_in_memory;
} brsp_t;

#if defined(__cplusplus)
extern "C" {
#endif

brsp_id_t brsp_new(brsp_t* sp);
brsp_id_t brsp_new1(brsp_t* sp, int size);
br_strv_t brsp_get(brsp_t sp, brsp_id_t t);
void brsp_set(brsp_t* sp, brsp_id_t t, br_strv_t str);
void brsp_remove(brsp_t* sp, brsp_id_t t);
void brsp_free(brsp_t* sp);

bool brsp_write(FILE* file, brsp_t sp);
bool brsp_read(FILE* file, brsp_t* sp);

#if defined(__cplusplus)
}
#endif
