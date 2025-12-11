#if !defined(BR_INCLUDE_BR_STR_H)
#  define BR_INCLUDE_BR_STR_H
#  include "include/br_str_header.h"
#endif

#if defined(BR_STR_IMPLEMENTATION)
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
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

bool br_str_realloc(br_str_t* s, size_t new_cap) {
  if (s->cap == 0) {
    BR_ASSERT(NULL == s->str);
    BR_ASSERT(0 == s->len);
    BR_ASSERT(0 == s->cap);
    s->str = BR_MALLOC(new_cap > 8 ? new_cap : 8);
    if (s->str == NULL) {
      LOGE("Failed to malloc %zu bytes: %s", new_cap, strerror(errno));
      return false;
    }
    s->cap = new_cap > 8 ? (unsigned int)new_cap : 8;
    return true;
  }
  if (s->cap < new_cap) {
    char* newS = BR_REALLOC(s->str, new_cap);
    if (newS != NULL) {
      s->str = newS;
      s->cap = (unsigned int)new_cap;
      return true;
    }
  } else return true;
  LOGE("Failed to realloc %zu bytes: %s", new_cap, strerror(errno));
  return false;
}

static inline void br_str_push_char_unsafe(br_str_t* s, char c) {
  BR_ASSERT(NULL != s->str);
  s->str[s->len++] = c;
}

bool br_str_push_char(br_str_t* s, char c) {
  if (c == '\0') {
    LOGI("Use br_str_push_zero if you wanna NULL terminate this string");
    BR_STACKTRACE();
  }
  if (s->len >= s->cap) if (false == br_str_realloc(s, s->cap * 2)) return false;
  br_str_push_char_unsafe(s, c);
  return true;
}

bool br_str_push_zero(br_str_t* s) {
  if (s->len >= s->cap) if (false == br_str_realloc(s, s->cap * 2)) return false;
  s->str[s->len] = '\0';
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
  while ((long)c / cur > 10) {
    cur *= 10;
  }
  while ((long)cur > 0) {
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

bool br_str_push_strv(br_str_t* s, br_strv_t const c) {
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
  br_str_push_zero(s);
  return true;
}

bool br_str_push_uninitialized(br_str_t* s, unsigned int n) {
  if (s->len + n >= s->cap) {
    size_t pot_size_1 = s->cap * 2, pot_size_2 = s->len + n;
    size_t new_size =  pot_size_2 > pot_size_1 ? pot_size_2 : pot_size_1;
    if (false == br_str_realloc(s, new_size)) return false;
  }

  s->len += n;
  return true;
}

bool br_strv_eq(br_strv_t s1, br_strv_t s2) {
  if (s1.len != s2.len) return false;
  for (unsigned int i = 0; i < s1.len; ++i) if (s1.str[i] != s2.str[i]) return false;
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

void br_str_copy2(br_str_t* out, br_str_t const from) {
  out->len = 0;
  br_str_push_br_str(out, from);
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

char* br_str_to_scrach(br_str_t s) {
  char* scrach = br_scrach_get(s.len + 1);
  br_str_to_c_str1(s, scrach);
  return scrach;
}

char* br_str_move_to_scrach(br_str_t s) {
  char* ret = br_str_to_scrach(s);
  br_str_free(s);
  return ret;
}

bool br_str_replace_one(br_str_t* out, br_strv_t in, br_strv_t to_replace, br_strv_t replace_with) {
  out->len = 0;

  uint32_t new_size = in.len + (replace_with.len > to_replace.len ? replace_with.len - to_replace.len : 0);

  br_str_realloc(out, (size_t)new_size);
  for (uint32_t i = 0; i < in.len; ++i) {
    bool is_match = true;
    if (to_replace.len + i <= in.len) {
      for (uint32_t j = 0; j < to_replace.len; ++j) {
        if (in.str[i + j] != to_replace.str[j]) {
          is_match = false;
          break;
        }
      }
    } else is_match = false;
    if (is_match) {
      br_str_push_strv(out, replace_with);
      br_str_push_strv(out, br_str_sub1(in, i + to_replace.len));
      return true;
    } else {
      br_str_push_char(out, in.str[i]);
    }
  }
  return false;
}

bool br_str_replace_one1(br_str_t* inout, br_strv_t to_replace, br_strv_t replace_with) {
  // TODO: This is retarded to do it like this... Change it!
  br_str_t tmp = { 0 };
  bool replaced = br_str_replace_one(&tmp, br_str_as_view(*inout), to_replace, replace_with);
  br_str_free(*inout);
  *inout = tmp;
  return replaced;
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
  static BR_THREAD_LOCAL char buff[32];
  int len = sprintf(buff, "%.*s", str.len, str.str);
  buff[len] = '\0';
  return atoi(buff);
}

float br_strv_to_float(br_strv_t str) {
  static BR_THREAD_LOCAL char buff[32];
  int len = sprintf(buff, "%.*s", str.len, str.str);
  buff[len] = '\0';
  return (float)atof(buff);
}

br_strv_t br_strv_trim_zeros(const br_strv_t buff) {
  br_strv_t ret = buff;
  bool was_dot = false;
  int i = (int)ret.len - 1;

  if (ret.len >= 2) {
    for (; i >= 0; --i) {
      if (ret.str[i] == '0') --ret.len;
      else if (ret.str[i] == '.') {
        --ret.len;
        was_dot = true;
        break;
      }
      else break;
    }
    if (ret.len == buff.len) return ret;
    if (ret.len == 0) {
      ret.len = 1;
      return ret;
    }
    for (; was_dot == false && i >= 0; --i) {
      was_dot = ret.str[i] == '.';
    }
    if (was_dot == false) ret = buff;
  }
  if (ret.len == 2 && ret.str[0] == '-' && ret.str[1] == '0') {
    ++ret.str;
    --ret.len;
  }
  return ret;
}

br_strv_t br_strv_triml(br_strv_t buff, char c) {
	while (buff.len > 0 && buff.str[0] == c) ++buff.str, --buff.len;
	return buff;
}

br_strv_t br_strv_trimr(br_strv_t buff, char c) {
	while (buff.len > 0 && buff.str[buff.len - 1] == c) --buff.len;
	return buff;
}

br_strv_t br_strv_splitrs(br_strv_t buff, br_strv_t split_strv) {
  for (int i = 0; i < (int)buff.len - (int)split_strv.len; ++i) {
    for (int j = 0; j < (int)split_strv.len; ++j) {
      if (buff.str[i + j] != split_strv.str[j]) goto next;
    }
    uint32_t offset = (uint32_t)i + split_strv.len;
    return BR_STRV(buff.str + offset, buff.len - offset);
next:;
  }
  return BR_STRV(buff.str + buff.len, 0);
}

br_strv_t br_strv_splitr(br_strv_t buff, char splitc) {
  while (buff.len > 0 && buff.str[0] != splitc) {
    ++buff.str;
    --buff.len;
  }

  if (buff.len > 0) {
    ++buff.str;
    --buff.len;
  }
  return buff;
}

br_strv_t br_strv_splitl(br_strv_t buff, char splitc) {
  br_strv_t og = buff;
  while (buff.len > 0 && buff.str[0] != splitc) {
    ++buff.str;
    --buff.len;
  }
  og.len -= buff.len;
  return og;
}

br_strv_t br_strv_any_splitr(br_strv_t buff, int n, char arr[], int* out_index) {
  while (buff.len > 0) {
    for (int j = 0; j < n; ++j) {
      if (buff.str[0] == arr[j]) {
        *out_index = j;
        goto out;
      }
    }
    ++buff.str;
    --buff.len;
  }

out:
  if (buff.len > 0) {
    ++buff.str;
    --buff.len;
  }
  return buff;
}

br_strv_t br_strv_any_rsplitr(br_strv_t buff, int n, char arr[], int* out_index) {
  br_strv_t out = BR_STRV(buff.str + buff.len, 0);
  for (uint32_t i = 0; i < buff.len; ++i) {
    --out.str;
    for (int j = 0; j < n; ++j) {
      if (buff.str[0] == arr[j]) {
        *out_index = j;
        ++out.str;
        return out;
      }
    }
    ++out.len;
  }
  return out;
}

br_strv_t br_strv_skip(br_strv_t buff, char to_skip) {
  while (buff.len > 0 && buff.str[0] == to_skip) {
    ++buff.str;
    --buff.len;
  }
  return buff;
}

bool  br_strv_starts_with(br_strv_t buff, br_strv_t starts_with) {
  if (buff.len < starts_with.len) return false;
  for (uint32_t i = 0; i < starts_with.len; ++i) if (buff.str[i] != starts_with.str[i]) return false;
  return true;
}

int br_strv_count(br_strv_t buff, char ch) {
  int sum = 0;
  for (uint32_t i = 0; i < buff.len; ++i) if (buff.str[i] == ch) ++sum;
  return sum;
}

bool br_strv_match(br_strv_t full, br_strv_t sub) {
  if (full.len < sub.len) return false;
  if (sub.len == 0) return true;
  uint32_t n = sub.len;
  uint32_t i = 0, j = 0;
  for (; i < full.len && j < n;) {
    if (full.str[i] == sub.str[j]) ++i, ++j;
    else ++i;
  }
  return j == n;
}

int br_strv_utf8_add(br_strv_t strv, int cur_pos, int n) {
  for (; n < 0 && cur_pos > 0; ++n) {
    while (cur_pos > 0 && (strv.str[--cur_pos] & 0b11000000) == 0b10000000);
  }
  for (; n > 0 && cur_pos < (int)strv.len; --n) {
    while (cur_pos < (int)strv.len && (strv.str[++cur_pos] & 0b11000000) == 0b10000000);
  }
  return cur_pos;
}


br_u32 br_strv_utf8_pop(br_strv_t* t) {
//  U+0000    U+007F    0yyyzzzz
//  U+0080    U+07FF    110xxxyy  10yyzzzz
//  U+0800    U+FFFF    1110wwww  10xxxxyy  10yyzzzz
//  U+010000  U+10FFFF  11110uvv  10vvwwww  10xxxxyy  10yyzzzz
//  U+uvwxyz
  br_u32 ret = 0;
  if (t->len == 0) return ret;

  br_u32 initial = (br_u32)br_strv_popu(*t);
  ret = initial;
  if (0 == (initial & 0x80)) return ret;

  BR_ASSERTF(initial != 0b10000000, "Decoding from the middle of the UTF8 character");

  BR_ASSERT(t->len > 0);
  br_u32 next = (br_u32)br_strv_popu(*t);
  ret &= 0b00011111;
  ret <<= 6;
  ret |= next & 0b00111111;
  if ((initial & 0b11100000) == 0b11000000) return ret;

  BR_ASSERT(t->len > 0);
  next = (br_u32)br_strv_popu(*t);
  ret &= 0b0000111100000000;
  ret <<= 6;
  ret |= next & 0b00111111;
  if ((initial & 0b11110000) == 0b11100000) return ret;

  BR_ASSERT(t->len > 0);
  next = (br_u32)br_strv_popu(*t);
  ret &= 0b000001111111111111111111;
  ret <<= 6;
  ret |= next & 0b00111111;
  if ((initial & 0b11111000) == 0b11110000) return ret;

  BR_UNREACHABLE("Bad UTF8 character...");
}

br_strv_t br_str_printf(br_str_t* out_str, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  br_str_printfvalloc(out_str, fmt, args);
  va_end(args);

  va_start(args, fmt);
  br_strv_t strv = br_str_printfv(out_str, fmt, args);
  va_end(args);

  return strv;
}

int br_str_printfvalloc(br_str_t* out_str, const char* fmt, va_list args) {
  int n = vsnprintf(NULL, 0, fmt, args);
  BR_ASSERT(n >= 0);
  bool ok_realloc = br_str_realloc(out_str, out_str->len + (br_u32)n + 1);
  if (ok_realloc) return n;
  else return 0;
}

br_strv_t br_str_printfv(br_str_t* out_str, const char* fmt, va_list args) {
  int n = vsnprintf(out_str->str + out_str->len, out_str->cap - out_str->len, fmt, args);
  BR_ASSERT(n >= 0);
  out_str->len += (br_u32)n;
  return br_str_as_view(*out_str);
}

br_strv_t br_scrach_printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  BR_ASSERT(n >= 0);
  char *result = (char*)br_scrach_get((size_t)n + 1);
  va_start(args, fmt);
  vsnprintf(result, (size_t)n + 1, fmt, args);
  va_end(args);
  br_scrach_free();

  return BR_STRV(result, (br_u32)n);
}

int br_scrach_printfvalloc(const char* fmt, va_list args) {
  int n = vsnprintf(NULL, 0, fmt, args);
  BR_ASSERT(n >= 0);
  br_scrach_get((size_t)n + 1);
  br_scrach_free();
  return n;
}

br_strv_t br_scrach_printfv(int n, const char* fmt, va_list args) {
  char *result = (char*)br_scrach_get((size_t)n + 1);
  vsnprintf(result, (size_t)n + 1, fmt, args);
  br_scrach_free();
  return BR_STRV(result, (br_u32)n);
}


static BR_THREAD_LOCAL char*  br_scrach = NULL;
static BR_THREAD_LOCAL size_t br_scrach_cur_cap = 0;
static BR_THREAD_LOCAL bool   br_scrach_is_taken = false;

#if defined(BR_RELEASE)
#  define SCRACH_GET_NAME br_scrach_get
#  define SCRACH_FREE_NAME br_scrach_free
#else
#  define SCRACH_GET_NAME _br_scrach_get
#  define SCRACH_FREE_NAME _br_scrach_free

BR_THREAD_LOCAL const char* br_scrach_alloc_at;
BR_THREAD_LOCAL int br_scrach_alloc_at_line;
BR_THREAD_LOCAL bool is_taken;
bool br_scrach_check_is_taken(void) {
  if (is_taken) {
    LOGE("Trying to get a scrach but already allocated at: %s:%d", br_scrach_alloc_at, br_scrach_alloc_at_line);
    BR_ASSERT(false);
    exit(1);
  }
  return true;
}
#endif
char* SCRACH_GET_NAME(size_t size) {
  BR_ASSERT(false == br_scrach_is_taken);
  br_scrach_is_taken = true;
  if (NULL == br_scrach) {
    size_t new_size = 1024 > size ? 1024 : size;
    br_scrach = BR_MALLOC(new_size);
    br_scrach_cur_cap = new_size;
    return br_scrach;
  }
  if (size > br_scrach_cur_cap) {
    size_t new_size = br_scrach_cur_cap * 2 > size ? br_scrach_cur_cap * 2: size;
    br_scrach = BR_REALLOC(br_scrach, new_size);
    br_scrach_cur_cap = new_size;
  }
  return br_scrach;
}

void SCRACH_FREE_NAME(void) {
  BR_ASSERT(br_scrach_is_taken == true);
  br_scrach_is_taken = false;
}

void br_scrach_finish(void) {
  br_scrach_is_taken = false;
  BR_FREE(br_scrach);
  br_scrach_cur_cap = 0;
}

#endif // defined(BR_STR_IMPLMENTATION)

