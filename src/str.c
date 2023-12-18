#include "src/br_plot.h"
#include "string.h"
#include "src/misc/tests.h"

br_str_t br_str_malloc(size_t size) {
  br_str_t br = {
    .str = BR_MALLOC(size),
    .len = 0,
    .cap = size
  };
  memset(br.str, 0, size);
  return br;
}

void br_str_free(br_str_t str) {
  BR_FREE(str.str);
}

void br_str_realloc(br_str_t* s, size_t new_cap) {
  s->str = BR_REALLOC(s->str, new_cap);
  s->cap = new_cap;
}

static inline void br_str_push_char_unsafe(br_str_t* s, char c) {
  s->str[s->len++] = c;
}

void br_str_push_char(br_str_t* s, char c) {
  if (s->len >= s->cap) br_str_realloc(s, s->cap * 2);
  br_str_push_char_unsafe(s, c);
}

void br_str_push_int(br_str_t* s, int c) {
  if (c == 0) {
    br_str_push_char(s, '0');
    return;
  }
  if (c < 0) {
    br_str_push_char(s, '-');
    c *= -1;
  }
  long cur = 1;
  while((long)c / cur > 10) {
    cur *= 10;
  }
  while((long)cur > 0) {
    br_str_push_char(s, '0' + (c / cur)); 
    c %= cur;
    cur /= 10;
  }
}

void br_str_push_float1(br_str_t* s, float c, int decimals) {
  if (c < 0.f) {
    br_str_push_char(s, '-');
    c *= -1;
  }
  int a = (int)c;
  br_str_push_int(s, a);
  c -= a;
  if (c > 0) {
    br_str_push_char(s, '.');
  }
  while (c > 0 && decimals--) {
    c *= 10;
    a = (int)c;
    br_str_push_char(s, '0' + a); 
    c -= a;
  }
  while (decimals--) {
    br_str_push_char(s, '0'); 
  }
}

void br_str_push_float(br_str_t* s, float c) {
  br_str_push_float1(s, c, 5);
}

void br_str_push_c_str(br_str_t* s, char const* c) {
  size_t size = strlen(c);
  if (size == 0) return;
  if (s->len + size > s->cap) {
    size_t pot_size_1 = s->cap * 2, pot_size_2 = s->len + size;
    size_t new_size =  pot_size_2 > pot_size_1 ? pot_size_2 : pot_size_1;
    br_str_realloc(s, new_size);
  }
  for (size_t i = 0; i < size; ++i) {
    br_str_push_char_unsafe(s, c[i]);
  }
}

char* br_str_to_c_str(br_str_t* s) {
  char* out_s = BR_MALLOC(s->len + 1);
  memcpy(s->str, out_s, s->len);
  out_s[s->len] = 0;
  return out_s;
}

void br_str_to_c_str1(br_str_t const* s, char* out_s) {
  memcpy(out_s, s->str, s->len);
  out_s[s->len] = 0;
}

br_strv_t br_str_sub(br_str_t* s, size_t start, size_t len) {
  return (br_strv_t) {
    .str = s->str + start,
    .len = len,
  };
}

br_strv_t br_strv_sub(br_strv_t* s, size_t start, size_t len) {
  return (br_strv_t) {
    .str = s->str + start,
    .len = len,
  };
}

char* br_strv_to_c_str(br_strv_t* s) {
  char* out_s = BR_MALLOC(s->len + 1);
  memcpy(s->str, out_s, s->len);
  out_s[s->len] = 0;
  return out_s;
}

void br_strv_to_c_str1(br_strv_t const* s, char* out_s) {
  memcpy(out_s, s->str, s->len);
  out_s[s->len] = 0;
}

TEST_CASE(str_tests) {
  char c[128];
  br_str_t br = br_str_malloc(2);
  br_str_push_char(&br, 'a'); br_str_to_c_str1(&br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a", c);
  br_str_push_int(&br, 69); br_str_to_c_str1(&br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69", c);
  br_str_push_int(&br, -69); br_str_to_c_str1(&br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-69", c);
  br_str_push_int(&br, 0); br_str_to_c_str1(&br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-690", c);
  br_str_push_c_str(&br, "nice"); br_str_to_c_str1(&br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-690nice", c);
  br_str_push_int(&br, 12345678); br_str_to_c_str1(&br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-690nice12345678", c);
  br_str_free(br);
}

