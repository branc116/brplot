#include "tests/src_tests/shl.h"

int main(void) {
  typedef struct {
    int* arr;
    size_t len, cap;
  } test_arr;

  test_arr a = {0};
  br_da_push(a, 1);
  TEST_EQUAL(a.len, 1);
  br_da_push(a, 2);
  TEST_EQUAL(a.len, 2);
  br_da_push(a, 3);
  TEST_EQUAL(a.len, 3);
  br_da_remove(a, 2);
  TEST_EQUAL(a.len, 2);
  br_da_remove_at(a, 1);
  TEST_EQUAL(a.len, 1);
  br_da_push(a, 10);
  br_da_remove_n_at(a, 2, 0);
  TEST_EQUAL(a.len, 0);
  br_da_free(a);
}
