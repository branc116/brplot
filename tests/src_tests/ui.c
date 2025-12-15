#include "tests/src_tests/shl.h"

int main(void) {
  BR_FILE* file = BR_FOPEN("test", "rwb");
  brui_window_t win = { .pl = { .kind = brpl_window_headless } };
  brui_window_init(&win);
  brui_save(file, &win);
  bool is_ok = brui_load(file, &win);
  TEST_EQUAL(is_ok, true);
  brui_window_deinit(&win);
}

