#include "br_plot.h"
#include "br_resampling2.h"
#include "br_smol_mesh.h"
#include "br_gui_internal.h"

#include "tracy/TracyC.h"
#include "rlgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

static br_data_t* br_data_init(br_data_t* g, int group_id);
static void br_data_push_point(br_data_t* g, Vector2 v);
static void br_data_3d_push_point(br_data_3d_t* g, Vector3 v);
static void br_data_deinit(br_data_t* g);
static bool br_data_realloc(br_data_t* pg, size_t new_cap);
static bool br_data_3d_realloc(br_data_3d_t* pg, size_t new_cap);
static Color color_get(int id);
static void br_bb_expand_with_point(bb_t* bb, Vector2 v);
static void br_bb_3d_expand_with_point(bb_3d_t* bb, Vector3 v);

BR_API void br_data_push_y(br_datas_t* pg_array, float y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  float x = pg->len == 0 ? 0.f : (pg->points[pg->len - 1].x + 1.f);
  br_data_push_point(pg, (Vector2){ .x = x, .y = y });
}

BR_API void br_data_push_x(br_datas_t* pg_array, float x, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  float y = pg->len == 0 ? 0.f : (pg->points[pg->len - 1].y + 1.f);
  br_data_push_point(pg, (Vector2){ .x = x, .y = y });
}

BR_API void br_data_push_xy(br_datas_t* pg_array, float x, float y, int group) {
  br_data_t* pg = br_data_get(pg_array, group);
  if (pg == NULL) return;
  br_data_push_point(pg, (Vector2){ .x = x, .y = y });
}

BR_API void br_data_push_xyz(br_datas_3d_t* pg_array, float x, float y, float z, int group) {
  br_data_3d_t* pg = br_data_3d_get(pg_array, group);
  if (pg == NULL) return;
  br_data_3d_push_point(pg, (Vector3){ .x = x, .y = y, .z = z });
}

BR_API void br_data_empty(br_data_t* pg) {
  pg->len = 0;
  resampling2_empty(pg->resampling);
}

BR_API void br_data_clear(br_datas_t* pg, br_plots_t* plots, int group_id) {
  size_t len = pg->len;
  bool found = false;
  // TODO: This is stupind, can be solved with just 1 swap.
  for (size_t i = 0; i < len; ++i) {
    if (pg->arr[i].group_id == group_id) {
      found = true;
      br_data_deinit(&pg->arr[i]);
    }
    if (found == true && i + 1 < len) {
      memcpy(&pg->arr[i], &pg->arr[i + 1], sizeof(br_data_t));
    }
  }
  if (found == true) {
    memset(&pg->arr[len - 1], 0, sizeof(br_data_t));
    --pg->len;
    br_plot_remove_group(*plots, group_id);
  }
}

void br_data_export(br_data_t const* pg, FILE* file) {
  for (size_t i = 0; i < pg->len; ++i) {
    Vector2 point = pg->points[i];
    fprintf(file, "%f,%f;%d\n", point.x, point.y, pg->group_id);
  }
}

void br_data_export_csv(br_data_t const* pg, FILE* file) {
  fprintf(file, "group,id,x,y\n");
  for (size_t i = 0; i < pg->len; ++i) {
    Vector2 point = pg->points[i];
    fprintf(file, "%d,%lu,%f,%f\n", pg->group_id, i, point.x, point.y);
  }
}

void br_datas_export(br_datas_t const* pg_array, FILE* file) {
  for (size_t i = 0; i < pg_array->len; ++i) {
    br_data_export(&pg_array->arr[i], file);
  }
}

void br_datas_export_csv(br_datas_t const* pg_array, FILE* file) {
  fprintf(file, "group,id,x,y\n");
  for (size_t j = 0; j < pg_array->len; ++j) {
    br_data_t* pg = &pg_array->arr[j];
    for (size_t i = 0; i < pg->len; ++i) {
      Vector2 point = pg->points[i];
      fprintf(file, "%d,%lu,%f,%f\n", pg->group_id, i, point.x, point.y);
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
      br_data_push_point(g, (Vector2){(float)g->len/128.f, sinf((float)g->len/128.f)});
  }
  {
    int group = 1;
    br_data_t* g = br_data_get(pg, group);
    if (NULL == g) return;
    for (int i = 0; i < 10*1024; ++i)
      br_data_push_point(g, (Vector2){-(float)g->len/128.f, sinf((float)g->len/128.f)});
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
      br_data_push_point(g, p);
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
        br_data_push_point(g, p);
      }
    }
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

static br_data_t* br_data_init(br_data_t* g, int group_id) {
  *g = (br_data_t) { .cap = 1024, .len = 0, .group_id = group_id,
    .is_selected = true,
    .points = BR_MALLOC(sizeof(Vector2) * 1024),
    .resampling = resampling2_malloc(),
    .color = color_get(group_id),
    .name = br_str_malloc(32),
    .is_new = true
  };
  if (NULL != g->name.str) {
    sprintf(g->name.str, "Plot #%d", group_id);
    g->name.len = (unsigned int)strlen(g->name.str);
  }
  return g;
}

static br_data_3d_t* br_data_3d_init(br_data_3d_t* g, int group_id) {
  *g = (br_data_3d_t) { .cap = 1024, .len = 0, .group_id = group_id,
    .is_selected = true,
    .points = BR_MALLOC(sizeof(Vector2) * 1024),
    .resampling = resampling2_malloc(),
    .color = color_get(group_id),
    .name = br_str_malloc(32),
    .is_new = true
  };
  if (NULL != g->name.str) {
    sprintf(g->name.str, "Plot #%d", group_id);
    g->name.len = (unsigned int)strlen(g->name.str);
  }
  return g;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
static Color base_colors[8] = { RED, GREEN, BLUE, LIGHTGRAY, PINK, GOLD, VIOLET, DARKPURPLE };
#pragma GCC diagnostic pop

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
  assert(pg);

  // TODO: da
  if (pg->len == 0) {
    pg->arr = BR_MALLOC(sizeof(br_data_t));
    if (NULL == pg->arr) return NULL;
    br_data_t* ret = br_data_init(&pg->arr[0], group);
    if (ret->points == NULL) return NULL;
    pg->cap = pg->len = 1;
    return ret;
  }

  for (size_t i = 0; i < pg->len; ++i) {
    if (pg->arr[i].group_id == group) {
      return &pg->arr[i];
    }
  }

  if (pg->len >= pg->cap) {
    size_t new_cap = pg->cap * 2;
    br_data_t* new_arr = BR_REALLOC(pg->arr, sizeof(br_data_t)*new_cap);
    if (NULL == new_arr) return NULL;
    pg->arr = new_arr;
    pg->cap = new_cap;
  }
  br_data_t* ret = br_data_init(&pg->arr[pg->len++], group);
  if (ret->points == NULL) {
    --pg->len;
    return NULL;
  }
  return ret;
}

BR_API br_data_3d_t* br_data_3d_get(br_datas_3d_t* pg, int group) {
  assert(pg);

  // TODO: da
  if (pg->len == 0) {
    pg->arr = BR_MALLOC(sizeof(br_data_3d_t));
    if (NULL == pg->arr) return NULL;
    br_data_3d_t* ret = br_data_3d_init(&pg->arr[0], group);
    if (ret->points == NULL) return NULL;
    pg->cap = pg->len = 1;
    return ret;
  }

  for (size_t i = 0; i < pg->len; ++i) {
    if (pg->arr[i].group_id == group) {
      return &pg->arr[i];
    }
  }

  if (pg->len >= pg->cap) {
    size_t new_cap = pg->cap * 2;
    br_data_3d_t* new_arr = BR_REALLOC(pg->arr, sizeof(br_data_3d_t)*new_cap);
    if (NULL == new_arr) return NULL;
    pg->arr = new_arr;
    pg->cap = new_cap;
  }
  br_data_3d_t* ret = br_data_3d_init(&pg->arr[pg->len++], group);
  if (ret->points == NULL) {
    --pg->len;
    return NULL;
  }
  return ret;
}

BR_API void br_data_set_name(br_datas_t* pg, int group, br_str_t name) {
  br_data_t* g = br_data_get(pg, group);
  if (pg == NULL) return;
  br_str_free(g->name);
  g->name = name;
}

static void br_data_push_point(br_data_t* g, Vector2 v) {
  if (g->len >= g->cap && false == br_data_realloc(g, g->cap * 2)) return;
  if (g->len == 0) g->bounding_box = (bb_t) { v.x, v.y, v.x, v.y };
  else             br_bb_expand_with_point(&g->bounding_box, v);
  g->points[g->len] = v;
  resampling2_add_point(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_data_3d_push_point(br_data_3d_t* g, Vector3 v) {
  if (g->len >= g->cap && false == br_data_3d_realloc(g, g->cap * 2)) return;
  if (g->len == 0) g->bounding_box = (bb_3d_t) { v.x, v.y, v.z, v.x, v.y, v.z };
  else             br_bb_3d_expand_with_point(&g->bounding_box, v);
  g->points[g->len] = v;
  resampling2_add_point_3d(g->resampling, g, (uint32_t)g->len);
  ++g->len;
}

static void br_data_deinit(br_data_t* g) {
  // Free points
  BR_FREE(g->points);
  resampling2_free(g->resampling);
  br_str_free(g->name);
  g->points = NULL;
  g->len = g->cap = 0;
}

static bool br_data_realloc(br_data_t* pg, size_t new_cap) {
  Vector2* new_arr = BR_REALLOC(pg->points, new_cap * sizeof(Vector2));
  if (new_arr == NULL) {
    LOG("Out of memory. Can't add any more lines. Buy more RAM, or close Chrome\n");
    return false;
  }
  pg->points = new_arr;
  pg->cap = new_cap;
  return true;
}

static bool br_data_3d_realloc(br_data_3d_t* pg, size_t new_cap) {
  Vector3* new_arr = BR_REALLOC(pg->points, new_cap * sizeof(Vector3));
  if (new_arr == NULL) {
    LOG("Out of memory. Can't add any more lines. Buy more RAM, or close Chrome\n");
    return false;
  }
  pg->points = new_arr;
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

