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
#define RESAMPLING_RAW_NODE_MAX_LEN (RESAMPLING_NODE_MAX_LEN * 8)

typedef enum {
  resampling2_kind_x,
  resampling2_kind_y,
  resampling2_kind_raw
} resampling2_node_kind_t;

struct resampling2_node_t {
  template<resampling2_node_kind_t kind>
  constexpr bool is_inside(Vector2 const* points, Rectangle rect) const {
    if (len == 0) return false;
    float minx = 0.f, miny = 0.f, maxx = 0.f, maxy = 0.f;
    if constexpr (kind == resampling2_kind_y) {
      miny = points[index_start].y;
      maxy = points[index_start + len - 1].y;
      minx = points[min_index].x;
      maxx = points[max_index].x;
      if (maxy < miny) std::swap(maxy, miny);
    } else {
      minx = points[index_start].x;
      maxx = points[index_start + len - 1].x;
      miny = points[min_index].y;
      maxy = points[max_index].y;
      if (maxx < minx) std::swap(maxx, minx);
    }
    return !((miny > rect.y) || (maxy < rect.y - rect.height) || (minx > rect.x + rect.width) || (maxx < rect.x));
  }

  template<resampling2_node_kind_t kind>
  constexpr Vector2 get_ratios(Vector2 const* points, float screen_width, float screen_height) const {
    Vector2 p1 = points[index_start], p2 = points[index_start + len - 1];
    float xr = 0.f, yr = 0.f;
    if constexpr (kind == resampling2_kind_x) xr = fabsf(p1.x - p2.x) / screen_width,  yr = (points[max_index].y - points[min_index].y) / screen_height;
    else                                      yr = fabsf(p1.y - p2.y) / screen_height, xr = (points[max_index].x - points[min_index].x) / screen_width;
    return {xr, yr};
  }

  uint32_t min_index = 0, max_index = 0;
  uint32_t index_start = 0, len = 0;
};

struct resampling2_raw_node_t {
  int64_t get_last_point() const {
    if (len == 0) return -1;
    return index_start + len - 1;
  }
  int64_t get_first_point() const {
    if (len == 0) return -1;
    return index_start;
  }

  uint32_t index_start = 0, len = 0;
  uint32_t minx_index = 0, maxx_index = 0, miny_index = 0, maxy_index = 0;
};

struct resampling2_nodes_allocator_t;

struct resampling2_nodes_t {
  resampling2_node_t arr = {};
  size_t child1 = 0;
  size_t child2 = 0;
  uint32_t depth = 0;

  int64_t get_last_point() const {
    if (arr.len == 0) return -1;
    return arr.index_start + arr.len - 1;
  }
  int64_t get_first_point() const {
    if (arr.len == 0) return -1;
    return arr.index_start;
  }
};

struct resampling2_nodes_allocator_t {
  resampling2_nodes_t * arr = NULL;
  size_t len = 0, cap = 0;
  bool is_rising = true, is_falling = true;
  constexpr int64_t get_first_point() const {
    if (len == 0) return -1;
    return arr[0].get_first_point();
  }
};

enum resampling2_axis : int32_t {
  AXIS_X,
  AXIS_Y
};

struct resampling2_all_roots {
  resampling2_node_kind_t kind;
  union {
    resampling2_nodes_allocator_t x;
    resampling2_nodes_allocator_t y;
    resampling2_raw_node_t raw;
  };
  constexpr resampling2_all_roots(resampling2_nodes_allocator_t nodes, resampling2_node_kind_t kind) : kind(kind), x(nodes) {}
  constexpr resampling2_all_roots(resampling2_raw_node_t raw) : kind(resampling2_kind_raw), raw(raw) {}
  constexpr int64_t get_last_point() const {
    switch (kind) {
      case resampling2_kind_raw: return raw.get_last_point();
      case resampling2_kind_y:   return y.arr[0].get_last_point();
      case resampling2_kind_x:   return x.arr[0].get_last_point();
      default: {
                 assert("Unhandled kind" && false);
                 return -1;
               }
    }
  }
  constexpr int64_t get_first_point() const {
    switch (kind) {
      case resampling2_kind_raw: return raw.get_first_point();
      case resampling2_kind_y:   return y.arr[0].get_first_point();
      case resampling2_kind_x:   return x.arr[0].get_first_point();
      default: {
                 assert("Unhandled kind" && false);
                 return -1;
               }
    }
  }
};

typedef struct resampling2_t {
  resampling2_all_roots* roots = nullptr;
  uint32_t roots_len = 0;
  uint32_t roots_cap = 0;

  resampling2_nodes_allocator_t temp_root_x = {}, temp_root_y = {};
  resampling2_raw_node_t temp_root_raw = {};
  bool temp_x_valid = true, temp_y_valid = true, temp_raw_valid = true;
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
static bool resampling2_push_root(resampling2_t* r, resampling2_all_roots root);
template<resampling2_node_kind_t kind>
static bool resampling2_add_point(resampling2_nodes_allocator_t* nodes, br_data_t const* pg, uint32_t index);
static bool resampling2_add_point_raw(resampling2_raw_node_t* node, Vector2 const* points, uint32_t index);
static void resampling2_draw(resampling2_raw_node_t raw, br_data_t const* pg, br_plot_t* rdi);
static void resampling2_draw(resampling2_all_roots r, br_data_t const* pg, br_plot_t* rdi);
static void resampling2_nodes_debug_print(FILE* file, resampling2_nodes_allocator_t const* r, size_t index);

resampling2_t* resampling2_malloc(void) {
   resampling2_t* r = (resampling2_t*)BR_CALLOC(1, sizeof(*r));
   if (r == NULL) return NULL;
   r->temp_x_valid = r->temp_y_valid = r->temp_raw_valid = true;
   return r;
}

void resampling2_empty(resampling2_t* res) {
  if (nullptr == res) return;
  for (uint32_t i = 0; i < res->roots_len; ++i) {
    if (res->roots[i].kind != resampling2_kind_raw) {
      resampling2_nodes_deinit(&res->roots[i].x);
    }
  }
  resampling2_nodes_deinit(&res->temp_root_x);
  resampling2_nodes_deinit(&res->temp_root_y);
  res->temp_root_x = {};
  res->temp_root_y = {};
  res->temp_root_raw = {};
  res->temp_raw_valid = res->temp_y_valid = res->temp_x_valid = true;
  res->roots_len = 0;
}

void resampling2_free(resampling2_t* r) {
  if (nullptr == r) return;
  for (uint32_t i = 0; i < r->roots_len; ++i) {
    if (r->roots[i].kind != resampling2_kind_raw)
    resampling2_nodes_deinit(&r->roots[i].x);
  }
  resampling2_nodes_deinit(&r->temp_root_x);
  resampling2_nodes_deinit(&r->temp_root_y);
  BR_FREE(r->roots);
  BR_FREE(r);
}

extern "C" void resampling2_add_point(resampling2_t* r, const br_data_t *pg, uint32_t index) {
  ZoneScopedN("resampline2_add_point0");
  bool was_valid_x = r->temp_x_valid, was_valid_y = r->temp_y_valid, was_valid_raw = r->temp_raw_valid;
  if (was_valid_x)   r->temp_x_valid   = resampling2_add_point<resampling2_kind_x>(&r->temp_root_x, pg, index);
  if (was_valid_y)   r->temp_y_valid   = resampling2_add_point<resampling2_kind_y>(&r->temp_root_y, pg, index);
  if (was_valid_raw) r->temp_raw_valid = resampling2_add_point_raw(&r->temp_root_raw, pg->points, index);
  if (r->temp_x_valid || r->temp_y_valid || r->temp_raw_valid) return;
  if (was_valid_x) {
    if (false == resampling2_push_root(r, resampling2_all_roots(r->temp_root_x, resampling2_kind_x))) {
      resampling2_nodes_deinit(&r->temp_root_x);
    }
    resampling2_nodes_deinit(&r->temp_root_y);
  } else if (was_valid_y) {
    if (false == resampling2_push_root(r, resampling2_all_roots(r->temp_root_y, resampling2_kind_y))) {
      resampling2_nodes_deinit(&r->temp_root_y);
    }
    resampling2_nodes_deinit(&r->temp_root_x);
  } else if (was_valid_raw) {
    resampling2_push_root(r, resampling2_all_roots(r->temp_root_raw) );
    resampling2_nodes_deinit(&r->temp_root_x);
    resampling2_nodes_deinit(&r->temp_root_y);
  } else assert(false);
  r->temp_root_x = {};
  r->temp_root_y = {};
  r->temp_root_raw = {};
  r->temp_raw_valid = r->temp_y_valid = r->temp_x_valid = true;
  resampling2_add_point(r, pg, index);
}

static void resampling2_nodes_deinit(resampling2_nodes_allocator_t* nodes) {
  if (nodes == NULL) return;
  BR_FREE(nodes->arr);
  nodes->len = 0;
  nodes->cap = 0;
  nodes->arr = NULL;
}

static bool resampling2_push_root(resampling2_t* r, resampling2_all_roots root) {
  if (r->roots_len == 0) {
    resampling2_all_roots* new_roots = (resampling2_all_roots*)BR_CALLOC(1, sizeof(resampling2_all_roots));
    if (new_roots == nullptr) return false;
    r->roots = new_roots;
    r->roots_cap = 1;
  }
  if (r->roots_len == r->roots_cap) {
    uint32_t new_cap = r->roots_cap * 2;
    resampling2_all_roots* new_root =  (resampling2_all_roots*)BR_REALLOC(r->roots, new_cap * sizeof(resampling2_all_roots));
    if (new_root == nullptr) return false;
    r->roots = new_root;
    r->roots_cap = new_cap;
  }
  r->roots[r->roots_len++] = root;
  return true;
}

template<resampling2_node_kind_t kind>
static bool resampling2_nodes_push_point(resampling2_nodes_allocator_t* nodes, size_t node_index, uint32_t index, Vector2 const* points) {
  resampling2_nodes_t node = nodes->arr[node_index];
  ++node.arr.len;
  if (node.arr.len == 1) {
    node.arr.index_start = 
    node.arr.max_index = 
    node.arr.min_index = index;
  } else {
    if constexpr (kind == resampling2_kind_x) {
      if (points[index].y < points[node.arr.min_index].y) node.arr.min_index = index;
      if (points[index].y > points[node.arr.max_index].y) node.arr.max_index = index;
    } else if constexpr (kind == resampling2_kind_y) {
      if (points[index].x < points[node.arr.min_index].x) node.arr.min_index = index;
      if (points[index].x > points[node.arr.max_index].x) node.arr.max_index = index;
    } else {
      static_assert(kind > resampling2_kind_y, "Bad resampling kind");
    }
    if (node.depth > 0) {
      if (false == resampling2_nodes_push_point<kind>(nodes, node.child2, index, points)) {
        return false;
      }
    }
    bool split = (node_index == 0 && node.arr.len == RESAMPLING_NODE_MAX_LEN * powers[node.depth]) ||
      (node_index != 0 && node.arr.len > RESAMPLING_NODE_MAX_LEN * powers[node.depth]);
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

template<resampling2_node_kind_t kind>
static bool resampling2_add_point(resampling2_nodes_allocator_t* nodes, const br_data_t *pg, uint32_t index) {
  ZoneScopedN("resampling2_add_point1");
  Vector2 p = pg->points[index];
  if (nodes->len == 0) {
    nodes->is_rising = nodes->is_falling = true;
    resampling2_nodes_t root = {};
    br_da_push((*nodes), root);
  }
  if (index == 0) {
    nodes->is_rising = true;
    nodes->is_falling = true;
    resampling2_nodes_push_point<kind>(nodes, 0, index, pg->points);
    return true;
  }
  float new_v, old_v;
  if constexpr (kind == resampling2_kind_y) new_v = p.y, old_v = pg->points[index - 1].y;
  else                                      new_v = p.x, old_v = pg->points[index - 1].x;
  if (new_v > old_v) {
    nodes->is_falling = false;
    if (nodes->is_rising) {
      resampling2_nodes_push_point<kind>(nodes, 0, index, pg->points);
    } else {
      return false;
    }
  } else if (new_v < old_v) {
    nodes->is_rising = false;
    if (nodes->is_falling) {
      resampling2_nodes_push_point<kind>(nodes, 0, index, pg->points);
    } else {
      return false;
    }
  } else if (new_v == old_v) {
      resampling2_nodes_push_point<kind>(nodes, 0, index, pg->points);
  }
  // point must be nan, so let's just ignore it...
  return true;
}

float something = 0.02f;
float something2 = 0.001f;
float stride_after = 0.06f;
int max_stride = 0;
int raw_c = 0;
int not_raw_c = 0;

template<resampling2_node_kind_t kind>
static void resampling2_draw(resampling2_nodes_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  assert(plot->kind == br_plot_kind_2d);
  ZoneScopedN("resampling2_draw_not_raw");
  Vector2 const* ps = pg->points;
  Rectangle rect = plot->dd.graph_rect;
  resampling2_nodes_t node = nodes->arr[index];
  if (false == node.arr.is_inside<kind>(ps, rect)) return;
  bool is_end = pg->len == node.arr.index_start + node.arr.len;
  if (node.depth == 0) { // This is the leaf node
    smol_mesh_gen_line_strip(plot->dd.line_shader, &ps[node.arr.index_start], node.arr.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  Vector2 ratios = node.arr.get_ratios<kind>(ps, rect.width, rect.height);
  float rmin = fminf(ratios.x, ratios.y);
  if (rmin < (node.depth == 1 ? something : something2)) {
    Vector2 first = ps[node.arr.index_start], last = ps[node.arr.index_start + node.arr.len - (is_end ? 1 : 0)];
    bool swp = false;
    if constexpr (kind == resampling2_kind_x) swp = (first.x < last.x) == (ps[node.arr.min_index].x < ps[node.arr.max_index].x);
    else                                      swp = (first.y < last.y) == (ps[node.arr.min_index].y < ps[node.arr.max_index].y);
    Vector2 pss[] = {
      first,
      swp ? ps[node.arr.min_index] : ps[node.arr.max_index],
      swp ? ps[node.arr.max_index] : ps[node.arr.min_index],
      last
    };
    smol_mesh_gen_line_strip(plot->dd.line_shader, pss, 4, pg->color);
  } else {
    resampling2_draw<kind>(nodes, node.child1, pg, plot);
    resampling2_draw<kind>(nodes, node.child2, pg, plot);
  }
}

static bool resampling2_add_point_raw(resampling2_raw_node_t* node, Vector2 const* points, uint32_t index) {
  if (node->len == RESAMPLING_NODE_MAX_LEN) return false;
  if (node->len == 0) {
    node->minx_index = node->maxx_index = node->miny_index = node->maxy_index = node->index_start = index;
  } else {
    if (points[node->minx_index].x > points[index].x) node->minx_index = index;
    if (points[node->miny_index].y > points[index].y) node->miny_index = index;
    if (points[node->maxx_index].x < points[index].x) node->maxx_index = index;
    if (points[node->maxy_index].y < points[index].y) node->maxy_index = index;
  }
  ++node->len;
  return true;
}

static void resampling2_draw(resampling2_raw_node_t raw, br_data_t const* pg, br_plot_t* plot) {
  ZoneScopedN("resampling2_draw_raw");
  assert(plot->kind == br_plot_kind_2d);
  Vector2 const* ps = pg->points;
  Rectangle rect = plot->dd.graph_rect;
  bool is_inside = !((ps[raw.miny_index].y > rect.y) || (ps[raw.maxy_index].y < rect.y - rect.height) ||
                     (ps[raw.maxx_index].x < rect.x) || (ps[raw.miny_index].x > rect.x + rect.width));
  if (!is_inside) return;

  smol_mesh_gen_line_strip(plot->dd.line_shader, &pg->points[raw.index_start], raw.len, pg->color);
}

static void resampling2_draw(resampling2_all_roots r, br_data_t const* pg, br_plot_t *rdi) {
  if      (r.kind == resampling2_kind_raw) resampling2_draw(r.raw, pg, rdi);
  else if (r.kind == resampling2_kind_x)   resampling2_draw<resampling2_kind_x>(&r.x, 0, pg, rdi);
  else if (r.kind == resampling2_kind_y)   resampling2_draw<resampling2_kind_y>(&r.y, 0, pg, rdi);
}

void resampling2_draw(resampling2_t const* res, br_data_t const* pg, br_plot_t* plot) {
  ZoneScopedN("resampline2_draw0");
  assert(plot->kind == br_plot_kind_2d);
  Rectangle rect = plot->dd.graph_rect;
  auto draw_if_inside = [pg, rect, plot](int64_t li, int64_t ci) {
    if (li >= 0 && ci >= 0) {
      Vector2 from = pg->points[li];
      Vector2 to = pg->points[ci];
      float min_y = fminf(from.y, to.y), max_y = fmax(from.y, to.y),
            min_x = fminf(from.x, to.x), max_x = fmax(from.x, to.x);
      bool is_inside = !((min_y > rect.y) || (max_y < rect.y - rect.height) ||
                         (max_x < rect.x) || (min_x > rect.x + rect.width));
      if (is_inside) smol_mesh_gen_line(plot->dd.line_shader, from, to, pg->color);
    }
  };

  int64_t last_index = -1, cur_index = -1;
  for (uint32_t i = 0; i < res->roots_len; ++i) {
    resampling2_draw(res->roots[i], pg, plot);
    draw_if_inside(last_index, res->roots[i].get_first_point());
    last_index = res->roots[i].get_last_point();
  }
  if (res->temp_x_valid) {
    resampling2_draw<resampling2_kind_x>(&res->temp_root_x, 0, pg, plot);
    cur_index = res->temp_root_x.get_first_point();
  }
  else if (res->temp_y_valid) {
    resampling2_draw<resampling2_kind_y>(&res->temp_root_y, 0, pg, plot);
    cur_index = res->temp_root_y.get_first_point();
  }
  else {
    resampling2_draw(res->temp_root_raw, pg, plot);
    cur_index = res->temp_root_raw.get_first_point();
  }
  draw_if_inside(last_index, cur_index);
}

static void resampling2_nodes_debug_print(FILE* file, resampling2_nodes_allocator_t const* r, size_t index) {
  if (r == NULL) return;
  fprintf(file, "--------------------------BEGIN------------------\n");
  for (size_t i = 0; i < r->len; ++i) {
    resampling2_nodes_t node = r->arr[i];
    for (size_t j = 0; j < node.depth; ++j) {
      fprintf(file, "  ");
    }
    fprintf(file, "#%lu depth=%d, len: %u, left: %lu, right: %lu\n", i, node.depth, node.arr.len, node.child1, node.child2);
  }
  return;
  resampling2_nodes_t node = r->arr[index];
  for (size_t i = 0; i < node.depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "len: %u, left: %lu, right: %lu\n", node.arr.len, node.child1, node.child2);
  resampling2_nodes_debug_print(file, r, node.child1);
  resampling2_nodes_debug_print(file, r, node.child2);
}

static void resampling2_raw_node_debug_print(FILE* file, resampling2_raw_node_t* node, int depth) {
  for (int i = 0; i < depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "Raw node, from: %u, len: %u\n", node->index_start, node->len);
}

static void resampling2_all_nodes_debug_print(FILE* file, resampling2_all_roots* r) {
  const char* str;
  if (r->kind == resampling2_kind_x) str = "x";
  if (r->kind == resampling2_kind_y) str = "y";
  if (r->kind == resampling2_kind_raw) str = "raw";
  fprintf(file, "Nodes %s\n", str);
}

static void resampling2_debug_print(FILE* file, resampling2_t* r) {
  fprintf(file, "\nRoots len: %u, Roots cap: %u, validxyr = %d,%d,%d\n", r->roots_len, r->roots_cap, r->temp_x_valid, r->temp_y_valid, r->temp_raw_valid);
  for (uint32_t i = 0; i < r->roots_len; ++i) {
    resampling2_all_nodes_debug_print(file, &r->roots[i]);
  }
  //resampling2_nodes_debug_print(file, &r->temp_root_x, 1);
  //resampling2_nodes_debug_print(file, &r->temp_root_y, 1);
  resampling2_raw_node_debug_print(file, &r->temp_root_raw, 1);
  return;
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
  resampling2_debug_print(stdout, r);
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
  resampling2_debug_print(stdout, r);
  printf("\nALLOCATIONS: %lu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %lu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}
#pragma GCC diagnostic pop
} // extern "C"
