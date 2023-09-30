#include "plotter.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

static points_group_t* points_group_init(points_group_t* g, int group_id);
static points_group_t* points_group_get(points_groups_t* pg_array, int group);
static void points_group_push_point(points_group_t* g, Vector2 v);
static void points_group_deinit(points_group_t* g);
static bool points_group_realloc(points_group_t* pg, size_t new_cap);

void points_group_push_y(points_groups_t* pg_array, float y, int group) {
  points_group_t* pg = points_group_get(pg_array, group);
  points_group_push_point(pg, (Vector2){ .x = (float)pg->len, .y = y });
}

void points_group_push_xy(points_groups_t* pg_array, float x, float y, int group) {
  points_group_t* pg = points_group_get(pg_array, group);
  points_group_push_point(pg, (Vector2){ .x = x, .y = y });
}

void points_group_clear(points_groups_t* pg, int group_id) {
  size_t len = pg->len;
  bool found = false;
  for (size_t i = 0; i < len; ++i) {
    if (pg->arr[i].group_id == group_id) {
      found = true;
      points_group_deinit(&pg->arr[i]);
    }
    if (found == true && i + 1 < len) {
      memcpy(&pg->arr[i], &pg->arr[i + 1], sizeof(points_group_t));
    }
  }
  if (found == true) {
    memset(&pg->arr[len - 1], 0, sizeof(points_group_t));
    --pg->len;
  }
}

void points_groups_deinit(points_groups_t* arr) {
  if (arr->arr == NULL) return;
  for (size_t i = 0; i < arr->len; ++i) {
    points_group_deinit(&arr->arr[i]);
  }
  arr->len = arr->cap = 0;
  free(arr->arr);
  arr->arr = NULL;
}

void points_group_add_test_points(points_groups_t* pg) {
  {
    int group = 0;
    points_group_t* g = points_group_get(pg, group);
    for (int i = 0; i < 1024; ++i)
      points_group_push_point(g, (Vector2){(float)g->len/128.f, sinf((float)g->len/128.f)});
  }
  {
    int group = 1;
    points_group_t* g = points_group_get(pg, group);
    for (int i = 0; i < 1024; ++i)
      points_group_push_point(g, (Vector2){-(float)g->len/128.f, sinf((float)g->len/128.f)});
  }
  {
    int group = 5;
    points_group_t* g = points_group_get(pg, group);
    for(int i = 0; i < 1024*1024; ++i) {
      float t = (float)(1 + g->len)*.1f;
      float x = sqrtf(t)*cosf(log2f(t));
      float y = sqrtf(t)*sinf(log2f(t));
      Vector2 p = {x, y };
      points_group_push_point(g, p);
    }
  }
  {
    points_group_t* g = points_group_get(pg, 6);
    int l = (int)g->len;
    for(int i = 0; i < 0; ++i) {
      for(int j = 0; j < 0; ++j) {
        int x = -50 + j + l;
        int y = (-50 + (i - j) + l) * (i % 2 == 0 ? 1 : -1);
        Vector2 p = {(float)x, (float)y};
        points_group_push_point(g, p);
      }
    }
  }
}

void points_groups_draw(points_groups_t const* pg, smol_mesh_t* line_mesh, smol_mesh_t* quad_mesh, Rectangle rect) {
  for (size_t j = 0; j < pg->len; ++j) {
    points_group_t * g = &pg->arr[j];
    if (g->is_selected) {
      resampling_draw(g->resampling, g, rect, line_mesh, quad_mesh);
    }
  }
  if (line_mesh->cur_len > 0) {
    smol_mesh_update(line_mesh);
    smol_mesh_draw(line_mesh);
  }
  if (quad_mesh->cur_len > 0) {
    smol_mesh_update(quad_mesh);
    smol_mesh_draw(quad_mesh);
  }
}

static Color color_get(int id);

static points_group_t* points_group_init(points_group_t* g, int group_id) {
  *g = (points_group_t) { .cap = 1024, .len = 0, .group_id = group_id,
    .is_selected = true,
    .points = malloc(sizeof(Vector2) * 1024),
    .resampling = resampling_malloc(),
    .color = color_get(group_id)
  };
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

static points_group_t* points_group_get(points_groups_t* pg, int group) {
  assert(pg);

  if (pg->len == 0) {
    pg->arr = malloc(sizeof(points_group_t));
    pg->cap = 1;
    return points_group_init(&pg->arr[pg->len++], group);
  }

  for (size_t i = 0; i < pg->len; ++i) {
    if (pg->arr[i].group_id == group) {
      return &pg->arr[i];
    }
  }

  if (pg->len >= pg->cap) {
    pg->cap *= 2;
    pg->arr = realloc(pg->arr, sizeof(points_group_t)*pg->cap);
    assert(pg->arr);
  }
  return points_group_init(&pg->arr[pg->len++], group);
}

static void points_group_push_point(points_group_t* g, Vector2 v) {
  if (g->len >= g->cap) {
    assert(points_group_realloc(g, g->cap * 2));
  }
  g->points[g->len] = v;
  resampling_add_point(g->resampling, g, g->len);
  ++g->len;
}

static void points_group_deinit(points_group_t* g) {
    // Free points
    free(g->points);
    resampling_free(g->resampling);
    g->points = NULL;
    g->len = g->cap = 0;
}

static bool points_group_realloc(points_group_t* pg, size_t new_cap) {
    Vector2* new_arr = realloc(pg->points, new_cap * sizeof(Vector2));
    if (new_arr == NULL) {
      fprintf(stderr, "Out of memory. Can't add any more lines. Buy more RAM, or close Chrome");
      return false;
    }
    pg->points = new_arr;
    pg->cap = new_cap;
    return true;
}

