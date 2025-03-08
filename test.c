#define BRPLOT_IMPLEMENTATION
#include ".generated/brplot.c"

int main() {
  br_data_id id = 0;
  for (int i = -10; i < 10; ++i) id = brp_1(i*i, id);
  brp_wait();
  return 0;
}

