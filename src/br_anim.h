#pragma once
#include "src/br_pp.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum br_anim_kind_t {
  br_anim_float,
} br_anim_kind_t;

typedef struct br_anim_t {
  br_anim_kind_t kind:8;
  bool is_alive;
  float current;
  float target;
} br_anim_t;

typedef struct br_anims_alive_t {
  int *arr;
  int *free_arr;
  br_i32 len, cap;
  br_i32 free_len;
  br_i32 free_next;
} br_anims_alive_t;

typedef struct br_anims_all_t {
  br_anim_t *arr;
  int *free_arr;
  br_i32 len, cap;
  br_i32 free_len;
  br_i32 free_next;
} br_anims_all_t;

typedef struct br_anims_t {
  br_anims_all_t all;
  br_anims_alive_t alive;
} br_anims_t;

typedef struct br_theme_t br_theme_t;
void br_anims_construct(br_theme_t* theme);

int br_anim_newf(br_anims_t* anims, float current, float target);
void br_anim_delete(br_anims_t* anims, int anim_handle);

void br_anim_tick(br_anims_t* anims, float dt);

void br_anim_setf(br_anims_t* anims, int anim_handle, float target_value);
float br_anim_getf(br_anims_t* anims, int anim_handle);
float br_anim_getft(br_anims_t* anims, int anim_handle);

bool br_anim_save(FILE* file, const br_anims_t* anims);
bool br_anim_load(FILE* file, br_anims_t* anims);
