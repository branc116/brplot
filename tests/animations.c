#define BRPLOT_IMPLEMENTATION
#include "../.generated/brplot.c"

int main() {
  br_data_id circle = 2, standing_wave = 3;
  float dr = 0.01f;
  for (float radius = 0.0f; true; radius += dr) {
	  for (float t = -10; t < 10; t += 0.1f) brp_2(radius*sinf(t),                 radius*cosf(t),        circle);
	  for (float t = -10; t < 10; t += 0.1f) brp_2(         t/2.f, cosf(2*BR_PI*radius)*sinf(t)/t, standing_wave);
	  brp_flush();
	  brp_empty(circle);
	  brp_empty(standing_wave);
	  if (radius > 1 || radius < 0.0) dr *= -1.f;
  }
  brp_focus_all();
  brp_wait();
  return 0;
}

