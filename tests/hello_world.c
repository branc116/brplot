#include <brplot.c>
#include <math.h>

int main() {
  for (float i = -10; i < 10; i+=0.1f) brp_1(i*i, 1);
  for (float i = -10; i < 10; i+=0.1f) brp_2(0.1f*sinf(i), 0.1f*cosf(i), 2);
  for (float i = -10; i < 10; i+=0.1f) brp_2(0.5f + 0.1f*sinf(i), 0.1f*cosf(i), 3);
  brp_focus_all();
  brp_wait();
  return 0;
}

