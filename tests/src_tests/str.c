#if !defined(BR_DEBUG)
#  define BR_DEBUG
#endif
#include "src/br_pp.h"
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"
#define BR_STR_IMPLEMENTATION
#include "src/br_str.h"
#include "src/br_test.h"

void str_tests(void) {
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

void str_trim(void) {
  br_strv_t z = br_strv_from_literal("00");
  z = br_strv_trim_zeros(z);
  TEST_EQUAL(z.len, 1);
  TEST_EQUAL(z.str[0], '0');

  z = br_strv_from_literal("10000000");
  z = br_strv_trim_zeros(z);
  TEST_EQUAL(z.len, 8);
  TEST_EQUAL(z.str[0], '1');

  z = br_strv_from_literal("100.0000");
  z = br_strv_trim_zeros(z);
  TEST_EQUAL(z.len, 3);
  TEST_EQUAL(z.str[0], '1');

  z = br_strv_from_literal("100.0001");
  z = br_strv_trim_zeros(z);
  TEST_EQUAL(z.len, 8);
  TEST_EQUAL(z.str[0], '1');

  z = br_strv_from_literal("100.00010");
  z = br_strv_trim_zeros(z);
  TEST_EQUAL(z.len, 8);
  TEST_EQUAL(z.str[0], '1');
}

void str_replace(void) {
  br_str_t out = { 0 };

  br_strv_t in = BR_STRL("fooo");
  br_str_replace_one(&out, in, BR_STRL("foo"), BR_STRL("bar"));
  TEST_STRNEQUAL(out.str, "bar", 3);

  in = BR_STRL("foo");
  br_str_replace_one(&out, in, BR_STRL("foo"), BR_STRL("bar"));
  TEST_STRNEQUAL(out.str, "bar", 3);

  in = BR_STRL("fo");
  br_str_replace_one(&out, in, BR_STRL("foo"), BR_STRL("bar"));
  TEST_STRNEQUAL(out.str, "fo", 2);

  in = BR_STRL("foo");
  br_str_replace_one(&out, in, BR_STRL("foo"), BR_STRL("barbaz"));
  TEST_STRNEQUAL(out.str, "barbaz", 6);

  in = BR_STRL("  foo");
  br_str_replace_one(&out, in, BR_STRL("foo"), BR_STRL("barbaz"));
  TEST_STRNEQUAL(out.str, "  barbaz", 8);

  in = BR_STRL("  foo  ");
  br_str_replace_one(&out, in, BR_STRL("foo"), BR_STRL("barbaz"));
  TEST_STRNEQUAL(out.str, "  barbaz  ", 10);

  in = BR_STRL("");
  br_str_replace_one(&out, in, BR_STRL("foo"), BR_STRL(""));
  TEST_EQUAL(out.len, 0);

  br_str_free(out);
}

void str_replace1(void) {
  br_str_t inout = { 0 };
  br_str_push_c_str(&inout, "foo");

  br_str_replace_one1(&inout, BR_STRL("foo"), BR_STRL("bar"));
  TEST_STRNEQUAL(inout.str, "bar", 3);

  inout.len = 0;
  br_str_push_c_str(&inout, "fo");
  br_str_replace_one1(&inout, BR_STRL("foo"), BR_STRL("bar"));
  TEST_STRNEQUAL(inout.str, "fo", 2);

  inout.len = 0;
  br_str_push_c_str(&inout, "foo");
  br_str_replace_one1(&inout, BR_STRL("foo"), BR_STRL("barbaz"));
  TEST_STRNEQUAL(inout.str, "barbaz", 6);

  inout.len = 0;
  br_str_push_c_str(&inout, "  foo");
  br_str_replace_one1(&inout, BR_STRL("foo"), BR_STRL("barbaz"));
  TEST_STRNEQUAL(inout.str, "  barbaz", 8);

  inout.len = 0;
  br_str_push_c_str(&inout, "  foo  ");
  br_str_replace_one1(&inout, BR_STRL("foo"), BR_STRL("barbaz"));
  TEST_STRNEQUAL(inout.str, "  barbaz  ", 10);

  inout.len = 0;
  br_str_push_c_str(&inout, "");
  br_str_replace_one1(&inout, BR_STRL("foo"), BR_STRL(""));
  TEST_EQUAL(inout.len, 0);

  br_str_free(inout);
}

int main(void) {
  str_tests();
  str_trim();
  str_replace();
  str_replace1();
}

void br_on_fatal_error(void) { }
void brgui_push_log_line(const char* fmt, ...) {(void)fmt;}
