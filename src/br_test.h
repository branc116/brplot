#include "src/br_pp.h"

#define TEST_NEQUAL(a, b)                                    \
  do {                                                       \
    if ((a) == (b)) {                                        \
      long long la = (long long)(a);                         \
      long long lb = (long long)(b);                         \
      LOGE("%s", #a " == " #b);                              \
      LOGF("%lld == %lld", la, lb);                          \
      BR_UNREACHABLE();                                      \
    }                                                        \
  } while (0)

#define TEST_EQUAL(a, b)                                     \
  do {                                                       \
    if ((a) != (b)) {                                        \
      long long la = (long long)(a);                         \
      long long lb = (long long)(b);                         \
      LOGE("%s", #a " != " #b);                              \
      LOGF("%s", "%lld != %lld", la, lb);                    \
      BR_UNREACHABLE();                                      \
    }                                                        \
  } while (0)

#define TEST_EQUALF(a, b)                                    \
  do {                                                       \
    float aa = (float)(a), bb = (float)(b);                  \
    if (fabsf(aa - bb) > 1e-5) {                             \
      LOGF("%f != %f ( " #a " != " #b " )", aa, bb);         \
    }                                                        \
  } while (0)

#define TEST_TRUE(a)                                         \
  do {                                                       \
    if (!(a)) {                                              \
      LOGF(#a " is not true");                               \
    }                                                        \
  } while (0)

#define TEST_STREQUAL(a, b)                                  \
  do {                                                       \
    if (strcmp((a), (b)) != 0) {                             \
      LOGE(#a " != " #b);                                    \
      LOGF("\"%s\" != \"%s\"",  (a), (b));                   \
    }                                                        \
  } while (0)

#define TEST_STRNEQUAL(a, b, len)                            \
  do {                                                       \
    if (strncmp((a), (b), (len)) != 0) {                     \
      LOGE(#a " != " #b);                                    \
      LOGF("\"%.*s\" != \"%.*s\"", (int)(len), (a), (int)(len), (b)); \
    }                                                        \
  } while (0)
