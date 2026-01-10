#include <brplot.h>
#include <math.h>

int main(void) {
  br_data_id circle = 2, standing_wave = 3;
  float dr = 0.01f;
  brp_label("circle", circle);
  brp_label("standing wave", standing_wave);
  float time_left = 5.f;
  for (float radius = 0.0f; time_left > 0; radius += dr) {
    for (float t = -10; t < 10; t += 0.1f) brp_2(radius*sinf(t),                 radius*cosf(t),        circle);
    for (float t = -10; t < 10; t += 0.1f) brp_2(         t/2.f, cosf(2*3.1415f*radius)*sinf(t)/t, standing_wave);
    brp_flush();
    brp_empty(circle);
    brp_empty(standing_wave);
    if (radius > 1 || radius < 0.0) dr *= -1.f;
    time_left -= 0.016f;
  }
  brp_focus_all();
  brp_wait();
  return 0;
}

