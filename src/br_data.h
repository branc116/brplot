#pragma once
#include "src/br_pp.h"
#include "include/br_str_header.h"
#include "src/br_math.h"

typedef struct br_resampling_t br_resampling_t;
typedef struct br_plot_t br_plot_t;
typedef struct br_plots_t br_plots_t;
typedef struct br_dagens_t br_dagens_t;
typedef int brsp_id_t;

#if !defined(BR_DATA_KIND_T)
typedef enum {
  br_data_kind_2d,
  br_data_kind_3d
} br_data_kind_t;
#define BR_DATA_KIND_T
#endif

typedef struct br_data_t {
  br_resampling_t* resampling;
  br_data_kind_t kind;
  int group_id;
  br_color_t color;
  brsp_id_t name;
  bool is_new;

  int series_handles[4]; // Max 4 series per data.. Why have more?
  int serieses_len;
} br_data_t;

typedef struct br_datas_t {
  br_data_t* arr;
  int* free_arr;

  int cap, len;
  int free_len, free_next;
} br_datas_t;

typedef struct brsp_t brsp_t;
typedef struct br_anims_t br_anims_t;
typedef struct br_serieses_t br_serieses_t;
void br_data_construct(brsp_t* sp, br_anims_t* anims, br_serieses_t* serieses);

int br_datas_get_new_id(br_datas_t* datas);
br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind);
br_data_t* br_data_get(br_datas_t* datas, int group);
br_data_t* br_data_get1(br_datas_t datas, int group);
br_data_t* br_data_get2(br_datas_t* datas, int group, br_data_kind_t kind);
void br_data_set_name(br_datas_t* datas, int group, br_str_t name);
void br_data_push_y(br_datas_t* datas, double y, int group);
void br_data_push_x(br_datas_t* datas, double x, int group);
void br_data_push_xy(br_datas_t* datas, double x, double y, int group);
void br_data_push_xyz(br_datas_t* datas, double x, double y, double z, int group);

float br_data_local(br_data_t data, br_u32 series_index, br_u32 index);

br_vec2d_t br_data_el_xy(br_datas_t datas, int group, br_u32 index);
br_vec2d_t br_data_el_xy1(br_data_t data, br_u32 index);
br_vec3d_t br_data_el_xyz(br_datas_t datas, int group, br_u32 index);
br_vec3d_t br_data_el_xyz1(br_data_t data, br_u32 index);
br_vec3d_t br_data_el_xyz2(br_data_t data, br_u32 index);

br_vec3_t br_data_v3_local(br_data_t data, br_u32 index);
br_vec2_t br_data_v2_local(br_data_t data, br_u32 index);
br_vec2_t br_data_v2_to_local(br_data_t data, br_vec2d_t v);
br_vec3_t br_data_v3_to_local(br_data_t data, br_vec3d_t v);

br_vec2d_t br_data_zoom_to_local(br_data_t data, br_vec2d_t v);
br_extentd_t br_data_ex_to_local(br_data_t data, br_extentd_t ex);

br_bb_t br_data_bb(br_data_t data);
br_mat_t br_data_model_mat(br_data_t data);

float* br_data_series_local(br_data_t data, int series_index);

// Only remove all points from a group, don't remove the group itself.
void br_data_empty(br_data_t* data);
void br_data_remove(br_datas_t* datas, int data_id);
size_t br_data_len(br_data_t data);
void br_data_export(br_data_t data, FILE* file);
void br_data_export_csv(br_data_t data, FILE* file);
void br_datas_draw(br_datas_t datas, br_plot_t* plot, br_extent_t extent);
void br_datas_add_test_points(br_datas_t* datas);
void br_data_deinit(br_data_t* data);
void br_datas_deinit(br_datas_t* datas);
// Only remove all points from all groups, don't remove groups themselfs.
void br_datas_empty(br_datas_t datas);
void br_datas_export(br_datas_t datas, FILE* file);
void br_datas_export_csv(br_datas_t datas, FILE* file);
br_color_t br_data_get_default_color(int data_id);
bool br_data_is_generated(br_dagens_t const* dagens, int groups_id);
bool br_data_malloc_axis(br_data_t* data, size_t len);
void br_data_free_axis(br_data_t* data);

size_t br_data_element_size(br_data_kind_t kind);

