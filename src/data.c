#include "src/br_data.h"
#include "src/br_da.h"
#include "src/br_data_generator.h"
#include "src/br_gui_internal.h"
#include "src/br_plot.h"
#include "src/br_pp.h"
#include "src/br_resampling2.h"
#include "src/br_str.h"
#include "src/br_math.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define DEF_CAP 1024

static br_data_t* br_data_init(br_data_t* g, int group_id, br_data_kind_t kind);
static void br_data_push_point2(br_data_t* g, br_vec2_t v);
static void br_data_push_point3(br_data_t* g, br_vec3_t v);
static void br_data_deinit(br_data_t* g);
static void br_bb_expand_with_point(bb_t* bb, br_vec2_t v);
static void br_bb_3d_expand_with_point(bb_3d_t* bb, br_vec3_t v);

static br_color_t base_colors[8];

void br_data_construct(void) {
  base_colors[0] = BR_RED;
  base_colors[1] = BR_GREEN;
  base_colors[2] = BR_BLUE;
  base_colors[3] = BR_LIGHTGRAY;
  base_colors[4] = BR_PINK;
  base_colors[5] = BR_GOLD;
  base_colors[6] = BR_VIOLET;
  base_colors[7] = BR_DARKPURPLE;
}

int br_datas_get_new_id(br_datas_t *datas) {
  int max = 1;
  for (size_t i = 0; i < datas->len; ++i) {
    if (datas->arr[i].group_id > max) max = datas->arr[i].group_id;
  }
  return max;
}

br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind) {
  br_data_t data;
  if (NULL == br_data_init(&data, group_id, kind)) return NULL;
  size_t index = datas->len;
  br_da_push(*datas, data);
  if (index == datas->len) {
    br_data_deinit(&data);
    return NULL;
  }
  return &datas->arr[index];
}

br_data_t* br_datas_create2(br_datas_t* datas, int group_id, br_data_kind_t kind, br_color_t color, size_t cap, br_str_t name) {
  BR_ASSERT(kind == br_data_kind_2d || kind == br_data_kind_3d);
  br_data_t* ret = NULL;

  cap = cap < DEF_CAP ? DEF_CAP : cap;
  br_data_t d = {
    .resampling = resampling2_malloc(kind),
    .cap = cap, .len = 0, .kind = kind,
    .group_id = group_id,
    .color = color,
    .name = name
  };
  if (NULL == d.resampling)                                                             goto error;
  if (NULL == (d.dd.xs = BR_MALLOC(sizeof(float) * cap)))                               goto error;
  if (NULL == (d.dd.ys = BR_MALLOC(sizeof(float) * cap)))                               goto error;
  if (kind == br_data_kind_3d) if (NULL == (d.ddd.zs = BR_MALLOC(sizeof(float) * cap))) goto error;
  br_da_push(*datas, d);
  ret = &datas->arr[datas->len - 1];
  if (ret->group_id != group_id)                                                        goto error;
  return ret;

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

void br_data_push_xyz(br_datas_t* pg_array, double x, double y, double z, int group) {
  br_data_t* pg = br_data_get2(pg_array, group, br_data_kind_3d);
  if (pg == NULL) return;
  if (pg->len == 0) {
    pg->ddd.rebase_x = x;
    pg->ddd.rebase_y = y;
    pg->ddd.rebase_z = z;
  }
  br_data_push_point3(pg, (br_vec3_t){ .x = (float)(x - pg->ddd.rebase_x), .y = (float)(y - pg->ddd.rebase_y), .z = (float)(z - pg->ddd.rebase_z) });
}

br_vec2d_t br_data_el_xy(br_datas_t datas, int group, int index) {
  br_data_t const* data = br_data_get1(datas, group);
  BR_ASSERT(data->kind == br_data_kind_2d);
  return BR_VEC2D(data->dd.xs[index] + data->dd.rebase_x, data->dd.ys[index] + data->dd.rebase_y);
}

br_vec3d_t br_data_el_xyz(br_datas_t datas, int group, int index) {
  br_data_t const* data = br_data_get1(datas, group);
  BR_ASSERT(data->kind == br_data_kind_3d);
  return BR_VEC3D(data->ddd.xs[index] + data->ddd.rebase_x, data->ddd.ys[index] + data->ddd.rebase_y, data->ddd.zs[index] + data->ddd.rebase_z);
}

//void br_data_push_expr_xy(br_datas_t* datas, br_data_expr_t x, br_data_expr_t y, int group) {
//  br_data_t* data = br_data_get2(datas, group, br_data_kind_expr_2d);
//  data->expr_2d.x_expr = x;
//  data->expr_2d.y_expr = y;
//  br_data_empty(data);
//}

void br_data_empty(br_data_t* pg) {
  pg->len = 0;
  resampling2_empty(pg->resampling);
}

static int br_data_compare(const void* data1, const void* data2) {
  return ((br_data_t*)data1)->group_id - ((br_data_t*)data2)->group_id;
}

void br_data_clear(br_datas_t* pg, br_plots_t* plots, int group_id) {
  for (size_t i = 0; i < pg->len; ++i) {
    if (pg->arr[i].group_id != group_id) continue;
    br_data_deinit(&pg->arr[i]);
    if (i != pg->len - 1) memcpy(&pg->arr[i], &pg->arr[pg->len - 1], sizeof(br_data_t));
    memset(&pg->arr[--pg->len], 0, sizeof(br_data_t));
    br_plot_remove_group(*plots, group_id);
  }
  if (pg->len > 1) qsort(pg->arr, pg->len, sizeof(pg->arr[0]), br_data_compare);
}

void br_data_export(br_data_t const* pg, FILE* file) {
  for (size_t i = 0; i < pg->len; ++i) {
    switch(pg->kind) {
      case br_data_kind_2d: {
        br_vec2_t point = BR_VEC2(pg->dd.xs[i], pg->dd.ys[i]);
        fprintf(file, "%f,%f;%d\n", point.x, point.y, pg->group_id);
        break;
      }
      case br_data_kind_3d: {
        br_vec3_t point = BR_VEC3(pg->ddd.xs[i], pg->ddd.ys[i], pg->ddd.zs[i]);
        fprintf(file, "%f,%f,%f;%d\n", point.x, point.y, point.z, pg->group_id);
        break;
      }
      default: assert(0);
    }
  }
}

void br_data_export_csv(br_data_t const* pg, FILE* file) {
  fprintf(file, "group,id,x,y,z\n");
  for (size_t i = 0; i < pg->len; ++i) {
    switch(pg->kind) {
      case br_data_kind_2d: {
        br_vec2_t point = BR_VEC2(pg->dd.xs[i], pg->dd.ys[i]);
        fprintf(file, "%d,%zu,%f,%f,\n", pg->group_id, i, point.x, point.y);
        break;
      }
      case br_data_kind_3d: {
        br_vec3_t point = BR_VEC3(pg->ddd.xs[i], pg->ddd.ys[i], pg->ddd.zs[i]);
        fprintf(file, "%d,%zu,%f,%f,%f\n", pg->group_id, i, point.x, point.y, point.z);
        break;
      }
      default: BR_ASSERT(0);
    }
  }
}

void br_datas_export(br_datas_t const* pg_array, FILE* file) {
  for (size_t i = 0; i < pg_array->len; ++i) {
    br_data_export(&pg_array->arr[i], file);
  }
}

void br_datas_export_csv(br_datas_t const* pg_array, FILE* file) {
  fprintf(file, "group,id,x,y,z\n");
  for (size_t j = 0; j < pg_array->len; ++j) {
    br_data_t* pg = &pg_array->arr[j];
    for (size_t i = 0; i < pg->len; ++i) {
      switch(pg->kind) {
        case br_data_kind_2d: {
          br_vec2_t point = BR_VEC2(pg->dd.xs[i], pg->dd.ys[i]);
          fprintf(file, "%d,%zu,%f,%f,\n", pg->group_id, i, point.x, point.y);
          break;
        }
        case br_data_kind_3d: {
          br_vec3_t point = BR_VEC3(pg->ddd.xs[i], pg->ddd.ys[i], pg->ddd.zs[i]);
          fprintf(file, "%d,%zu,%f,%f,%f\n", pg->group_id, i, point.x, point.y, point.z);
          break;
        }
        default: assert(0);
      }
    }
  }
}

void br_datas_deinit(br_datas_t* arr) {
  if (arr->arr == NULL) return;
  for (size_t i = 0; i < arr->len; ++i) br_data_deinit(&arr->arr[i]);
  br_da_free(*arr);
}

void br_datas_empty(br_datas_t* pg) {
  for (size_t i = 0; i < pg->len; ++i) br_data_empty(&pg->arr[i]);
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
  {
//    br_data_push_expr_xy(pg, (br_data_expr_t){ .kind = br_data_expr_kind_reference_y, .group_id = 5 }, (br_data_expr_t){ .kind = br_data_expr_kind_reference_x, .group_id = 5 }, 12);
  }
}

// Custom Blend Modes

void br_datas_draw(br_datas_t pg, br_plot_t* plot, br_shaders_t* shaders) {
  if (plot->kind == br_plot_kind_2d) {
    TracyCFrameMarkStart("br_datas_draw_2d");
    for (int j = 0; j < plot->groups_to_show.len; ++j) {
      int group = plot->groups_to_show.arr[j];
      br_data_t const* g = br_data_get1(pg, group);
      if (g->len == 0) continue;
      resampling2_draw(g->resampling, g, plot, shaders);
    }
    TracyCFrameMarkEnd("br_datas_draw_2d");
  } else {
    TracyCFrameMarkStart("br_datas_draw_3d");
    for (int j = 0; j < plot->groups_to_show.len; ++j) {
      int group = plot->groups_to_show.arr[j];
      br_data_t const* g = br_data_get1(pg, group);
      if (g->len == 0) continue;
      resampling2_draw(g->resampling, g, plot, shaders);
    }
    TracyCFrameMarkEnd("br_datas_draw_3d");
  }
  resampling2_change_something(pg);
}

static br_data_t* br_data_init(br_data_t* g, int group_id, br_data_kind_t kind) {
  BR_ASSERT(kind == br_data_kind_2d || kind == br_data_kind_3d);

  *g = (br_data_t) {
    .resampling = resampling2_malloc(kind),
    .cap = DEF_CAP, .len = 0, .kind = kind,
    .group_id = group_id,
    .color = br_data_get_default_color(group_id),
    .name = br_str_malloc(32),
    .is_new = true,
  };
  if (NULL == g->name.str)                                                                     goto error;
  if (NULL == g->resampling)                                                                   goto error;
  if (NULL == (g->dd.xs = BR_MALLOC(sizeof(float) * DEF_CAP)))                                 goto error;
  if (NULL == (g->dd.ys = BR_MALLOC(sizeof(float) * DEF_CAP)))                                 goto error;
  if (kind == br_data_kind_3d) if (NULL == (g->ddd.zs = BR_MALLOC(sizeof(br_vec3_t) * DEF_CAP))) goto error;
  if (false == br_str_push_literal(&g->name, "Data #"))                                        goto error;
  if (false == br_str_push_int(&g->name, group_id))                                            goto error;
  return g;

error:
  br_data_deinit(g);
  return NULL;
}

br_color_t br_data_get_default_color(int group_id) {
  group_id = abs(group_id);
  static int base_colors_count = sizeof(base_colors)/sizeof(br_color_t);
  float count = 2.f;
  br_color_t c = base_colors[group_id%base_colors_count];
  group_id /= base_colors_count;
  while (group_id > 0) {
    c.r = (unsigned char)(((float)c.r + (float)base_colors[group_id%base_colors_count].r) / count);
    c.g = (unsigned char)(((float)c.g + (float)base_colors[group_id%base_colors_count].g) / count);
    c.b = (unsigned char)(((float)c.b + (float)base_colors[group_id%base_colors_count].b) / count);
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
  for (size_t i = 0; i < pg.len; ++i) if (pg.arr[i].group_id == group)
    return &pg.arr[i];
  return NULL;
}

br_data_t* br_data_get(br_datas_t* pg, int group) {
  return br_data_get2(pg, group, br_data_kind_2d);
}

br_data_t* br_data_get2(br_datas_t* pg, int group, br_data_kind_t kind) {
  BR_ASSERT(pg);

  // TODO: da
  if (pg->len == 0) {
    pg->arr = BR_MALLOC(sizeof(br_data_t));
    if (NULL == pg->arr) return NULL;
    br_data_t* ret = br_data_init(&pg->arr[0], group, kind);
    if (ret == NULL) return NULL;
    pg->cap = pg->len = 1;
    return ret;
  }

  for (size_t i = 0; i < pg->len; ++i) {
    if (pg->arr[i].group_id != group) continue;
    return pg->arr[i].kind == kind ? &pg->arr[i] : NULL;
  }

  if (pg->len >= pg->cap) {
    size_t new_cap = pg->cap * 2;
    br_data_t* new_arr = BR_REALLOC(pg->arr, sizeof(br_data_t)*new_cap);
    if (NULL == new_arr) return NULL;
    pg->arr = new_arr;
    pg->cap = new_cap;
  }
  br_data_t* ret = br_data_init(&pg->arr[pg->len++], group, kind);
  if (NULL == ret) {
    --pg->len;
    return NULL;
  }
  qsort(pg->arr, pg->len, sizeof(pg->arr[0]), br_data_compare);
  return br_data_get2(pg, group, kind);
}

void br_data_set_name(br_datas_t* pg, int group, br_str_t name) {
  br_data_t* g = br_data_get(pg, group);
  if (pg == NULL) return;
  br_str_free(g->name);
  g->name = name;
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
  if (g->len == 0) g->dd.bounding_box = (bb_t) { v.x, v.y, v.x, v.y };
  else             br_bb_expand_with_point(&g->dd.bounding_box, v);
  g->dd.xs[g->len] = v.x;
  g->dd.ys[g->len] = v.y;
  resampling2_add_point(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_data_push_point3(br_data_t* g, br_vec3_t v) {
  if (g->len >= g->cap && false == br_data_realloc(g, g->cap * 2)) return;
  if (g->len == 0) g->ddd.bounding_box = (bb_3d_t) { v.x, v.y, v.z, v.x, v.y, v.z };
  else             br_bb_3d_expand_with_point(&g->ddd.bounding_box, v);
  g->ddd.xs[g->len] = v.x;
  g->ddd.ys[g->len] = v.y;
  g->ddd.zs[g->len] = v.z;
  resampling2_add_point(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_data_deinit(br_data_t* g) {
  BR_ASSERT(g->kind == br_data_kind_2d || g->kind == br_data_kind_3d);

  BR_FREE(g->dd.xs); g->dd.xs = NULL;
  BR_FREE(g->dd.ys); g->dd.ys = NULL;
  if (br_data_kind_3d == g->kind) BR_FREE(g->ddd.zs), g->ddd.zs = NULL;
  resampling2_free(g->resampling);
  br_str_free(g->name);
  g->len = g->cap = 0;
}

static void br_bb_expand_with_point(bb_t* bb, br_vec2_t v) {
  bb->xmax = fmaxf(bb->xmax, v.x);
  bb->xmin = fminf(bb->xmin, v.x);
  bb->ymax = fmaxf(bb->ymax, v.y);
  bb->ymin = fminf(bb->ymin, v.y);
}

static void br_bb_3d_expand_with_point(bb_3d_t* bb, br_vec3_t v) {
  bb->xmax = fmaxf(bb->xmax, v.x);
  bb->xmin = fminf(bb->xmin, v.x);
  bb->ymax = fmaxf(bb->ymax, v.y);
  bb->ymin = fminf(bb->ymin, v.y);
  bb->zmax = fmaxf(bb->zmax, v.z);
  bb->zmin = fminf(bb->zmin, v.z);
}

