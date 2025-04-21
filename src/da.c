#include "src/br_da.h"
#include "src/br_pp.h"


#if defined(BR_UNIT_TEST)
#include "external/tests.h"
TEST_CASE(da) {
  typedef struct {
    int* arr;
    size_t len, cap;
  } test_arr;

  test_arr a = {0};
  br_da_push(a, 1);
  br_da_push(a, 2);
  br_da_push(a, 3);
  br_da_remove(a, 2);
  br_da_remove_at(a, 2);
  br_da_remove_n_at(a, 1, 0);
  br_da_free(a);
}
#endif
