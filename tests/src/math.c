#include "src/br_math.h"

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "external/tests.h"
#pragma GCC diagnostic pop
TEST_CASE(vec2s) {
  br_vec2_t a = BR_VEC2(1.f, 2.f), b = BR_VEC2(3.f, 5.f);
  br_vec2_t res = br_vec2_scale(a, 10.f);
  TEST_EQUALF(res.x, 10.f);
  TEST_EQUALF(res.y, 20.f);
  res = br_vec2_sub(a, b);
  TEST_EQUALF(res.x, -2.f);
  TEST_EQUALF(res.y, -3.f);
  res = br_vec2_mul(a, b);
  TEST_EQUALF(res.x, 3.f);
  TEST_EQUALF(res.y, 10.f);
  TEST_EQUALF(br_vec2_len2(BR_VEC2(3.f,4.f)), 25.f);
  TEST_EQUALF(br_vec2_len(BR_VEC2(3.f,4.f)), 5.f);
  TEST_EQUALF(br_vec2_dist2(a, b), 13.f);
}

TEST_CASE(vec3s) {
  br_vec3_t a = BR_VEC3(1.f, 2.f, 3.f), b = BR_VEC3(5.f, 8.f, 13.f);
  br_vec3_t res = br_vec3_scale(a, 10.f);
  TEST_EQUALF(res.x, 10.f);
  TEST_EQUALF(res.y, 20.f);
  TEST_EQUALF(res.z, 30.f);
  res = br_vec3_sub(a, b);
  TEST_EQUALF(res.x, -4.f);
  TEST_EQUALF(res.y, -6.f);
  TEST_EQUALF(res.z, -10.f);
  res = br_vec3_mul(a, b);
  TEST_EQUALF(res.x, 5.f);
  TEST_EQUALF(res.y, 16.f);
  TEST_EQUALF(res.z, 39.f);
  TEST_EQUALF(br_vec3_len2(BR_VEC3(0.f, 3.f, 4.f)), 25.f);
  TEST_EQUALF(br_vec3_len(BR_VEC3(0.f, 3.f, 4.f)), 5.f);
  TEST_EQUALF(br_vec3_dist2(a, b), 152.f);
  TEST_EQUALF(br_vec3_dist(a, b), sqrtf(152.f));
  res = br_vec3_pow(a, 2.f);
  TEST_EQUALF(res.x, 1.f);
  TEST_EQUALF(res.y, 4.f);
  TEST_EQUALF(res.z, 9.f);
  TEST_EQUALF(br_vec3_dot(a, b), 5.f+16.f+39.f);
  TEST_EQUALF(br_vec3_cross(BR_VEC3(1,0,0), BR_VEC3(0,1,0)).z, 1.f);
  TEST_EQUALF(br_vec3_cross(BR_VEC3(0,0,1), BR_VEC3(0,1,0)).x, -1.f);
  res = br_vec3_normalize(a);
  TEST_EQUALF(br_vec3_len2(res), 1.f);
  TEST_EQUALF(br_vec3_angle(BR_VEC3(1,0,0), BR_VEC3(0,1,0)), BR_PI/2);
  TEST_EQUALF(br_vec3_angle(BR_VEC3(2,0,0), BR_VEC3(0,1,0)), BR_PI/2);
  TEST_EQUALF(br_vec3_angle(BR_VEC3(2,0,0), BR_VEC3(1,0,0)), 0.f);
  res = br_vec3_rot(BR_VEC3(2,0,0), BR_VEC3(0,1,0), BR_PI/2);
  TEST_EQUALF(res.x, 0.f);
  TEST_EQUALF(res.y, 0.f);
  TEST_EQUALF(res.z, -2.f);
  res = br_vec3_perpendicular(BR_VEC3(1,0,0));
  TEST_EQUALF(br_vec3_dot(res, BR_VEC3(1,0,0)), 0.f);
}
#endif
