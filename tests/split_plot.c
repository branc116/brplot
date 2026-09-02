#define BRPLOT_IMPLEMENTATION
#include "include/brplot.h"

int main(void) {
  for (int j = 0; j < 10; ++j) {
    brp_2(NAN, NAN, 0);
    double factor = 10000;
    for (int i = 0; i < 10*factor; ++i) {
      brp_2(j*1000 + i/factor, (i+j)/factor, 0);
    }
  }
  brp_wait();
}
// tcc -I. tests/split_plot.c -o bin/split_plot -lm && bin/split_plot
