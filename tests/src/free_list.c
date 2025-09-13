#include "src/br_pp.h"
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"
#define BR_FREAD test_read
#define BR_FWRITE test_write
#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"
#define BR_UNIT_TEST
#define BR_UNIT_TEST_IMPLEMENTATION
#include "external/tests.h"
#include "src/br_da.h"

#include <errno.h>
#include <string.h>

#define MEM_FILE_CAP 4096
static BR_THREAD_LOCAL unsigned char mem_file[MEM_FILE_CAP];
static BR_THREAD_LOCAL int mem_file_pointer_read;
static BR_THREAD_LOCAL int mem_file_pointer_write;

size_t test_read(void* dest, size_t el_size, size_t n, void* null) {
  size_t size = n * el_size;
  memcpy(dest, mem_file + mem_file_pointer_read, size);
  mem_file_pointer_read += size;
  BR_ASSERT(mem_file_pointer_read <= mem_file_pointer_write);
  return n;
}

size_t test_write(void* src, size_t el_size, size_t n, void* null) {
  size_t size = n * el_size;
  memcpy(mem_file + mem_file_pointer_write, src, size);
  mem_file_pointer_write += size;
  BR_ASSERT(mem_file_pointer_write <= MEM_FILE_CAP);
  return n;
}

TEST_CASE(free_list_serizalize) {
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

int main(void) {}
void br_on_fatal_error(void) {}
// cc -fsanitize=address -ggdb -I. tests/src/free_list.c -o bin/free_list_tests && bin/free_list_tests --unittest
