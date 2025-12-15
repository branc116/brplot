#if !defined(BR_INCLUDE_BR_STRING_POOL_HEADER_H)
#define BR_INCLUDE_BR_STRING_POOL_HEADER_H
#include "src/br_pp.h"
#include "src/br_da.h"
#include "include/br_str_header.h"
#include "include/br_free_list_header.h"

#if !defined(BR_ASSERTF)
#  define BR_ASSERTF(EXPR, FMT, ...)
#endif

#if !defined(BR_MALLOC)
#  define BR_MALLOC(SIZE) malloc(SIZE)
#endif

#if !defined(BR_FREE)
#  define BR_FREE(PTR) free(PTR)
#endif

#if !defined(BR_LOGE)
#  define BR_LOGE(...)
#endif

#if !defined(BR_FREAD)
#  define BR_FREAD fread
#endif

#if !defined(BR_FILE)
#  define BR_FILE FILE
#endif

#if !defined(BR_FWRITE)
#  define BR_FWRITE fwrite
#endif

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

brsp_id_t brsp_new(brsp_t* sp);
brsp_id_t brsp_new1(brsp_t* sp, int size);
brsp_id_t brsp_push(brsp_t* sp, br_strv_t sv);
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

bool brsp_write(BR_FILE* file, brsp_t* sp);
bool brsp_read(BR_FILE* file, brsp_t* sp);

#endif // !defined(BR_INCLUDE_BR_STRING_POOL_HEADER_H)
