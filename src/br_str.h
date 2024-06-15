#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_str_t {
  char* str;
  unsigned int len;
  unsigned int cap;
} br_str_t;

typedef struct br_strv_t {
  const char* str;
  unsigned int len;
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
#define    br_str_push_literal(s, literal) br_str_push_br_strv(s, br_strv_from_literal(literal))
char*      br_str_to_c_str(br_str_t s);
char*      br_str_move_to_c_str(br_str_t* s);
br_str_t   br_str_copy(br_str_t s);
br_str_t   br_str_from_c_str(const char* str);
void       br_str_to_c_str1(br_str_t s, char* out_s);
#define    br_str_invalidata(s) s = CLITERAL(br_str_t){0}
bool       br_strv_eq(br_strv_t s1, br_strv_t s2);
#define    br_str_sub(s, start, new_length) ((br_strv_t) { .str = s.str + (start), .len = (new_length) })
#define    br_str_sub1(s, start) ((br_strv_t) { .str = s.str + (start), .len = s.len - (start) })
#define    br_str_as_view(s) ((br_strv_t) { .str = s.str, .len = s.len })
#define    br_strv_sub(s, start, new_length) ((br_strv_t) { .str = s.str + (start), .len = (new_length) })
#define    br_strv_sub1(s, start) ((br_strv_t) { .str = s.str + (start), .len = s.len - (start) })
#define    br_strv_from_literal(literal) ((br_strv_t) { .str = literal, .len = sizeof(literal) - 1 })
char*      br_strv_to_c_str(br_strv_t s);
void       br_strv_to_c_str1(br_strv_t s, char* out_s);
br_strv_t  br_strv_from_c_str(const char* s);

#ifdef __cplusplus
}
#endif
