#pragma once
#include "br_data.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct resampling2_t resampling2_t;
typedef struct br_plot_t br_plot_t;

void br_resampling2_construct(void);
resampling2_t* resampling2_malloc(br_data_kind_t kind);
void resampling2_empty(resampling2_t* res);
void resampling2_free(resampling2_t* res);
void resampling2_draw(resampling2_t* res, br_data_t const* pg, br_plot_t* rdi, br_shaders_t* shaders);
// TODO: index should be size_t...
void resampling2_add_point(resampling2_t* res, br_data_t const* pg, uint32_t index);
void resampling2_reset(resampling2_t* res);
void resampling2_change_something(br_datas_t pg);
// In seconds
double br_resampling2_get_draw_time(resampling2_t* res);
float br_resampling2_get_something(resampling2_t* res);
float br_resampling2_get_something2(resampling2_t* res);


// Internal
typedef struct {
  bool has_old;
  br_vec2_t old;
  br_vec2_t mid;
} br_line_culler_t;

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
  br_vec3_t curvature;
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
  br_line_culler_t culler;
} resampling2_t;

bool resampling2_nodes_2d_is_inside(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, br_extent_t rect);
bool resampling2_nodes_2d_is_inside_3d(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mat);
br_vec2_t resampling2_nodes_2d_get_ratios(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, float screen_width, float screen_height);
br_vec2_t resampling2_nodes_2d_get_ratios_3d(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mvp);

bool resampling2_nodes_3d_is_inside(resampling2_nodes_3d_t const* res, br_data_3d_t const* data, br_mat_t mvp);
br_vec2_t resampling2_nodes_3d_get_ratios(resampling2_nodes_3d_t const* res, br_data_3d_t const* data, br_vec3_t eye, br_vec3_t look_dir);

#ifdef __cplusplus
}
#endif

