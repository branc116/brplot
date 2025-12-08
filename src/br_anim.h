#pragma once
#include "src/br_pp.h"
#include "src/br_math.h"

#include <stdbool.h>
#include <stdio.h>

typedef enum br_anim_kind_t {
  br_anim_float,
  br_anim_extent
} br_anim_kind_t;

typedef struct br_anim_t {
  br_anim_kind_t kind:8;
  bool is_alive:1;
  bool is_instant:1;
  bool trace:1;
  union {
    struct {
      float current;
      float target;
    } f;
    struct {
      br_extent_t current;
      br_extent_t target;
    } ex;
  };
} br_anim_t;

typedef struct br_anims_alive_t {
  br_i32* arr;
  br_i32* free_arr;
  br_i32 len, cap;
  br_i32 free_len;
  br_i32 free_next;
} br_anims_alive_t;

typedef struct br_anims_all_t {
  br_anim_t* arr;
  br_i32* free_arr;
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

void br_anims_tick(br_anims_t* anims, float dt);

int br_animf_new(br_anims_t* anims, float current, float target);
int br_animex_new(br_anims_t* anims, br_extent_t current, br_extent_t target);

void br_anim_delete(br_anims_t* anims, int anim_handle);
bool br_anim_alive(br_anims_t* anims, int anim_handle);
void br_anim_instant(br_anims_t* anims, int anim_handle);
#if BR_DEBUG
void br_anim_trace(br_anims_t* anims, int anim_handle);
#else
#define br_anim_trace(...)
#endif


void br_animf_set(br_anims_t* anims, int anim_handle, float target_value);
float br_animf(br_anims_t* anims, int anim_handle);

void br_animex_set(br_anims_t* anims, int anim_handle, br_extent_t target_value);
br_extent_t br_animex(br_anims_t* anims, int anim_handle);
br_extent_t br_animex_get_target(br_anims_t* anims, int anim_handle);
br_extent_t br_anim_rebase(br_anims_t* anims, int anim_handle, br_vec2_t rebase_for);

bool br_anim_save(FILE* file, const br_anims_t* anims);
bool br_anim_load(FILE* file, br_anims_t* anims);
