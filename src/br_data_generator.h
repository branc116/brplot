#pragma once
#include "br_data.h"
#include <stdio.h>

typedef enum {
  br_dagen_state_inprogress,
  br_dagen_state_finished,
  br_dagen_state_failed,
} br_dagen_state_t;

typedef enum {
  br_dagen_kind_file,
  br_dagen_kind_expr,
} br_dagen_kind_t;

typedef struct {
  FILE* file;
  size_t x_left, y_left, z_left;
} br_dagen_file_t;

typedef enum {
  br_dagen_expr_kind_reference_x,
  br_dagen_expr_kind_reference_y,
  br_dagen_expr_kind_reference_z,
  br_dagen_expr_kind_add,
  br_dagen_expr_kind_mul,
} br_dagen_expr_kind_t;

typedef struct br_dagen_expr_t {
  br_dagen_expr_kind_t kind;
  union {
    int group_id;
    struct {
      uint32_t op1;
      uint32_t op2;
    } operands;
  };
} br_dagen_expr_t;

typedef struct {
  br_dagen_expr_t* arr;
  size_t len, cap;
} br_dagen_exprs_t;

typedef struct br_dagen_2d_expr_t {
  uint32_t x_expr_index, y_expr_index;
  br_dagen_exprs_t arena;
} br_dagen_expr_2d_t;

typedef struct {
  br_dagen_kind_t kind;
  br_data_kind_t data_kind;
  br_dagen_state_t state;
  int group_id;
  union {
    br_dagen_file_t file;
    br_dagen_expr_2d_t expr_2d;
  };
} br_dagen_t;

typedef struct br_dagens_t {
  br_dagen_t* arr;
  size_t len, cap;
} br_dagens_t;

typedef struct br_plotter_t br_plotter_t;

#if defined(__cplusplus)
extern "C" {
#endif
bool br_dagen_push_expr_xy(br_dagens_t* dagens, br_datas_t* datas, br_dagen_exprs_t arena, uint32_t x, uint32_t y, int group_id);
bool br_dagens_add_expr_str(br_dagens_t* dagens, br_datas_t* datas, br_strv_t str, int group_id);
bool br_dagen_push_file(br_dagens_t* dagens, br_datas_t* datas, br_data_desc_t* temp_data, FILE* file);
void br_dagens_remove_dependent(br_datas_t* datas, int group_id);
void br_dagens_handle(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots, double until);
void br_dagens_free(br_dagens_t* dagens);
#if defined(__cplusplus)
}
#endif
