#include "br_resampling2.h"
#include "br_pp.h"
#include "br_data.h"
#include "br_plot.h"
#include "br_smol_mesh.h"
#include "br_da.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raylib.h"
#include "raymath.h"
#pragma GCC diagnostic pop
#include "tracy/Tracy.hpp"

#include <algorithm>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define RESAMPLING_NODE_MAX_LEN 128

struct resampling2_nodes_t {
  uint32_t min_index_x = 0, max_index_x = 0;
  uint32_t min_index_y = 0, max_index_y = 0;
  uint32_t index_start = 0, len = 0;
  size_t child1 = 0;
  size_t child2 = 0;
  uint32_t depth = 0;

  constexpr bool is_inside(Vector2 const* points, Rectangle rect) const {
    if (len == 0) return false;
    float minx = points[min_index_x].x, miny = points[min_index_y].y, maxx = points[max_index_x].x, maxy = points[max_index_y].y;
    return !((miny > rect.y) || (maxy < rect.y - rect.height) || (minx > rect.x + rect.width) || (maxx < rect.x));
  }

  constexpr bool is_inside_3d(Vector2 const* points, Matrix mat) {
    if (len == 0) return false;
    Vector2 minx = Vector2TransformScale(points[min_index_x], mat),
            miny = Vector2TransformScale(points[min_index_y], mat),
            maxx = Vector2TransformScale(points[max_index_x], mat),
            maxy = Vector2TransformScale(points[max_index_y], mat);
    float my = fminf(fminf(minx.y, miny.y), fminf(maxy.y, maxx.y));
    float mx = fminf(fminf(minx.x, miny.x), fminf(maxy.x, maxx.x));
    float My = fmaxf(fmaxf(minx.y, miny.y), fmaxf(maxy.y, maxx.y));
    float Mx = fmaxf(fmaxf(minx.x, miny.x), fmaxf(maxy.x, maxx.x));
    Rectangle rect = { -1, 1, 2, 2 };
    return CheckCollisionRecs(rect, Rectangle { mx, My, Mx - mx, My - my });
  }
  constexpr Vector2 get_ratios(Vector2 const* points, float screen_width, float screen_height) const {
    float xr = points[max_index_x].x - points[min_index_x].x, yr = points[max_index_y].y - points[min_index_y].y;
    return {xr / screen_width, yr / screen_height};
  }
  constexpr Vector2 get_ratios_3d(Vector2 const* points, Matrix mvp) const {
    Vector2 minx = Vector2TransformScale(points[min_index_x], mvp),
            miny = Vector2TransformScale(points[min_index_y], mvp),
            maxx = Vector2TransformScale(points[max_index_x], mvp),
            maxy = Vector2TransformScale(points[max_index_y], mvp);
    float my = fminf(fminf(minx.y, miny.y), fminf(maxy.y, maxx.y));
    float mx = fminf(fminf(minx.x, miny.x), fminf(maxy.x, maxx.x));
    float My = fmaxf(fmaxf(minx.y, miny.y), fmaxf(maxy.y, maxx.y));
    float Mx = fmaxf(fmaxf(minx.x, miny.x), fmaxf(maxy.x, maxx.x));
    return {(Mx - mx) / 2.f, (My - my) / 2.f};
  }
  int64_t get_last_point() const {
    if (len == 0) return -1;
    return index_start + len - 1;
  }
  int64_t get_first_point() const {
    if (len == 0) return -1;
    return index_start;
  }
};

struct resampling2_nodes_allocator_t {
  resampling2_nodes_t * arr = NULL;
  size_t len = 0, cap = 0;
  constexpr int64_t get_first_point() const {
    if (len == 0) return -1;
    return arr[0].get_first_point();
  }
};

typedef struct resampling2_t {
  resampling2_nodes_allocator_t resamp;
} resampling2_t;

static uint32_t powers[32] = {0};
static uint32_t powers_base = 2;
static void __attribute__((constructor(101))) construct_powers(void) {
  powers[0] = 1;
  powers[1] = powers_base;
  for (int i = 2; i < 32; ++i) {
    powers[i] = powers[i - 1] * powers_base;
  }
}

static void resampling2_nodes_deinit(resampling2_nodes_allocator_t* nodes);
static bool resampling2_nodes_push_point(resampling2_nodes_allocator_t* nodes, size_t node_index, uint32_t index, Vector2 const* points);
static void resampling2_nodes_debug_print(FILE* file, resampling2_nodes_allocator_t const* r, size_t index);

resampling2_t* resampling2_malloc(void) {
   resampling2_t* r = (resampling2_t*)BR_CALLOC(1, sizeof(*r));
   if (r == NULL) return NULL;
   return r;
}

void resampling2_empty(resampling2_t* res) {
  if (nullptr == res) return;
  resampling2_nodes_deinit(&res->resamp);
}

void resampling2_free(resampling2_t* r) {
  if (nullptr == r) return;
  resampling2_nodes_deinit(&r->resamp);
  BR_FREE(r);
}

extern "C" void resampling2_add_point(resampling2_t* r, const br_data_t *pg, uint32_t index) {
  if (r->resamp.len == 0) {
    resampling2_nodes_t root = {};
    br_da_push((r->resamp), root);
  }
  resampling2_nodes_push_point(&r->resamp, 0, index, pg->points);
}

static void resampling2_nodes_deinit(resampling2_nodes_allocator_t* nodes) {
  if (nodes == NULL) return;
  BR_FREE(nodes->arr);
  nodes->len = 0;
  nodes->cap = 0;
  nodes->arr = NULL;
}

static bool resampling2_nodes_push_point(resampling2_nodes_allocator_t* nodes, size_t node_index, uint32_t index, Vector2 const* points) {
  resampling2_nodes_t node = nodes->arr[node_index];
  ++node.len;
  if (node.len == 1) {
    node.index_start = 
    node.max_index_y = 
    node.min_index_y =
    node.max_index_x = 
    node.min_index_x = index;
  } else {
    if (points[index].y < points[node.min_index_y].y) node.min_index_y = index;
    if (points[index].y > points[node.max_index_y].y) node.max_index_y = index;
    if (points[index].x < points[node.min_index_x].x) node.min_index_x = index;
    if (points[index].x > points[node.max_index_x].x) node.max_index_x = index;
    if (node.depth > 0) {
      if (false == resampling2_nodes_push_point(nodes, node.child2, index, points)) {
        return false;
      }
    }
    bool split = (node_index == 0 && node.len == RESAMPLING_NODE_MAX_LEN * powers[node.depth]) ||
      (node_index != 0 && node.len > RESAMPLING_NODE_MAX_LEN * powers[node.depth]);
    if (split) {
      resampling2_nodes_t left = node;
      resampling2_nodes_t right = {};
      br_da_push(*nodes, left);
      node.child1 = nodes->len - 1;
      br_da_push(*nodes, right);
      node.child2 = nodes->len - 1;
      ++node.depth;
    }
  }
  nodes->arr[node_index] = node;
  return true;
}

float something = 0.02f;
float something2 = 0.001f;
float stride_after = 0.06f;
int max_stride = 0;
int raw_c = 0;
int not_raw_c = 0;

static void resampling2_draw(resampling2_nodes_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  assert(plot->kind == br_plot_kind_2d);
  ZoneScopedN("resampling2_2d");
  Vector2 const* ps = pg->points;
  Rectangle rect = plot->dd.graph_rect;
  resampling2_nodes_t node = nodes->arr[index];
  if (false == node.is_inside(ps, rect)) return;
  bool is_end = pg->len == node.index_start + node.len;
  if (node.depth == 0) { // This is the leaf node
    smol_mesh_gen_line_strip(plot->dd.line_shader, &ps[node.index_start], node.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  Vector2 ratios = node.get_ratios(ps, rect.width, rect.height);
  float rmin = fminf(ratios.x, ratios.y);
  if (rmin < (node.depth == 1 ? something : something2)) {
    size_t indexies[] = {
      node.index_start,
      node.min_index_x,
      node.min_index_y,
      node.max_index_x,
      node.max_index_y,
      node.index_start + node.len - (is_end ? 1 : 0)
    };
    std::sort(indexies, &indexies[5]);
    Vector2 pss[] = {
      ps[indexies[0]], ps[indexies[1]],
      ps[indexies[2]], ps[indexies[3]],
      ps[indexies[4]], ps[indexies[5]],
    };
    smol_mesh_gen_line_strip(plot->dd.line_shader, pss, 6, pg->color);
  } else {
    resampling2_draw(nodes, node.child1, pg, plot);
    resampling2_draw(nodes, node.child2, pg, plot);
  }
}

static void resampling2_draw_3d(resampling2_nodes_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  assert(plot->kind == br_plot_kind_3d);
  ZoneScopedN("resampling2_3d");
  Vector2 const* ps = pg->points;
  resampling2_nodes_t node = nodes->arr[index];
  Matrix mvp = plot->ddd.line_shader->uvs.m_mvp_uv;
  if (false == node.is_inside_3d(ps, mvp)) return;
  bool is_end = pg->len == node.index_start + node.len;
  if (node.depth == 0) { // This is the leaf node
    smol_mesh_3d_gen_line_strip2(plot->ddd.line_shader, &ps[node.index_start], node.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  Vector2 ratios = node.get_ratios_3d(ps, mvp);
  float rmin = fminf(ratios.x, ratios.y);
  if (rmin < (node.depth == 1 ? something : something2)) {
    size_t indexies[] = {
      node.index_start,
      node.min_index_x,
      node.min_index_y,
      node.max_index_x,
      node.max_index_y,
      node.index_start + node.len - (is_end ? 1 : 0)
    };
    std::sort(indexies, &indexies[5]);
    Vector2 pss[] = {
      ps[indexies[0]], ps[indexies[1]],
      ps[indexies[2]], ps[indexies[3]],
      ps[indexies[4]], ps[indexies[5]],
    };
    smol_mesh_3d_gen_line_strip2(plot->ddd.line_shader, pss, 6, pg->color);
  } else {
    resampling2_draw_3d(nodes, node.child1, pg, plot);
    resampling2_draw_3d(nodes, node.child2, pg, plot);
  }
}

void resampling2_draw(resampling2_t const* res, br_data_t const* pg, br_plot_t* plot) {
  ZoneScopedN("resampline2_draw0");

  switch (plot->kind) {
    case br_plot_kind_2d: resampling2_draw(&res->resamp, 0, pg, plot); break;
    case br_plot_kind_3d: resampling2_draw_3d(&res->resamp, 0, pg, plot); break;
  }
}

static void resampling2_nodes_debug_print(FILE* file, resampling2_nodes_allocator_t const* r, size_t index) {
  if (r == NULL) return;
  fprintf(file, "--------------------------BEGIN------------------\n");
  for (size_t i = 0; i < r->len; ++i) {
    resampling2_nodes_t node = r->arr[i];
    for (size_t j = 0; j < node.depth; ++j) {
      fprintf(file, "  ");
    }
    fprintf(file, "#%lu depth=%d, len: %u, left: %lu, right: %lu\n", i, node.depth, node.len, node.child1, node.child2);
  }
  return;
  resampling2_nodes_t node = r->arr[index];
  for (size_t i = 0; i < node.depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "len: %u, left: %lu, right: %lu\n", node.len, node.child1, node.child2);
  resampling2_nodes_debug_print(file, r, node.child1);
  resampling2_nodes_debug_print(file, r, node.child2);
}

#define PRINT_ALLOCS(prefix) \
  printf("\n%s ALLOCATIONS: %zu ( %zuKB ) | %lu (%zuKB)\n", prefix, \
      context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);

extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "src/misc/tests.h"
TEST_CASE(resampling) {
  Vector2 points[] = { {0, 1}, {1, 2}, {2, 4}, {3, 2} };
  br_data_t pg;
  memset(&pg, 0, sizeof(pg));
  pg.cap = 4;
  pg.len = 4;
  pg.points = points;
  pg.resampling = NULL;
  resampling2_t* r = resampling2_malloc();
  for (int i = 0; i < 2*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 64*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %lu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}

#define N 2048
TEST_CASE(resampling2) {
  Vector2 points[N];
  for (int i = 0; i < N; ++i) {
    points[i].x = sinf(3.14f / 4.f * (float)i);
    points[i].y = cosf(3.14f / 4.f * (float)i);
  }
  br_data_t pg;
  memset(&pg, 0, sizeof(pg));
  pg.cap = N;
  pg.len = N;
  pg.points = points;
  pg.resampling = NULL;
  resampling2_t* r = resampling2_malloc();
  for (int i = 0; i < N; ++i) resampling2_add_point(r, &pg, (uint32_t)i);
  printf("\nALLOCATIONS: %lu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %lu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}
#pragma GCC diagnostic pop
} // extern "C"
