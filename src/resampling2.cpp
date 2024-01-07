#include "br_plot.h"
#include "br_help.h"
#include "stdint.h"
#include "math.h"
#include "string.h"
#include "assert.h"
#include "raylib/src/raymath.h"

#define RESAMPLING_NODE_MAX_LEN 128

typedef enum {
  resampling2_kind_x,
  resampling2_kind_y,
  resampling2_kind_raw
} resampling2_node_kind_t;

struct resampling2_node_t{
  uint32_t min_index = 0, max_index = 0;
  uint32_t index_start = 0, len = 0;
};

struct resampling2_raw_node_t {
  uint32_t index_start = 0, len = 0;
};

typedef struct resampling2_nodes_s {
  resampling2_node_t* arr = NULL;
  struct resampling2_nodes_s* parent = NULL;
  uint32_t len = 0;
  uint32_t cap = 0;
  bool is_rising = true, is_falling = true;
} resampling2_nodes_t;

enum resampling2_axis : int32_t {
  AXIS_X,
  AXIS_Y
};

struct resampling2_all_roots{
  resampling2_node_kind_t kind;
  union {
    resampling2_nodes_t x;
    resampling2_nodes_t y;
    resampling2_raw_node_t raw;
  };
  constexpr resampling2_all_roots(resampling2_nodes_t nodes, resampling2_node_kind_t kind) : kind(kind), x(nodes) {}
  constexpr resampling2_all_roots(resampling2_raw_node_t raw) : kind(resampling2_kind_raw), raw(raw) {}
};

typedef struct resampling2_s {
  resampling2_all_roots* roots;
  uint32_t roots_len;
  uint32_t roots_cap;

  resampling2_nodes_t temp_root_x, temp_root_y;
  resampling2_raw_node_t temp_root_raw;
  bool temp_x_valid = true, temp_y_valid = true, temp_raw_valid = true;
} resampling2_t;

void resampling2_nodes_deinit(resampling2_nodes_t* nodes) {
  if (nodes == NULL) return;
  resampling2_nodes_deinit(nodes->parent);
  BR_FREE(nodes->arr);
  BR_FREE(nodes->parent);
}

resampling2_t* resampling2_malloc(void) {
   resampling2_t* r = (resampling2_t*)BR_CALLOC(1, sizeof(resampling2_t));
   r->temp_x_valid = r->temp_y_valid = r->temp_raw_valid = true;
   return r;
}

void resampling2_nodes_empty(resampling2_nodes_t* nodes) {
  if (nodes == NULL) return;
  resampling2_nodes_empty(nodes->parent);
  nodes->len = 0;
}

void resampling2_empty(resampling2_t* res) {
  for (uint32_t i = 0; i < res->roots_len; ++i) {
    if (res->roots[i].kind != resampling2_kind_raw) {
      resampling2_nodes_empty(&res->roots[i].x);
    }
  }
  resampling2_nodes_deinit(&res->temp_root_x);
  resampling2_nodes_deinit(&res->temp_root_y);
  res->temp_root_x = {};
  res->temp_root_y = {};
  res->temp_root_raw = {};
  res->temp_raw_valid = res->temp_y_valid = res->temp_x_valid = true;
}

void resampling2_free(resampling2_t* r) {
  for (uint32_t i = 0; i < r->roots_len; ++i) {
    if (r->roots[i].kind != resampling2_kind_raw)
    resampling2_nodes_deinit(&r->roots[i].x);
  }
  resampling2_nodes_deinit(&r->temp_root_x);
  resampling2_nodes_deinit(&r->temp_root_y);
  BR_FREE(r->roots);
  BR_FREE(r);
}

void resampling2_push_root(resampling2_t* r, resampling2_all_roots root) {
  if (r->roots_len == 0) {
    r->roots = (resampling2_all_roots*)BR_MALLOC(sizeof(resampling2_all_roots));
    r->roots_cap = 1;
  }
  if (r->roots_len == r->roots_cap) {
    uint32_t new_cap = r->roots_cap * 2;
    r->roots = (resampling2_all_roots*)BR_REALLOC(r->roots, new_cap * sizeof(resampling2_all_roots));
    r->roots_cap = new_cap;
  }
  r->roots[r->roots_len++] = root;
}

resampling2_nodes_t* resampling2_nodes_get_parent(resampling2_nodes_t* nodes) {
  if (nodes->parent == NULL) {
    nodes->parent = (resampling2_nodes_t*)BR_CALLOC(1, sizeof(resampling2_nodes_t));
  }
  return nodes->parent;
}

resampling2_node_t* resampling2_get_last_node(resampling2_nodes_t* nodes) {
  if (nodes->len == 0) {
    nodes->arr = (resampling2_node_t*)BR_CALLOC(1, sizeof(resampling2_node_t));
    nodes->len = 1;
    nodes->cap = 1;
  }
  return &nodes->arr[nodes->len - 1];
}

static uint32_t powers[32] = {0};
static uint32_t powers_base = 2;
static void __attribute__((constructor(101))) construct_powers(void) {
  powers[0] = 1;
  powers[1] = powers_base;
  for (int i = 2; i < 32; ++i) {
    powers[i] = powers[i - 1] * powers_base;
  }
}

static void resampling2_nodesy_push_point(resampling2_nodes_t* nodes, uint32_t index, Vector2 const* points, uint8_t depth) {
  resampling2_node_t* node = resampling2_get_last_node(nodes);
  unsigned int node_i = nodes->len - 1;
  if (node->len == 0) {
    node->min_index = index;
    node->max_index = index;
    node->len = 1;
    node->index_start = index;
  } else if (node->len < (RESAMPLING_NODE_MAX_LEN * powers[depth])) {
    if (points[node->min_index].x > points[index].x) node->min_index = index;
    if (points[node->max_index].x < points[index].x) node->max_index = index;
    ++node->len;
  } else if (node->len == (RESAMPLING_NODE_MAX_LEN * powers[depth])) {
    unsigned int new_cap = nodes->cap * 2;
    if (nodes->len == nodes->cap) {
      nodes->arr = (resampling2_node_t*)BR_REALLOC(nodes->arr, sizeof(resampling2_node_t) * (new_cap));
      for (unsigned int i = nodes->cap; i < new_cap; ++i) {
        nodes->arr[i] = {};
      }
      nodes->cap = new_cap;
    }
    ++nodes->len;
    node = &nodes->arr[node_i];
    if (nodes->parent == NULL) {
      nodes->parent = (resampling2_nodes_t*)BR_CALLOC(1, sizeof(resampling2_nodes_t));
      nodes->parent->arr = (resampling2_node_t*)BR_CALLOC(1, sizeof(resampling2_node_t));
      nodes->parent->len = 1;
      nodes->parent->cap = 1;
      memcpy(nodes->parent->arr, node, sizeof(resampling2_node_t));
      resampling2_nodesy_push_point(nodes, index, points, depth);
    } else {
      resampling2_nodesy_push_point(nodes, index, points, depth);
    }
    return;
  } else {
    printf("%d\n", *(int*)0);
  }
  if (nodes->parent != NULL) resampling2_nodesy_push_point(nodes->parent, index, points, depth + 1);
}

static void resampling2_nodesx_push_point(resampling2_nodes_t* nodes, uint32_t index, Vector2 const* points, uint8_t depth) {
  resampling2_node_t* node = resampling2_get_last_node(nodes);
  unsigned int node_i = nodes->len - 1;
  if (node->len == 0) {
    node->min_index = index;
    node->max_index = index;
    node->len = 1;
    node->index_start = index;
  } else if (node->len < (RESAMPLING_NODE_MAX_LEN * powers[depth])) {
    if (points[node->min_index].y > points[index].y) node->min_index = index;
    if (points[node->max_index].y < points[index].y) node->max_index = index;
    ++node->len;
  } else if (node->len == (RESAMPLING_NODE_MAX_LEN * powers[depth])) {
    unsigned int new_cap = nodes->cap * 2;
    if (nodes->len == nodes->cap) {
      nodes->arr = (resampling2_node_t*)BR_REALLOC(nodes->arr, sizeof(resampling2_node_t) * (new_cap));
      for (unsigned int i = nodes->cap; i < new_cap; ++i) {
        nodes->arr[i] = {};
      }
      nodes->cap = new_cap;
    }
    ++nodes->len;
    node = &nodes->arr[node_i];
    if (nodes->parent == NULL) {
      nodes->parent = (resampling2_nodes_t*)BR_CALLOC(1, sizeof(resampling2_nodes_t));
      nodes->parent->arr = (resampling2_node_t*)BR_CALLOC(1, sizeof(resampling2_node_t));
      nodes->parent->len = 1;
      nodes->parent->cap = 1;
      memcpy(nodes->parent->arr, node, sizeof(resampling2_node_t));
      resampling2_nodesx_push_point(nodes, index, points, depth);
    } else {
      resampling2_nodesx_push_point(nodes, index, points, depth);
    }
    return;
  } else {
    printf("%d\n", *(int*)0);
  }
  if (nodes->parent != NULL) resampling2_nodesx_push_point(nodes->parent, index, points, depth + 1);
}

static bool resampling2_add_point_raw(resampling2_raw_node_t* node, uint32_t index) {
  if (node->len == RESAMPLING_NODE_MAX_LEN) return false;
  if (node->len == 0) node->index_start = index;
  ++node->len;
  return true;
}

static bool resampling2_add_point_y(resampling2_nodes_t* nodes, const points_group_t *pg, uint32_t index) {
  Vector2 p = pg->points[index];
  if (nodes->len == 0) nodes->is_rising = nodes->is_falling = true;
  if (index == 0) {
    nodes->is_rising = true;
    nodes->is_falling = true;
    resampling2_nodesy_push_point(nodes, index, pg->points, 0);
    return true;
  } else if (p.y > pg->points[index - 1].y) {
    nodes->is_falling = false;
    if (nodes->is_rising) {
      resampling2_nodesy_push_point(nodes, index, pg->points, 0);
    } else {
      return false;
    }
  } else if (p.y < pg->points[index - 1].y) {
    nodes->is_rising = false;
    if (nodes->is_falling) {
      resampling2_nodesy_push_point(nodes, index, pg->points, 0);
    } else {
      return false;
    }
  } else if (p.y == pg->points[index - 1].y) {
      resampling2_nodesy_push_point(nodes, index, pg->points, 0);
  }
  // point must be nan, so let's just ignore it...
  return true;
}

bool resampling2_add_point_x(resampling2_nodes_t* nodes, const points_group_t *pg, uint32_t index) {
  Vector2 p = pg->points[index];
  if (nodes->len == 0) nodes->is_rising = nodes->is_falling = true;
  if (index == 0) {
    nodes->is_rising = true;
    nodes->is_falling = true;
    resampling2_nodesx_push_point(nodes, index, pg->points, 0);
    return true;
  } else if (p.x > pg->points[index - 1].x) {
    nodes->is_falling = false;
    if (nodes->is_rising) {
      resampling2_nodesx_push_point(nodes, index, pg->points, 0);
    } else {
      return false;
    }
  } else if (p.x < pg->points[index - 1].x) {
    nodes->is_rising = false;
    if (nodes->is_falling) {
      resampling2_nodesx_push_point(nodes, index, pg->points, 0);
    } else {
      return false;
    }
  } else if (p.x == pg->points[index - 1].x) {
      resampling2_nodesx_push_point(nodes, index, pg->points, 0);
  }
  // point must be nan, so let's just ignore it...
  return true;
}

void resampling2_add_point(resampling2_t* r, const points_group_t *pg, uint32_t index) {
  bool was_valid_x = r->temp_x_valid, was_valid_y = r->temp_y_valid, was_valid_raw = r->temp_raw_valid;
  if (was_valid_x)   r->temp_x_valid   = resampling2_add_point_x(&r->temp_root_x, pg, index);
  if (was_valid_y)   r->temp_y_valid   = resampling2_add_point_y(&r->temp_root_y, pg, index);
  if (was_valid_raw) r->temp_raw_valid = resampling2_add_point_raw(&r->temp_root_raw, index);
  if (r->temp_x_valid || r->temp_y_valid || r->temp_raw_valid) return;
  if (was_valid_x) {
    resampling2_push_root(r, resampling2_all_roots(r->temp_root_x, resampling2_kind_x));
    resampling2_nodes_deinit(&r->temp_root_y);
  } else if (was_valid_y) {
    resampling2_push_root(r, resampling2_all_roots(r->temp_root_y, resampling2_kind_y));
    resampling2_nodes_deinit(&r->temp_root_x);
  } else if (was_valid_raw) {
    resampling2_push_root(r, resampling2_all_roots(r->temp_root_raw) );
    resampling2_nodes_deinit(&r->temp_root_x);
    resampling2_nodes_deinit(&r->temp_root_y);
  } else printf("%d\n", *(int*)0);
  r->temp_root_x = {};
  r->temp_root_y = {};
  r->temp_root_raw = {};
  r->temp_raw_valid = r->temp_y_valid = r->temp_x_valid = true;
  resampling2_add_point(r, pg, index);
}

bool resampling2_nodex_inside_rect(resampling2_node_t node, Vector2 const* const points, Rectangle rect) {
  float firstx = points[node.index_start].x;
  float lastx = points[node.index_start + node.len - 1].x;
  if (lastx < firstx) {
    float tmp = firstx;
    firstx = lastx;
    lastx = tmp;
  }
  if (lastx < rect.x) return false;
  if (firstx > rect.x + rect.width) return false;
  if (points[node.min_index].y > rect.y + rect.height) return false;
  if (points[node.max_index].y < rect.y) return false;
  return true;
}

bool resampling2_nodey_inside_rect(resampling2_node_t node, Vector2* points, Rectangle rect) {
  float firsty = points[node.index_start].y;
  float lasty = points[node.index_start + node.len - 1].y;
  if (lasty < firsty) {
    float tmp = firsty;
    firsty = lasty;
    lasty = tmp;
  }
  if (lasty < rect.y) return false;
  if (firsty > rect.y + rect.height) return false;
  if (points[node.min_index].x > rect.x + rect.width) return false;
  if (points[node.max_index].x < rect.x) return false;
  return true;
}

void resampling2_draw_raw(resampling2_raw_node_t raw, points_group_t const* pg, points_groups_draw_in_t *rdi) {
  (void)raw, (void)pg, (void)rdi;
  //smol_mesh_gen_line_strip(rdi->line_mesh, &pg->points[raw.index_start], raw.len, pg->color);
}

ssize_t resampling2_get_first_insidex(resampling2_nodes_t* nodes, Vector2 const * const points, Rectangle rect, uint32_t start_index) {
  while (start_index % powers_base != 0) {
    if (start_index == nodes->len) return -1;
    if (resampling2_nodex_inside_rect(nodes->arr[start_index], points, rect)) return start_index;
    ++start_index;
  }
  if (nodes->parent != NULL) {
    ssize_t new_i = resampling2_get_first_insidex(nodes->parent, points, rect, start_index / powers_base);
    if (new_i < 0) return -1;
    start_index = ((uint32_t)new_i) * powers_base;
  }
  for (; start_index < nodes->len; ++start_index) {
    if (resampling2_nodex_inside_rect(nodes->arr[start_index], points, rect)) {
      return (ssize_t)start_index;
    }
  }
  return -1;
}

float something = 0.02f;
float something2 = 0.001f;
float stride_after = 0.06f;
int max_stride = 10;
int raw_c = 0;
int not_raw_c = 0;

void resampling2_draw_x(resampling2_nodes_t* nodes, points_group_t const* pg, points_groups_draw_in_t *rdi) {
  //float step = rdi->rect.width / 1024;
  ssize_t j = 0;
  j = resampling2_get_first_insidex(nodes, pg->points, rdi->rect, (uint32_t)j);
  while (j != -1) {
    resampling2_node_t n = nodes->arr[j];
    Vector2* ps = pg->points;
    Vector2 first = ps[n.index_start], last = ps[n.index_start + n.len - 1];
    float width_ratio = fabsf(last.x - first.x) / rdi->rect.width;
    float height_ratio = (ps[n.max_index].y - ps[n.min_index].y) / rdi->rect.height;
    int depth = 0;
    if (fminf(width_ratio, height_ratio) > something) {
      if (width_ratio > stride_after && height_ratio > stride_after) {
        smol_mesh_gen_line_strip(rdi->line_mesh, &ps[n.index_start], n.len, pg->color);
      } else {
        int cur_stride = 1 + (int)((stride_after - fminf(width_ratio, height_ratio)) / (stride_after - something) * (float)max_stride);
        smol_mesh_gen_line_strip_stride(rdi->line_mesh, &ps[n.index_start], n.len, pg->color, cur_stride);
      }
      raw_c++;
    } else {
      resampling2_nodes_t* curn = nodes;
      ssize_t curj = j;
      while (curn->parent != NULL && curj % powers_base == 0 && (fminf(width_ratio, height_ratio) < something2)) {
        curj /= powers_base;
        curn = curn->parent;
        n = curn->arr[curj];
        first = ps[n.index_start], last = ps[n.index_start + n.len - 1];
        width_ratio = fabsf(last.x - first.x) / rdi->rect.width;
        height_ratio = (ps[n.max_index].y - ps[n.min_index].y) / rdi->rect.height;
        ++depth;
      }
      bool swp = (first.x < last.x) == (ps[n.min_index].x < ps[n.max_index].x);
      Vector2 pss[] = { 
        first,
        swp ? ps[n.min_index] : ps[n.max_index],
        swp ? ps[n.max_index] : ps[n.min_index],
        last
      };
      smol_mesh_gen_line_strip(rdi->line_mesh, pss, 4, pg->color);
      not_raw_c++;
    }
    j += (ssize_t)powers[depth];
    if (j < nodes->len) {
      uint32_t next_index = nodes->arr[j].index_start;
      smol_mesh_gen_line(rdi->line_mesh, last, ps[next_index], pg->color);
    }
    j = resampling2_get_first_insidex(nodes, pg->points, rdi->rect, (uint32_t)j);
  }
}

void resampling2_draw_y(resampling2_nodes_t* nodes, points_group_t const* pg, points_groups_draw_in_t *rdi) {
  return;
  //float step = rdi->rect.width / 1024;
  for (uint32_t j = 0; j < nodes->len; ++j) {
    if (resampling2_nodey_inside_rect(nodes->arr[j], pg->points, rdi->rect)) {
      smol_mesh_gen_line_strip(rdi->line_mesh, &pg->points[nodes->arr[j].index_start], (size_t)nodes->arr[j].len, pg->color);
    }
  }
}

void resampling2_draw_r(resampling2_all_roots r, points_group_t const* pg, points_groups_draw_in_t *rdi) {
  if (r.kind == resampling2_kind_raw) {
    resampling2_draw_raw(r.raw, pg, rdi);
  } else if (r.kind == resampling2_kind_x) {
    resampling2_draw_x(&r.x, pg, rdi);
  } else if (r.kind == resampling2_kind_y) {
    resampling2_draw_y(&r.y, pg, rdi);
  }
}

void resampling2_draw(resampling2_t *res, points_group_t const *pg, points_groups_draw_in_t *rdi) {
  printf("last: %f,%f, len: %lu\n", pg->points[pg->len - 1].x, pg->points[pg->len - 1].y, pg->len);
  for (uint32_t i = 0; i < res->roots_len; ++i) {
    resampling2_draw_r(res->roots[i], pg, rdi);
  }
  if (res->temp_x_valid) resampling2_draw_x(&res->temp_root_x, pg, rdi);
  else if (res->temp_y_valid) resampling2_draw_y(&res->temp_root_y, pg, rdi);
  else resampling2_draw_raw(res->temp_root_raw, pg, rdi);
}

static void resampling2_node_debug_print(FILE* file, resampling2_node_t* r, int depth) {
  if (r == NULL) return;
  for (int i = 0; i < depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "Node len: %u\n", r->len);
}

static void resampling2_nodes_debug_print(FILE* file, resampling2_nodes_t* r, int depth) {
  if (r == NULL) return;
  for (int i = 0; i < depth; ++i) {
    fprintf(file, "  ");
  }
  fprintf(file, "len: %u, cap: %u, rising: %d, falling: %d\n", r->len, r->cap, r->is_rising, r->is_falling);
  for (unsigned int i = 0; i < r->len; ++i) {
    resampling2_node_debug_print(file, &r->arr[i], depth + 1);
  }
  resampling2_nodes_debug_print(file, r->parent, depth + 1);
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
  resampling2_nodes_debug_print(file, &r->temp_root_x, 1);
  resampling2_nodes_debug_print(file, &r->temp_root_y, 1);
  resampling2_raw_node_debug_print(file, &r->temp_root_raw, 1);
  return;
}

#define PRINT_ALLOCS(prefix) \
  printf("\n%s ALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", prefix, \
      context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);

extern "C" {
#include "src/misc/tests.h"
TEST_CASE(resampling) {
  Vector2 points[] = { {0, 1}, {1, 2}, {2, 4}, {3, 2} };
  points_group_t pg = {
    .cap = 4,
    .len = 4,
    .points = points,
    .resampling = NULL
  };
  resampling2_t* r = resampling2_malloc();
  for (int i = 0; i < 2*1024; ++i) resampling2_add_point(r, &pg, 3);
  resampling2_debug_print(stdout, r);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 64*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}

#define N 2048
TEST_CASE(resampling2) {
  Vector2 points[N];
  for (int i = 0; i < N; ++i) {
    points[i].x = sinf(3.14f / 4.f * (float)i);
    points[i].y = cosf(3.14f / 4.f * (float)i);
  }
  points_group_t pg = {
    .cap = N,
    .len = N,
    .points = points,
    .resampling = NULL
  };
  resampling2_t* r = resampling2_malloc();
  for (int i = 0; i < N; ++i) resampling2_add_point(r, &pg, (uint32_t)i);
  resampling2_debug_print(stdout, r);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 64*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 1024*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %lu ( %luKB ) | %lu (%luKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}

} // extern "C"
