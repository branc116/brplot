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
  size_t data_left;
} br_dagen_file_t;

typedef enum {
  br_dagen_expr_kind_reference_x,
  br_dagen_expr_kind_reference_y,
  br_dagen_expr_kind_reference_z,
} br_dagen_expr_kind_t;

typedef struct br_dagen_expr_t {
  br_dagen_expr_kind_t kind;
  union {
    int group_id;
  };
} br_dagen_expr_t;

typedef struct br_dagen_2d_expr_t {
  br_dagen_expr_t x_expr, y_expr;
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

typedef struct {
  br_dagen_t* arr;
  size_t len, cap;
} br_dagens_t;

typedef struct br_plotter_t br_plotter_t;

#if defined(__cplusplus)
extern "C" {
#endif
void br_dagen_push_expr_xy(br_dagens_t* dagens, br_datas_t* datas, br_dagen_expr_t y, br_dagen_expr_t x, int group_id);
bool br_dagen_push_file(br_dagens_t* dagens, br_data_t* temp_data, FILE* file);
void br_dagens_handle(br_plotter_t* br, double until);
#if defined(__cplusplus)
}
#endif
