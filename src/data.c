#include "src/br_data.h"
#include "include/br_str_header.h"
#include "include/br_string_pool_header.h"
#include "src/br_anim.h"
#include "src/br_da.h"
#include "src/br_data_generator.h"
#include "src/br_math.h"
#include "src/br_memory.h"
#include "src/br_plot.h"
#include "src/br_resampling.h"
#include "src/br_series.h"

#define DEF_CAP 1024

static br_data_t* br_datas_create2(br_datas_t* datas, int group_id, br_data_kind_t kind, br_color_t color, size_t cap, brsp_id_t name);
static br_data_t br_data_init(int group_id, br_data_kind_t kind);
static void br_data_push_point2(br_data_t* g, br_vec2_t v);
static void br_data_push_point3(br_data_t* g, br_vec3_t v);

static BR_THREAD_LOCAL struct {
  brsp_t* sp;
  br_color_t base_colors[8];
  br_anims_t* anims;
  br_serieses_t* serieses;
} br_data;

void br_data_construct(brsp_t* sp, br_anims_t* anims, br_serieses_t* serieses) {
  br_data.sp = sp;
  br_data.anims = anims;
  br_data.serieses = serieses;
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
  br_data_t data = { 0 };
  brfl_foreachv(data, *datas) if (data.group_id > max) max = data.group_id;
  return max;
}

br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind) {
  br_data_t data = br_data_init(group_id, kind);
  int index = brfl_push(*datas, data);
  return br_da_getp(*datas, index);
}

void br_data_push_y(br_datas_t* pg_array, double y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  br_serieses_push_len(pg->series_handles[0]);
  br_serieses_push(pg->series_handles[1], y);
}

void br_data_push_x(br_datas_t* pg_array, double x, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  br_serieses_push(pg->series_handles[0], x);
  br_serieses_push_len(pg->series_handles[1]);
}

void br_data_push_xy(br_datas_t* pg_array, double x, double y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  br_serieses_push(pg->series_handles[0], x);
  br_serieses_push(pg->series_handles[1], y);
}

void br_data_push_xyz(br_datas_t* datas, double x, double y, double z, int group) {
  br_data_t* data = br_data_get2(datas, group, br_data_kind_3d);
  if (data == NULL) return;
  br_serieses_push(data->series_handles[0], x);
  br_serieses_push(data->series_handles[1], y);
  br_serieses_push(data->series_handles[2], z);
}

float br_data_local(br_data_t data, br_u32 series_index, br_u32 index) {
  BR_ASSERTF(series_index < (br_u32)data.serieses_len, "series_index=%u, data.serieses_len=%d", series_index, data.serieses_len);
  int series_handle = data.series_handles[series_index];
  br_series_t series = br_da_get(*br_data.serieses, series_handle);
  float value = br_series(series, index);
  return value;
}

br_vec2d_t br_data_el_xy(br_datas_t datas, int group, br_u32 index) {
  return br_data_el_xy1(*br_data_get1(datas, group), index);
}

br_vec2d_t br_data_el_xy1(br_data_t data, br_u32 index) {
  BR_ASSERT(data.kind == br_data_kind_2d);
  int* srs = data.series_handles;
  return BR_VEC2D(br_serieses_zero(srs[0], index), br_serieses_zero(srs[1], index));
}

br_vec3d_t br_data_el_xyz(br_datas_t datas, int group, br_u32 index) {
  return br_data_el_xyz1(*br_data_get1(datas, group), index);
}

br_vec3d_t br_data_el_xyz1(br_data_t data, br_u32 index) {
  BR_ASSERT(data.kind == br_data_kind_3d);
  int* srs = data.series_handles;
  return BR_VEC3D(br_serieses_zero(srs[0], index), br_serieses_zero(srs[1], index), br_serieses_zero(srs[2], index));
}

br_vec3d_t br_data_el_xyz2(br_data_t data, br_u32 index) {
  int* srs = data.series_handles;
  switch (data.kind) {
    case br_data_kind_2d: {
      return BR_VEC3D(br_serieses_zero(srs[0], index), br_serieses_zero(srs[1], index), 0.0);
    } break;
    case br_data_kind_3d: return br_data_el_xyz1(data, index);
    default: BR_UNREACHABLE("Data kind: %d", data.kind);
  }
  BR_RETURN_IF_TINY_C((br_vec3d_t){0});
}

br_vec3_t br_data_v3_local(br_data_t data, br_u32 index) {
  br_vec3_t ret = { 0 };
  br_serieses_t sers = *br_data.serieses;
  int to = data.serieses_len < 3 ? data.serieses_len : 3;
  for (int i = 0; i < to; ++i) {
    br_series_t s1 = br_da_get(sers, data.series_handles[i]);
    ret.arr[i] = br_series(s1, index);
  }
  return ret;
}

br_vec2_t br_data_v2_local(br_data_t data, br_u32 index) {
  br_vec2_t ret = { 0 };
  br_serieses_t sers = *br_data.serieses;
  int to = data.serieses_len < 2 ? data.serieses_len : 2;
  for (int i = 0; i < to; ++i) {
    br_series_t s1 = br_da_get(sers, data.series_handles[i]);
    ret.arr[i] = br_series(s1, index);
  }
  return ret;
}

br_vec2_t br_data_v2_to_local(br_data_t data, br_vec2d_t v) {
  br_vec2_t ret;
  for (int i = 0; i < 2; ++i) {
    br_series_t s1 = br_da_get(*br_data.serieses, data.series_handles[i]);
    ret.arr[i] = br_series_to_local(s1, v.arr[i]);
  }
  return ret;
}

br_vec3_t br_data_v3_to_local(br_data_t data, br_vec3d_t v) {
  br_vec3_t ret;
  for (int i = 0; i < 3; ++i) {
    br_series_t s1 = br_da_get(*br_data.serieses, data.series_handles[i]);
    ret.arr[i] = br_series_to_local(s1, v.arr[i]);
  }
  return ret;
}

br_vec2d_t br_data_zoom_to_local(br_data_t data, br_vec2d_t v) {
  br_vec2d_t ret;
  for (int i = 0; i < 2; ++i) {
    br_series_t s1 = br_da_get(*br_data.serieses, data.series_handles[i]);
    ret.arr[i] = v.arr[i] / s1.scale;
  }
  return ret;
}

br_extentd_t br_data_ex_to_local(br_data_t data, br_extentd_t ex) {
  br_vec2d_t tl = ex.pos;
  br_vec2d_t br = br_vec2d_add(ex.pos, ex.size);
  br_vec2_t local_tl = br_data_v2_to_local(data, tl);
  br_vec2_t local_br = br_data_v2_to_local(data, br);

  return BR_EXTENTD(local_tl.x, local_tl.y, local_br.x - local_tl.x, local_br.y - local_tl.y);
}

br_bb_t br_data_bb(br_data_t data) {
  BR_ASSERT(data.serieses_len >= 2);
  br_serieses_t sers = *br_data.serieses;
  br_series_t sx = br_da_get(sers, data.series_handles[0]);
  br_series_t sy = br_da_get(sers, data.series_handles[1]);
  return BR_BB((float)sx.min, (float)sy.min, (float)sx.max, (float)sy.max);
}

float* br_data_series_local(br_data_t data, int series_index) {
  br_series_t s = br_da_get(*br_data.serieses, data.series_handles[series_index]);
  return s.arr;
}

void br_data_empty(br_data_t* pg) {
  for (int i = 0; i < pg->serieses_len; ++i) {
    br_serieses_empty(pg->series_handles[i]);
  }
  br_resampling_empty(pg->resampling);
}

size_t br_data_len(br_data_t data) {
  size_t ret = 0x8FFFFFFF;
  for (int j = 0; j < data.serieses_len; ++j) {
    int handle = data.series_handles[j];
    br_series_t s = br_da_get(*br_data.serieses, handle);
    size_t sl = br_series_len(s);
    if (sl < ret) ret = sl;
  }
  return ret;
}

void br_data_export(br_data_t data, FILE* file) {
  size_t len = br_data_len(data);
  for (size_t i = 0; i < len; ++i) {
    for (int j = 0; j < data.serieses_len; ++j) {
      double val = br_serieses_zero(data.series_handles[j], i);
      if (j != 0) fprintf(file, ",");
      fprintf(file, "%f", val);
    }
    fprintf(file, ";%d\n", data.group_id);
  }
}

void br_data_export_csv(br_data_t data, FILE* file) {
  fprintf(file, "group,id,x,y,z\n");
  size_t len = br_data_len(data);
  for (size_t i = 0; i < len; ++i) {
    fprintf(file, "%d,%zu\n", data.group_id, i);
    for (int j = 0; j < data.serieses_len; ++j) {
      double val = br_serieses_zero(data.series_handles[j], i);
      fprintf(file, ",%f", val);
    }
  }
}

void br_datas_export(br_datas_t datas, FILE* file) {
  br_data_t data = { 0 };
  brfl_foreachv(data, datas) br_data_export(data, file);
}

void br_datas_export_csv(br_datas_t datas, FILE* file) {
  fprintf(file, "group,id,x,y,z\n");
  br_data_t data = { 0 };
  brfl_foreachv(data, datas) {
    size_t len = br_data_len(data);
    for (size_t i = 0; i < len; ++i) {
      fprintf(file, "%d,%zu\n", data.group_id, i);
      for (int j = 0; j < data.serieses_len; ++j) {
        double val = br_serieses_zero(data.series_handles[j], i);
        fprintf(file, ",%f", val);
      }
    }
  }
}

void br_data_deinit(br_data_t* g) {
  for (int j = 0; j < g->serieses_len; ++j) {
    br_serieses_release(g->series_handles[j]);
  }
  br_resampling_free(g->resampling);
  brsp_remove(br_data.sp, g->name);
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
    double len = (double)br_data_len(*g);
    for (int i = 0; i < 1024; ++i)
      br_data_push_xy(pg, (i + len)/128.0, (i + len)/128.0, group);
  }
  return;
  {
    int group = 0;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    double len = (double)br_data_len(*g);
    for (int i = 0; i < 1024; ++i)
      br_data_push_xy(pg, (i + len)/128.0, sin((i + len)/128.0), group);
  }
  {
    int group = 1;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    double len = (double)br_data_len(*g);
    for (int i = 0; i < 10*1024; ++i)
      br_data_push_xy(pg, -(i + len)/128.0, sin((i + len)/128.0), group);
  }
  {
    int group = 5;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    double len = (double)br_data_len(*g);
    for(int i = 0; i < 1024*1024; ++i) {
      double t = (double)(1 + len + i)*.1;
      double x = sqrt(t)*cos(log2(t));
      double y = sqrt(t)*sin(log2(t));
      br_data_push_xy(pg, x, y, group);
    }
  }
  {
    br_data_t* g = br_data_get(pg, 6);
    if (NULL == g) return;
  }
  {
    int group = 11;
    br_data_t* g = br_data_get2(pg, group, br_data_kind_3d);
    if (NULL == g) return;
    double len = (double)br_data_len(*g);
    for(int i = 0; i < 1024*50; ++i) {
      double t = (double)(1 + len + i)*.1;
      double x = sqrt(t)*cos(log2(t));
      double y = sqrt(t)*sin(log2(t));
      double z = 2.f * sin(t*0.05f);
      br_data_push_point3(g, BR_VEC3((float)x, (float)y, (float)z));
    }
  }
}

void br_datas_draw(br_datas_t pg, br_plot_t* plot, br_extent_t extent) {
  for (int j = 0; j < plot->data_info.len; ++j) {
    br_plot_data_t di = plot->data_info.arr[j];
    br_data_t const* g = br_data_get1(pg, di.group_id);
    BR_ASSERTF(g, " groupid=%d\n", di.group_id);
    br_resampling_draw(g->resampling, g, plot, br_animf(br_data.anims, di.thickness_multiplyer_ah), extent);
  }
  br_resampling_change_something(pg);
}

static br_data_t br_data_init(int group_id, br_data_kind_t kind) {
  BR_ASSERT(kind == br_data_kind_2d || kind == br_data_kind_3d);

  br_data_t d = {
    .resampling = br_resampling_malloc(kind),
    .kind = kind,
    .group_id = group_id,
    .color = br_data_get_default_color(group_id),
    .name = brsp_new(br_data.sp),
    .is_new = true,
  };
  BR_ASSERTF(NULL != d.resampling, "Out of memory, resampling...");
  BR_ASSERTF(0 != d.name, "Out of memory, name...");

  br_series_t init = { 0 };
  switch (kind) {
    case br_data_kind_2d: {
      d.serieses_len = 2;
      d.series_handles[0] = brfl_push(*br_data.serieses, init);
      d.series_handles[1] = brfl_push(*br_data.serieses, init);
    } break;
    case br_data_kind_3d: {
      d.serieses_len = 3;
      d.series_handles[0] = brfl_push(*br_data.serieses, init);
      d.series_handles[1] = brfl_push(*br_data.serieses, init);
      d.series_handles[2] = brfl_push(*br_data.serieses, init);
    } break;
    default: BR_UNREACHABLE("kind: %d", kind);
  }

  br_strv_t sv = br_scrach_printf("Data #%d", group_id);
  brsp_set(br_data.sp, d.name, sv);
  return d;
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
  br_data_t new_data = br_data_init(group, kind);
  int handle = brfl_push(*pg, new_data);
  return br_da_getp(*pg, handle);
}

// TODO: name should be sp_id, This is bad..
void br_data_set_name(br_datas_t* pg, int group, br_str_t name) {
  br_data_t* g = br_data_get(pg, group);
  if (pg == NULL) return;
  brsp_set(br_data.sp, g->name, br_str_as_view(name));
  br_str_free(name);
}

size_t br_data_element_size(br_data_kind_t kind) {
  switch (kind) {
    case br_data_kind_2d: return sizeof(br_vec2_t);
    case br_data_kind_3d: return sizeof(br_vec3_t);
    default: BR_UNREACHABLE("data kind: %d", kind);
  }
  BR_RETURN_IF_TINY_C(0);
}

static br_data_t* br_datas_create2(br_datas_t* datas, int group_id, br_data_kind_t kind, br_color_t color, size_t cap, brsp_id_t name) {
  BR_ASSERT(kind == br_data_kind_2d || kind == br_data_kind_3d);

  bool success = false;
  int handle = -1;

  cap = cap < DEF_CAP ? DEF_CAP : cap;
  br_data_t d = {
    .resampling = br_resampling_malloc(kind),
    .kind = kind,
    .group_id = group_id,
    .color = color,
    .name = name
  };
  if (NULL == d.resampling) BR_ERROR("Failed to malloc resampl");

  br_series_t init = { 0 };
  switch (kind) {
    case br_data_kind_2d: {
      d.serieses_len = 2;
      d.series_handles[0] = brfl_push(*br_data.serieses, init);
      d.series_handles[1] = brfl_push(*br_data.serieses, init);
    } break;
    case br_data_kind_3d: {
      d.serieses_len = 3;
      d.series_handles[0] = brfl_push(*br_data.serieses, init);
      d.series_handles[1] = brfl_push(*br_data.serieses, init);
      d.series_handles[2] = brfl_push(*br_data.serieses, init);
    } break;
    default: BR_UNREACHABLE("kind: %d", kind);
  }
  handle = brfl_push(*datas, d);

error:
  if (!success) br_data_deinit(&d);
  return success ? br_da_getp(*datas, handle) : NULL;
}


static void br_data_push_point2(br_data_t* g, br_vec2_t v) {
  br_serieses_push(g->series_handles[0], v.x);
  br_serieses_push(g->series_handles[1], v.y);
}

static void br_data_push_point3(br_data_t* g, br_vec3_t v) {
  br_serieses_push(g->series_handles[0], v.x);
  br_serieses_push(g->series_handles[1], v.y);
  br_serieses_push(g->series_handles[2], v.z);
}

