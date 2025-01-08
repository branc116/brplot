#pragma once
#include "stdbool.h"
#include "stddef.h"

static inline float maxf(float a, float b) {
  return a > b ? a : b;
}

static inline int maxi32(int a, int b) {
  return a > b ? a : b;
}

static inline int mini32(int a, int b) {
  return a < b ? a : b;
}

static inline size_t minui64(size_t a, size_t b) {
  return a < b ? a : b;
}

static inline size_t maxui64(size_t a, size_t b) {
  return a > b ? a : b;
}

static inline float minf(float a, float b) {
  return a > b ? b : a;
}

static inline float absf(float a) {
  return a > 0 ? a : -a;
}
static inline float signf(float a) {
  return a > 0.f ?  1.f :
         a < 0.f ? -1.f : 0.f;
}
static inline int signi(int a) {
  return a > 0 ?  1 :
         a < 0 ? -1 : 0;
}

static inline bool help_near_zero(float value) {
  return absf(value) < 1e-6;
}

extern unsigned char const br_font_data[];
extern long long const br_font_data_size;

