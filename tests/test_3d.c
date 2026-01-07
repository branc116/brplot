#define BRPLOT_IMPLEMENTATION
#include <brplot.h>
#include <math.h>

int main(void) {
  double off = 0.1;
  for (;;) {
    double phi = off;
    for (double z = -1; z < 1; z+=0.001, phi += 0.1) {
      double d = sqrt((1 - z*z) / 1);
      double x = d * cos(phi);
      double y = d * sin(phi);
      brp_3(x, y, z, 0);
    }
    brp_flush();
    brp_empty(0);
    off += 0.1;
  }
  brp_wait();
  return 0;
}
// cc -I. tests/test_3d.c bin/brplot.a -lm 
