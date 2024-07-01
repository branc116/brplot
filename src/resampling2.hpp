#pragma once
#include "br_data.h"

#include "raylib.h"

#include <stdint.h>
#include <stdlib.h>

struct resampling2_nodes_t {
  uint32_t index_start = 0, len = 0;
  size_t child1 = 0;
  size_t child2 = 0;
  uint32_t depth = 0;
  uint32_t min_index_x = 0, max_index_x = 0;
  uint32_t min_index_y = 0, max_index_y = 0;
};

struct resampling2_nodes_2d_t {
  resampling2_nodes_t base;

  bool is_inside(float const* xs, float const* ys, Rectangle rect) const;
  bool is_inside_3d(float const* xs, float const* ys, Matrix mat) const;
  Vector2 get_ratios(float const* xs, float const* ys, float screen_width, float screen_height) const;
  Vector2 get_ratios_3d(float const* xs, float const* ys, Matrix mvp) const;
};


struct resampling2_nodes_3d_t {
  resampling2_nodes_t base;
  uint32_t min_index_z = 0, max_index_z = 0;
  Vector3 curvature = {0, 0, 0};

  bool is_inside(br_data_3d_t const* data, Matrix mvp) const;
  Vector2 get_ratios(br_data_3d_t const* data, Vector3 eye, Vector3 look_dir) const;
};

template<typename arr_t>
struct resampling2_nodes_allocator_t {
  arr_t * arr = NULL;
  size_t len = 0, cap = 0;
};

using resampling2_nodes_2d_allocator_t = resampling2_nodes_allocator_t<resampling2_nodes_2d_t>;
using resampling2_nodes_3d_allocator_t = resampling2_nodes_allocator_t<resampling2_nodes_3d_t>;
typedef struct resampling2_t {
  br_data_kind_t kind;
  union {
    resampling2_nodes_2d_allocator_t dd;
    resampling2_nodes_3d_allocator_t ddd;
    resampling2_nodes_allocator_t<void> common;
  };
  double render_time = 0.0;
  float something = 0.02f;
  float something2 = 0.001f;
  uint32_t draw_count = 0;
} resampling2_t;
