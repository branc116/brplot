#if !defined(BR_INCLUDE_BR_STR_HEADER_H)
#define BR_INCLUDE_BR_STR_HEADER_H
#include "src/br_pp.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
#  define BR_LITERAL(TYPE) TYPE
#else
#  define BR_LITERAL(TYPE) (TYPE)
#endif

#define br_str_invalidata(s) s = BR_LITERAL(br_str_t){0}
#define br_str_push_literal(s, literal) br_str_push_strv(s, br_strv_from_literal(literal))
#define br_strv_from_literal(literal) (BR_LITERAL(br_strv_t) { literal, sizeof(literal) - 1 })

#define br_str_sub(s, start, new_length) ((br_strv_t) { .str = s.str + (start), .len = (new_length) })
#define br_str_sub1(s, start) ((br_strv_t) { .str = s.str + (start), .len = ((start) >= s.len ? 0 : s.len - (start)) })
#define br_str_as_view(S) ((br_strv_t) { .str = (S).str, .len = (S).len })
#define br_strv_pop(S)  (--(S).len, *(S).str++)
#define br_strv_popu(S) (--(S).len, *(uint8_t*)((S).str++))
#define br_str_free(S) if (NULL != (S).str) BR_FREE((S).str);

#define BR_STRV(STR, LEN) ((br_strv_t) { .str = (STR), .len = (LEN) })
#define BR_STRL(STR) ((br_strv_t) { .str = (STR), .len = (uint32_t)(sizeof((STR)) - 1) })

#define BR_STRV_FOREACH_UTF8(SV, CHAR) for (br_u32 CHAR = br_strv_utf8_pop(&(SV)); CHAR; CHAR = br_strv_utf8_pop(&(SV)))


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
bool       br_str_realloc(br_str_t* s, size_t new_cap);
bool       br_str_push_char(br_str_t* s, char c);
bool       br_str_push_zero(br_str_t* s);
bool       br_str_push_int(br_str_t* s, int c);
bool       br_str_push_float1(br_str_t* s, float c, int decimals);
bool       br_str_push_float(br_str_t* s, float c);
bool       br_str_push_br_str(br_str_t* s, br_str_t const c);
bool       br_str_push_strv(br_str_t* s, br_strv_t const c);
bool       br_str_push_c_str(br_str_t* s, char const* c);
bool       br_str_push_uninitialized(br_str_t* s, unsigned int n);
char*      br_str_to_c_str(br_str_t s);
char*      br_str_move_to_c_str(br_str_t* s);
br_str_t   br_str_copy(br_str_t s);
void       br_str_copy2(br_str_t* out, br_str_t from);
br_str_t   br_str_from_c_str(const char* str);
void       br_str_to_c_str1(br_str_t s, char* out_s);
char*      br_str_to_scrach(br_str_t s);
char*      br_str_move_to_scrach(br_str_t s);
bool       br_str_replace_one(br_str_t* out, br_strv_t in, br_strv_t to_replace, br_strv_t replace_with);
bool       br_str_replace_one1(br_str_t* inout, br_strv_t to_replace, br_strv_t replace_with);

bool       br_strv_eq(br_strv_t s1, br_strv_t s2);
char*      br_strv_to_scrach(br_strv_t s);
char*      br_strv_to_c_str(br_strv_t s);
void       br_strv_to_c_str1(br_strv_t s, char* out_s);
br_strv_t  br_strv_from_c_str(const char* s);
int        br_strv_to_int(br_strv_t str);
float      br_strv_to_float(br_strv_t str);
br_strv_t  br_strv_trim_zeros(br_strv_t buff);
br_strv_t  br_strv_triml(br_strv_t buff, char c);
br_strv_t  br_strv_trimr(br_strv_t buff, char c);
br_strv_t  br_strv_splitrs(br_strv_t buff, br_strv_t split_strv);
br_strv_t  br_strv_splitr(br_strv_t buff, char splitc);
br_strv_t  br_strv_splitl(br_strv_t buff, char splitc);
br_strv_t  br_strv_any_splitr(br_strv_t buff, int n, char arr[], int* out_index);
br_strv_t  br_strv_any_rsplitr(br_strv_t buff, int n, char arr[], int* out_index);
br_strv_t  br_strv_skip(br_strv_t buff, char to_skip);
bool       br_strv_starts_with(br_strv_t buff, br_strv_t starts_with);
int        br_strv_count(br_strv_t buff, char ch);
bool       br_strv_match(br_strv_t full, br_strv_t sub);
int        br_strv_utf8_add(br_strv_t, int cur_pos, int n);
br_u32     br_strv_utf8_pop(br_strv_t* t);


br_strv_t br_str_printf(br_str_t* out_str, const char* fmt, ...);
int br_str_printfvalloc(br_str_t* out_str, const char* fmt, va_list args);
br_strv_t br_str_printfv(br_str_t* out_str, const char* fmt, va_list args);
br_strv_t  br_scrach_printf(const char* fmt, ...);
int br_scrach_printfvalloc(const char* fmt, va_list args);
br_strv_t br_scrach_printfv(int n, const char* fmt, va_list args);

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
void br_scrach_finish(void);

#endif
