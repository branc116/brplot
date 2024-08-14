#include "src/br_plot.h"
#include "src/br_pp.h"
#include "src/br_str.h"
#include "raylib.h"

#include <string.h>

br_str_t br_str_malloc(size_t size) {
  br_str_t br = {
    .str = BR_MALLOC(size),
    .len = 0,
    .cap = (unsigned int)size
  };
  if (br.str != NULL) memset(br.str, 0, size);
  else                br.cap = 0;
  return br;
}

void br_str_free(br_str_t str) {
  BR_FREE(str.str);
}

bool br_str_realloc(br_str_t* s, size_t new_cap) {
  if (s->cap == 0) {
    BR_ASSERT(NULL == s->str);
    BR_ASSERT(0 == s->len);
    BR_ASSERT(0 == s->cap);
    s->str = BR_MALLOC(new_cap > 8 ? new_cap : 8);
    if (s->str == NULL) return false;
    s->cap = new_cap > 8 ? (unsigned int)new_cap : 8;
    return true;
  }
  if (s->cap < new_cap) {
    char * newS = BR_REALLOC(s->str, new_cap);
    if (newS != NULL) {
      s->str = newS;
      s->cap = (unsigned int)new_cap;
      return true;
    }
  }
  return false;
}

static inline void br_str_push_char_unsafe(br_str_t* s, char c) {
  BR_ASSERT(NULL != s->str);
  s->str[s->len++] = c;
}

bool br_str_push_char(br_str_t* s, char c) {
  if (s->len >= s->cap) if (false == br_str_realloc(s, s->cap * 2)) return false;
  br_str_push_char_unsafe(s, c);
  return true;
}

bool br_str_push_int(br_str_t* s, int c) {
  if (c == 0) {
    return br_str_push_char(s, '0');
  }
  if (c < 0) {
    if (false == br_str_push_char(s, '-')) return false;
    c *= -1;
  }
  long cur = 1;
  while((long)c / cur > 10) {
    cur *= 10;
  }
  while((long)cur > 0) {
    if (false == br_str_push_char(s, '0' + (char)((long)c / cur))) return false;
    c = (int)((long)c % cur);
    cur /= 10;
  }
  return true;
}

bool br_str_push_float1(br_str_t* s, float c, int decimals) {
  if (c < 0.f) {
    if (false == br_str_push_char(s, '-')) return false;
    c *= -1.f;
  }
  int a = (int)c;
  if (false == br_str_push_int(s, a)) return false;
  c -= (float)a;
  if (c > 0.f) {
    if (false == br_str_push_char(s, '.')) return false;
  }
  while (c > 0.f && decimals-- >= 0) {
    c *= 10.f;
    a = (int)c;
    if (false == br_str_push_char(s, '0' + (char)a)) return false;
    c -= (float)a;
  }
  while (decimals-- >= 0) {
    if (false == br_str_push_char(s, '0')) return false;
  }
  return true;
}

bool br_str_push_float(br_str_t* s, float c) {
  return br_str_push_float1(s, c, 5);
}

bool br_str_push_br_str(br_str_t* s, br_str_t const c) {
  size_t size = c.len;
  if (size == 0) return true;
  if (s->len + size > s->cap) {
    size_t pot_size_1 = s->cap * 2, pot_size_2 = s->len + size;
    size_t new_size =  pot_size_2 > pot_size_1 ? pot_size_2 : pot_size_1;
    if (false == br_str_realloc(s, new_size)) return false;
  }
  for (size_t i = 0; i < size; ++i) {
    br_str_push_char_unsafe(s, c.str[i]);
  }
  return true;
}

bool br_str_push_br_strv(br_str_t* s, br_strv_t const c) {
  size_t size = c.len;
  if (size == 0) return true;
  if (s->len + size > s->cap) {
    size_t pot_size_1 = s->cap * 2, pot_size_2 = s->len + size;
    size_t new_size =  pot_size_2 > pot_size_1 ? pot_size_2 : pot_size_1;
    if (false == br_str_realloc(s, new_size)) return false;
  }
  for (size_t i = 0; i < size; ++i) {
    br_str_push_char_unsafe(s, c.str[i]);
  }
  return true;
}

bool br_str_push_c_str(br_str_t* s, char const* c) {
  size_t size = strlen(c);
  if (size == 0) return true;
  if (s->len + size > s->cap) {
    size_t pot_size_1 = s->cap * 2, pot_size_2 = s->len + size;
    size_t new_size =  pot_size_2 > pot_size_1 ? pot_size_2 : pot_size_1;
    if (false == br_str_realloc(s, new_size)) return false;
  }
  for (size_t i = 0; i < size; ++i) {
    br_str_push_char_unsafe(s, c[i]);
  }
  return true;
}

bool br_strv_eq(br_strv_t s1, br_strv_t s2) {
  if (s1.len != s2.len) return false;
  for (unsigned int i = 0; i < s1.len; ++i)
    if (s1.str[i] != s2.str[i]) return false;
  return true;
}

char* br_str_to_c_str(const br_str_t s) {
  char* out_s = BR_MALLOC(s.len + 1);
  if (out_s == NULL) return NULL;
  memcpy(out_s, s.str, s.len);
  out_s[s.len] = 0;
  return out_s;
}

char* br_str_move_to_c_str(br_str_t* s) {
  br_str_push_char(s, '\0');
  char* ret = s->str;
  *s = (br_str_t){0};
  return ret;
}

br_str_t br_str_copy(br_str_t s) {
  br_str_t r = { .str = BR_MALLOC(s.len), .len = s.len, .cap = s.len };
  if (r.str == NULL) {
    r.cap = 0;
    return r;
  }
  memcpy(r.str, s.str, s.len);
  return r;
}

br_str_t br_str_from_c_str(const char* str) {
  unsigned int len = (unsigned int)strlen(str);
  br_str_t r = { .str = BR_MALLOC(len), .len = len, .cap = len };
  if (r.str == NULL) {
    r.cap = 0;
    r.len = 0;
    return r;
  }
  memcpy(r.str, str, len);
  return r;
}

void br_str_to_c_str1(br_str_t s, char* out_s) {
  memcpy(out_s, s.str, s.len);
  out_s[s.len] = 0;
}

char* br_strv_to_scrach(br_strv_t s) {
  char* scrach = br_scrach_get(s.len + 1);
  br_strv_to_c_str1(s, scrach);
  return scrach;
}

char* br_strv_to_c_str(br_strv_t s) {
  char* out_s = BR_MALLOC(s.len + 1);
  if (out_s == NULL) return NULL;
  memcpy(out_s, s.str, s.len);
  out_s[s.len] = 0;
  return out_s;
}

void br_strv_to_c_str1(br_strv_t s, char* out_s) {
  memcpy(out_s, s.str, s.len);
  out_s[s.len] = 0;
}

br_strv_t br_strv_from_c_str(const char* s) {
  return (br_strv_t) { .str = s, .len = (unsigned int)strlen((s)) };
}

int br_strv_to_int(br_strv_t str) {
  static char buff[32];
  int len = sprintf(buff, "%.*s", str.len, str.str);
  buff[len] = '\0';
  return atoi(buff);
}

static RL_THREAD_LOCAL char*  scrach = NULL;
static RL_THREAD_LOCAL size_t scrach_cur_cap = 0;
static RL_THREAD_LOCAL bool   scrach_is_taken = false;

char* br_scrach_get(size_t size) {
  BR_ASSERT(false == scrach_is_taken);
  scrach_is_taken = true;
  if (NULL == scrach) {
    size_t new_size = 1024 > size ? 1024 : size;
    scrach = BR_MALLOC(new_size);
    scrach_cur_cap = new_size;
    return scrach;
  }
  if (size > scrach_cur_cap) {
    size_t new_size = scrach_cur_cap * 2 > size ? scrach_cur_cap * 2: size;
    scrach = BR_REALLOC(scrach, new_size);
    scrach_cur_cap = new_size;
  }
  return scrach;
}

void br_scrach_free(void) {
  BR_ASSERT(scrach_is_taken == true);
  scrach_is_taken = false;
}

#ifndef _MSC_VER
#include "external/tests.h"
TEST_CASE(str_tests) {
  char c[128];
  br_str_t br = br_str_malloc(2);
  br_str_push_char(&br, 'a'); br_str_to_c_str1(br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a", c);
  br_str_push_int(&br, 69); br_str_to_c_str1(br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69", c);
  br_str_push_int(&br, -69); br_str_to_c_str1(br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-69", c);
  br_str_push_int(&br, 0); br_str_to_c_str1(br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-690", c);
  br_str_push_c_str(&br, "nice"); br_str_to_c_str1(br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-690nice", c);
  br_str_push_int(&br, 12345678); br_str_to_c_str1(br, c);
  TEST_EQUAL('a', br.str[0]);
  TEST_STREQUAL("a69-690nice12345678", c);
  br_str_free(br);
}
#endif
