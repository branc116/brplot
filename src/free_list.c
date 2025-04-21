#include "src/br_pp.h"
#define BR_LOGE LOGE
#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"
#include "src/br_da.h"

typedef struct {
  int* arr;
  int* free_arr;
  int len, cap;
  int free_len;
  int free_next;
} ints;

typedef struct {
  void* arr;
  int* free_arr;
  int len, cap;
  int free_len;
  int free_next;
} brfl_proto_fl_t;


#if !defined(_MSC_VER)
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
#endif
