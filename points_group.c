#include "points_group.h"

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

static points_group_t* points_group_init(points_group_t* g, int cap, int group_id, Vector2* points) {
    g->cap = cap;
    g->len = 0;
    g->group_id = group_id;
    g->is_selected = true;
    g->points = points;
    g->smol_meshes_len = 0;
    return g;
}

points_group_t* points_group_get(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, Vector2* all_points, int group) {
  assert(pg_array);
  assert(pg_array_len != NULL);
  assert(*pg_array_len < pg_array_cap);

  if (*pg_array_len == 0) {
    return points_group_init(&pg_array[(*pg_array_len)++], POINTS_CAP, group, all_points);
  }

  int max_size = 0;
  points_group_t* max_group = &pg_array[0];
  for (int i = 0; i < *pg_array_len; ++i) {
    if (pg_array[i].group_id == group) {
      return &pg_array[i];
    }
    int size = pg_array[i].cap;
    if (size > max_size) {
      max_group = &pg_array[i];
      max_size = size;
    }
  }
  int l = max_group->cap;
  Vector2* ns2 = max_group->points + (l - (l / 2));
  max_group->cap = l - (l / 2);
  return points_group_init(&pg_array[(*pg_array_len)++], l / 2, group, ns2);
}

void points_group_push_point(points_group_t* g, Vector2 v) {
  if (g->len + 1 > g->cap) {
    fprintf(stderr, "Trying to add point to a group thats full");
    exit(-1);
  }
  g->points[g->len++] = v;
}

void points_groups_draw(points_group_t* gs, int len, Shader shader, int color_uniform, Color* colors) {
  for (int j = 0; j < len; ++j) {
    points_group_t * g = &gs[j];
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
      int ml = g->smol_meshes_len;
      Color c = colors[j];
      Vector3 cv = {c.r/255.f, c.g/255.f, c.b/255.f};
      SetShaderValue(shader, color_uniform, &cv, SHADER_UNIFORM_VEC3);
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

void points_group_add_test_points(points_group_t* pg_array, int* pg_len, int pg_array_cap, Vector2* all_points) {
  for (int harm = 1; harm <= 4; ++harm) {
    for(int i = 0; i < 1025; ++i) {
      int group = harm;
      points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, all_points, group);
      float x = g->len*.1;
      float y = (float)x*0.1;
      Vector2 p = {x*.1, .1*harm*sin(10.*y/(1<<harm)) };
      points_group_push_point(g, p);
    }
  }

  for(int i = 0; i < 1025; ++i) {
    int group = 5;
    points_group_t* g = points_group_get(pg_array, pg_len, pg_array_cap, all_points, group);
    float t = g->len*.1;
    float x = sqrtf(t)*cos(log2f(t));
    float y = sqrtf(t)*sin(log2f(t));
    Vector2 p = {x, y };
    points_group_push_point(g, p);
  }
}

