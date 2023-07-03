#include "plotter.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define sample_points_cap 1024
static Vector2 sample_points[sample_points_cap];

static points_group_t* points_group_init(points_group_t* g, int group_id);
static points_group_t* points_group_get(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, int group);
static void points_group_push_point(points_group_t* g, Vector2 v);
static void points_group_deinit(points_group_t* g);
static Vector2 interpolate(Vector2 a, Vector2 b, float x);
static Vector2 const* binary_search(Vector2 const* lb, Vector2 const* ub, float x_value);
static bool points_group_realloc(points_group_t* pg, int new_cap);
static int points_group_sample_points(points_group_t const* g, Rectangle rect, Vector2* out_points, int max_number_of_points);

void points_group_push_y(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, float y, int group) {
  points_group_t* pg = points_group_get(pg_array, pg_array_len, pg_array_cap, group);
  points_group_push_point(pg, (Vector2){ .x = pg->len, .y = y });
}

void points_group_push_xy(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, float x, float y, int group) {
  points_group_t* pg = points_group_get(pg_array, pg_array_len, pg_array_cap, group);
  points_group_push_point(pg, (Vector2){ .x = x, .y = y });
}

void points_group_clear(points_group_t* pg_array, int* pg_array_len, int group_id) {
  int len = *pg_array_len;
  bool found = false;
  for (int i = 0; i < len; ++i) {
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

void points_group_clear_all(points_group_t* pg_array, int* pg_array_len) {
  int len = *pg_array_len;
  for (int i = 0; i < len; ++i) {
    points_group_t* g = &pg_array[i];
    points_group_deinit(g);
  }
  memset(pg_array, 0, sizeof(pg_array[0]) * *pg_array_len);
  *pg_array_len = 0;
}

void points_group_add_test_points(points_group_t* pg_array, int* pg_len, int pg_array_cap) {
  for (int harm = 1; harm <= 4; ++harm) {
    int group = harm, points_to_add = 10025;
    points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, group);
    // This can only be called from render thread and render thread must realloc points array.
    if (g->cap <= g->len + points_to_add) {
      if (!points_group_realloc(g, 2 * (g->len + points_to_add))) {
        continue;
      }
    }
    for(int i = 0; i < points_to_add; ++i) {
      float x = g->len*.1;
      float y = (float)x*0.1;
      Vector2 p = {x*.1, .1*harm*sin(10.*y/(1<<harm)) };
      points_group_push_point(g, p);
    }
  }
/*
  for(int i = 0; i < 1025; ++i) {
    int group = 5;
    points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, all_points, group);
    float t = g->len*.1;
    float x = sqrtf(t)*cos(log2f(t));
    float y = sqrtf(t)*sin(log2f(t));
    Vector2 p = {x, y };
    points_group_push_point(g, p);
  }
*/
}

void points_groups_draw(points_group_t* gs, int len, Shader shader, int color_uniform, Color* colors, Rectangle rect) {
  for (int j = 0; j < len; ++j) {
    points_group_t * g = &gs[j];
    if (g->is_sorted) continue; // Don't move to gpu if group is soreted, no need for it...
    if (g->len / PTOM_COUNT > g->smol_meshes_len ) {
      int indx = g->smol_meshes_len++;
      smol_mesh_t* sm = &g->meshes[indx];
      smol_mesh_init(sm);
      assert(smol_mesh_gen_line_strip(sm, g->points, g->len, indx*sm->length));
      smol_mesh_upload(sm, false);
    }
  }
  for (int j = 0; j < len; ++j) {
    points_group_t * g = &gs[j];
    if (g->is_selected) {
      Color c = colors[j];
      Vector3 cv = {c.r/255.f, c.g/255.f, c.b/255.f};
      SetShaderValue(shader, color_uniform, &cv, SHADER_UNIFORM_VEC3);
      if (g->is_sorted) {
        int len = points_group_sample_points(g, rect, sample_points, sample_points_cap);
        if (len > 1){
          smol_mesh_gen_line_strip(smol_mesh_get_temp(), sample_points, len, 0);
          smol_mesh_update(smol_mesh_get_temp());
          smol_mesh_draw_line_strip(smol_mesh_get_temp(), shader);
        }

      } else {
        int ml = g->smol_meshes_len;
        if (smol_mesh_gen_line_strip(smol_mesh_get_temp(), g->points, g->len, g->smol_meshes_len*(PTOM_COUNT))) {
          smol_mesh_update(smol_mesh_get_temp());
          smol_mesh_draw_line_strip(smol_mesh_get_temp(), shader);
        }
        for (int k = 0; k < ml; ++k) {
          smol_mesh_draw_line_strip(&g->meshes[k], shader);
        }
      }
    }
  }
}

static int points_group_sample_points(points_group_t const* g, Rectangle rect, Vector2* out_points, int max_number_of_points) {
  if (g->len == 0) {
    return 0;
  }
  int out_index = 0, i = 0;
  float step = 1.1*rect.width/max_number_of_points;
  Vector2 const* lb = g->points, *ub = &g->points[g->len - 1];
  while (lb != NULL && i < max_number_of_points) {
    float cur = rect.x + step * i++;
    Vector2 const* clb = binary_search(lb, ub, cur); // Clb will alway be LESS than g->len - 1, so Clb + 1 will always be LESS OR EQUAL to g->len - 1
    if (clb != NULL) {
      out_points[out_index++] = interpolate(clb[0], clb[1], cur);
      lb = clb;
    }
  }
  return out_index;
}

static points_group_t* points_group_init(points_group_t* g, int group_id) {
    g->cap = 1024;
    g->len = 0;
    g->group_id = group_id;
    g->is_selected = true;
    g->points = malloc(sizeof(Vector2) * 1024);
    g->smol_meshes_len = 0;
    g->is_sorted = true;
    return g;
}

static points_group_t* points_group_get(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, int group) {
  assert(pg_array);
  assert(pg_array_len != NULL);

  if (*pg_array_len == 0) {
    return points_group_init(&pg_array[(*pg_array_len)++], group);
  }

  for (int i = 0; i < *pg_array_len; ++i) {
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
  if (g->len > 0 && g->is_sorted && g->points[g->len - 1].x > v.x) {
    g->is_sorted = false;
  }
  g->points[g->len++] = v;
}

static void points_group_deinit(points_group_t* g) {
    // Remove smol meshes
    int sm_len = g->smol_meshes_len;
    for (int j = 0; j < sm_len; ++j) {
      smol_mesh_unload(&g->meshes[j]);
    }
    // Free points
    free(g->points);
    g->points = NULL;
    g->len = 0;
}

static Vector2 interpolate(Vector2 a, Vector2 b, float x) {
  assert(x >= a.x);
  assert(x <= b.x);
  assert(a.x <= b.x);
  float k = (x - a.x)/(b.x - a.x);
  return (Vector2){.x = a.x*(1 - k) + b.x*k, .y =  a.y*(1 - k) + b.y*k };
}

static Vector2 const* binary_search(Vector2 const* lb, Vector2 const* ub, float x_value) {
  if (lb->x > x_value)
    return NULL;
  if (ub->x < x_value)
    return NULL;
  while (ub - lb > 1) {
    Vector2 const* mid = lb + (ub - lb)/2;
    if (mid->x < x_value) lb = mid;
    else if (mid->x > x_value) ub = mid;
    else return mid;
  }
  return lb;
}

static bool points_group_realloc(points_group_t* pg, int new_cap) {
    Vector2* new_arr = realloc(pg->points, new_cap * sizeof(Vector2));
    if (new_arr == NULL) {
      fprintf(stderr, "Out of memory. Can't add any more lines. Buy more RAM, or close Chrome");
      return false;
    }
    pg->points = new_arr;
    pg->cap = new_cap;
    return true;
}

