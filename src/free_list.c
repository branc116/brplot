#include "src/br_pp.h"
#if !defined(BR_LOGE)
#  define BR_LOGE LOGE
#endif
#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"
#include "src/br_da.h"

#if defined(BR_UNIT_TEST)
#include "external/tests.h"

TEST_CASE(free_list_test) {
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

TEST_CASE(free_list_test2) {
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

TEST_CASE(free_list_test3) {
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
#endif
