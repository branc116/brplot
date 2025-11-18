#pragma once
#include "src/br_data.h"
#include "src/br_mesh.h"

#include <stdint.h>

typedef struct br_resampling_t br_resampling_t;
typedef struct br_plot_t br_plot_t;
typedef struct br_plot_data_t br_plot_data_t;
typedef struct br_shaders_t br_shaders_t;

void br_resampling_construct(br_shaders_t* shaders, float* min_something, float* cull_min);
br_resampling_t* br_resampling_malloc(br_data_kind_t kind);
void br_resampling_empty(br_resampling_t* res);
void br_resampling_free(br_resampling_t* res);
void br_resampling_draw(br_resampling_t* res, br_data_t const* pg, br_plot_t* rdi, br_plot_data_t const* pd, br_extent_t extent);
// TODO: index should be size_t...
void br_resampling_add_point(br_resampling_t* res, br_data_t const* pg, uint32_t index);

bool br_resampling_get_point_at2(br_data_t data, br_vec2d_t vec, float* dist, br_u32* out_index);

void br_resampling_reset(br_resampling_t* res);
void br_resampling_change_something(br_datas_t pg);
// In seconds
double br_resampling_get_draw_time(br_resampling_t* res);
float br_resampling_get_something(br_resampling_t* res);
float br_resampling_get_something2(br_resampling_t* res);


typedef struct br_resampling_nodes_t {
  br_u32 index_start, len;
  br_u32 child1;
  br_u32 child2;
  br_u32 depth;
  br_u32 min_index_x, max_index_x;
  br_u32 min_index_y, max_index_y;
} br_resampling_nodes_t;

typedef struct br_resampling_nodes_2d_t {
  br_resampling_nodes_t base;
} br_resampling_nodes_2d_t;

typedef struct br_resampling_nodes_3d_t {
  br_resampling_nodes_t base;
  br_u32 min_index_z, max_index_z;
  br_vec3_t curvature;
} br_resampling_nodes_3d_t;

typedef struct br_resampling_nodes_2d_allocator_t {
  br_resampling_nodes_2d_t* arr;
  br_u32 len, cap;
} br_resampling_nodes_2d_allocator_t;

typedef struct br_resampling_nodes_3d_allocator_t {
  br_resampling_nodes_3d_t* arr;
  br_u32 len, cap;
} br_resampling_nodes_3d_allocator_t;

typedef struct br_resampling_nodes_common_t {
  br_resampling_nodes_t* arr;
  br_u32 len, cap;
} br_resampling_nodes_common_t;

// Internal
typedef struct {
  bool has_old;
  br_vec2_t old;
  br_vec2_t mid;
  br_mesh_line_t args;
} br_line_culler_t;

typedef struct br_resampling_t {
  br_data_kind_t kind;
  union {
    br_resampling_nodes_2d_allocator_t dd;
    br_resampling_nodes_3d_allocator_t ddd;
    br_resampling_nodes_common_t common;
  };
  double render_time;
  float something;
  float something2;
  br_u32 draw_count;
  br_line_culler_t culler;
  br_mesh_line_3d_t args_3d;
} br_resampling_t;

