#pragma once
#include "src/br_pp.h"

#define BR_SERIES_SUPPORT_CAP 16UL

typedef struct br_series_t {
  float* arr;
  br_u64 len, cap;

  double* support;

  double offset;
  double scale;

  double min, max;
  double average;
} br_series_t;

typedef struct br_series_view_t {
  float* arr;
  br_u64 len;

  br_i32 series_handle;
  br_u64 series_offset;
} br_series_view_t;

typedef struct br_series_view_zero_t {
  double* arr;
  br_u64 len;

  br_i32 series_handle;
  br_u64 series_offset;
} br_series_view_zero_t;

typedef struct br_serieses_t {
  br_series_t* arr;
  int len, cap;

  int* free_arr;
  int free_len, free_next;
} br_serieses_t;

bool br_series_deinit(br_series_t* s);

bool br_series_push(br_series_t* s, double value);

float                 br_series(br_series_t s, br_u64 index);
float*                br_series_local(br_series_t s);
br_series_view_t      br_series_view(br_series_t s, br_u64 index, br_u64 len);
double                br_series_zero(br_series_t s, br_u64 index);
br_series_view_zero_t br_series_view_zero(br_series_t s, br_u64 index, br_u64 len);

float br_series_to_local(br_series_t s, double value);
double br_series_to_zero(br_series_t s, float value);

size_t br_series_len(br_series_t s);

bool br_series_write(BR_FILE* file, br_series_t s);
bool br_series_read (BR_FILE* file, br_series_t* s);

bool br_serieses_write(BR_FILE* file, br_serieses_t s);
bool br_serieses_read (BR_FILE* file, br_serieses_t* s);

// NOTE: These funcions can only be called when the br_serieses_construct was called.
void br_serieses_construct(br_serieses_t* s);
int                   br_serieses_new(void);
void                  br_serieses_delete(int handle);
void                  br_serieses_push(int handle, double value);
void                  br_serieses_push_len(int handle); // Push the current length into the series..
float                 br_serieses_val(int handle, size_t index);
double                br_serieses_zero(int handle, size_t index);
br_u64                br_serieses_len(int handle);
br_series_view_zero_t br_serieses_view_zero(int handle, size_t index, size_t offset);
void                  br_serieses_empty(int handle);
void                  br_serieses_release(int handle);
br_series_view_t      br_serieses_view(int handle, br_u64 index, br_u64 len);
br_series_t           br_serieses_get(int handle);
