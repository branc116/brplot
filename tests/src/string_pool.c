#define BR_FREAD test_read
#define BR_FWRITE test_write
#include "src/br_pp.h"
static size_t test_read(void* dest, size_t el_size, size_t n, void* null);
static size_t test_write(void* src, size_t el_size, size_t n, void* null);

#include <errno.h>

#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"
#define BR_UNIT_TEST
#define BR_UNIT_TEST_IMPLEMENTATION
#include "external/tests.h"
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"
#define BRSP_IMPLEMENTATION
#include "src/br_string_pool.h"

#define MEM_FILE_CAP 4096
static BR_THREAD_LOCAL unsigned char mem_file[MEM_FILE_CAP];
static BR_THREAD_LOCAL int mem_file_pointer_read;
static BR_THREAD_LOCAL int mem_file_pointer_write;

static size_t test_read(void* dest, size_t el_size, size_t n, void* null) {
  (void)null;
  size_t size = n * el_size;
  memcpy(dest, mem_file + mem_file_pointer_read, size);
  mem_file_pointer_read += size;
  BR_ASSERT(mem_file_pointer_read <= mem_file_pointer_write);
  return n;
}

static size_t test_write(void* src, size_t el_size, size_t n, void* null) {
  (void)null;
  size_t size = n * el_size;
  memcpy(mem_file + mem_file_pointer_write, src, size);
  mem_file_pointer_write += size;
  BR_ASSERT(mem_file_pointer_write <= MEM_FILE_CAP);
  return n;
}


static void brsp_debug(brsp_t sp) {
  int len_sum = 0;
  int cap_sum = 0;
  LOGI("\n");
  LOGI("sp = { .len = %d, .cap = %d, .free_len = %d, .free_next = %d .pool = { .len = %u, .cap = %u } }", sp.len, sp.cap, sp.free_len, sp.free_next, sp.pool.len, sp.pool.cap);
  brfl_foreach(i, sp) {
    LOGI("sp[%d] = { .next_free = %d, .start_index = %d, .len = %d, .cap = %d }", i, sp.free_arr[i], sp.arr[i].start_index, sp.arr[i].len, sp.arr[i].cap);
    len_sum += sp.arr[i].len;
    cap_sum += sp.arr[i].cap;
  }
  LOGI("F");
  brfl_foreach_free(i, sp) {
    LOGI("sp[%d] = { .next_free = %d, .start_index = %d, .len = %d, .cap = %d }", i, sp.free_arr[i], sp.arr[i].start_index, sp.arr[i].len, sp.arr[i].cap);
  }
  LOGI("efficiency len/cap: %f", (float)len_sum / (float)sp.pool.cap);
  LOGI("efficiency len/len: %f", (float)len_sum / (float)sp.pool.len);
  LOGI("efficiency cap/len: %f", (float)cap_sum / (float)sp.pool.len);
  LOGI("efficiency cap/cap: %f", (float)cap_sum / (float)sp.pool.cap);
  LOGI("\n");
}


TEST_CASE(string_pool_init) {
  (void)brsp_debug;
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  br_strv_t str = brsp_get(sp, t);
  TEST_EQUAL(str.len, 0);
  brsp_set(&sp, t, BR_STRL("HEllo"));
  brsp_remove(&sp, t);
  brsp_free(&sp);
}

TEST_CASE(string_pool_resize) {
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  br_str_t s = { 0 };
  for (int i = 0; i < 129; ++i) {
    br_str_push_char(&s, 'c');
    brsp_set(&sp, t, br_str_as_view(s));
  }
  brsp_remove(&sp, t);
  brsp_free(&sp);
  br_str_free(s);
}

TEST_CASE(string_pool_resize2) {
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  brsp_id_t t2 = brsp_new(&sp);
  br_str_t s = { 0 };
  br_str_t s2 = { 0 };
  for (int i = 0; i < 256; ++i) {
    br_str_push_char(&s, 'c');
    brsp_set(&sp, t, br_str_as_view(s));

    br_str_push_char(&s2, 'd');
    br_str_push_char(&s2, 'd');
    br_str_push_char(&s2, 'd');
    brsp_set(&sp, t2, br_str_as_view(s2));
  }
  TEST_STRNEQUAL(s.str, brsp_get(sp, t).str, s.len);
  brsp_remove(&sp, t);
  brsp_remove(&sp, t2);
  brsp_free(&sp);
  br_str_free(s);
  br_str_free(s2);
}

TEST_CASE(string_pool_read_write) {
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  br_str_t s = { 0 };
  for (int i = 0; i < 129; ++i) {
    br_str_push_char(&s, 'c');
    brsp_set(&sp, t, br_str_as_view(s));
  }
  brsp_write(NULL, &sp);

  brsp_remove(&sp, t);
  brsp_free(&sp);
  br_str_free(s);

  brsp_t sp2 = { 0 };
  brsp_read(NULL, &sp2);
  br_strv_t news = brsp_get(sp2, t);
  TEST_EQUAL(news.len, 129);
  for (int i = 0; i < 129; ++i) {
    TEST_EQUAL(news.str[i], 'c');
  }
  brsp_free(&sp);
}

int main(void) {}
// clang -fsanitize=address -ggdb -I. tests/src/string_pool.c -o bin/string_pool_tests && bin/string_pool_tests --unittest
