typedef struct {
  unsigned char* arr;
  int len, cap;
  int read_index;
} br_test_file_t;

#if defined(FUZZ)
//#  define BR_DISABLE_LOG
#endif

#define BR_FREAD test_read
#define BR_FWRITE test_write
#define BR_FILE_T br_test_file_t
#include "src/br_pp.h"
static size_t test_read(void* dest, size_t el_size, size_t n, br_test_file_t* null);
static size_t test_write(void* src, size_t el_size, size_t n, br_test_file_t* null);

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

static size_t test_read(void* dest, size_t el_size, size_t n, br_test_file_t* d) {
  size_t size = n * el_size;
  if (size + d->read_index > d->len) {
    errno = 1;
    return 0;
  }
  memcpy(dest, d->arr + d->read_index, size);
  d->read_index += size;
  BR_ASSERT(d->read_index <= d->len);
  return n;
}

static size_t test_write(void* src, size_t el_size, size_t n, br_test_file_t* d) {
  size_t size = n * el_size;
  if (d->cap < d->len + size) {
    errno = 2;
    return 0;
  }
  memcpy(d->arr + d->len, src, size);
  d->len += size;
  BR_ASSERT(d->len <= d->cap);
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
  TEST_EQUAL(sp.free_len, 1);
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
  TEST_EQUAL(sp.free_len, 2);
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
  br_test_file_t tf = { .arr = mem_file, .len = 0, .cap = sizeof(mem_file), .read_index = 0 };
  TEST_EQUAL(sp.free_len, 1);
  brsp_write(&tf, &sp);

  brsp_remove(&sp, t);
  brsp_free(&sp);
  br_str_free(s);

  brsp_t sp2 = { 0 };
  brsp_read(&tf, &sp2);
  br_strv_t news = brsp_get(sp2, t);
  TEST_EQUAL(news.len, 129);
  TEST_EQUAL(sp2.free_len, 1);
  for (int i = 0; i < 129; ++i) {
    TEST_EQUAL(news.str[i], 'c');
  }
  brsp_free(&sp);
}

TEST_CASE(string_pool_read_remove_write) {
  br_test_file_t tf = { .arr = mem_file, .len = 0, .cap = sizeof(mem_file), .read_index = 0 };
  brsp_t sp = { 0 };
  brsp_id_t t  = brsp_new(&sp);
  brsp_id_t t2 = brsp_new(&sp);
  brsp_id_t trest[] = { brsp_new(&sp), brsp_new(&sp), brsp_new(&sp) };
  br_str_t s = { 0 };
  for (int i = 0; i < 129; ++i) {
    br_str_push_char(&s, 'c');
    brsp_set(&sp, t, br_str_as_view(s));
  }
  brsp_insert_char_at_end(&sp, t2, 'd');
  TEST_EQUAL(5, sp.free_len);
  brsp_remove(&sp, t);
  TEST_EQUAL(4, sp.free_len);
  brsp_write(&tf, &sp);

  brsp_free(&sp);
  br_str_free(s);

  brsp_t sp2 = { 0 };
  brsp_read(&tf, &sp2);
  TEST_EQUAL(4, sp2.free_len);
  br_strv_t news = brsp_get(sp2, t2);
  TEST_EQUAL(news.str[0], 'd');
  TEST_EQUAL(news.len, 1);
  brsp_free(&sp);
}

TEST_CASE(string_pool_is_in) {
  brsp_t sp = { 0 };
  brsp_id_t t  = brsp_new(&sp);
  TEST_EQUAL(1, brsp_is_in(sp, t));
  TEST_EQUAL(0, brsp_is_in(sp, t + 1));
  TEST_EQUAL(0, brsp_is_in(sp, t - 1));
  TEST_EQUAL(0, brsp_is_in(sp, 0));
  TEST_EQUAL(0, brsp_is_in(sp, sp.len + 1));
  TEST_EQUAL(0, brsp_is_in(sp, 0xfffffff));
  brsp_free(&sp);
}

TEST_CASE(string_pool_copy) {
  brsp_t sp = { 0 };
  brsp_id_t t  = brsp_new(&sp);
  brsp_insert_strv_at_end(&sp, t, BR_STRL("Hello"));
  brsp_id_t t2  = brsp_copy(&sp, t);
  TEST_NEQUAL(t, t2);
  br_strv_t s = brsp_get(sp, t);
  br_strv_t s2 = brsp_get(sp, t2);
  TEST_STRNEQUAL(s.str, s2.str, s.len); 
  brsp_free(&sp);
}

TEST_CASE(string_pool_compress) {
  brsp_t sp = { 0 };
  brsp_id_t t  = brsp_new(&sp);
  brsp_insert_strv_at_end(&sp, t, BR_STRL("Hello"));
  brsp_insert_strv_at_end(&sp, t, BR_STRL("Hello"));
  brsp_id_t t2  = brsp_copy(&sp, t);
  brsp_id_t t3  = brsp_copy(&sp, t);
  brsp_id_t t4  = brsp_copy(&sp, t);
  brsp_compress(&sp, 1.0f, 0);
  TEST_NEQUAL(t, t2);
  TEST_STRNEQUAL(brsp_get(sp, t).str, brsp_get(sp, t2).str, 5);
  TEST_STRNEQUAL(brsp_get(sp, t).str, brsp_get(sp, t3).str, 5);
  TEST_STRNEQUAL(brsp_get(sp, t).str, brsp_get(sp, t4).str, 5);
  brsp_remove(&sp, t3);
  brsp_remove(&sp, t2);
  brsp_compress(&sp, 2.0f, 16);
  TEST_STRNEQUAL(brsp_get(sp, t).str, brsp_get(sp, t4).str, 5);
  TEST_EQUAL(sp.free_len, 2);
  brsp_free(&sp);
}

#if defined(FUZZ)
const char* __asan_default_options(void) {
  return "verbosity=0:"
    "sleep_before_dying=120:"
    "print_scariness=true:"
    "allocator_may_return_null=1:"
    "soft_rss_limit_mb=1512222"
    ;
}
int LLVMFuzzerTestOneInput(unsigned char *str, size_t str_len) {
  mem_file_pointer_write = str_len;
  br_test_file_t tf = { .arr = str, .cap = str_len, .len = str_len, .read_index = 0 };
  brsp_t sp = { 0 };
  brsp_read(&tf, &sp);
  brsp_id_t id = brsp_new(&sp);
  brsp_insert_strv_at_end(&sp, id, BR_STRL("Hello"));
  br_strv_t s = brsp_get(sp, id);
  BR_ASSERT(s.str[0] == 'H');
  BR_ASSERT(s.str[1] == 'e');
  BR_ASSERT(s.str[2] == 'l');
  BR_ASSERT(s.str[3] == 'l');
  BR_ASSERT(s.str[4] == '0');
  brsp_free(&sp);
  return 0;
}
#else
int main(void) {}
#endif
// clang -fsanitize=address -ggdb -I. tests/src/string_pool.c -o bin/string_pool_tests && bin/string_pool_tests --unittest
