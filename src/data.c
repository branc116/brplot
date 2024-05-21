#include "br_plot.h"
#include "br_resampling2.h"
#include "br_gui_internal.h"
#include "br_pp.h"

#include "tracy/TracyC.h"
#include "rlgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

static br_data_t* br_data_init(br_data_t* g, int group_id, br_data_kind_t kind);
static void br_data_push_point2(br_data_t* g, Vector2 v);
static void br_data_push_point3(br_data_t* g, Vector3 v);
static void br_data_deinit(br_data_t* g);
static bool br_data_realloc(br_data_t* pg, size_t new_cap);
static Color color_get(int id);
static void br_bb_expand_with_point(bb_t* bb, Vector2 v);
static void br_bb_3d_expand_with_point(bb_3d_t* bb, Vector3 v);

static Color base_colors[8];

void br_data_construct(void) {
  base_colors[0] = RED;
  base_colors[1] = GREEN;
  base_colors[2] = BLUE;
  base_colors[3] = LIGHTGRAY;
  base_colors[4] = PINK;
  base_colors[5] = GOLD;
  base_colors[6] = VIOLET;
  base_colors[7] = DARKPURPLE;
}

BR_API void br_data_push_y(br_datas_t* pg_array, float y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  float x = pg->len == 0 ? 0.f : (pg->dd.points[pg->len - 1].x + 1.f);
  br_data_push_point2(pg, (Vector2){ .x = x, .y = y });
}

BR_API void br_data_push_x(br_datas_t* pg_array, float x, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  float y = pg->len == 0 ? 0.f : (pg->dd.points[pg->len - 1].y + 1.f);
  br_data_push_point2(pg, (Vector2){ .x = x, .y = y });
}

BR_API void br_data_push_xy(br_datas_t* pg_array, float x, float y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  br_data_push_point2(pg, (Vector2){ .x = x, .y = y });
}

BR_API void br_data_push_xyz(br_datas_t* pg_array, float x, float y, float z, int group) {
  br_data_t* pg = br_data_get2(pg_array, group, br_data_kind_3d);
  if (pg == NULL) return;
  br_data_push_point3(pg, (Vector3){ .x = x, .y = y, .z = z });
}

//void br_data_push_expr_xy(br_datas_t* datas, br_data_expr_t x, br_data_expr_t y, int group) {
//  br_data_t* data = br_data_get2(datas, group, br_data_kind_expr_2d);
//  data->expr_2d.x_expr = x;
//  data->expr_2d.y_expr = y;
//  br_data_empty(data);
//}

BR_API void br_data_empty(br_data_t* pg) {
  pg->len = 0;
  resampling2_empty(pg->resampling);
}

static int br_data_compare(const void* data1, const void* data2) {
  return ((br_data_t*)data1)->group_id - ((br_data_t*)data2)->group_id;
}

BR_API void br_data_clear(br_datas_t* pg, br_plots_t* plots, int group_id) {
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
        Vector2 point = pg->dd.points[i];
        fprintf(file, "%f,%f;%d\n", point.x, point.y, pg->group_id);
        break;
      }
      case br_data_kind_3d: {
        Vector3 point = pg->ddd.points[i];
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
        Vector2 point = pg->dd.points[i];
        fprintf(file, "%d,%zu,%f,%f,\n", pg->group_id, i, point.x, point.y);
        break;
      }
      case br_data_kind_3d: {
        Vector3 point = pg->ddd.points[i];
        fprintf(file, "%d,%zu,%f,%f,%f\n", pg->group_id, i, point.x, point.y, point.z);
        break;
      }
      default: assert(0);
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
          Vector2 point = pg->dd.points[i];
          fprintf(file, "%d,%zu,%f,%f,\n", pg->group_id, i, point.x, point.y);
          break;
        }
        case br_data_kind_3d: {
          Vector3 point = pg->ddd.points[i];
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
  for (size_t i = 0; i < arr->len; ++i) {
    br_data_deinit(&arr->arr[i]);
  }
  arr->len = arr->cap = 0;
  BR_FREE(arr->arr);
  arr->arr = NULL;
}

BR_API void br_datas_empty(br_datas_t* pg) {
  for (size_t i = 0; i < pg->len; ++i) {
    br_data_empty(&pg->arr[i]);
  }
}

void br_datas_add_test_points(br_datas_t* pg) {
  {
    int group = 0;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for (int i = 0; i < 1024; ++i)
      br_data_push_point2(g, (Vector2){(float)g->len/128.f, sinf((float)g->len/128.f)});
  }
  {
    int group = 1;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for (int i = 0; i < 10*1024; ++i)
      br_data_push_point2(g, (Vector2){-(float)g->len/128.f, sinf((float)g->len/128.f)});
  }
  {
    int group = 5;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for(int i = 0; i < 1024*1024; ++i) {
      float t = (float)(1 + g->len)*.1f;
      float x = sqrtf(t)*cosf(log2f(t));
      float y = sqrtf(t)*sinf(log2f(t));
      Vector2 p = {x, y};
      br_data_push_point2(g, p);
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
        Vector2 p = {(float)x, (float)y};
        br_data_push_point2(g, p);
      }
    }
  }
  {
    br_data_t* g = br_data_get2(pg, 11, br_data_kind_3d);
    if (NULL == g) return;
    for(int i = 0; i < 1024; ++i) {
      float t = (float)(1 + g->len)*.1f;
      float x = sqrtf(t)*cosf(log2f(t));
      float y = sqrtf(t)*sinf(log2f(t));
      float z = 2.f * sinf(t*0.05f);
      Vector3 p = {x, y, z};
      br_data_push_point3(g, p);
    }
  }
  {
//    br_data_push_expr_xy(pg, (br_data_expr_t){ .kind = br_data_expr_kind_reference_y, .group_id = 5 }, (br_data_expr_t){ .kind = br_data_expr_kind_reference_x, .group_id = 5 }, 12);
  }
}

// Custom Blend Modes
#define GL_SRC_ALPHA 0x0302
#define GL_DST_ALPHA 0x0304
#define GL_MAX 0x8008

void br_datas_draw(br_datas_t pg, br_plot_t* plot) {
  if (plot->kind == br_plot_kind_2d) {
    TracyCFrameMarkStart("br_datas_draw_2d");
    rlSetBlendFactors(GL_SRC_ALPHA, GL_DST_ALPHA, GL_MAX);
    rlSetBlendMode(BLEND_CUSTOM);
    for (int j = 0; j < plot->groups_to_show.len; ++j) {
      int group = plot->groups_to_show.arr[j];
      br_data_t const* g = br_data_get1(pg, group);
      if (g->len == 0) continue;
      resampling2_draw(g->resampling, g, plot);
    }
    if (plot->dd.line_shader->len > 0) {
      br_shader_line_draw(plot->dd.line_shader);
      plot->dd.line_shader->len = 0;
    }
    rlSetBlendMode(BLEND_ALPHA);
    TracyCFrameMarkEnd("br_datas_draw_2d");
  } else {
    TracyCFrameMarkStart("br_datas_draw_3d");
    int h = (int)plot->graph_screen_rect.height;
    rlViewport((int)plot->graph_screen_rect.x, (int)plot->resolution.y - h - (int)plot->graph_screen_rect.y, (int)plot->graph_screen_rect.width, h);
    rlDisableBackfaceCulling();
    rlEnableDepthTest();
    for (int j = 0; j < plot->groups_to_show.len; ++j) {
      int group = plot->groups_to_show.arr[j];
      br_data_t const* g = br_data_get1(pg, group);
      if (g->len == 0) continue;
#if !defined(RELEASE)
      if (NULL == g) {
        // TODO: Rename groups to datas
        LOGE("Trying to get a group with id = group, but that groups don't exist..\n"
             "NOTE: Groups that exist are:\n");
        for (int i = 0; pg.len; ++i) {
        LOGI("      *%d (len = %zu)\n", pg.arr[i].group_id, pg.arr[i].len);
        }
      }
#endif
      resampling2_draw(g->resampling, g, plot);
    }
    if (plot->ddd.line_shader->len > 0) {
      br_shader_line_3d_draw(plot->ddd.line_shader);
      plot->ddd.line_shader->len = 0;
    }
    rlDisableDepthTest();
    rlEnableBackfaceCulling();
    rlViewport(0, 0, (int)plot->resolution.x, (int)plot->resolution.y);
    TracyCFrameMarkEnd("br_datas_draw_3d");
  }
}

static br_data_t* br_data_init(br_data_t* g, int group_id, br_data_kind_t kind) {
  *g = (br_data_t) {
    .resampling = resampling2_malloc(kind),
    .cap = 1024, .len = 0, .kind = kind,
    .group_id = group_id,
    .color = color_get(group_id),
    .name = br_str_malloc(32),
    .is_new = true,
    .dd = {
      .points = BR_MALLOC(br_data_element_size(kind) * 1024),
    }
  };
  if (NULL != g->name.str) {
    sprintf(g->name.str, "Data #%d", group_id);
    g->name.len = (unsigned int)strlen(g->name.str);
  }
  return g;
}

static Color color_get(int id) {
  id = abs(id);
  static int base_colors_count = sizeof(base_colors)/sizeof(Color);
  float count = 2.f;
  Color c = base_colors[id%base_colors_count];
  id /= base_colors_count;
  while (id > 0) {
    c.r = (unsigned char)(((float)c.r + (float)base_colors[id%base_colors_count].r) / count);
    c.g = (unsigned char)(((float)c.g + (float)base_colors[id%base_colors_count].g) / count);
    c.b = (unsigned char)(((float)c.b + (float)base_colors[id%base_colors_count].b) / count);
    id /= base_colors_count;
    count += 1;
  }
  return c;
}

BR_API br_data_t* br_data_get1(br_datas_t pg, int group) {
  for (size_t i = 0; i < pg.len; ++i) if (pg.arr[i].group_id == group)
    return &pg.arr[i];
  return NULL;
}

BR_API br_data_t* br_data_get(br_datas_t* pg, int group) {
  return br_data_get2(pg, group, br_data_kind_2d);
}

BR_API br_data_t* br_data_get2(br_datas_t* pg, int group, br_data_kind_t kind) {
  assert(pg);

  // TODO: da
  if (pg->len == 0) {
    pg->arr = BR_MALLOC(sizeof(br_data_t));
    if (NULL == pg->arr) return NULL;
    br_data_t* ret = br_data_init(&pg->arr[0], group, kind);
    if (ret->dd.points == NULL) return NULL;
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
  if (ret->dd.points == NULL) {
    --pg->len;
    return NULL;
  }
  qsort(pg->arr, pg->len, sizeof(pg->arr[0]), br_data_compare);
  return br_data_get2(pg, group, kind);
}

BR_API void br_data_set_name(br_datas_t* pg, int group, br_str_t name) {
  br_data_t* g = br_data_get(pg, group);
  if (pg == NULL) return;
  br_str_free(g->name);
  g->name = name;
}

size_t br_data_element_size(br_data_kind_t kind) {
  switch (kind) {
    case br_data_kind_2d:      return sizeof(Vector2);
    case br_data_kind_3d:      return sizeof(Vector3);
    default: assert(0);
  }
}

static void br_data_push_point2(br_data_t* g, Vector2 v) {
  if (g->len >= g->cap && false == br_data_realloc(g, g->cap * 2)) return;
  if (g->len == 0) g->dd.bounding_box = (bb_t) { v.x, v.y, v.x, v.y };
  else             br_bb_expand_with_point(&g->dd.bounding_box, v);
  g->dd.points[g->len] = v;
  resampling2_add_point(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_data_push_point3(br_data_t* g, Vector3 v) {
  if (g->len >= g->cap && false == br_data_realloc(g, g->cap * 2)) return;
  if (g->len == 0) g->ddd.bounding_box = (bb_3d_t) { v.x, v.y, v.z, v.x, v.y, v.z };
  else             br_bb_3d_expand_with_point(&g->ddd.bounding_box, v);
  g->ddd.points[g->len] = v;
  resampling2_add_point(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_data_deinit(br_data_t* g) {
  // Free points
  BR_FREE(g->dd.points);
  resampling2_free(g->resampling);
  br_str_free(g->name);
  g->dd.points = NULL;
  g->len = g->cap = 0;
}

static bool br_data_realloc(br_data_t* pg, size_t new_cap) {
  BR_ASSERT(pg->cap > 0);
  BR_ASSERT(new_cap > 0);

  Vector2* new_arr = BR_REALLOC(pg->dd.points, new_cap * br_data_element_size(pg->kind));
  if (new_arr == NULL) {
    LOG("Out of memory. Can't add any more lines. Buy more RAM, or close Chrome\n");
    return false;
  }
  pg->dd.points = new_arr;
  pg->cap = new_cap;
  return true;
}

static void br_bb_expand_with_point(bb_t* bb, Vector2 v) {
  bb->xmax = fmaxf(bb->xmax, v.x);
  bb->xmin = fminf(bb->xmin, v.x);
  bb->ymax = fmaxf(bb->ymax, v.y);
  bb->ymin = fminf(bb->ymin, v.y);
}

static void br_bb_3d_expand_with_point(bb_3d_t* bb, Vector3 v) {
  bb->xmax = fmaxf(bb->xmax, v.x);
  bb->xmin = fminf(bb->xmin, v.x);
  bb->ymax = fmaxf(bb->ymax, v.y);
  bb->ymin = fminf(bb->ymin, v.y);
  bb->zmax = fmaxf(bb->zmax, v.z);
  bb->zmin = fminf(bb->zmin, v.z);
}

