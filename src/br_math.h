#pragma once
#include <math.h>
#include <stddef.h>
#include <float.h>
#include <stdbool.h>

#define BR_MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define BR_MIN(X, Y) ((X) < (Y) ? (X) : (Y))

#define BR_VEC2(X, Y) ((br_vec2_t) { .x = (X), .y = (Y) })
#define BR_VEC2I(X, Y) ((br_vec2i_t) { .x = (X), .y = (Y) })
#define BR_VEC2D(X, Y) ((br_vec2d_t) { .x = (X), .y = (Y) })
#define BR_VEC2_TOI(V) ((br_vec2i_t) { .x = (int)(V).x, .y = (int)(V).y })
#define BR_VEC2_TOD(V) ((br_vec2d_t) { .x = (double)(V).x, .y = (double)(V).y })
#define BR_VEC2I_TOF(V) ((br_vec2_t) { .x = (float)(V).x, .y = (float)(V).y })
#define BR_VEC2I_TOD(V) ((br_vec2d_t) { .x = (double)(V).x, .y = (double)(V).y })
#define BR_VEC2I_SUB(A, B) ((br_vec2i_t) { .x = (A).x - (B).x, .y = (A).y - (B).y })
#define BR_VEC2I_SCALE(V, B) ((br_vec2i_t) { .x = (V).x * (B), .y = (V).y * (B) })
#define BR_VEC2D_TOF(V) ((br_vec2_t) { .x = (float)(V).x, .y = (float)(V).y })
#define BR_VEC2_(V) (V).x, (V).y
#define BR_VEC3_(V) (V).x, (V).y, (V).z
#define BR_VEC3D(X, Y, Z) ((br_vec3d_t) { .x = (X), .y = (Y), .z = (Z) })
#define BR_VEC3_TOC(V, A) ((br_color_t) { .r = (unsigned char)((V).x * 255.f), .g = (unsigned char)((V).y * 255.f), .b = (unsigned char)((V).z * 255.f), .a = (A) })
#define BR_SIZE(WIDTH, HEIGHT) ((br_size_t) { .width = (WIDTH), .height = (HEIGHT) })
#define BR_SIZE_TOI(SZ) ((br_sizei_t) { .width = (int)((SZ).width), .height = (int)((SZ).height) })
#define BR_SIZEI(WIDTH, HEIGHT) ((br_sizei_t) { .width = (WIDTH), .height = (HEIGHT) })
#define BR_SIZEI_TOF(SZ) ((br_size_t) { .width = (float)((SZ).width), .height = (float)((SZ).height) })

#define BR_EXTENT_ASPECT(E) ((E).height / (E).width)
#define BR_EXTENT(X, Y, WIDTH, HEIGHT) (br_extent_t) { .arr = { (X), (Y), (WIDTH), (HEIGHT) } }
#define BR_EXTENT2(POS, SIZE) (br_extent_t) { .pos = (POS), .size = (SIZE) }
#define BR_EXTENTD(X, Y, WIDTH, HEIGHT) (br_extentd_t) { .arr = { (X), (Y), (WIDTH), (HEIGHT) } }
#define BR_EXTENT_TOBB(E) ((br_bb_t) { .min_x = (E).x, .min_y = (E).y, .max_x = (E).x + (E).width, .max_y = (E).y + (E).height })
#define BR_EXTENT_TOI(E) ((br_extenti_t) { .arr = { (int)(E).arr[0], (int)(E).arr[1], (int)(E).arr[2], (int)(E).arr[3] } })
#define BR_EXTENTI_TOF(E) ((br_extent_t) { .arr = { (float)(E).arr[0], (float)(E).arr[1], (float)(E).arr[2], (float)(E).arr[3] } })
#define BR_EXTENTI_TOD(E) ((br_extentd_t) { .arr = { (double)(E).arr[0], (double)(E).arr[1], (double)(E).arr[2], (double)(E).arr[3] } })
#define BR_EXTENTI_TOBB(E) ((br_bb_t) { .min_x = (float)(E).x, .min_y = (float)(E).y, .max_x = (float)(E).x + (float)(E).width, .max_y = (float)(E).y + (float)(E).height })
#define BR_EXTENTD_TOBB(E) ((br_bbd_t) { .min_x = (double)(E).x, .min_y = (double)(E).y, .max_x = (double)(E).x + (double)(E).width, .max_y = (double)(E).y + (double)(E).height })
#define BR_EXTENTI_ASPECT(E) ((float)(E).height / (float)(E).width)
#define BR_EXTENTD_ASPECT(E) ((E).height / (E).width)
#define BR_EXTENTI(X, Y, WIDTH, HEIGHT) (br_extenti_t) { .arr = { (X), (Y), (WIDTH), (HEIGHT) } }
#define BR_EXTENTI2(POS, SIZE) (br_extenti_t) { { .pos = POS, .size = SIZE } }
#define BR_EXTENTD_TOF(E) ((br_extent_t) { .arr = { (float)(E).arr[0], (float)(E).arr[1], (float)(E).arr[2], (float)(E).arr[3] } })
#define BR_EXTENT_(EX) (EX).x, (EX).y, (EX).width, (EX).height

#define BR_BB(Xm, Ym, XM, YM) (br_bb_t) { .arr = { (Xm), (Ym), (XM), (YM) } }
#define BR_BB2(MIN, MAX) (br_bb_t) { .min = (MIN), .max = (MAX) }
#define BR_BB_TOEX(BB) (br_extent_t) { .arr = { (BB).min_x, (BB).min_y, (BB).max_x - (BB).min_x, (BB).max_y - (BB).min_y } }
#define BR_BBD_TOF(BB) (br_bb_t) { .arr = { (float)(BB).arr[0], (float)(BB).arr[1], (float)(BB).arr[2], (float)(BB).arr[3] } }
#define BR_BBW(BB) ((BB).max_x - (BB).min_x)
#define BR_BBH(BB) ((BB).max_y - (BB).min_y)
#define BR_BB_(BB) (BB).min_x, (BB).min_y, (BB).max_x, (BB).max_y

#define BR_VEC4(X, Y, Z, W) ((br_vec4_t) { .x = (X), .y = (Y), .z = (Z), .w = (W) }) 
#define BR_VEC42(XY, ZW) ((br_vec4_t) { .xy = (XY), .zw = (ZW)  }) 
#define BR_VEC3(X, Y, Z) ((br_vec3_t) { .x = (X), .y = (Y), .z = (Z) }) 
#define BR_VEC_ELS(X) (sizeof((X).arr) / sizeof((X).arr[0]))
#define BR_COLOR_TO4(X)  BR_VEC4(((X).r) / 255.f, ((X).g) / 255.f, ((X).b) / 255.f, ((X).a) / 255.f)
#define BR_COLOR_COMP(X)  ((X).r), ((X).g), ((X).b), ((X).a)
#define BR_COLOR_COMPF(X)  ((float)(X).r / 255.f), ((float)(X).g / 255.f), ((float)(X).b / 255.f), ((float)(X).a / 255.f)
#define BR_COLOR(R, G, B, A) ((br_color_t) { .r = (R), .g = (G), .b = (B), .a = (A) })

#define BR_RED BR_COLOR(200, 10, 10, 255)
#define BR_LIME BR_COLOR(10, 200, 10, 255)
#define BR_ORANGE BR_COLOR(180, 180, 10, 255)
#define BR_WHITE BR_COLOR(180, 180, 180, 255)
#define BR_GREEN BR_COLOR(10, 200, 10, 255)
#define BR_BLUE BR_COLOR(10, 10, 200, 255)
#define BR_LIGHTGRAY BR_COLOR(100, 100, 100, 255)
#define BR_PINK BR_COLOR(150, 80, 80, 255)
#define BR_GOLD BR_COLOR(50, 180, 50, 255)
#define BR_VIOLET BR_COLOR(180, 10, 180, 255)
#define BR_DARKPURPLE BR_COLOR(80, 10, 80, 255)

#define BR_PI 3.14159265358979323846f
#define BR_Z_MAX 100000
#define BR_Z_TO_GL(__Z) (1 - 2.f*((float)(__Z)/(float)BR_Z_MAX))
#define BR_SCREEN_TO_GL(point, screen) BR_VEC2((float)(point).x / (float)(screen).width * 2.f - 1.f, (float)(point).y / (float)(screen).height * 2.f - 1.f)

typedef struct {
  union {
    struct {
      float x, y;
    };
    float arr[2];
  };
} br_vec2_t;

typedef struct {
  union {
    struct {
      double x, y;
    };
    double arr[2];
  };
} br_vec2d_t;

typedef struct {
  union {
    struct {
      int x, y;
    };
    int arr[2];
  };
} br_vec2i_t;

typedef struct {
  union {
    struct {
      int width, height;
    };
    br_vec2i_t vec;
    int arr[2];
  };
} br_sizei_t;

typedef struct {
  union {
    struct {
      float width, height;
    };
    br_vec2_t vec;
    float arr[2];
  };
} br_size_t;

typedef struct {
  union {
    struct {
      int x, y, width, height;
    };
    struct {
      br_vec2i_t pos;
      br_sizei_t size;
    };
    int arr[4];
  };
} br_extenti_t;

typedef struct {
  union {
    struct {
      float x, y, width, height;
    };
    struct {
      br_vec2_t pos;
      br_size_t size;
    };
    float arr[4];
  };
} br_extent_t;

typedef struct {
  union {
    struct {
      double x, y, width, height;
    };
    struct {
      br_vec2d_t pos;
    };
    double arr[4];
  };
} br_extentd_t;

typedef struct {
  union {
    struct {
      float min_x, min_y, max_x, max_y;
    };
    struct {
      br_vec2_t min;
      br_vec2_t max;
    };
    float arr[4];
  };
} br_bb_t;

typedef struct {
  union {
    struct {
      double min_x, min_y, max_x, max_y;
    };
    struct {
      br_vec2d_t min;
      br_vec2d_t max;
    };
    double arr[4];
  };
} br_bbd_t;

typedef struct {
  union {
    struct {
      float x, y, z;
    };
    br_vec2_t xy;
    float arr[3];
  };
} br_vec3_t;

typedef struct {
  union {
    struct {
      double x, y, z;
    };
    br_vec2d_t xy;
    double arr[3];
  };
}  br_vec3d_t;

typedef struct {
  union {
    struct {
      float x, y, z, w;
    };
    struct {
      float r, g, b, a;
    };
    struct {
      br_vec2_t xy;
      br_vec2_t zw;
    };
    struct {
      br_vec3_t xyz;
    };
    float arr[4];
  };
}  br_vec4_t;

typedef struct {
  unsigned char r, g, b, a;
} br_color_t;

typedef struct {
  union {
    struct {
      float m0, m4, m8, m12;
      float m1, m5, m9, m13;
      float m2, m6, m10, m14;
      float m3, m7, m11, m15;
    };
    float arr[16];
    br_vec4_t rows[4];
  };
} br_mat_t;

//------------------------float----------------------------------

static inline float br_float_clamp(float x, float m, float M) {
  if (x < m) return m;
  if (x > M) return M;
  return x;
}

static inline float br_float_lerp(float from, float to, float factor) {
  return from * (1 - factor) + to * (factor);
}

static inline float br_float_lerp2(float from, float to, float factor) {
  float fact2 = sqrtf(factor);
  return from * (1 - fact2) + to * (fact2);
}

static inline bool br_float_near_zero(float value) {
  return fabsf(value) < 1e-6;
}

//------------------------br_vec2_t------------------------------

static inline br_vec2_t br_vec2i_tof(br_vec2i_t v) {
  return (br_vec2_t) { .x = (float)v.x, .y = (float)v.y };
}

static inline br_vec2i_t br_vec2_toi(br_vec2_t v) {
  return (br_vec2i_t) { .x = (int)v.x, .y = (int)v.y };
}

static inline br_vec2_t br_vec2_min(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(fminf(a.x, b.x), fminf(a.y, b.y));
}

static inline br_vec2_t br_vec2_max(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(fmaxf(a.x, b.x), fmaxf(a.y, b.y));
}

static inline br_vec2_t br_vec2_scale(br_vec2_t v, float s) {
  return ((br_vec2_t) { .x = v.x * s, .y = v.y * s });
}

static inline br_vec2_t br_vec2_add(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(a.x + b.x, a.y + b.y);
}

static inline br_vec2_t br_vec2_adds(br_vec2_t a, br_size_t b) {
  return BR_VEC2(a.x + b.width, a.y + b.height);
}

static inline br_vec2_t br_vec2_sub(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(a.x - b.x, a.y - b.y);
}

static inline bool br_vec2_eq(br_vec2_t a, br_vec2_t b) {
  return a.x == b.x && a.y == b.y;
}

static inline br_vec2i_t br_vec2i_add(br_vec2i_t a, br_vec2i_t b) {
  return BR_VEC2I(a.x + b.x, a.y + b.y);
}

static inline br_vec2i_t br_vec2i_sub(br_vec2i_t a, br_vec2i_t b) {
  return BR_VEC2I(a.x - b.x, a.y - b.y);
}

static inline float br_vec2_len2(br_vec2_t a) {
  float sum = 0.f;
  for (size_t i = 0; i < BR_VEC_ELS(a); ++i) sum += a.arr[i] * a.arr[i];
  return sum;
}

static inline float br_vec2_len(br_vec2_t a) {
  return sqrtf(br_vec2_len2(a));
}

static inline br_vec2_t br_vec2_mul(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(a.x * b.x, a.y * b.y);
}


static inline br_vec2_t br_vec2_div(br_vec2_t a, br_vec2_t b) {
  return BR_VEC2(a.x / b.x, a.y / b.y);
}

static inline br_vec3_t br_vec3_cross(br_vec3_t a, br_vec3_t b);

static inline bool br_vec2_ccv(br_vec2_t a, br_vec2_t b, br_vec2_t c) {
  br_vec2_t ab = br_vec2_sub(b, a);
  br_vec2_t cb = br_vec2_sub(b, c);
  return br_vec3_cross(BR_VEC3(ab.x, ab.y, 0), BR_VEC3(cb.x, cb.y, 0)).z > 0;
}

static inline br_vec2_t br_vec2_lerp(br_vec2_t a, br_vec2_t b, float t) {
  return BR_VEC2(a.x*(1-t) + b.x*t,
                 a.y*(1-t) + b.y*t);
}

static inline br_size_t br_size_lerp(br_size_t a, br_size_t b, float t) {
  return BR_SIZE(a.width*(1-t) + b.width*t,
                 a.height*(1-t) + b.height*t);
}

static inline float br_vec2_dist2(br_vec2_t a, br_vec2_t b) {
  float len2 = br_vec2_len2(br_vec2_sub(a, b));
  return len2;
}

static inline br_vec3_t br_vec2_transform_scale(br_vec2_t v, br_mat_t mat) {
  br_vec3_t result = { 0 };
  float x = v.x;
  float y = v.y;
  result.x = (mat.m0*x + mat.m4*y + mat.m12);
  result.y = (mat.m1*x + mat.m5*y + mat.m13);
  result.z = (mat.m2*x + mat.m6*y + mat.m14);
  if (fabsf(result.z) > 0.00001f) {
    result.x /= fabsf(result.z);
    result.y /= fabsf(result.z);
  }
  return result;
}

static inline br_vec2_t br_vec2_stog(br_vec2_t v, br_sizei_t screen) {
  return BR_VEC2(v.x / (float)screen.width * 2.f - 1.f, (1.f - v.y / (float)screen.height) * 2.f - 1.f);
}

static inline br_vec2_t br_vec2_normalize(br_vec2_t a) {
  float len2 = br_vec2_len2(a);
  if (fabsf(len2) > FLT_EPSILON) {
    float len = sqrtf(len2);
    for (size_t i = 0; i < BR_VEC_ELS(a); ++i) a.arr[i] /= len;
    return a;
  }
  return BR_VEC2(0,0);
}

//------------------------br_vec2d_t------------------------------

static inline br_vec2d_t br_vec2d_scale(br_vec2d_t v, double s) {
  return BR_VEC2D(v.x*s, v.y*s);
}

static inline br_vec2d_t br_vec2d_add(br_vec2d_t v, br_vec2d_t w) {
  return BR_VEC2D(v.x+w.x, v.y+w.y);
}

static inline br_vec2d_t br_vec2d_sub(br_vec2d_t v, br_vec2d_t w) {
  return BR_VEC2D(v.x-w.x, v.y-w.y);
}

static inline br_vec2d_t br_vec2d_mul(br_vec2d_t a, br_vec2d_t b) {
  return BR_VEC2D(a.x * b.x, a.y * b.y);
}

static inline br_vec2d_t br_vec2d_div(br_vec2d_t v, br_vec2d_t w) {
  return BR_VEC2D(v.x/w.x, v.y/w.y);
}

static inline br_vec2d_t br_vec2d_max(br_vec2d_t v, br_vec2d_t w) {
  return BR_VEC2D(fmax(v.x, w.x), fmax(v.y, w.y));
}

static inline br_vec2d_t br_vec2d_lerp(br_vec2d_t a, br_vec2d_t b, double t) {
  return BR_VEC2D(a.x*(1-t) + b.x*t,
                  a.y*(1-t) + b.y*t);
}


static inline double br_vec2d_len2(br_vec2d_t a) {
  double sum = 0.f;
  for (size_t i = 0; i < BR_VEC_ELS(a); ++i) sum += a.arr[i] * a.arr[i];
  return sum;
}

static inline double br_vec2d_len(br_vec2d_t a) {
  return sqrt(br_vec2d_len2(a));
}

static inline double br_vec2d_dist2(br_vec2d_t a, br_vec2d_t b) {
  double len2 = br_vec2d_len2(br_vec2d_sub(a, b));
  return len2;
}

static inline br_vec2d_t br_vec2d_normalize(br_vec2d_t a) {
  double len2 = br_vec2d_len2(a);
  if (fabs(len2) > DBL_EPSILON) {
    double len = sqrt(len2);
    for (size_t i = 0; i < BR_VEC_ELS(a); ++i) a.arr[i] /= len;
    return a;
  }
  return BR_VEC2D(0,0);
}

//------------------------size------------------------------

static inline br_sizei_t br_sizei_sub(br_sizei_t a, br_sizei_t b) {
  return BR_SIZEI(a.width - b.width, a.height - b.height);
}

static inline br_size_t br_size_sub(br_size_t a, br_size_t b) {
  return BR_SIZE(a.width - b.width, a.height - b.height);
}

static inline br_size_t br_size_subv(br_size_t a, br_vec2_t b) {
  return BR_SIZE(a.width - b.x, a.height - b.y);
}

static inline br_sizei_t br_sizei_subvi(br_sizei_t a, br_vec2i_t b) {
  return BR_SIZEI(a.width - b.x, a.height - b.y);
}

static inline br_size_t br_size_addv(br_size_t a, br_vec2_t b) {
  return BR_SIZE(a.width + b.x, a.height + b.y);
}

static inline br_size_t br_size_scale(br_size_t a, float b) {
  return BR_SIZE(a.width * b, a.height * b);
}

//------------------------vec3------------------------------

static inline br_vec3_t br_vec3_add(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline br_vec3_t br_vec3_sub(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline br_vec3_t br_vec3_max(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z));
}

static inline br_vec3_t br_vec3_scale(br_vec3_t a, float s) {
  return BR_VEC3(a.x * s, a.y * s, a.z * s);
}

static inline br_vec3_t br_vec3_pow(br_vec3_t a, float exponent) {
  return BR_VEC3(powf(a.x, exponent), powf(a.y, exponent), powf(a.z, exponent));
}

static inline float br_vec3_len2(br_vec3_t a) {
  float sum = 0.f;
  for (size_t i = 0; i < BR_VEC_ELS(a); ++i) sum += a.arr[i] * a.arr[i];
  return sum;
}

static inline br_vec3_t br_vec3_abs(br_vec3_t a) {
  return BR_VEC3(fabsf(a.x), fabsf(a.y), fabsf(a.z));
}

static inline float br_vec3_len(br_vec3_t a) {
  return sqrtf(br_vec3_len2(a));
}

static inline float br_vec3_dist2(br_vec3_t a, br_vec3_t b) {
  float len2 = br_vec3_len2(br_vec3_sub(a, b));
  return len2;
}

static inline float br_vec3_dist(br_vec3_t a, br_vec3_t b) {
  return sqrtf(br_vec3_dist2(a, b));
}

static inline br_vec3_t br_vec3_mul(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(a.x*b.x, a.y*b.y, a.z*b.z);
}

static inline float br_vec3_dot(br_vec3_t a, br_vec3_t b) {
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline br_vec3_t br_vec3_cross(br_vec3_t a, br_vec3_t b) {
  return BR_VEC3(
    a.y*b.z - a.z*b.y,
    a.z*b.x - a.x*b.z,
    a.x*b.y - a.y*b.x);
}

static inline bool br_vec3_ccv(br_vec3_t a, br_vec3_t b, br_vec3_t c) {
  br_vec3_t ab = br_vec3_sub(b, a);
  br_vec3_t cb = br_vec3_sub(b, c);
  return br_vec3_cross(ab, cb).z > 0;
}

static inline br_vec3_t br_vec3_normalize(br_vec3_t a) {
  float len2 = br_vec3_len2(a);
  if (fabsf(len2) > FLT_EPSILON) {
    float len = sqrtf(len2);
    for (size_t i = 0; i < BR_VEC_ELS(a); ++i) a.arr[i] /= len;
    return a;
  }
  return BR_VEC3(0,0,0);
}

// TODO: This look sus...
static inline br_vec3_t br_vec3_transform_scale(br_vec3_t v, br_mat_t mat) {
  br_vec3_t result = { 0 };

  float x = v.x, y = v.y, z = v.z;

  result.x = mat.m0*x + mat.m4*y + mat.m8*z + mat.m12;
  result.y = mat.m1*x + mat.m5*y + mat.m9*z + mat.m13;
  result.z = mat.m2*x + mat.m6*y + mat.m10*z + mat.m14;
  float w =  mat.m3*x + mat.m7*y + mat.m11*z + mat.m15;

  if (fabsf(w) > 0.00001f) {
    result.x /= w;
    result.y /= w;
    result.z /= w;
  }

  return result;
}

static inline float br_vec3_angle(br_vec3_t v1, br_vec3_t v2) {
  float len = br_vec3_len(br_vec3_cross(v1, v2));
  float dot = br_vec3_dot(v1, v2);
  return atan2f(len, dot);
}

static inline br_vec3_t br_vec3_rot(br_vec3_t v, br_vec3_t axis, float angle) { 
  float as = sinf(angle);
  // v + sin(angle)*axis x v + (1 - cos(angle))*axis x ( axis x v )
   br_vec3_t a = br_vec3_cross(br_vec3_scale(axis, as), v);
   br_vec3_t b = br_vec3_cross(br_vec3_scale(axis, (1 - cosf(angle))), br_vec3_cross(axis, v));
   return br_vec3_add(v, br_vec3_add(a, b));
}

static inline br_vec3_t br_vec3_perpendicular(br_vec3_t v) {
  size_t min = 0;
  float min_val = fabsf(v.arr[0]);
  br_vec3_t ord = { 0 };

  for (size_t i = 1; i < 3; ++i) {
    float c = fabsf(v.arr[i]);
    if (c < min_val) min = i, min_val = c;
  }
  ord.arr[min] = 1;
  return br_vec3_cross(v, ord);
}

static inline br_vec3_t br_vec3_clamp(br_vec3_t v, float m, float M) {
  v.x = br_float_clamp(v.x, m, M);
  v.y = br_float_clamp(v.y, m, M);
  v.z = br_float_clamp(v.z, m, M);
  return v;
}

// ------------------br_color_t-----------------

static inline br_color_t br_color_lighter(br_color_t c, float factor) {
  br_vec3_t cv = BR_COLOR_TO4(c).xyz;
  br_vec3_t newc = br_vec3_clamp(br_vec3_scale(cv, factor + 1.f), 0.f, 1.f);
  return BR_VEC3_TOC(newc, c.a);
}

static inline br_color_t br_color_darker(br_color_t c, float factor) {
  return br_color_lighter(c, -factor);
}

static inline br_color_t br_color_greener(br_color_t c, float factor) {
  br_vec3_t cv = BR_COLOR_TO4(c).xyz;
  float g = (1+factor);
  float g2 = g*g;
  float k2 = 1 + (cv.y*cv.y)*(1.f - g2)/(cv.x*cv.x+cv.z*cv.z);
  float k = sqrtf(k2);
  cv.x *= k;
  cv.y *= g;
  cv.z *= k;
  return BR_VEC3_TOC(cv, c.a);
}

// ------------------br_extent_t-----------------

static inline br_vec2_t br_extent_tr(br_extent_t extent) {
  return BR_VEC2(extent.x + extent.width, extent.y);
}

static inline br_vec2_t br_extent_bl(br_extent_t extent) {
  return BR_VEC2(extent.x, extent.y + extent.height);
}

static inline br_vec2_t br_extent_br(br_extent_t extent) {
  return BR_VEC2(extent.x + extent.width, extent.y + extent.height);
}

static inline br_vec2_t br_extent_tl2(br_extent_t extent, float x, float y) {
  return BR_VEC2(extent.x + x, extent.y + y);
}

static inline br_vec2_t br_extent_tr2(br_extent_t extent, float x, float y) {
  return BR_VEC2(extent.x + extent.width - x, extent.y + y);
}

static inline bool br_extent_eq(br_extent_t a, br_extent_t b) {
  if (a.x != b.x) return false;
  if (a.y != b.y) return false;
  if (a.width != b.width) return false;
  if (a.height != b.height) return false;
  return true;
}

static inline bool br_extenti_eq(br_extenti_t a, br_extenti_t b) {
  if (a.x != b.x) return false;
  if (a.y != b.y) return false;
  if (a.width != b.width) return false;
  if (a.height != b.height) return false;
  return true;
}

static inline br_extent_t br_extent_add(br_extent_t a, br_vec2_t b) {
  return BR_EXTENT(a.x + b.x, a.y + b.y, a.width, a.height);
}

static inline br_extent_t br_extent_sub(br_extent_t a, br_vec2_t b) {
  return BR_EXTENT(a.x - b.x, a.y - b.y, a.width, a.height);
}

static inline br_extent_t br_extent_lerp(br_extent_t a, br_extent_t b, float x) {
  return BR_EXTENT(
    (int)a.x == (int)b.x ? b.x : br_float_lerp(a.x, b.x, x),
    (int)a.y == (int)b.y ? b.y : br_float_lerp(a.y, b.y, x),
    (int)a.width == (int)b.width ? b.width : br_float_lerp(a.width, b.width, x),
    (int)a.height == (int)b.height ? b.height : br_float_lerp(a.height, b.height, x)
  );
}

// ------------------br_mat_t--------------------
static inline br_mat_t br_mat_perspective(float fovY, float aspect, float nearPlane, float farPlane) {
  br_mat_t result = { 0 };

  float top = nearPlane*tanf(fovY*0.5f);
  float bottom = -top;
  float right = top*aspect;
  float left = -right;

  // TODO: Check this out..
  // MatrixFrustum(-right, right, -top, top, near, far);
  float rl = (right - left);
  float tb = (top - bottom);
  float fn = (farPlane - nearPlane);

  result.m0 = (nearPlane*2.0f)/rl;
  result.m5 = (nearPlane*2.0f)/tb;
  result.m8 = (right + left)/rl;
  result.m9 = (top + bottom)/tb;
  result.m10 = -(farPlane + nearPlane)/fn;
  result.m11 = -1.0f;
  result.m14 = -(farPlane*nearPlane*2.0f)/fn;

  return result;
}

static inline br_mat_t br_mat_look_at(br_vec3_t eye, br_vec3_t target, br_vec3_t up) {
  br_vec3_t vz = br_vec3_normalize(br_vec3_sub(eye, target));
  br_vec3_t vx = br_vec3_normalize(br_vec3_cross(up, vz));
  br_vec3_t vy = br_vec3_cross(vz, vx);

  return (br_mat_t) { .rows = {
    BR_VEC4(vx.x, vx.y, vx.z, -br_vec3_dot(vx, eye)),
    BR_VEC4(vy.x, vy.y, vy.z, -br_vec3_dot(vy, eye)),
    BR_VEC4(vz.x, vz.y, vz.z, -br_vec3_dot(vz, eye)),
    BR_VEC4(   0,    0,    0,                   1.f)
  }};
}

static inline br_mat_t br_mat_mul(br_mat_t left, br_mat_t right) {
  br_mat_t result = { 0 };

  result.m0 = left.m0*right.m0 + left.m1*right.m4 + left.m2*right.m8 + left.m3*right.m12;
  result.m1 = left.m0*right.m1 + left.m1*right.m5 + left.m2*right.m9 + left.m3*right.m13;
  result.m2 = left.m0*right.m2 + left.m1*right.m6 + left.m2*right.m10 + left.m3*right.m14;
  result.m3 = left.m0*right.m3 + left.m1*right.m7 + left.m2*right.m11 + left.m3*right.m15;
  result.m4 = left.m4*right.m0 + left.m5*right.m4 + left.m6*right.m8 + left.m7*right.m12;
  result.m5 = left.m4*right.m1 + left.m5*right.m5 + left.m6*right.m9 + left.m7*right.m13;
  result.m6 = left.m4*right.m2 + left.m5*right.m6 + left.m6*right.m10 + left.m7*right.m14;
  result.m7 = left.m4*right.m3 + left.m5*right.m7 + left.m6*right.m11 + left.m7*right.m15;
  result.m8 = left.m8*right.m0 + left.m9*right.m4 + left.m10*right.m8 + left.m11*right.m12;
  result.m9 = left.m8*right.m1 + left.m9*right.m5 + left.m10*right.m9 + left.m11*right.m13;
  result.m10 = left.m8*right.m2 + left.m9*right.m6 + left.m10*right.m10 + left.m11*right.m14;
  result.m11 = left.m8*right.m3 + left.m9*right.m7 + left.m10*right.m11 + left.m11*right.m15;
  result.m12 = left.m12*right.m0 + left.m13*right.m4 + left.m14*right.m8 + left.m15*right.m12;
  result.m13 = left.m12*right.m1 + left.m13*right.m5 + left.m14*right.m9 + left.m15*right.m13;
  result.m14 = left.m12*right.m2 + left.m13*right.m6 + left.m14*right.m10 + left.m15*right.m14;
  result.m15 = left.m12*right.m3 + left.m13*right.m7 + left.m14*right.m11 + left.m15*right.m15;

  return result;
}

// ------------------collisions--------------------
static inline bool br_col_vec2_extent(br_extent_t ex, br_vec2_t point) {
  return (point.x >= ex.x) && (point.x < (ex.x + ex.width)) && (point.y >= ex.y) && (point.y < (ex.y + ex.height));
}
static inline bool br_col_vec2_bb(br_bb_t ex, br_vec2_t point) {
  return (point.x >= ex.min_x) && (point.x < (ex.max_x)) && (point.y >= ex.min_y) && (point.y < ex.max_y);
}

static inline bool br_col_extents(br_extent_t a, br_extent_t b) {
  return ((a.x < (b.x + b.width) && (a.x + a.width) > b.x) &&
          (a.y < (b.y + b.height) && (a.y + a.height) > b.y));
}

// ----------------br_bb_t-------------------------

static inline br_vec2_t br_bb_tr(br_bb_t bb) {
  return BR_VEC2(bb.max.x, bb.min.y);
}

static inline br_vec2_t br_bb_bl(br_bb_t bb) {
  return BR_VEC2(bb.min.x, bb.max.y);
}

static inline br_bb_t br_bb_sub(br_bb_t bb, br_vec2_t vec) {
  return (br_bb_t) { .min = br_vec2_sub(bb.min, vec), .max = br_vec2_sub(bb.max, vec) };
}

static inline br_bb_t br_bb_add(br_bb_t bb, br_vec2_t v) {
  return (br_bb_t) { .min = br_vec2_add(bb.min, v), .max = br_vec2_add(bb.max, v) };
}

static inline br_bb_t br_bb_union(br_bb_t b1, br_bb_t b2) {
  return BR_BB2(br_vec2_min(b1.min, b2.min), br_vec2_max(b1.max, b2.max));
}

static inline br_vec4_t br_bb_clip_dists(br_bb_t limit, br_vec2_t pos) {
  br_vec4_t p = BR_VEC4(
      (pos.x - limit.min_x),
      (pos.y - limit.min_y),
      (limit.max_x - pos.x),
      (limit.max_y - pos.y)
  );
  return p;
}

static inline br_bb_t br_bb_expand_with_point(br_bb_t bb, br_vec2_t v) {
  return BR_BB2(br_vec2_min(bb.min, v), br_vec2_max(bb.max, v));
}

// ----------------int-------------------------

static inline int br_i_min(int a, int b) {
  return a < b ? a : b;
}

static inline int br_i_max(int a, int b) {
  return a > b ? a : b;
}

static inline int br_i_max3(int a, int b, int c) {
  return br_i_max(br_i_max(a, b), c);
}
