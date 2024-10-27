#pragma once

#define BR_VEC2(X, Y) ((br_vec2_t) { .x = (float)(X), .y = (float)(Y) })
#define BR_VEC2_TOI(V) ((br_vec2i_t) { .x = (int)(V).x, .y = (int)(V).y })
#define BR_VEC2_SUB(A, B) ((br_vec2_t) { .x = (A).x - (B).x, .y = (A).y - (B).y })
#define BR_VEC2I_SUB(A, B) ((br_vec2i_t) { .x = (A).x - (B).x, .y = (A).y - (B).y })
#define BR_VEC2I_SCALE(V, B) ((br_vec2i_t) { .x = (V).x * (B), .y = (V).y * (B) })
#define BR_SIZEI(WIDTH, HEIGHT) ((br_sizei_t) { .width = (WIDTH), .height = (HEIGHT) })
#define BR_EXTENTI_TOF(E) ((br_extent_t) { .arr = { (float)(E).arr[0], (float)(E).arr[1], (float)(E).arr[2], (float)(E).arr[3] } })
#define BR_EXTENT_REC(E) ((Rectangle) { .x = (E).pos.x, .y = (E).pos.y, .width = (E).size.width, .height = (E).size.height })
#define BR_EXTENT_ASPECT(E) ((float)(E).height / (float)(E).width)
#define BR_EXTENTI(X, Y, WIDTH, HEIGHT) (br_extenti_t) { .arr = { (X), (Y), (WIDTH), (HEIGHT) } }

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

static inline br_vec2_t br_vec2i_tof(br_vec2i_t v) {
  return (br_vec2_t) { .x = (float)v.x, .y = (float)v.y };
}

static inline br_vec2i_t br_vec2_toi(br_vec2_t v) {
  return (br_vec2i_t) { .x = (int)v.x, .y = (int)v.y };
}

static inline br_vec2_t br_vec2_scale(br_vec2_t v, float s) {
  return ((br_vec2_t) { .x = v.x * s, .y = v.y * s });
}
