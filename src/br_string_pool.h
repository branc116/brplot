#pragma once
#include "src/br_pp.h"
#include "src/br_str.h"

typedef int brsp_id_t;

typedef struct brsp_node_t {
  int start_index;
  int len, cap;
} brsp_node_t;

typedef struct brsp_t {
  brsp_node_t* arr;
  int* free_arr;
  int len, cap;
  int free_len, free_next;

  br_str_t pool;
} brsp_t;

#if defined(__cplusplus)
extern "C" {
#endif

brsp_id_t brsp_new(brsp_t* sp);
brsp_id_t brsp_new1(brsp_t* sp, int size);
br_strv_t brsp_get(brsp_t sp, brsp_id_t t);
br_strv_t brsp_try_get(brsp_t sp, brsp_id_t t);
bool brsp_is_in(brsp_t sp, brsp_id_t t);
void brsp_set(brsp_t* sp, brsp_id_t t, br_strv_t str);
void brsp_insert_char(brsp_t* sp, brsp_id_t t, int at, unsigned char c);
int brsp_insert_unicode(brsp_t* sp, brsp_id_t t, int at, uint32_t u);
void brsp_insert_char_at_end(brsp_t* sp, brsp_id_t id, char c);
void brsp_insert_strv_at_end(brsp_t* sp, brsp_id_t id, br_strv_t sv);
void brsp_zero(brsp_t* sp, brsp_id_t id);
void brsp_clear(brsp_t* sp, brsp_id_t id);
char brsp_remove_char_end(brsp_t* sp, brsp_id_t id);
int  brsp_remove_utf8_after(brsp_t* sp, brsp_id_t id, int position);
void brsp_remove(brsp_t* sp, brsp_id_t t);
brsp_id_t brsp_copy(brsp_t* sp, brsp_id_t id);

bool brsp_compress(brsp_t* sp, float factor, int slack);
void brsp_free(brsp_t* sp);

bool brsp_write(FILE* file, brsp_t* sp);
bool brsp_read(FILE* file, brsp_t* sp);

#if defined(__cplusplus)
}
#endif
