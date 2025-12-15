#include "tests/src_tests/shl.h"

int main(void) {
  {
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

  {
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
  {
    br_mat_t m = { .arr = {
        1, 1, 1, -1,
        1, 1, -1, 1,
        1, -1, 1, 1,
        -1, 1, 1, 1,
      }
    };
    float det = br_mat_det(m);
    TEST_EQUALF(det, -16);
  }
  {
    br_mat_t m = { .arr = {
        1, 0, 0, 0,
        0, 2, -1, 0,
        0, -1, 3, 0,
        0, 0, 0, 4
      }
    };
    float det = br_mat_det(m);
    TEST_EQUALF(det, 20);
  }
  {
    br_mat_t m = { .arr = {
        1.11419, 0.0, -1.5051, -0.361676,
        0.71714, 1.5983, 0.530884, -0.12714,
        -0.701784, 0.487444, -0.519517, 5.67892,
        -0.701784, 0.487444, -0.519517, 5.68092,
      }
    };
    float det = br_mat_det(m);
    TEST_EQUALF(det, -0.006852222642255142);
    br_mat_t inv = br_mat_transpose(br_mat_inverse(m));
    br_mat_t expected_inv = { .arr = {
      0.317729, 0.214028, -2065.49, 2064.79,
      -1.52961e-7, 0.477006, 1354.92, -1354.43,
      -0.429201, 0.15844, -1408.83, 1408.31,
      0.0, 0.0, -500.25, 500.25
    }};
    for (int i = 0; i < 16; ++i) {
      TEST_EQUALF(expected_inv.arr[i], inv.arr[i]);
    }
    br_mat_t ident = br_mat_mul(m, inv);
    det = br_mat_det(ident);
    TEST_EQUALF(det, 1);
  }
  {
    br_mat_t m = { .arr = {
        1, 0, 0, 0,
        0, 2, -1, 0,
        0, -1, 3, 0,
        0, 0, 0, 4
      }
    };
    br_mat_t inv = br_mat_inverse(m);
    br_mat_t ident = br_mat_mul(m, inv);
    float det = br_mat_det(ident);
    TEST_EQUALF(det, 1);
  }
  {
    bool col;
    col = br_col_line_bb(BR_VEC3(0, 0, 100), BR_VEC3(0, 0, -100), (br_bb3_t) { .min = BR_VEC3(-1, -1, -1), .max = BR_VEC3(1, 1, 1) });
    TEST_EQUAL(true, col);
    col = br_col_line_bb(BR_VEC3(1, 0, 100), BR_VEC3(0, 0, -100), (br_bb3_t) { .min = BR_VEC3(-1, -1, -1), .max = BR_VEC3(1, 1, 1) });
    TEST_EQUAL(true, col);
    col = br_col_line_bb(BR_VEC3(2, 0, 100), BR_VEC3(0, 0, -100), (br_bb3_t) { .min = BR_VEC3(-1, -1, -1), .max = BR_VEC3(1, 1, 1) });
    TEST_EQUAL(true, col);
    col = br_col_line_bb(BR_VEC3(20, 0, 100), BR_VEC3(0, 0, -100), (br_bb3_t) { .min = BR_VEC3(-1, -1, -1), .max = BR_VEC3(1, 1, 1) });
    TEST_EQUAL(false, col);
    col = br_col_line_bb(BR_VEC3(1, 1, 100), BR_VEC3(0, 0, -100), (br_bb3_t) { .min = BR_VEC3(-1, -1, -1), .max = BR_VEC3(1, 1, 1) });
    TEST_EQUAL(true, col);
  }
}
