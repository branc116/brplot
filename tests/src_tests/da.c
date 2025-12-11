#include "src/br_pp.h"

#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"

#include "src/br_da.h"
#include "src/br_test.h"

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

void br_on_fatal_error(void) {}
void brgui_push_log_line(const char* fmt, ...) {(void)fmt;}
