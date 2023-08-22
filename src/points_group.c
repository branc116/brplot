#include "plotter.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

static points_group_t* points_group_init(points_group_t* g, int group_id);
static points_group_t* points_group_get(points_group_t* pg_array, size_t* pg_array_len, size_t pg_array_cap, int group);
static void points_group_push_point(points_group_t* g, Vector2 v);
static void points_group_deinit(points_group_t* g);
static bool points_group_realloc(points_group_t* pg, size_t new_cap);

void points_group_push_y(points_group_t* pg_array, size_t* pg_array_len, size_t pg_array_cap, float y, int group) {
  points_group_t* pg = points_group_get(pg_array, pg_array_len, pg_array_cap, group);
  points_group_push_point(pg, (Vector2){ .x = (float)pg->len, .y = y });
}

void points_group_push_xy(points_group_t* pg_array, size_t* pg_array_len, size_t pg_array_cap, float x, float y, int group) {
  points_group_t* pg = points_group_get(pg_array, pg_array_len, pg_array_cap, group);
  points_group_push_point(pg, (Vector2){ .x = x, .y = y });
}

void points_group_clear(points_group_t* pg_array, size_t* pg_array_len, int group_id) {
  size_t len = *pg_array_len;
  bool found = false;
  for (size_t i = 0; i < len; ++i) {
    if (pg_array[i].group_id == group_id) {
      found = true;
      points_group_deinit(&pg_array[i]);
    }
    if (found == true && i + 1 < len) {
      memcpy(&pg_array[i], &pg_array[i + 1], sizeof(points_group_t));
    }
  }
  if (found == true) {
    memset(&pg_array[len - 1], 0, sizeof(pg_array[0]));
    --*pg_array_len;
  }
}

void points_group_clear_all(points_group_t* pg_array, size_t* pg_array_len) {
  size_t len = *pg_array_len;
  for (size_t i = 0; i < len; ++i) {
    points_group_t* g = &pg_array[i];
    points_group_deinit(g);
  }
  memset(pg_array, 0, sizeof(points_group_t) * *pg_array_len);
  *pg_array_len = 0;
}

void points_group_add_test_points(points_group_t* pg_array, size_t* pg_len, size_t pg_array_cap) {
  {
    int group = 0;
    points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, group);
    for (int i = 0; i < 1024; ++i)
      points_group_push_point(g, (Vector2){(float)g->len, sinf((float)g->len/128.f)});
  }
  {
    int group = 1;
    points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, group);
    for (int i = 0; i < 1024; ++i)
      points_group_push_point(g, (Vector2){-(float)g->len, sinf((float)g->len/128.f)});
  }
  for(int i = 0; i < 1024*1024; ++i) {
    int group = 5;
    points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, group);
    float t = (float)(1 + g->len)*.1f;
    float x = sqrtf(t)*cosf(log2f(t));
    float y = sqrtf(t)*sinf(log2f(t));
    Vector2 p = {x, y };
    points_group_push_point(g, p);
  }
  {
    points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, 6);
    int l = (int)g->len;
    for(int i = 0; i < 0; ++i) {
      for(int j = 0; j < 0; ++j) {
        printf("i = %d\n", i);
        int x = -50 + j + l;
        int y = (-50 + (i - j) + l) * (i % 2 == 0 ? 1 : -1);
        Vector2 p = {(float)x, (float)y};
        points_group_push_point(g, p);
      }
    }
  }
}

void points_groups_draw(points_group_t* gs, size_t len, smol_mesh_t* line_mesh, smol_mesh_t* quad_mesh, Color* colors, Rectangle rect) {
  for (size_t j = 0; j < len; ++j) {
    points_group_t * g = &gs[j];
    if (g->is_selected) {
      Color c = colors[j];
      resampling_draw(g->resampling, rect, line_mesh, quad_mesh, c);
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

static points_group_t* points_group_init(points_group_t* g, int group_id) {
    g->cap = 1024;
    g->len = 0;
    g->group_id = group_id;
    g->is_selected = true;
    g->points = malloc(sizeof(Vector2) * 1024);
    g->resampling = resampling_malloc((Vector2 const **)&g->points);
    return g;
}

static points_group_t* points_group_get(points_group_t* pg_array, size_t* pg_array_len, size_t pg_array_cap, int group) {
  assert(pg_array);
  assert(pg_array_len != NULL);

  if (*pg_array_len == 0) {
    return points_group_init(&pg_array[(*pg_array_len)++], group);
  }

  for (size_t i = 0; i < *pg_array_len; ++i) {
    if (pg_array[i].group_id == group) {
      return &pg_array[i];
    }
  }

  assert(*pg_array_len < pg_array_cap);
  return points_group_init(&pg_array[(*pg_array_len)++], group);
}

static void points_group_push_point(points_group_t* g, Vector2 v) {
  if (g->len >= g->cap) {
    assert(points_group_realloc(g, g->cap * 2));
  }
  g->points[g->len] = v;
  resampling_add_point(g->resampling, g->len);
  ++g->len;
}

static void points_group_deinit(points_group_t* g) {
    // Free points
    free(g->points);
    resampling_free(g->resampling);
    g->points = NULL;
    g->len = 0;
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

