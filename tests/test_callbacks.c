#define BRPLOT_IMPLEMENTATION
#include "include/brplot.h"

typedef struct sine_args {
  float A, F;
} sine_args;

void cb(br_plotter_t* plotter, void* data) {
  sine_args* sa = data;
  brui_resizable_temp_push(BR_STRL("Test"));
    brui_sliderf(BR_STRL("Amlitude"), &sa->A);
    brui_sliderf(BR_STRL("Frequency"), &sa->F);
  brui_resizable_temp_pop();
}

volatile sine_args args = { .A = 1, .F = 1 };
int main(void) {
  brp_add_callback(cb, &args);

  double x = 0;
  while (1) {
    double y = args.A*sin(args.F*x);
    brp_2(x, y, 0);
    x += 0.1;
    brpl_sleep(0.016);
  }
  brp_wait();
}
// gcc -I. -o bin/test_callbacks tests/test_callbacks.c -lm && bin/test_callbacks
