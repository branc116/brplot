#include "src/br_pp.h"
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"
#define BR_FREAD test_read
#define BR_FWRITE test_write
#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"
#include "src/br_test.h"
#include "src/br_da.h"

#include <errno.h>
#include <string.h>

#define MEM_FILE_CAP 4096
static BR_THREAD_LOCAL unsigned char mem_file[MEM_FILE_CAP];
static BR_THREAD_LOCAL size_t mem_file_pointer_read;
static BR_THREAD_LOCAL size_t mem_file_pointer_write;

size_t test_read(void* dest, size_t el_size, size_t n, void* null) {
  (void)null;
  size_t size = n * el_size;
  memcpy(dest, mem_file + mem_file_pointer_read, size);
  mem_file_pointer_read += size;
  BR_ASSERT(mem_file_pointer_read <= mem_file_pointer_write);
  return n;
}

size_t test_write(void* src, size_t el_size, size_t n, void* null) {
  (void)null;
  size_t size = n * el_size;
  memcpy(mem_file + mem_file_pointer_write, src, size);
  mem_file_pointer_write += size;
  BR_ASSERT(mem_file_pointer_write <= MEM_FILE_CAP);
  return n;
}

void free_list_test() {
  struct {
    int* arr;
    int* free_arr;
    int len, cap;
    int free_len;
    int free_next;
  } is = { 0 };
  int one_handle = brfl_push(is, 1);
  TEST_EQUAL(one_handle, 0);
  int two_handle = brfl_push(is, 2);
  TEST_EQUAL(two_handle, 1);
  int one = br_da_get(is, one_handle);
  TEST_EQUAL(one, 1);
  int two = br_da_get(is, two_handle);
  TEST_EQUAL(two, 2);
  brfl_remove(is, one_handle);
  TEST_EQUAL(is.free_len, 1);
  TEST_EQUAL(is.free_next, one_handle);
  two = br_da_get(is, two_handle);
  TEST_EQUAL(two, 2);
  brfl_free(is);
}

void free_list_test2() {
  struct {
    int* arr;
    int* free_arr;
    int len, cap;
    int free_len;
    int free_next;
  } is = { 0 };
  int one_handle = brfl_push(is, 1);
  int two_handle = brfl_push(is, 2);
  int three_handle = brfl_push(is, 3);
  brfl_remove(is, three_handle);
  brfl_remove(is, two_handle);
  brfl_remove(is, one_handle);
  one_handle = brfl_push(is, 1);
  two_handle = brfl_push(is, 2);
  three_handle = brfl_push(is, 3);
  TEST_EQUAL(one_handle, 0);
  TEST_EQUAL(two_handle, 1);
  TEST_EQUAL(three_handle, 2);
  brfl_free(is);
}

void free_list_test3() {
  struct {
    int* arr, * free_arr;
    int len, cap;
    int free_len, free_next;
  } is = { 0 };
  (void)brfl_push(is, -1);
  (void)brfl_push(is, -1);
  (void)brfl_push(is, -1);
  int one_handle = brfl_push(is, 1);
  int two_handle = brfl_push(is, 2);
  int three_handle = brfl_push(is, 3);
  (void)brfl_push(is, -1);
  (void)brfl_push(is, -1);
  (void)brfl_push(is, -1);
  brfl_remove(is, two_handle);
  brfl_remove(is, three_handle);
  brfl_remove(is, one_handle);
  (void)brfl_push(is, -1);
  one_handle = brfl_push(is, 1);
  two_handle = brfl_push(is, 2);
  three_handle = brfl_push(is, 3);
  TEST_EQUAL(one_handle, 5);
  TEST_EQUAL(two_handle, 4);
  TEST_EQUAL(three_handle, 9);
  brfl_free(is);
}

void free_list_test4() {
  struct {
    int* arr, *free_arr;
    int len, cap;
    int free_len, free_next;
  } is = { 0 };
  int one_handle = brfl_push(is, 1);
  int two_handle = brfl_push(is, 2);
  int error = 0;
  brfl_write(NULL, is, error);
  brfl_free(is);
  brfl_read(NULL, is, error);
  TEST_EQUAL(0, error);
  TEST_EQUAL(2, is.len);
  TEST_EQUAL(1, br_da_get(is, one_handle));
  TEST_EQUAL(2, br_da_get(is, two_handle));
  brfl_free(is);
}

int main(void) {
  free_list_test();
  free_list_test2();
  free_list_test3();
  free_list_test4();

}
void br_on_fatal_error(void) {}
void brgui_push_log_line(const char* fmt, ...) {(void)fmt;}
