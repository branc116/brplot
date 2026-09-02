#define BRPLOT_IMPLEMENTATION
#include <brplot.h>
#include <math.h>

int main(void) {
  for (int i = 0; i < 14; ++i) {
    for (float i = -10; i < 10; i+=0.1f) brp_1(i*i, 1);
    brp_focus_all();
    brp_wait();
  }
  return 0;
}

// tcc -o bin/reopen -I. -Iinclude tests/reopen.c -lm && bin/reopen
