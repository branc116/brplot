#pragma once
#include "br_pp.h"
#include "br_str.h"
#include "raylib.h"
#include "br_shaders.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct resampling2_t resampling2_t;
typedef struct br_plot_t br_plot_t;
typedef struct br_plots_t br_plots_t;
typedef struct br_dagens_t br_dagens_t;

typedef struct min_distances_t {
  br_vec2_t graph_point;
  br_vec2_t graph_point_x;
  br_vec2_t graph_point_y;
} min_distances_t;

typedef struct bb_3d_t {
  float xmin, ymin, zmin, xmax, ymax, zmax;
} bb_3d_t;

typedef struct bb_t {
  float xmin, ymin, xmax, ymax;
} bb_t;

#if !defined(BR_DATA_KIND_T)
typedef enum {
  br_data_kind_2d,
  br_data_kind_3d
} br_data_kind_t;
#define BR_DATA_KIND_T
#endif

typedef struct br_data_2d_t {
  float* xs;
  float* ys;
  bb_t bounding_box;
} br_data_2d_t;

typedef struct br_data_3d_t {
  float* xs;
  float* ys;
  float* zs;
  bb_3d_t bounding_box;
} br_data_3d_t;

typedef struct br_data_t {
  resampling2_t* resampling;
  size_t cap, len;
  br_data_kind_t kind;
  int group_id;
  Color color;
  br_str_t name;
  bool is_new;
  union {
    br_data_2d_t dd;
    br_data_3d_t ddd;
  };
} br_data_t;

typedef struct br_datas_t {
  size_t cap, len;
  br_data_t* arr;
} br_datas_t;

typedef struct {
  int group_id;
  br_str_t name;
} br_data_desc_t;

typedef struct {
  br_data_desc_t* arr;
  size_t len, cap;
} br_data_descs_t;

void br_data_construct(void);
int br_datas_get_new_id(br_datas_t* datas);
BR_API br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind);
BR_API br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind);
BR_API br_data_t* br_datas_create2(br_datas_t* datas, int group_id, br_data_kind_t kind, Color color, size_t cap, br_str_t name);
BR_API br_data_t* br_data_get(br_datas_t* pg_array, int group);
BR_API br_data_t* br_data_get1(br_datas_t pg, int group);
BR_API br_data_t* br_data_get2(br_datas_t* pg_array, int group, br_data_kind_t kind);
BR_API void br_data_set_name(br_datas_t* pg_array, int group, br_str_t name);
BR_API void br_data_push_y(br_datas_t* pg, float y, int group);
BR_API void br_data_push_x(br_datas_t* pg, float x, int group);
BR_API void br_data_push_xy(br_datas_t* pg, float x, float y, int group);
BR_API void br_data_push_xyz(br_datas_t* pg, float x, float y, float z, int group);
// TODO: this should be br_plotter_clear_data()
BR_API void br_data_clear(br_datas_t* pg, br_plots_t* plots, int group_id);
// Only remove all points from a group, don't remove the group itself.
BR_API void br_data_empty(br_data_t* pg);
void br_data_export(br_data_t const* pg, FILE* file);
void br_data_export_csv(br_data_t const* pg, FILE* file);
void br_datas_draw(br_datas_t pg_array, br_plot_t* shader, br_shaders_t* shaders);
void br_datas_add_test_points(br_datas_t* pg_array);
void br_datas_deinit(br_datas_t* pg_array);
// Only remove all points from all groups, don't remove groups themselfs.
BR_API void br_datas_empty(br_datas_t* pg_array);
void br_datas_export(br_datas_t const* pg_array, FILE* file);
void br_datas_export_csv(br_datas_t const* pg_array, FILE* file);
Color br_data_get_default_color(int group_id);
bool br_data_is_generated(br_dagens_t const* dagens, int groups_id);
bool br_data_realloc(br_data_t* data, size_t new_cap);

size_t br_data_element_size(br_data_kind_t kind);

#ifdef __cplusplus
}
#endif
