#include "src/br_pp.h"
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"
#include "src/br_test.h"
#include "src/resampling.c"
#include "tests/src/mock_tl.c"
#include "tests/src/mock_smol_mesh.c"

void resampling(void) {
  float xs[] = { 0, 1, 2, 3 };
  float ys[] = { 1, 2, 4, 2 };
  br_data_t pg;
  memset(&pg, 0, sizeof(pg));
  pg.cap = 4;
  pg.len = 4;
  pg.dd.xs = xs;
  pg.dd.ys = ys;
  pg.resampling = NULL;
  br_resampling_t* r = br_resampling_malloc(br_data_kind_2d);
  for (int i = 0; i < 2*1024; ++i) br_resampling_add_point(r, &pg, 3);
  br_resampling_add_point(r, &pg, 3);
  for (int i = 0; i < 64*1024; ++i) br_resampling_add_point(r, &pg, 3);
  br_resampling_free(r);
}

#define N 2048
void resampling2(void) {
  float xs[N];
  float ys[N];
  for (int i = 0; i < N; ++i) {
    xs[i] = sinf(3.14f / 4.f * (float)i);
    ys[i] = cosf(3.14f / 4.f * (float)i);
  }
  br_data_t pg;
  memset(&pg, 0, sizeof(pg));
  pg.cap = N;
  pg.len = N;
  pg.dd.xs = xs;
  pg.dd.ys = ys;
  pg.resampling = NULL;
  br_resampling_t* r = br_resampling_malloc(br_data_kind_2d);
  for (int i = 0; i < N; ++i) br_resampling_add_point(r, &pg, (uint32_t)i);
  br_resampling_add_point(r, &pg, 3);
  br_resampling_free(r);
}

int main(void) {
  resampling();
  resampling2();
}

void br_on_fatal_error(void) {}
