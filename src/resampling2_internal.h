#pragma once
#include "br_data.h"

#include "raylib.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct resampling2_nodes_t {
  uint32_t index_start, len;
  size_t child1;
  size_t child2;
  uint32_t depth;
  uint32_t min_index_x, max_index_x;
  uint32_t min_index_y, max_index_y;
} resampling2_nodes_t;

typedef struct resampling2_nodes_2d_t {
  resampling2_nodes_t base;
} resampling2_nodes_2d_t;

typedef struct resampling2_nodes_3d_t {
  resampling2_nodes_t base;
  uint32_t min_index_z, max_index_z;
  Vector3 curvature;
} resampling2_nodes_3d_t;

typedef struct resampling2_nodes_2d_allocator_t {
  resampling2_nodes_2d_t* arr;
  size_t len, cap;
} resampling2_nodes_2d_allocator_t;

typedef struct resampling2_nodes_3d_allocator_t {
  resampling2_nodes_3d_t* arr;
  size_t len, cap;
} resampling2_nodes_3d_allocator_t;

typedef struct resampling2_nodes_common_t {
  resampling2_nodes_t* arr;
  size_t len, cap;
} resampling2_nodes_common_t;

typedef struct resampling2_t {
  br_data_kind_t kind;
  union {
    resampling2_nodes_2d_allocator_t dd;
    resampling2_nodes_3d_allocator_t ddd;
    resampling2_nodes_common_t common;
  };
  double render_time;
  float something;
  float something2;
  uint32_t draw_count;
} resampling2_t;

bool resampling2_nodes_2d_is_inside(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, Rectangle rect);
bool resampling2_nodes_2d_is_inside_3d(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, Matrix mat);
Vector2 resampling2_nodes_2d_get_ratios(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, float screen_width, float screen_height);
Vector2 resampling2_nodes_2d_get_ratios_3d(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, Matrix mvp);

bool resampling2_nodes_3d_is_inside(resampling2_nodes_3d_t const* res, br_data_3d_t const* data, Matrix mvp);
Vector2 resampling2_nodes_3d_get_ratios(resampling2_nodes_3d_t const* res, br_data_3d_t const* data, Vector3 eye, Vector3 look_dir);
