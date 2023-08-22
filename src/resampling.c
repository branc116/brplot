#include "plotter.h"
#include "assert.h"
#include "stdlib.h"
#include "stdint.h"
#include "math.h"
#include <raylib.h>
#include "tests.h"

#define temp_points_count 1024

typedef struct {
  int index, next_index;
  float factor; // 1 => val i exactly on point, 0 => next_val is exactly on point, (0, 1) -> valid, else notfound
} binary_search_res;

static binary_search_res binary_search(float const* lb, float const* ub, float value, int stride);
static size_t points_group_sample_points(Vector2 const* points, size_t len, resampling_dir dir, Rectangle rect, Vector2* out_points, size_t max_number_of_points);
static bool help_check_collision_bb_rec(bb_t bb, Rectangle rec);
static Rectangle help_bb_to_rect(bb_t bb);
static void resampling_add_interval(resampling_t* res);

resampling_t* resampling_malloc(Vector2 const ** points) {
  resampling_t* ret = (resampling_t*)malloc(sizeof(resampling_t));
  ret->points = points;
  ret->intervals = NULL;
  ret->intervals_cap = ret->intervals_count = 0;
  ret->temp_points = malloc(temp_points_count * sizeof(Vector2));
  return ret;
}

void resampling_free(resampling_t* res) {
  if (res->intervals) {
    free(res->intervals);
  }
  free(res->temp_points);
  free(res);
}

size_t resampling_draw(resampling_t* res, Rectangle screen, smol_mesh_t* lines_mesh, smol_mesh_t* quad_mesh, Color color) {
  (void)quad_mesh;
  size_t ret = res->resampling_count = res->raw_count = 0;
  for (size_t i = 0; i < res->intervals_count; ++i) {
    resamping_interval_t* inter = &res->intervals[i];
    Vector2 const * ps = &(*res->points)[inter->from];
    smol_mesh_gen_bb(lines_mesh, inter->bounds, color);
    if (!help_check_collision_bb_rec(inter->bounds, screen)) continue;
    if (inter->count < 128) {
      smol_mesh_gen_line_strip(lines_mesh, ps, inter->count, color);
      ret += inter->count;
      ++res->raw_count;
    } else {
      size_t s = points_group_sample_points(ps, inter->count, inter->dir, screen, res->temp_points, temp_points_count);
      if (s == 0) continue;
      smol_mesh_gen_line_strip(lines_mesh, res->temp_points, s, color);
      ret += s;
      ++res->resampling_count;
    }
  }
  return ret;
}

void resampling_add_point(resampling_t* res, size_t index) { 
  Vector2 const * all_points =  *res->points;
  Vector2 point = all_points[index];
  if (res->intervals_count == 0)
    resampling_add_interval(res);
  resamping_interval_t* last_interval = &res->intervals[res->intervals_count - 1];
  int new_dir = last_interval->dir;
  if (last_interval->count != 0) {
    size_t last_index = last_interval->from + last_interval->count - 1;
    Vector2 last_point = all_points[last_index];
    bool is_left  = point.x <= last_point.x,
         is_right = point.x >= last_point.x,
         is_up    = point.y >= last_point.y,
         is_down  = point.y <= last_point.y;
    new_dir &=  
     (~resampling_dir_left  | (is_left  ? resampling_dir_left  : 0)) &
     (~resampling_dir_right | (is_right ? resampling_dir_right : 0)) &
     (~resampling_dir_up    | (is_up    ? resampling_dir_up    : 0)) &
     (~resampling_dir_down  | (is_down  ? resampling_dir_down  : 0));
  } else {
    last_interval->from = index;
  }
  if (new_dir == resampling_dir_null) {
    resampling_add_interval(res);
    resampling_add_point(res, index - 1);
    resampling_add_point(res, index);
  } else {
    last_interval->dir = (resampling_dir)new_dir;
    if (last_interval->count == 0) {
      last_interval->bounds.xmax = last_interval->bounds.xmin = point.x;
      last_interval->bounds.ymax = last_interval->bounds.ymin = point.y;
    } else {
      last_interval->bounds.xmax = maxf(last_interval->bounds.xmax, point.x);
      last_interval->bounds.ymax = maxf(last_interval->bounds.ymax, point.y);
      last_interval->bounds.xmin = minf(last_interval->bounds.xmin, point.x);
      last_interval->bounds.ymin = minf(last_interval->bounds.ymin, point.y);
    }
    ++last_interval->count;
  }
}

static void resampling_add_interval(resampling_t* res) {
  size_t new_index = res->intervals_count++;
  if (res->intervals_cap <= res->intervals_count) {
    size_t new_cap = res->intervals_count * 2;
    res->intervals = res->intervals_cap == 0 ? malloc(new_cap * sizeof(resamping_interval_t)) : realloc(res->intervals, sizeof(resamping_interval_t) * new_cap);
    res->intervals_cap = new_cap;
  }
  res->intervals[new_index] = (resamping_interval_t) { .from = 0, .count = 0, .dir = 15, .bounds = {0} };
  return;
}

static size_t points_group_sample_points(Vector2 const* points, size_t len, resampling_dir dir, Rectangle rect, Vector2* out_points, size_t max_number_of_points) {
  if (len == 0) {
    return 0;
  }
  float step_margin = 1.0f, step_offset = 2.f;
  size_t out_index = 0, i = 0;
  if (dir & resampling_dir_right) {
    float step = step_margin*rect.width/(float)max_number_of_points;
    Vector2 const* lb = points, *ub = &points[len - 1];
    while (lb != NULL && i < max_number_of_points) {
      float cur = rect.x + step * (-step_offset + (float)(i++));
      binary_search_res res = binary_search(&lb->x, &ub->x, cur, 2);
      if (res.factor <= 1.f) {
        out_points[out_index++] = (Vector2){lb[res.index].x * res.factor + lb[res.next_index].x * (1 - res.factor),
                                            lb[res.index].y * res.factor + lb[res.next_index].y * (1 - res.factor)};
        lb = &lb[res.index];
      }
    }
  } else if (dir & resampling_dir_left) {
    float step = step_margin*rect.width/(float)max_number_of_points;
    Vector2 const* ub = points, *lb = &points[len - 1];
    while (lb != NULL && i < max_number_of_points) {
      float cur = rect.x + rect.width - step * (step_offset + (float)(i++));
      binary_search_res res = binary_search(&lb->x, &ub->x, cur, -2);
      if (res.factor <= 1.f) {
        out_points[out_index++] = (Vector2){lb[res.index].x * res.factor + lb[res.next_index].x * (1 - res.factor),
                                            lb[res.index].y * res.factor + lb[res.next_index].y * (1 - res.factor)};
        ub = &lb[res.next_index];
      }
    }
  } else if (dir & resampling_dir_up) {
    float step = step_margin*rect.height/(float)max_number_of_points;
    Vector2 const* lb = points, *ub = &points[len - 1];
    while (lb != NULL && i < max_number_of_points) {
      float cur = rect.y - rect.height + step * (-step_offset + (float)(i++));
      binary_search_res res = binary_search(&lb->y, &ub->y, cur, 2);
      if (res.factor <= 1.f) {
        out_points[out_index++] = (Vector2){lb[res.index].x * res.factor + lb[res.next_index].x * (1 - res.factor),
                                            lb[res.index].y * res.factor + lb[res.next_index].y * (1 - res.factor)};
        lb = &lb[res.index];
      }
    }
  } else if (dir & resampling_dir_down) {
    float step = step_margin*rect.height/(float)max_number_of_points;
    Vector2 const* ub = points, *lb = &points[len - 1];
    while (lb != NULL && i < max_number_of_points) {
      float cur = rect.y - step * (step_offset + (float)(i++));
      binary_search_res res = binary_search(&lb->y, &ub->y, cur, -2);
      if (res.factor <= 1.f) {
        out_points[out_index++] = (Vector2){lb[res.index].x * res.factor + lb[res.next_index].x * (1 - res.factor),
                                            lb[res.index].y * res.factor + lb[res.next_index].y * (1 - res.factor)};
        ub = &lb[res.next_index];
      }
    }
  } else {
    assert(0);
  }
  return out_index;
}

static binary_search_res binary_search(float const* lb, float const* ub, float value, int stride) {
  if (*lb > value)
    return (binary_search_res){0,0,69.f};
  if (*ub < value)
    return (binary_search_res){0,0,69.f};
  assert(stride != 0);
  assert((ub - lb) % stride == 0);
  int index_lb = 0, index_ub = (int)(ub - lb) / stride, index_mid = 0;
  float const* mid = lb;

  while (index_ub - index_lb > 1) {
    index_mid = (index_lb + index_ub)/2;
    mid = &lb[index_mid * stride];
    if (*mid  < value) index_lb = index_mid;
    else if (*mid > value) index_ub = index_mid;
    else break;
  }
  index_mid *= signi(stride);
  float const * next = mid + stride;
  if ((stride > 0 && next <= ub && next >= lb) || (stride < 0 && next >= ub && next <= lb)) {
    float fact = (value - *mid) / (*next - *mid);
    return (binary_search_res){index_mid, index_mid + signi(stride), 1 - fact};
  } else {
    return (binary_search_res){index_mid, index_mid, 1};
  }
}

TEST_CASE(binary_search_tests) {
  Vector2 vec[] = {{0, 1}, {1, 2}, {2, 3}};
  TEST_EQUAL(1, binary_search(&vec[0].x, &vec[2].x, 1.f, 2).index);

  Vector2 vec2[] = {{0, 1}, {1, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}};
  TEST_EQUAL(4, binary_search(&vec2[0].x, &vec2[5].x, 6.f, 2).index);
  TEST_EQUAL(4, binary_search(&vec2[0].x, &vec2[5].x, 7.f, 2).index);
  TEST_TRUE(1.f > binary_search(&vec2[0].x, &vec2[5].x, 7.f, 2).factor);

  TEST_EQUAL(1, ((float*)&vec2[1])[-1]); // Checking negative indicies...
  Vector2 vec3[] = {{8, 1}, {6, 1}, {4, 3}, {2, 5}, {1, 7}, {0, 9}};
  TEST_EQUAL(-4, binary_search(&vec3[5].x, &vec3[0].x, 6.f, -2).index); //lb=5, index = -4, lb[-4] == vec3[1]
                                                                        //
  Vector2 vecy[] = {{10, 1}, {10, 2}, {5, 3}, {15, 5}, {20, 7}, {10, 9}};
  TEST_EQUAL(4, binary_search(&vecy[0].y, &vecy[5].y, 7.f, 2).index);

  Vector2 vecy2[] = {{10, 9}, {10, 7}, {5, 5}, {15, 3}, {20, 2}, {10, 1}};
  TEST_EQUAL(-4, binary_search(&vecy2[5].y, &vecy2[0].y, 7.f, -2).index);
}

static bool __attribute__((unused)) help_check_collision_bb_p(bb_t bb, Vector2 p) {
  return bb.xmin <= p.x && bb.xmax >= p.x && bb.ymin <= p.y && bb.ymax >= p.y;
}

static bool __attribute__((unused)) help_check_collision_bb_rec(bb_t bb, Rectangle rec) {
  return (bb.xmin <= (rec.x + rec.width)) &&
         (bb.xmax >= rec.x) &&
         (bb.ymin <= rec.y) &&
         (bb.ymax >= (rec.y - rec.height));
}

static void __attribute__((unused)) help_rolling_average(Vector2* old_avg, Vector2 new_point, size_t new_count) {
  float factor = (float)(new_count - 1) / (float)(new_count);
  old_avg->x *= factor;
  old_avg->y *= factor;
  old_avg->x += new_point.x / (float)(new_count);
  old_avg->y += new_point.y / (float)(new_count);
}

static Vector2 __attribute__((unused)) help_get_tangent(Vector2 const * points, size_t index) {
  Vector2 a, b;
  if (index == 0) {
    a = points[index];
    b = points[index + 1];
  } else {
    b = points[index];
    a = points[index - 1];
  }
  Vector2 tg = (Vector2){ b.x - a.x, b.y - a.y };
  float tg_norm = sqrtf(tg.x * tg.x + tg.y * tg.y);
  Vector2 tg_n = (Vector2){ tg.x / tg_norm, tg.y / tg_norm };
  if (tg_n.x == tg_n.x && tg_n.y == tg_n.y)
    return tg_n;
  return (Vector2){0};
}

static Rectangle __attribute__((unused)) help_bb_to_rect(bb_t bb) {
  return (Rectangle){ .x = bb.xmin, .y = bb.xmin, .height = bb.ymax - bb.ymin, .width = bb.xmax - bb.xmin };
}
