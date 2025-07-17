#include "src/br_pp.h"
#define BR_UNIT_TEST
#define BR_UNIT_TEST_IMPLEMENTATION
#include "external/tests.h"
#include "src/br_da.h"

TEST_CASE(da) {
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
  br_da_remove_n_at(a, 1, 0);
  TEST_EQUAL(a.len, 0);
  br_da_free(a);
}

int main(void) {}
void br_on_fatal_error(void) {}
