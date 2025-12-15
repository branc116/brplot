#include <errno.h>
#include "tests/src_tests/shl.h"

void free_list_test0() {
  struct {
    int* arr;
    int* free_arr;
    int len, cap;
    int free_len;
    int free_next;
  } is = { 0 };
  int value = 0;
  int zero_handle = brfl_push(is, value);
  value = 1;
  int one_handle = brfl_push(is, value);
  value = 2;
  int two_handle = brfl_push(is, value);
  brfl_remove(is, one_handle);
  value = 1;
  one_handle = brfl_push(is, value);
  brfl_foreach_free(i, is) {
    BR_ASSERT(one_handle != i);
  }
  brfl_free(is);
}
void free_list_test() {
  struct {
    int* arr;
    int* free_arr;
    int len, cap;
    int free_len;
    int free_next;
  } is = { 0 };
  int value = 1;
  int one_handle = brfl_push(is, value);
  TEST_EQUAL(one_handle, 0);
  value = 2;
  int two_handle = brfl_push(is, value);
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
  int value = 1;
  int one_handle = brfl_push(is, value);
  value = 2;
  int two_handle = brfl_push(is, value);
  value = 3;
  int three_handle = brfl_push(is, value);
  brfl_remove(is, three_handle);
  brfl_remove(is, two_handle);
  brfl_remove(is, one_handle);
  value = 1;
  one_handle = brfl_push(is, value);
  value = 2;
  two_handle = brfl_push(is, value);
  value = 3;
  three_handle = brfl_push(is, value);
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
  int value = -1;
  (void)brfl_push(is, value);
  (void)brfl_push(is, value);
  (void)brfl_push(is, value);
  value = 1;
  int one_handle = brfl_push(is, value);
  value = 2;
  int two_handle = brfl_push(is, value);
  value = 3;
  int three_handle = brfl_push(is, value);
  value = -1;
  (void)brfl_push(is, value);
  (void)brfl_push(is, value);
  (void)brfl_push(is, value);
  brfl_remove(is, two_handle);
  brfl_remove(is, three_handle);
  brfl_remove(is, one_handle);
  (void)brfl_push(is, value);
  value = 1;
  one_handle = brfl_push(is, value);
  value = 2;
  two_handle = brfl_push(is, value);
  value = 3;
  three_handle = brfl_push(is, value);
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
  int value = 1;
  int one_handle = brfl_push(is, value);
  value = 2;
  int two_handle = brfl_push(is, value);
  int error = 0;
  BR_FILE* file = BR_FOPEN("test", "rb+");
  brfl_write(file, is, error);
  brfl_free(is);
  brfl_read(file, is, error);
  TEST_EQUAL(0, error);
  TEST_EQUAL(2, is.len);
  TEST_EQUAL(1, br_da_get(is, one_handle));
  TEST_EQUAL(2, br_da_get(is, two_handle));
  brfl_free(is);
}

int main(void) {
  free_list_test0();
  free_list_test();
  free_list_test2();
  free_list_test3();
  free_list_test4();

}
