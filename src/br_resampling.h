#pragma once
#include "src/br_data.h"
#include "src/br_mesh.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_resampling_t br_resampling_t;
typedef struct br_plot_t br_plot_t;
typedef struct br_plot_data_t br_plot_data_t;

void br_resampling_construct(void);
br_resampling_t* br_resampling_malloc(br_data_kind_t kind);
void br_resampling_empty(br_resampling_t* res);
void br_resampling_free(br_resampling_t* res);
void br_resampling_draw(br_resampling_t* res, br_data_t const* pg, br_plot_t* rdi, br_plot_data_t const* pd);
// TODO: index should be size_t...
void br_resampling_add_point(br_resampling_t* res, br_data_t const* pg, uint32_t index);
void br_resampling_reset(br_resampling_t* res);
void br_resampling_change_something(br_datas_t pg);
// In seconds
double br_resampling_get_draw_time(br_resampling_t* res);
float br_resampling_get_something(br_resampling_t* res);
float br_resampling_get_something2(br_resampling_t* res);


// Internal
typedef struct {
  bool has_old;
  br_vec2_t old;
  br_vec2_t mid;
  br_mesh_line_t args;
} br_line_culler_t;

typedef struct br_resampling_nodes_t {
  uint32_t index_start, len;
  size_t child1;
  size_t child2;
  uint32_t depth;
  uint32_t min_index_x, max_index_x;
  uint32_t min_index_y, max_index_y;
} br_resampling_nodes_t;

typedef struct br_resampling_nodes_2d_t {
  br_resampling_nodes_t base;
} br_resampling_nodes_2d_t;

typedef struct br_resampling_nodes_3d_t {
  br_resampling_nodes_t base;
  uint32_t min_index_z, max_index_z;
  br_vec3_t curvature;
} br_resampling_nodes_3d_t;

typedef struct br_resampling_nodes_2d_allocator_t {
  br_resampling_nodes_2d_t* arr;
  size_t len, cap;
} br_resampling_nodes_2d_allocator_t;

typedef struct br_resampling_nodes_3d_allocator_t {
  br_resampling_nodes_3d_t* arr;
  size_t len, cap;
} br_resampling_nodes_3d_allocator_t;

typedef struct br_resampling_nodes_common_t {
  br_resampling_nodes_t* arr;
  size_t len, cap;
} br_resampling_nodes_common_t;

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
  uint32_t draw_count;
  br_line_culler_t culler;
  br_mesh_line_3d_t args_3d;
} br_resampling_t;

#ifdef __cplusplus
}
#endif

