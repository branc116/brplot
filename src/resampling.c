#include "br_plot.h"
#include "br_help.h"
#include "assert.h"
#include "stdlib.h"
#include "stdint.h"
#include "math.h"
#include <raylib.h>
#include <raymath.h>
#include "src/misc/tests.h"

#define temp_points_count 1024

typedef struct {
  int index, next_index;
  float factor; // 1 => val i exactly on point, 0 => next_val is exactly on point, (0, 1) -> valid, else notfound
} binary_search_res;

typedef struct {
  size_t size;
  int max_index;
  int min_index;
} sample_points_res;

static binary_search_res binary_search(float const* lb, float const* ub, float value, int stride);
static size_t points_group_sample_points(Vector2 const* points, size_t len, resampling_dir dir, Rectangle rect, Rectangle normal_rect, Vector2* out_points, size_t max_number_of_points);
static bool help_check_collision_bb_rec(bb_t bb, Rectangle rec);
static Rectangle help_bb_to_rect(bb_t bb);
static void resampling_add_interval(resampling_t* res);

resampling_t* resampling_malloc(void) {
  resampling_t* ret = (resampling_t*)BR_MALLOC(sizeof(resampling_t));
  ret->intervals = NULL;
  ret->intervals_cap = ret->intervals_count = 0;
  ret->temp_points = BR_MALLOC(temp_points_count * sizeof(Vector2));
  return ret;
}

void resampling_free(resampling_t* res) {
  if (res->intervals) {
    BR_FREE(res->intervals);
  }
  BR_FREE(res->temp_points);
  BR_FREE(res);
}

size_t resampling_draw(resampling_t* res, points_group_t const* points, Rectangle screen, smol_mesh_t* lines_mesh, smol_mesh_t* quad_mesh) {
  (void)quad_mesh;
  size_t ret = res->resampling_count = res->raw_count = 0;
  Vector2 neigh[2] = {0};
  Rectangle normal_screen = screen;
  normal_screen.y -= normal_screen.height*1.1f;
  normal_screen.height *= 1.2f;
  normal_screen.x -= normal_screen.width*.1f;
  normal_screen.width *= 1.2f;

  for (size_t i = 0; i < res->intervals_count; ++i) {
    size_t spr = 0;
    resamping_interval_t* inter = &res->intervals[i];
    resampling_dir d = inter->dir & resampling_dir_right ? resampling_dir_right :
                       inter->dir & resampling_dir_left  ? resampling_dir_left  :
                       inter->dir & resampling_dir_up    ? resampling_dir_up    :
                       resampling_dir_down;
    Vector2 const * ps = &(points->points)[inter->from];
    if (context.debug_bounds) smol_mesh_gen_bb(lines_mesh, inter->bounds, points->color);
    if (!help_check_collision_bb_rec(inter->bounds, screen)) continue;
    if (inter->count < 128) {
      smol_mesh_gen_line_strip(lines_mesh, ps, inter->count, points->color);
      ret += inter->count;
      ++res->raw_count;
    } else {
      spr = points_group_sample_points(ps, inter->count, d, screen, normal_screen, res->temp_points, temp_points_count);
      if (spr == 0) continue;
      smol_mesh_gen_line_strip(lines_mesh, res->temp_points, spr, points->color);
      ret += spr;
      ++res->resampling_count;
      if (CheckCollisionPointRec(ps[0], normal_screen)) {
        neigh[0] = res->temp_points[0];
        neigh[1] = ps[0];
        smol_mesh_gen_line_strip(lines_mesh, neigh, 2, points->color);
      }
      if (CheckCollisionPointRec(ps[inter->count - 1], normal_screen)) {
        neigh[0] = res->temp_points[spr - 1];
        neigh[1] = ps[inter->count - 1];
        smol_mesh_gen_line_strip(lines_mesh, neigh, 2, points->color);
      }
    }
  }
  return ret;
}

void resampling_add_point(resampling_t* res, points_group_t const* pg, size_t index) {
  Vector2 const * all_points =  pg->points;
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
    resampling_add_point(res, pg, index - 1);
    resampling_add_point(res, pg, index);
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
    res->intervals = res->intervals_cap == 0 ? BR_MALLOC(new_cap * sizeof(resamping_interval_t)) : BR_REALLOC(res->intervals, sizeof(resamping_interval_t) * new_cap);
    res->intervals_cap = new_cap;
  }
  res->intervals[new_index] = (resamping_interval_t) { .from = 0, .count = 0, .dir = 15, .bounds = {0} };
  return;
}
static bool check_collision_rec_line(Rectangle rec, Vector2 start, Vector2 end) {
  if (CheckCollisionPointRec(start, rec)) return true;
  if (CheckCollisionPointRec(end, rec)) return true;
  if (CheckCollisionLines(start, end, (Vector2){rec.x, rec.y}, (Vector2){rec.x + rec.width, rec.y}, NULL)) return true;
  if (CheckCollisionLines(start, end, (Vector2){rec.x, rec.y}, (Vector2){rec.x, rec.y + rec.height}, NULL)) return true;
  if (CheckCollisionLines(start, end, (Vector2){rec.x + rec.width, rec.y}, (Vector2){rec.x + rec.width, rec.y + rec.height}, NULL)) return true;
  if (CheckCollisionLines(start, end, (Vector2){rec.x, rec.y + rec.height}, (Vector2){rec.x + rec.width, rec.y + rec.height}, NULL)) return true;
  return false;
}

static size_t points_group_sample_points(Vector2 const* points, size_t len, resampling_dir dir, Rectangle rect, Rectangle normal_rect, Vector2* out_points, size_t max_number_of_points) {
  if (len == 0) {
    return 0;
  }
  size_t i = 0, size = 0;
  float step = (dir == resampling_dir_right || dir == resampling_dir_left ? rect.width : rect.height) * 2.f / (float)max_number_of_points;
  binary_search_res res;
  Vector2 const* lb = points, *ub = &points[len - 1];
  if (dir == resampling_dir_left || dir == resampling_dir_down) lb = &points[len - 1], ub = points;

#define branch(D, field, stride, B, cur_expr)                                                    \
  if (dir == D) {                                                                                  \
    Vector2 last_point = {0};                                                                    \
    while (lb != NULL && i < max_number_of_points && size < max_number_of_points) {         \
      float cur = cur_expr * (float)(i++);                                                       \
      res = binary_search(&lb->field, &ub->field, cur, stride);                                  \
      Vector2 p = lb[res.index];                                                                 \
      if (res.factor <= 1.f && (size == 0 || p.x != last_point.x || p.y != last_point.y)) { \
        if (check_collision_rec_line(normal_rect, p, lb[res.next_index])) {                      \
          out_points[size++] = p;                                                           \
          if (size + 1 < max_number_of_points)                                              \
            out_points[size++] = lb[res.next_index];                                        \
        }                                                                                        \
        B = &lb[res.next_index];                                                                 \
        last_point = p;                                                                          \
      }                                                                                          \
    }                                                                                            \
  }                                                                                              \

  branch(resampling_dir_right, x,  2, lb, rect.x               + step) else
  branch(resampling_dir_left,  x, -2, ub, rect.x + rect.width  - step) else
  branch(resampling_dir_up,    y,  2, lb, rect.y - rect.height + step) else
  branch(resampling_dir_down,  y, -2, ub, rect.y               - step) else
  assert(false);
#undef branch
  return size;
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
  index_mid = index_lb;
  mid = &lb[index_mid * stride];
  index_mid *= signi(stride);
  float const * next = mid + stride;
  if ((stride > 0 && next <= ub && next >= lb) || (stride < 0 && next >= ub && next <= lb)) {
    return (binary_search_res){index_mid, index_mid + signi(stride), 0.f};
  } else {
    return (binary_search_res){index_mid, index_mid, 1.f};
  }
}

TEST_CASE(binary_search_tests) {
  Vector2 vec[] = {{0, 1}, {1, 2}, {2, 3}};
  TEST_EQUAL(1, binary_search(&vec[0].x, &vec[2].x, 1.1f, 2).index);

  Vector2 vec2[] = {{0, 1}, {1, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}};
  TEST_EQUAL(4, binary_search(&vec2[0].x, &vec2[5].x, 6.1f, 2).index);
  TEST_EQUAL(4, binary_search(&vec2[0].x, &vec2[5].x, 7.1f, 2).index);
  TEST_TRUE(1.f > binary_search(&vec2[0].x, &vec2[5].x, 7.1f, 2).factor);

  TEST_EQUAL(1, ((float*)&vec2[1])[-1]); // Checking negative indicies...
  Vector2 vec3[] = {{8, 1}, {6, 1}, {4, 3}, {2, 5}, {1, 7}, {0, 9}};
  TEST_EQUAL(-4, binary_search(&vec3[5].x, &vec3[0].x, 6.1f, -2).index); //lb=5, index = -4, lb[-4] == vec3[1]
                                                                        //
  Vector2 vecy[] = {{10, 1}, {10, 2}, {5, 3}, {15, 5}, {20, 7}, {10, 9}};
  TEST_EQUAL(4, binary_search(&vecy[0].y, &vecy[5].y, 7.1f, 2).index);

  Vector2 vecy2[] = {{10, 9}, {10, 7}, {5, 5}, {15, 3}, {20, 2}, {10, 1}};
  TEST_EQUAL(-4, binary_search(&vecy2[5].y, &vecy2[0].y, 7.1f, -2).index);
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
