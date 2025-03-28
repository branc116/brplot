#pragma once
#include "src/br_pp.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#define BR_LITERAL(TYPE) TYPE
#else
#define BR_LITERAL(TYPE) (TYPE)
#endif



#define br_str_invalidata(s) s = BR_LITERAL(br_str_t){0}
#define br_str_push_literal(s, literal) br_str_push_br_strv(s, br_strv_from_literal(literal))
#define br_strv_from_literal(literal) (BR_LITERAL(br_strv_t) { literal, sizeof(literal) - 1 })

#ifdef __cplusplus
#else
#define br_str_sub(s, start, new_length) ((br_strv_t) { .str = s.str + (start), .len = (new_length) })
#define br_str_sub1(s, start) ((br_strv_t) { .str = s.str + (start), .len = s.len - (start) })
#define br_str_as_view(s) ((br_strv_t) { .str = s.str, .len = s.len })

#define br_strv_sub(s, start, new_length) ((br_strv_t) { .str = s.str + (start), .len = (new_length) })
#define br_strv_sub1(s, start) ((br_strv_t) { .str = s.str + (start), .len = s.len - (start) })
#endif

#define BR_STRV(STR, LEN) ((br_strv_t) { .str = (STR), .len = (LEN) })
#define BR_STRL(STR) ((br_strv_t) { .str = (STR), .len = (uint32_t)(sizeof((STR)) - 1) })


typedef struct br_str_t {
  char* str;
  unsigned int len;
  unsigned int cap;
} br_str_t;

typedef struct br_strv_t {
  const char* str;
  uint32_t len;
} br_strv_t;

br_str_t   br_str_malloc(size_t size);
void       br_str_free(br_str_t str);
bool       br_str_realloc(br_str_t* s, size_t new_cap);
bool       br_str_push_char(br_str_t* s, char c);
bool       br_str_push_int(br_str_t* s, int c);
bool       br_str_push_float1(br_str_t* s, float c, int decimals);
bool       br_str_push_float(br_str_t* s, float c);
bool       br_str_push_br_str(br_str_t* s, br_str_t const c);
bool       br_str_push_br_strv(br_str_t* s, br_strv_t const c);
bool       br_str_push_c_str(br_str_t* s, char const* c);
bool       br_str_push_uninitialized(br_str_t* s, unsigned int n);
char*      br_str_to_c_str(br_str_t s);
char*      br_str_move_to_c_str(br_str_t* s);
br_str_t   br_str_copy(br_str_t s);
br_str_t   br_str_from_c_str(const char* str);
void       br_str_to_c_str1(br_str_t s, char* out_s);
char*      br_str_to_scrach(br_str_t s);
char*      br_str_move_to_scrach(br_str_t s);

bool       br_strv_eq(br_strv_t s1, br_strv_t s2);
char*      br_strv_to_scrach(br_strv_t s);
char*      br_strv_to_c_str(br_strv_t s);
void       br_strv_to_c_str1(br_strv_t s, char* out_s);
br_strv_t  br_strv_from_c_str(const char* s);
int        br_strv_to_int(br_strv_t str);
br_strv_t  br_strv_trim_zeros(br_strv_t buff);
br_strv_t  br_strv_splitrs(br_strv_t buff, br_strv_t split_strv);
br_strv_t  br_strv_splitr(br_strv_t buff, char splitc);
br_strv_t  br_strv_splitl(br_strv_t buff, char splitc);
br_strv_t  br_strv_skip(br_strv_t buff, char to_skip);
bool       br_strv_starts_with(br_strv_t buff, br_strv_t starts_with);
int        br_strv_count(br_strv_t buff, char ch);

#if defined(BR_RELEASE)
char*      br_scrach_get(size_t len);
void       br_scrach_free(void);
#else
char*      _br_scrach_get(size_t len);
void       _br_scrach_free(void);
extern BR_THREAD_LOCAL const char* br_scrach_alloc_at;
extern BR_THREAD_LOCAL int br_scrach_alloc_at_line;
extern BR_THREAD_LOCAL bool is_taken;
bool br_scrach_check_is_taken(void);
#define br_scrach_get(len) ( \
    br_scrach_check_is_taken(), \
    br_scrach_alloc_at = __FILE__, \
    br_scrach_alloc_at_line = __LINE__, \
    is_taken = true, \
    _br_scrach_get(len))
#define br_scrach_free() do { \
  is_taken = false; \
  _br_scrach_free(); \
} while(0)

#endif

#ifdef __cplusplus
}
#endif
