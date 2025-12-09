#include "src/br_data.h"
#include "src/br_da.h"
#include "src/br_data_generator.h"
#include "src/br_gui.h"
#include "src/br_plot.h"
#include "src/br_pp.h"
#include "src/br_resampling.h"
#include "include/br_str_header.h"
#include "src/br_math.h"
#include "src/br_string_pool.h"
#include "src/br_memory.h"
#include "src/br_anim.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEF_CAP 1024

static br_data_t* br_data_init(br_data_t* g, int group_id, br_data_kind_t kind);
static void br_data_push_point2(br_data_t* g, br_vec2_t v);
static void br_data_push_point3(br_data_t* g, br_vec3_t v);
static void br_bb_3d_expand_with_point(bb_3d_t* bb, br_vec3_t v);

static BR_THREAD_LOCAL struct {
  brsp_t* sp;
  br_color_t base_colors[8];
  br_anims_t* anims;
} br_data;

void br_data_construct(brsp_t* sp, br_anims_t* anims) {
  br_data.sp = sp;
  br_data.anims = anims;
  br_data.base_colors[0] = BR_RED;
  br_data.base_colors[1] = BR_GREEN;
  br_data.base_colors[2] = BR_BLUE;
  br_data.base_colors[3] = BR_LIGHTGRAY;
  br_data.base_colors[4] = BR_PINK;
  br_data.base_colors[5] = BR_GOLD;
  br_data.base_colors[6] = BR_VIOLET;
  br_data.base_colors[7] = BR_DARKPURPLE;
}

int br_datas_get_new_id(br_datas_t *datas) {
  int max = 1;
  br_data_t data;
  brfl_foreachv(data, *datas) if (data.group_id > max) max = data.group_id;
  return max;
}

br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind) {
  br_data_t data;
  if (NULL == br_data_init(&data, group_id, kind)) return NULL;
  int index = brfl_push(*datas, data);
  return br_da_getp(*datas, index);
}

br_data_t* br_datas_create2(br_datas_t* datas, int group_id, br_data_kind_t kind, br_color_t color, size_t cap, brsp_id_t name) {
  BR_ASSERT(kind == br_data_kind_2d || kind == br_data_kind_3d);

  cap = cap < DEF_CAP ? DEF_CAP : cap;
  br_data_t d = {
    .resampling = br_resampling_malloc(kind),
    .cap = cap, .len = 0, .kind = kind,
    .group_id = group_id,
    .color = color,
    .name = name
  };
  if (NULL == d.resampling)                                                             goto error;
  if (NULL == (d.dd.xs = BR_MALLOC(sizeof(float) * cap)))                               goto error;
  if (NULL == (d.dd.ys = BR_MALLOC(sizeof(float) * cap)))                               goto error;
  if (kind == br_data_kind_3d) if (NULL == (d.ddd.zs = BR_MALLOC(sizeof(float) * cap))) goto error;
  int handle = brfl_push(*datas, d);
  return br_da_getp(*datas, handle);

error:
  br_data_deinit(&d);
  return NULL;
}

void br_data_push_y(br_datas_t* pg_array, double y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  float x = 0.f;
  if (pg->len == 0) {
    pg->dd.rebase_x = 0;
    pg->dd.rebase_y = y;
  } else {
    x = (pg->dd.xs[pg->len - 1] + 1.f);
  }
  br_data_push_point2(pg, (br_vec2_t){ .x = x, .y = (float)(y - pg->dd.rebase_y) });
}

void br_data_push_x(br_datas_t* pg_array, double x, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  float y = 0;
  if (pg->len == 0) {
    pg->dd.rebase_x = x;
    pg->dd.rebase_y = 0;
  } else {
    y = (pg->dd.ys[pg->len - 1] + 1.f);
  }
  br_data_push_point2(pg, (br_vec2_t){ .x = (float)(x - pg->dd.rebase_x), .y = y });
}

void br_data_push_xy(br_datas_t* pg_array, double x, double y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  if (pg->len == 0) {
    pg->dd.rebase_x = x;
    pg->dd.rebase_y = y;
  }
  br_data_push_point2(pg, (br_vec2_t){ .x = (float)(x - pg->dd.rebase_x), .y = (float)(y - pg->dd.rebase_y) });
}

void br_data_push_xyz(br_datas_t* datas, double x, double y, double z, int group) {
  br_data_t* data = br_data_get2(datas, group, br_data_kind_3d);
  if (data == NULL) return;
  if (data->len == 0) {
    data->ddd.rebase = BR_VEC3D(x, y, z);
  }
  br_vec3d_t p = br_vec3d_sub(BR_VEC3D(x, y, z), data->ddd.rebase);
  br_data_push_point3(data, BR_VEC3D_TOF(p));
}

br_vec2d_t br_data_el_xy(br_datas_t datas, int group, br_u32 index) {
  return br_data_el_xy1(*br_data_get1(datas, group), index);
}

br_vec2d_t br_data_el_xy1(br_data_t data, br_u32 index) {
  BR_ASSERT(data.kind == br_data_kind_2d);
  return BR_VEC2D(data.dd.xs[index] + data.dd.rebase_x, data.dd.ys[index] + data.dd.rebase_y);
}

br_vec3d_t br_data_el_xyz(br_datas_t datas, int group, br_u32 index) {
  return br_data_el_xyz1(*br_data_get1(datas, group), index);
}

br_vec3d_t br_data_el_xyz1(br_data_t data, br_u32 index) {
  BR_ASSERT(data.kind == br_data_kind_3d);
  br_vec3d_t p = BR_VEC3D((double)data.ddd.xs[index], (double)data.ddd.ys[index], (double)data.ddd.zs[index]);
  return br_vec3d_add(p, data.ddd.rebase);
}

br_vec3d_t br_data_el_xyz2(br_data_t data, br_u32 index) {
  switch (data.kind) {
    case br_data_kind_2d: {
      br_vec3d_t p = BR_VEC3D((double)data.dd.xs[index], (double)data.dd.ys[index], 0.0);
      return br_vec3d_add(p, BR_VEC3D(data.dd.rebase_x, data.dd.rebase_y, 0.0));
    } break;
    case br_data_kind_3d: return br_data_el_xyz1(data, index);
    default: BR_UNREACHABLE("Data kind: %d", data.kind);
  }
}

br_vec3_t  br_data_el_xyz_rebased(br_data_t data, br_u32 index) {
  switch (data.kind) {
    case br_data_kind_2d: return BR_VEC3(data.dd.xs[index], data.dd.ys[index], 0.f);
    case br_data_kind_3d: return BR_VEC3(data.dd.xs[index], data.dd.ys[index], data.ddd.zs[index]);
    default: BR_UNREACHABLE("Data kind: %d", data.kind);
  }
}

void br_data_empty(br_data_t* pg) {
  pg->len = 0;
  br_resampling_empty(pg->resampling);
}

void br_data_export(br_data_t data, FILE* file) {
  for (size_t i = 0; i < data.len; ++i) {
    switch(data.kind) {
      case br_data_kind_2d: {
        br_vec2_t point = BR_VEC2(data.dd.xs[i], data.dd.ys[i]);
        fprintf(file, "%f,%f;%d\n", point.x, point.y, data.group_id);
      } break;
      case br_data_kind_3d: {
        br_vec3_t point = BR_VEC3(data.ddd.xs[i], data.ddd.ys[i], data.ddd.zs[i]);
        fprintf(file, "%f,%f,%f;%d\n", point.x, point.y, point.z, data.group_id);
      } break;
      default: BR_UNREACHABLE("Kind %d not supported", data.kind);
    }
  }
}

void br_data_export_csv(br_data_t data, FILE* file) {
  fprintf(file, "group,id,x,y,z\n");
  for (size_t i = 0; i < data.len; ++i) {
    switch(data.kind) {
      case br_data_kind_2d: {
        br_vec2_t point = BR_VEC2(data.dd.xs[i], data.dd.ys[i]);
        fprintf(file, "%d,%zu,%f,%f,\n", data.group_id, i, point.x, point.y);
      } break;
      case br_data_kind_3d: {
        br_vec3_t point = BR_VEC3(data.ddd.xs[i], data.ddd.ys[i], data.ddd.zs[i]);
        fprintf(file, "%d,%zu,%f,%f,%f\n", data.group_id, i, point.x, point.y, point.z);
      } break;
      default: BR_UNREACHABLE("Kind %d not supported", data.kind);
    }
  }
}

void br_datas_export(br_datas_t datas, FILE* file) {
  br_data_t data;
  brfl_foreachv(data, datas) br_data_export(data, file);
}

void br_datas_export_csv(br_datas_t datas, FILE* file) {
  fprintf(file, "group,id,x,y,z\n");
  br_data_t data;
  brfl_foreachv(data, datas) {
    for (size_t i = 0; i < data.len; ++i) {
      switch(data.kind) {
        case br_data_kind_2d: {
          br_vec2_t point = BR_VEC2(data.dd.xs[i], data.dd.ys[i]);
          fprintf(file, "%d,%zu,%f,%f,\n", data.group_id, i, point.x, point.y);
        } break;
        case br_data_kind_3d: {
          br_vec3_t point = BR_VEC3(data.ddd.xs[i], data.ddd.ys[i], data.ddd.zs[i]);
          fprintf(file, "%d,%zu,%f,%f,%f\n", data.group_id, i, point.x, point.y, point.z);
        } break;
        default: BR_UNREACHABLE("Kind %d not supported", data.kind);
      }
    }
  }
}

void br_data_deinit(br_data_t* g) {
  BR_ASSERT(g->kind == br_data_kind_2d || g->kind == br_data_kind_3d);

  BR_FREE(g->dd.xs); g->dd.xs = NULL;
  BR_FREE(g->dd.ys); g->dd.ys = NULL;
  if (br_data_kind_3d == g->kind) BR_FREE(g->ddd.zs), g->ddd.zs = NULL;
  br_resampling_free(g->resampling);
  brsp_remove(br_data.sp, g->name);
  g->len = g->cap = 0;
}

void br_datas_deinit(br_datas_t* datas) {
  if (datas->arr == NULL) return;
  brfl_foreach(i, *datas) br_data_deinit(&datas->arr[i]);
  brfl_free(*datas);
}

void br_datas_empty(br_datas_t datas) {
  brfl_foreach(i, datas) br_data_empty(&datas.arr[i]);
}

void br_data_remove(br_datas_t* datas, int data_id) {
  brfl_foreach(i, *datas) {
    if (datas->arr[i].group_id != data_id) continue;
    br_data_deinit(&datas->arr[i]);
    brfl_remove(*datas, i);
  }
}

void br_datas_add_test_points(br_datas_t* pg) {
  {
    int group = 100;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for (int i = 0; i < 1024; ++i)
      br_data_push_xy(pg, (double)g->len/128.0, (double)g->len/128.0, group);
  }
  {
    int group = 0;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for (int i = 0; i < 1024; ++i)
      br_data_push_xy(pg, (double)g->len/128.0, sin((double)g->len/128.0), group);
  }
  {
    int group = 1;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for (int i = 0; i < 10*1024; ++i)
      br_data_push_xy(pg, -(double)g->len/128.0, sin((double)g->len/128.0), group);
  }
  {
    int group = 5;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for(int i = 0; i < 1024*1024; ++i) {
      double t = (double)(1 + g->len)*.1;
      double x = sqrt(t)*cos(log2(t));
      double y = sqrt(t)*sin(log2(t));
      br_data_push_xy(pg, x, y, group);
    }
  }
  {
    br_data_t* g = br_data_get(pg, 6);
    if (NULL == g) return;
    int l = (int)g->len;
    for(int i = 0; i < 0; ++i) {
      for(int j = 0; j < 0; ++j) {
        int x = -50 + j + l;
        int y = (-50 + (i - j) + l) * (i % 2 == 0 ? 1 : -1);
        br_vec2_t p = BR_VEC2((float)x, (float)y);
        br_data_push_point2(g, p);
      }
    }
  }
  {
    int group = 11;
    br_data_t* g = br_data_get2(pg, group, br_data_kind_3d);
    if (NULL == g) return;
    for(int i = 0; i < 1024*50; ++i) {
      double t = (double)(1 + g->len)*.1;
      double x = sqrt(t)*cos(log2(t));
      double y = sqrt(t)*sin(log2(t));
      double z = 2.f * sin(t*0.05f);
      br_data_push_xyz(pg, x, y, z, group);
    }
  }
}

void br_datas_draw(br_datas_t pg, br_plot_t* plot, br_extent_t extent) {
  for (int j = 0; j < plot->data_info.len; ++j) {
    br_plot_data_t di = plot->data_info.arr[j];
    br_data_t const* g = br_data_get1(pg, di.group_id);
    if (g->len == 0) continue;
    br_resampling_draw(g->resampling, g, plot, br_animf(br_data.anims, di.thickness_multiplyer_ah), extent);
  }
  br_resampling_change_something(pg);
}

static br_data_t* br_data_init(br_data_t* g, int group_id, br_data_kind_t kind) {
  BR_ASSERT(kind == br_data_kind_2d || kind == br_data_kind_3d);

  *g = (br_data_t) {
    .resampling = br_resampling_malloc(kind),
    .cap = DEF_CAP, .len = 0, .kind = kind,
    .group_id = group_id,
    .color = br_data_get_default_color(group_id),
    .name = brsp_new(br_data.sp),
    .is_new = true,
  };
  if (NULL == g->resampling)                                                                 goto error;
  if (NULL == (g->dd.xs = BR_MALLOC(sizeof(float) * DEF_CAP)))                               goto error;
  if (NULL == (g->dd.ys = BR_MALLOC(sizeof(float) * DEF_CAP)))                               goto error;
  if (kind == br_data_kind_3d) if (NULL == (g->ddd.zs = BR_MALLOC(sizeof(float) * DEF_CAP))) goto error;
  char* scrach = br_scrach_get(64);
    int n = sprintf(scrach, "Data #%d", group_id);
    brsp_set(br_data.sp, g->name, BR_STRV(scrach, (uint32_t)n));
  br_scrach_free();
  return g;

error:
  br_data_deinit(g);
  return NULL;
}

br_color_t br_data_get_default_color(int group_id) {
  group_id = abs(group_id);
  static int base_colors_count = sizeof(br_data.base_colors)/sizeof(br_color_t);
  float count = 2.f;
  br_color_t c = br_data.base_colors[group_id%base_colors_count];
  group_id /= base_colors_count;
  while (group_id > 0) {
    c.r = (unsigned char)(((float)c.r + (float)br_data.base_colors[group_id%base_colors_count].r) / count);
    c.g = (unsigned char)(((float)c.g + (float)br_data.base_colors[group_id%base_colors_count].g) / count);
    c.b = (unsigned char)(((float)c.b + (float)br_data.base_colors[group_id%base_colors_count].b) / count);
    group_id /= base_colors_count;
    count += 1;
  }
  return c;
}

bool br_data_is_generated(br_dagens_t const* dagens, int group_id) {
  for (size_t i = 0; i < dagens->len; ++i)
    if (dagens->arr[i].kind == br_dagen_kind_expr && dagens->arr[i].group_id == group_id)
      return true;
  return false;
}

bool br_data_realloc(br_data_t* data, size_t new_cap) {
  BR_ASSERT((data->kind == br_data_kind_2d) || (data->kind == br_data_kind_3d));
  if (data->cap >= new_cap) return true;
  float* xs = BR_REALLOC(data->dd.xs, sizeof(data->dd.xs[0]) * new_cap);
  if (NULL == xs) return false;
  data->dd.xs = xs;
  float* ys = BR_REALLOC(data->dd.ys, sizeof(data->dd.ys[0]) * new_cap);
  if (NULL == ys) return false;
  data->dd.ys = ys;
  if (data->kind == br_data_kind_2d) {
    data->cap = new_cap;
    return true;
  }
  float* zs = BR_REALLOC(data->ddd.zs, sizeof(data->ddd.zs[0]) * new_cap);
  if (NULL == zs) return false;
  data->ddd.zs = zs;
  data->cap = new_cap;
  return true;
}

br_data_t* br_data_get1(br_datas_t pg, int group) {
  brfl_foreach(i, pg) if (pg.arr[i].group_id == group) return &pg.arr[i];
  return NULL;
}

br_data_t* br_data_get(br_datas_t* pg, int group) {
  return br_data_get2(pg, group, br_data_kind_2d);
}

br_data_t* br_data_get2(br_datas_t* pg, int group, br_data_kind_t kind) {
  BR_ASSERT(pg);
  brfl_foreach(i, *pg) {
    if (pg->arr[i].group_id != group) continue;
    return pg->arr[i].kind == kind ? &pg->arr[i] : NULL;
  }
  br_data_t new_data;
  if (NULL == br_data_init(&new_data, group, kind)) return NULL;
  int handle = brfl_push(*pg, new_data);
  return br_da_getp(*pg, handle);
}

void br_data_set_name(br_datas_t* pg, int group, br_str_t name) {
  br_data_t* g = br_data_get(pg, group);
  if (pg == NULL) return;
  brsp_set(br_data.sp, g->name, br_str_as_view(name));
  br_str_free(name);
}

size_t br_data_element_size(br_data_kind_t kind) {
  switch (kind) {
    case br_data_kind_2d:      return sizeof(br_vec2_t);
    case br_data_kind_3d:      return sizeof(br_vec3_t);
    default: BR_ASSERT(0); return 0;
  }
}

static void br_data_push_point2(br_data_t* g, br_vec2_t v) {
  if (g->len >= g->cap && false == br_data_realloc(g, g->cap * 2)) return;
  if (g->len == 0) g->dd.bounding_box = BR_BB(v.x, v.y, v.x, v.y);
  else             g->dd.bounding_box = br_bb_expand_with_point(g->dd.bounding_box, v);
  g->dd.xs[g->len] = v.x;
  g->dd.ys[g->len] = v.y;
  br_resampling_add_point(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_data_push_point3(br_data_t* g, br_vec3_t v) {
  if (g->len >= g->cap && false == br_data_realloc(g, g->cap * 2)) return;
  if (g->len == 0) g->ddd.bounding_box = (bb_3d_t) { v.x, v.y, v.z, v.x, v.y, v.z };
  else             br_bb_3d_expand_with_point(&g->ddd.bounding_box, v);
  g->ddd.xs[g->len] = v.x;
  g->ddd.ys[g->len] = v.y;
  g->ddd.zs[g->len] = v.z;
  br_resampling_add_point(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_bb_3d_expand_with_point(bb_3d_t* bb, br_vec3_t v) {
  bb->xmax = fmaxf(bb->xmax, v.x);
  bb->xmin = fminf(bb->xmin, v.x);
  bb->ymax = fmaxf(bb->ymax, v.y);
  bb->ymin = fminf(bb->ymin, v.y);
  bb->zmax = fmaxf(bb->zmax, v.z);
  bb->zmin = fminf(bb->zmin, v.z);
}

