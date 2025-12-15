#include "src/br_pp.h"
#include "src/br_resampling.h"
#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_math.h"
#include "src/br_memory.h"
#include "src/br_mesh.h"
#include "src/br_platform.h"
#include "src/br_plot.h"
#include "src/br_shaders.h"
#include "include/br_free_list_header.h"

#include <stdint.h>
#include <math.h>
#include <string.h>

// Memory[MB] - MAX_LEN
// 53         - 128
// 54.5       - 64
// 57.8       - 32
// 64.1       - 16
// 64.78      - 8
// 78.1       - 4
// 104.8      - 2
#define RESAMPLING_NODE_MAX_LEN 16

static void br_line_culler_push_point(br_line_culler_t* lc, br_vec2_t p, br_vec2_t plot_size);
static void br_line_culler_push_line_strip(br_vec2_t const* points, size_t n, br_line_culler_t* lc, br_vec2_t plot_size);
static void br_line_culler_end(br_line_culler_t* lc);
static void br_line_culler_push_line_strip2(float const* xs, float const* ys, size_t n, br_line_culler_t* lc, br_vec2_t plot_size);

static inline float min4(float a, float b, float c, float d);
static inline float min6(float a, float b, float c, float d, float e, float f);
static inline float max3(float a, float b, float c);
static inline float max4(float a, float b, float c, float d);
static inline float max6(float a, float b, float c, float d, float e, float f);

static inline bool br_resampling_nodes_2d_is_inside(br_resampling_nodes_t res, float const* xs, float const* ys, br_extent_t rect);
static bool br_resampling_nodes_2d_is_inside_3d(br_resampling_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mat);
static br_vec2_t br_resampling_nodes_2d_get_ratios(br_resampling_nodes_2d_t const* res, float const* xs, float const* ys, float screen_width, float screen_height);
static br_vec2_t br_resampling_nodes_2d_get_ratios(br_resampling_nodes_2d_t const* res, float const* xs, float const* ys, float screen_width, float screen_height);
static br_vec2_t br_resampling_nodes_2d_get_ratios_3d(br_resampling_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mvp);

// TODO: Look at this should this take into consideration rebasing of data..
static br_vec3_t br_data_3d_get_v3(br_data_3d_t const* data, uint32_t index);
static bool br_resampling_nodes_3d_is_inside(br_resampling_nodes_3d_t const* res, br_data_3d_t const* data, br_mat_t mvp);
static void br_resampling_debug_3d(br_resampling_t const* r, br_resampling_nodes_3d_t const* res, br_data_3d_t const* data);
static br_vec2_t br_resampling_nodes_3d_get_ratios(br_resampling_nodes_3d_t const* res, br_data_3d_t const* data, br_vec3_t look_dir);
static void br_resampling_nodes_deinit(br_resampling_t* nodes);
static bool br_resampling_nodes_2d_push_point(br_resampling_nodes_2d_allocator_t* nodes, size_t node_index, uint32_t index, float const* xs, float const* ys);
static bool br_resampling_nodes_3d_push_point(br_resampling_nodes_3d_allocator_t* nodes, size_t node_index, uint32_t index, float const* xs, float const* ys, float const* zs);


static BR_THREAD_LOCAL struct {
  uint32_t powers_base;
  uint32_t powers[32];
  br_shaders_t* shaders;
  float* min_sampling;
  float* cull_min;
} br_resampling;

void br_resampling_construct(br_shaders_t* shaders, float* min_sampling, float* cull_min) {
  br_resampling.powers_base = 2;

  br_resampling.powers[0] = 1;
  br_resampling.powers[1] = br_resampling.powers_base;
  for (int i = 2; i < 32; ++i) {
    br_resampling.powers[i] = br_resampling.powers[i - 1] * br_resampling.powers_base;
  }

  br_resampling.shaders = shaders;
  br_resampling.min_sampling = min_sampling;
  br_resampling.cull_min = cull_min;
}

br_resampling_t* br_resampling_malloc(br_data_kind_t kind) {
  br_resampling_t* r = (br_resampling_t*)BR_CALLOC(1, sizeof(*r));
  if (r == NULL) return NULL;
  r->kind = kind;
  r->something = 0.02f;
  r->something2 = 0.001f;
  return r;
}

void br_resampling_empty(br_resampling_t* res) {
  if (NULL == res) return;
  br_resampling_nodes_deinit(res);
}

void br_resampling_free(br_resampling_t* r) {
  if (NULL == r) return;
  br_resampling_nodes_deinit(r);
  BR_FREE(r);
}

void br_resampling_add_point(br_resampling_t* r, const br_data_t *pg, uint32_t index) {
  if (r->dd.len == 0) {
    switch (r->kind) {
      case br_data_kind_2d: { br_da_push_t(br_u32, (r->dd), (br_resampling_nodes_2d_t){0}); break; }
      case br_data_kind_3d: { br_da_push_t(br_u32, (r->ddd), (br_resampling_nodes_3d_t){0}); break; }
      default: BR_UNREACHABLE("kind %d", r->kind);
    }
  }
  switch (r->kind) {
    case br_data_kind_2d: { br_resampling_nodes_2d_push_point(&r->dd, 0, index, pg->dd.xs, pg->dd.ys); break; }
    case br_data_kind_3d: { br_resampling_nodes_3d_push_point(&r->ddd, 0, index, pg->ddd.xs, pg->ddd.ys, pg->ddd.zs); break; }
    default: BR_UNREACHABLE("kind %d", r->kind);
  }
}

#define BR_STACK_PUSH(VAL) stack[stack_len++] = VAL
#define BR_STACK_POP() stack_len > 0 ? stack[--stack_len] : 0xFFFFFFFF;
bool br_resampling_get_point_at2(br_data_t data, br_vec2d_t vecd, float* dist, br_u32* out_index) {
  // NOTE: (2**64)*64 = 2**69 = 10**19 elements
  //       data.len can not be larger than 2**64, so we are good..
  br_u32 stack[64] /* = undefined */;
  int stack_len = 0;
  br_u32 cur_node /* = undefined */; ;
  br_vec2_t vec = BR_VEC2((float)(vecd.x - data.dd.rebase_x), (float)(vecd.y - data.dd.rebase_y));
  br_extent_t ex = BR_EXTENT(vec.x - *dist/2, vec.y + *dist/2, *dist, *dist);
  br_resampling_nodes_2d_allocator_t rns = data.resampling->dd;
  if (rns.len == 0) return false;
  bool found = false;

  BR_STACK_PUSH(0);
  while (stack_len > 0) {
    cur_node = BR_STACK_POP();
    br_resampling_nodes_2d_t node = rns.arr[cur_node];
    if (br_resampling_nodes_2d_is_inside(node.base, data.dd.xs, data.dd.ys, ex)) {
      if (node.base.depth == 0) {
        for (br_u32 i = 0; i < node.base.len; ++i) {
          br_u32 index = i + node.base.index_start;
          float cur_dist = br_vec2_dist(BR_VEC2(data.dd.xs[index], data.dd.ys[index]), vec);
          if (cur_dist < *dist) {
            *out_index = index;
            *dist = cur_dist;
            found = true;
          }
        }
      } else {
        BR_STACK_PUSH(node.base.child1);
        BR_STACK_PUSH(node.base.child2);
      }
    }
  }
  return found;
}

br_vec3d_t br_resampling_get_rebase3(br_data_t data) {
  switch (data.kind) {
    case br_data_kind_3d: return data.ddd.rebase;
    case br_data_kind_2d: return BR_VEC3D(data.dd.rebase_x, data.dd.rebase_y, 0.0);
    default: BR_UNREACHABLE("Data kind: %d", data.kind);
  }
}

br_bb3_t br_resampling_get_bb3_rebased(br_resampling_t* res, br_data_t data, br_u32 node_index) {
  switch (data.kind) {
    case br_data_kind_3d: {
      br_resampling_nodes_3d_allocator_t rns = res->ddd;
      br_resampling_nodes_3d_t node = rns.arr[node_index];
      return (br_bb3_t){ .min = BR_VEC3(data.ddd.xs[node.base.min_index_x], data.ddd.ys[node.base.min_index_y], data.ddd.zs[node.min_index_z]),
                         .max = BR_VEC3(data.ddd.xs[node.base.max_index_x], data.ddd.ys[node.base.max_index_y], data.ddd.zs[node.max_index_z]) };
    } break;
    case br_data_kind_2d: {
      br_resampling_nodes_2d_allocator_t rns = res->dd;
      br_resampling_nodes_2d_t node = rns.arr[node_index];
      return (br_bb3_t){ .min = BR_VEC3(data.ddd.xs[node.base.min_index_x], data.ddd.ys[node.base.min_index_y], 0.f),
                         .max = BR_VEC3(data.ddd.xs[node.base.max_index_x], data.ddd.ys[node.base.max_index_y], 0.f) };
    } break;
    default: BR_UNREACHABLE("Data kind: %d", data.kind); break;
  }
}

static void br_resampling_get_info(br_resampling_t* res, br_data_kind_t kind, br_u32 node_index, br_u32* child1, br_u32* child2, br_u32* depth, br_u32* index_start, br_u32* node_len) {
  br_resampling_nodes_t base;
  switch (kind) {
    case br_data_kind_2d: base = res->dd.arr[node_index].base; break;
    case br_data_kind_3d: base = res->ddd.arr[node_index].base; break;
    default: BR_UNREACHABLE("Data kind: %d", kind); break;
  }
  *child1 = base.child1;
  *child2 = base.child2;
  *depth = base.depth;
  *index_start = base.index_start;
  *node_len = base.len;
}


bool br_resampling_get_point_at3(br_data_t data, br_vec3d_t from, br_vec3d_t to, float* dist, br_u32* out_index) {
  if (data.len == 0) return false;
  if (data.resampling->common.len == 0) return false;

  // NOTE: (2**64)*64 = 2**69 = 10**19 elements
  //       data.len can not be larger than 2**64, so we are good..
  br_u32 stack[64] /* = undefined */;
  int stack_len = 0;
  br_u32 cur_node /* = undefined */; ;
  br_vec3d_t rebase = br_resampling_get_rebase3(data);
  br_u32 child1, child2, depth, index_start, node_len;

  from = br_vec3d_sub(from, rebase);
  to =   br_vec3d_sub(to,   rebase);
  br_vec3_t fromf = BR_VEC3D_TOF(from);
  br_vec3_t tof =   BR_VEC3D_TOF(to);
  bool found = false;

  BR_STACK_PUSH(0);
  while (stack_len > 0) {
    cur_node = BR_STACK_POP();
    br_bb3_t bb = br_resampling_get_bb3_rebased(data.resampling, data, cur_node);
    br_resampling_get_info(data.resampling, data.kind, cur_node, &child1, &child2, &depth, &index_start, &node_len);
    if (br_col_line_bb(fromf, tof, bb) || br_vec3_line_dist2(fromf, tof, br_vec3_scale(br_vec3_add(bb.min, bb.max), 0.5f)) < *dist) {
      if (depth == 0) {
        for (br_u32 i = 0; i < node_len; ++i) {
          br_u32 index = i + index_start;
          float cur_dist = br_vec3_line_dist2(fromf, tof, br_data_el_xyz_rebased(data, index));
          if (cur_dist < *dist) {
            *out_index = index;
            *dist = cur_dist;
            found = true;
          }
        }
      } else {
        BR_STACK_PUSH(child1);
        BR_STACK_PUSH(child2);
      }
    }
  }
  return found;
}
#undef BR_STACK_PUSH
#undef BR_STACK_POP

void br_resampling_reset(br_resampling_t* res) {
  res->common.len = 0;
}

static void br_resampling_nodes_deinit(br_resampling_t* nodes) {
  if (nodes == NULL) return;
  if (nodes->common.arr) BR_FREE(nodes->common.arr);
  nodes->common.len = 0;
  nodes->common.cap = 0;
  nodes->common.arr = NULL;
}

static bool br_resampling_nodes_2d_push_point(br_resampling_nodes_2d_allocator_t* nodes, size_t node_index, uint32_t index, float const* xs, float const* ys) {
  br_resampling_nodes_2d_t node = nodes->arr[node_index];
  ++node.base.len;
  if (node.base.len == 1) {
    node.base.index_start =
    node.base.max_index_y =
    node.base.min_index_y =
    node.base.max_index_x =
    node.base.min_index_x = index;
  } else {
    if (ys[index] < ys[node.base.min_index_y]) node.base.min_index_y = index;
    if (ys[index] > ys[node.base.max_index_y]) node.base.max_index_y = index;
    if (xs[index] < xs[node.base.min_index_x]) node.base.min_index_x = index;
    if (xs[index] > xs[node.base.max_index_x]) node.base.max_index_x = index;
    if (node.base.depth > 0) {
      if (false == br_resampling_nodes_2d_push_point(nodes, node.base.child2, index, xs, ys)) {
        return false;
      }
    }
    uint32_t power = br_resampling.powers[node.base.depth];
    bool split = (node_index == 0 && node.base.len == RESAMPLING_NODE_MAX_LEN * power) ||
      (node_index != 0 && node.base.len > RESAMPLING_NODE_MAX_LEN * power);
    if (split) {
      br_resampling_nodes_2d_t left = node;
      br_resampling_nodes_2d_t right = {0};
      br_da_push_t(br_u32, *nodes, left);
      node.base.child1 = nodes->len - 1;
      br_da_push_t(br_u32, *nodes, right);
      node.base.child2 = nodes->len - 1;
      ++node.base.depth;
    }
  }
  nodes->arr[node_index] = node;
  return true;
}

static bool br_resampling_nodes_3d_push_point(br_resampling_nodes_3d_allocator_t* nodes, size_t node_index, uint32_t index,
    float const* xs, float const* ys, float const* zs) {
  br_resampling_nodes_3d_t node = nodes->arr[node_index];
  ++node.base.len;
  if (node.base.len == 1) {
    node.base.index_start =
    node.max_index_z =
    node.min_index_z =
    node.base.max_index_y =
    node.base.min_index_y =
    node.base.max_index_x =
    node.base.min_index_x = index;
    node.curvature = BR_VEC3(0, 0, 0);
  } else {
    br_vec3_t v = BR_VEC3(xs[index], ys[index], zs[index]);
    if (zs[index] < zs[node.min_index_z]) node.min_index_z = index;
    if (zs[index] > zs[node.max_index_z]) node.max_index_z = index;
    if (ys[index] < ys[node.base.min_index_y]) node.base.min_index_y = index;
    if (ys[index] > ys[node.base.max_index_y]) node.base.max_index_y = index;
    if (xs[index] < xs[node.base.min_index_x]) node.base.min_index_x = index;
    if (xs[index] > xs[node.base.max_index_x]) node.base.max_index_x = index;
    if (index > 2) {
      // TODO: This needs not be calculated on every depth...
      br_vec3_t m1 = BR_VEC3(xs[index - 1], ys[index - 1], zs[index - 1]);
      br_vec3_t m2 = BR_VEC3(xs[index - 2], ys[index - 2], zs[index - 2]);
      br_vec3_t d1 = br_vec3_sub(v, m1);
      br_vec3_t d2 = br_vec3_sub(m1, m2);
      br_vec3_t cur = br_vec3_sub(br_vec3_normalize(d1), br_vec3_normalize(d2));
      cur.x = fabsf(cur.x);
      cur.y = fabsf(cur.y);
      cur.z = fabsf(cur.z);
      node.curvature = br_vec3_add(node.curvature, cur);
    }
    if (node.base.depth > 0) {
      if (false == br_resampling_nodes_3d_push_point(nodes, node.base.child2, index, xs, ys, zs)) {
        return false;
      }
    }
    uint32_t power = br_resampling.powers[node.base.depth];
    bool split = (node_index == 0 && node.base.len == RESAMPLING_NODE_MAX_LEN * power) ||
      (node_index != 0 && node.base.len > RESAMPLING_NODE_MAX_LEN * power);
    if (split) {
      br_resampling_nodes_3d_t left = node;
      br_resampling_nodes_3d_t right = {0};
      br_da_push_t(br_u32, *nodes, left);
      node.base.child1 = nodes->len - 1;
      br_da_push_t(br_u32, *nodes, right);
      node.base.child2 = nodes->len - 1;
      ++node.base.depth;
    }
  }
  nodes->arr[node_index] = node;
  return true;
}

static int size_t_cmp(void const* a, void const* b) {
  long long const* ap = a, *bp = b;
  return (int)(*ap - *bp);
}

static void br_resampling_draw22(br_resampling_nodes_2d_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_extent_t plot_extent) {
  BR_ASSERT(pg->kind == br_data_kind_2d);
  float const* xs = pg->dd.xs;
  float const* ys = pg->dd.ys;
  br_vec2_t plot_size = plot_extent.size.vec;
  br_resampling_nodes_2d_t node = nodes->arr[index];
  if (false == br_resampling_nodes_2d_is_inside(node.base, xs, ys, plot_extent)) {
    br_line_culler_end(&pg->resampling->culler);
    return;
  }
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) {
    // This is the leaf node
    br_line_culler_push_line_strip2(&xs[node.base.index_start], &ys[node.base.index_start], node.base.len + (is_end ? 0 : 1), &pg->resampling->culler, plot_size);
    return;
  }
  br_vec2_t ratios = br_resampling_nodes_2d_get_ratios(&node, xs, ys, plot_extent.width, plot_extent.height);
  float rmin = fminf(ratios.x, ratios.y);
  if (rmin < (node.base.depth == 1 ? pg->resampling->something : pg->resampling->something2)) {
    size_t indexies[] = {
      node.base.index_start,
      node.base.min_index_x,
      node.base.min_index_y,
      node.base.max_index_x,
      node.base.max_index_y,
      node.base.index_start + node.base.len - (is_end ? 1 : 0)
    };
    qsort(indexies, sizeof(indexies)/sizeof(indexies[0]), sizeof(indexies[0]), &size_t_cmp);
    br_vec2_t pss[] = {
      BR_VEC2(xs[indexies[0]], ys[indexies[0]]), BR_VEC2(xs[indexies[1]], ys[indexies[0]]),
      BR_VEC2(xs[indexies[2]], ys[indexies[2]]), BR_VEC2(xs[indexies[3]], ys[indexies[3]]),
      BR_VEC2(xs[indexies[4]], ys[indexies[4]]), BR_VEC2(xs[indexies[5]], ys[indexies[5]]),
    };
    br_line_culler_push_line_strip(pss, 6, &pg->resampling->culler, plot_size);
  } else {
    br_resampling_draw22(nodes, node.base.child1, pg, plot_extent);
    br_resampling_draw22(nodes, node.base.child2, pg, plot_extent);
  }
}

static void br_resampling_draw32(br_resampling_t const* const res, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  BR_ASSERTF(plot->kind == br_plot_kind_3d, "Kind is %d", plot->kind);
  BR_ASSERTF(pg->kind == br_data_kind_2d, "Kind is %d", pg->kind);
  //ZoneScopedN("br_resampling_3d");
  float const* xs = pg->dd.xs;
  float const* ys = pg->dd.ys;
  br_resampling_nodes_2d_t node = res->dd.arr[index];
  br_mat_t mvp = res->args_3d.mvp;
  if (false == br_resampling_nodes_2d_is_inside_3d(&node, xs, ys, mvp)) return;
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) { // This is the leaf node
    br_mesh_3d_gen_line_strip3(res->args_3d, &xs[node.base.index_start], &ys[node.base.index_start], node.base.len + (is_end ? 0 : 1));
    return;
  }
  br_vec2_t ratios = br_resampling_nodes_2d_get_ratios_3d(&node, xs, ys, mvp);
  float rmin = fminf(ratios.x, ratios.y);
  if (rmin < (node.base.depth == 1 ? res->something : res->something2 )) {
    size_t indexies[] = {
      node.base.index_start,
      node.base.min_index_x,
      node.base.min_index_y,
      node.base.max_index_x,
      node.base.max_index_y,
      node.base.index_start + node.base.len - (is_end ? 1 : 0)
    };
    qsort(indexies, sizeof(indexies)/sizeof(indexies[0]), sizeof(indexies[0]), &size_t_cmp);
    br_vec2_t pss[] = {
      BR_VEC2(xs[indexies[0]], ys[indexies[0]]), BR_VEC2(xs[indexies[1]], ys[indexies[1]]),
      BR_VEC2(xs[indexies[2]], ys[indexies[2]]), BR_VEC2(xs[indexies[3]], ys[indexies[3]]),
      BR_VEC2(xs[indexies[4]], ys[indexies[4]]), BR_VEC2(xs[indexies[5]], ys[indexies[5]]),
    };
    br_mesh_3d_gen_line_strip2(res->args_3d, pss, 6);
  } else {
    br_resampling_draw32(res, node.base.child1, pg, plot);
    br_resampling_draw32(res, node.base.child2, pg, plot);
  }
}

static void br_resampling_draw33(br_resampling_t const* const res, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  BR_ASSERTF(plot->kind == br_plot_kind_3d, "Kind is %d", plot->kind);
  BR_ASSERTF(pg->kind == br_data_kind_3d, "Kind is %d", pg->kind);
  //ZoneScopedN("br_resampling_3d");
  float const* xs = pg->ddd.xs;
  float const* ys = pg->ddd.ys;
  float const* zs = pg->ddd.zs;
  br_resampling_nodes_3d_t node = res->ddd.arr[index];
  br_mat_t mvp = res->args_3d.mvp;
  br_vec3_t eye = plot->ddd.eye;
  br_vec3_t target = plot->ddd.target;
  if (false == br_resampling_nodes_3d_is_inside(&node, &pg->ddd, mvp)) return;
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) { // This is the leaf node
    size_t st = node.base.index_start;
    br_mesh_3d_gen_line_strip1(res->args_3d, &xs[st], &ys[st], &zs[st], node.base.len + (is_end ? 0 : 1));
    return;
  }
  br_vec2_t ratios = br_resampling_nodes_3d_get_ratios(&node, &pg->ddd, br_vec3_sub(target, eye));
  BR_ASSERT(ratios.x > 0);
  BR_ASSERT(ratios.y > 0);
  float rmin = fmaxf(ratios.x, ratios.y);
  if (rmin < (node.base.depth == 1 ? pg->resampling->something2 : pg->resampling->something)) {
    size_t indexies[] = {
      node.base.index_start,
      node.base.min_index_x,
      node.base.min_index_y,
      node.min_index_z,
      node.base.max_index_x,
      node.base.max_index_y,
      node.max_index_z,
      node.base.index_start + node.base.len - (is_end ? 1 : 0)
    };

    qsort(indexies, sizeof(indexies)/sizeof(indexies[0]), sizeof(indexies[0]), &size_t_cmp);
    size_t cur = indexies[0];
    for (size_t i = 1; i < 8; ++i) {
      if (cur == indexies[i]) continue;
      br_mesh_3d_gen_line(&res->args_3d, BR_VEC3(xs[cur], ys[cur], zs[cur]),  BR_VEC3(xs[indexies[i]], ys[indexies[i]], zs[indexies[i]]));
      cur = indexies[i];
    }
  } else {
    br_resampling_draw33(res, node.base.child1, pg, plot);
    br_resampling_draw33(res, node.base.child2, pg, plot);
  }
}

void br_resampling_draw(br_resampling_t* res, br_data_t const* pg, br_plot_t* plot, float pd_thickness, br_extent_t extent) {
  double start = brpl_time();
  if (res->common.len == 0) return;
  switch (pg->kind) {
    case br_data_kind_2d: {
      switch (plot->kind) {
        case br_plot_kind_2d: {
          res->culler.args.screen_size = BR_VEC2_TOD(extent.size.vec);
          res->culler.args.zoom = plot->dd.zoom;
          res->culler.args.offset = plot->dd.offset;
          res->culler.args.offset.x -= pg->dd.rebase_x;
          res->culler.args.offset.y -= pg->dd.rebase_y;
          res->culler.args.line_thickness = plot->dd.line_thickness * pd_thickness;
          res->culler.args.prev[0] = BR_VEC2(0, 0);
          res->culler.args.prev[1] = BR_VEC2(0, 0);

          br_resampling.shaders->line->uvs.color_uv = BR_COLOR_TO4(pg->color).xyz;
          float aspect = extent.width/extent.height;
          br_extentd_t plot_rect = BR_EXTENTD(
            -aspect*plot->dd.zoom.x/2.0 + plot->dd.offset.x - pg->dd.rebase_x,
                    plot->dd.zoom.y/2.f + plot->dd.offset.y - pg->dd.rebase_y,
            aspect*plot->dd.zoom.x,
            plot->dd.zoom.y);

          br_resampling_draw22(&res->dd, 0, pg, BR_EXTENTD_TOF(plot_rect));
          br_line_culler_end(&res->culler);
          br_shader_line_draw(br_resampling.shaders->line);
        } break;
        case br_plot_kind_3d: {
          br_vec3_t target = plot->ddd.target;
          target.x -= (float)pg->dd.rebase_x;
          target.y -= (float)pg->dd.rebase_y;
          br_vec3_t eye = plot->ddd.eye;
          eye.x -= (float)pg->dd.rebase_x;
          eye.y -= (float)pg->dd.rebase_y;
          br_vec2_t re = (br_vec2_t) { .x = extent.width, .y = extent.height };
          br_mat_t per = br_mat_perspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
          br_mat_t look = br_mat_look_at(eye, target, plot->ddd.up);
          br_resampling.shaders->line_3d->uvs.m_mvp_uv = br_mat_mul(look, per);
          br_resampling.shaders->line_3d->uvs.eye_uv = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
          br_resampling.shaders->line_3d->uvs.color_uv = BR_COLOR_TO4(pg->color).xyz;
          res->args_3d.line_thickness = pd_thickness;
          res->args_3d.mvp = br_resampling.shaders->line_3d->uvs.m_mvp_uv;
          res->args_3d.eye = eye;
          res->args_3d.target = target;

          br_resampling_draw32(res, 0, pg, plot);
          br_shader_line_3d_draw(br_resampling.shaders->line_3d);
        } break;
      }
    } break;
    case br_data_kind_3d: {
      switch (plot->kind) {
        case br_plot_kind_2d: BR_UNREACHABLE("Can't draw 3d data on 2d plot..");
        case br_plot_kind_3d: {
          br_vec3d_t target = BR_VEC3_TOD(plot->ddd.target);
          br_vec3d_t eye = BR_VEC3_TOD(plot->ddd.eye);
          target = br_vec3d_sub(target, pg->ddd.rebase);
          eye = br_vec3d_sub(eye, pg->ddd.rebase);
          br_vec2_t re = extent.size.vec;
          br_mat_t per = br_mat_perspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
          br_mat_t look = br_mat_look_at(BR_VEC3D_TOF(eye), BR_VEC3D_TOF(target), plot->ddd.up);
          br_resampling.shaders->line_3d->uvs.m_mvp_uv = br_mat_mul(look, per);
          br_resampling.shaders->line_3d->uvs.eye_uv = br_vec3_sub(plot->ddd.eye, plot->ddd.target); //TODO: Is this realy eye or eye target vector?
          br_resampling.shaders->line_3d->uvs.color_uv = BR_COLOR_TO4(pg->color).xyz;
          res->args_3d.line_thickness = pd_thickness;
          res->args_3d.mvp = br_resampling.shaders->line_3d->uvs.m_mvp_uv;
          res->args_3d.eye = BR_VEC3D_TOF(eye);
          res->args_3d.target = BR_VEC3D_TOF(target);

          br_resampling_draw33(res, 0, pg, plot); break;
          br_shaders_draw_all(*br_resampling.shaders);
        }
      }
    } break;
    default: BR_UNREACHABLE("Unknown data kind: %d", pg->kind);
  }
  res->render_time = brpl_time() - start;
  ++res->draw_count;
}

void br_resampling_change_something(br_datas_t pg) {
  uint32_t draw_count = 0;
  brfl_foreach(i, pg) draw_count += pg.arr[i].resampling->draw_count;
  if (draw_count == 0) return;
  double target = 0.016 / (double)draw_count;
  brfl_foreach(i, pg) {
    if (pg.arr[i].resampling->draw_count == 0) continue;
    double delta = (pg.arr[i].resampling->render_time - target);
    double mul = 1.0 + 10.0f* (delta);

    pg.arr[i].resampling->something *= (float)mul;
    pg.arr[i].resampling->something2 *= (float)mul;
    float mins = *br_resampling.min_sampling;
  //pg.arr[i].resampling->something = mins;
  //pg.arr[i].resampling->something2 = mins;
    if (pg.arr[i].resampling->something < mins) pg.arr[i].resampling->something = mins;
    if (pg.arr[i].resampling->something2 < mins) pg.arr[i].resampling->something2 = mins;
    pg.arr[i].resampling->draw_count = 0;
  }
}

double br_resampling_get_draw_time(br_resampling_t* res) {
  return res->render_time;
}

float br_resampling_get_something(br_resampling_t* res) {
  return res->something;
}

float br_resampling_get_something2(br_resampling_t* res) {
  return res->something2;
}

static void br_line_culler_push_point(br_line_culler_t* lc, br_vec2_t p, br_vec2_t plot_size) {
  if (lc->has_old == false) {
    lc->old = p;
    lc->has_old = true;
  }

  br_vec2_t d =
  br_vec2_mul(
    br_vec2_div(
      br_vec2_sub(p, lc->old),
      //plot->dd.graph_rect.size.vec),
      plot_size),
    BR_VEC2D_TOF(lc->args.screen_size)
  );

  const float min_dist = *br_resampling.cull_min;
  if (fabsf(d.x) + fabsf(d.y) < min_dist) {
    lc->mid = p;
    return;
  }

  br_mesh_gen_line(&lc->args, lc->old, p);
  lc->mid = lc->old = p;
}

static void br_line_culler_push_line_strip(br_vec2_t const* points, size_t n, br_line_culler_t* lc, br_vec2_t plot_size) {
  for (size_t i = 0; i < n; ++i) {
    br_line_culler_push_point(lc, points[i], plot_size);
  }
}

static void br_line_culler_end(br_line_culler_t* lc) {
  if (false == br_vec2_eq(lc->old, lc->mid)) {
    br_mesh_gen_line(&lc->args, lc->old, lc->mid);
  }
  lc->has_old = false;
  lc->args.prev[0] = (br_vec2_t){ 0 };
  lc->args.prev[1] = (br_vec2_t){ 0 };
}

static void br_line_culler_push_line_strip2(float const* xs, float const* ys, size_t n, br_line_culler_t* lc, br_vec2_t plot_size) {
  for (size_t i = 0; i < n; ++i) {
    br_line_culler_push_point(lc, BR_VEC2(xs[i], ys[i]), plot_size);
  }
}

static inline float min4(float a, float b, float c, float d) {
  return fminf(fminf(a, b), fminf(c, d));
}

static inline float min6(float a, float b, float c, float d, float e, float f) {
  return fminf(min4(a, b, c, d), fminf(e, f));
}

static inline float max3(float a, float b, float c) {
  return fmaxf(fmaxf(a, b), c);
}

static inline float max4(float a, float b, float c, float d) {
  return fmaxf(fmaxf(a, b), fmaxf(c, d));
}

static inline float max6(float a, float b, float c, float d, float e, float f) {
  return fmaxf(max4(a, b, c, d), fmaxf(e, f));
}

static inline bool br_resampling_nodes_2d_is_inside(br_resampling_nodes_t res, float const* xs, float const* ys, br_extent_t rect) {
  if (res.len == 0) return false;
  float minx = xs[res.min_index_x], miny = ys[res.min_index_y], maxx = xs[res.max_index_x], maxy = ys[res.max_index_y];
  return !((miny > rect.y) || (maxy < rect.y - rect.height) || (minx > rect.x + rect.width) || (maxx < rect.x));
}

static bool br_resampling_nodes_2d_is_inside_3d(br_resampling_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mat) {
  if (res->base.len == 0) return false;
  br_vec3_t minx = br_vec2_transform_scale(BR_VEC2(xs[res->base.min_index_x], ys[res->base.min_index_x]), mat),
            miny = br_vec2_transform_scale(BR_VEC2(xs[res->base.min_index_y], ys[res->base.min_index_y]), mat),
            maxx = br_vec2_transform_scale(BR_VEC2(xs[res->base.max_index_x], ys[res->base.max_index_x]), mat),
            maxy = br_vec2_transform_scale(BR_VEC2(xs[res->base.max_index_y], ys[res->base.max_index_y]), mat);
  float mx = min4(minx.x, miny.x, maxy.x, maxx.x);
  float Mx = max4(minx.x, miny.x, maxy.x, maxx.x);
  float my = min4(minx.y, miny.y, maxy.y, maxx.y);
  float My = max4(minx.y, miny.y, maxy.y, maxx.y);
  float Mz = max4(minx.z, miny.z, maxy.z, maxx.z);
  float quad_size = 2.1f;

  br_extent_t rect = BR_EXTENT(quad_size / -2, quad_size / -2, quad_size, quad_size);
  return Mz > 0.f && br_col_extents(rect, BR_EXTENT(mx, my, Mx - mx, My - my));
}

br_vec2_t br_resampling_nodes_2d_get_ratios(br_resampling_nodes_2d_t const* res, float const* xs, float const* ys, float screen_width, float screen_height) {
  float xr = xs[res->base.max_index_x] - xs[res->base.min_index_x], yr = ys[res->base.max_index_y] - ys[res->base.min_index_y];
  return BR_VEC2(xr / screen_width, yr / screen_height);
}

static br_vec2_t br_resampling_nodes_2d_get_ratios_3d(br_resampling_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mvp) {
  br_vec3_t minx = br_vec2_transform_scale(BR_VEC2(xs[res->base.min_index_x], ys[res->base.min_index_x]), mvp),
            miny = br_vec2_transform_scale(BR_VEC2(xs[res->base.min_index_y], ys[res->base.min_index_y]), mvp),
            maxx = br_vec2_transform_scale(BR_VEC2(xs[res->base.max_index_x], ys[res->base.max_index_x]), mvp),
            maxy = br_vec2_transform_scale(BR_VEC2(xs[res->base.max_index_y], ys[res->base.max_index_y]), mvp);
  float mx = min4(minx.x, miny.x, maxy.x, maxx.x);
  float my = min4(minx.y, miny.y, maxy.y, maxx.y);
  float Mx = max4(minx.x, miny.x, maxy.x, maxx.x);
  float My = max4(minx.y, miny.y, maxy.y, maxx.y);
  return BR_VEC2((Mx - mx) / 2.f, (My - my) / 2.f);
}

static br_vec3_t br_data_3d_get_v3(br_data_3d_t const* data, uint32_t index) {
  return BR_VEC3(data->xs[index], data->ys[index], data->zs[index]);
}

static bool br_resampling_nodes_3d_is_inside(br_resampling_nodes_3d_t const* res, br_data_3d_t const* data, br_mat_t mvp) {
    br_vec3_t bb_min = BR_VEC3(
        br_data_3d_get_v3(data, res->base.min_index_x).x,
        br_data_3d_get_v3(data, res->base.min_index_y).y,
        br_data_3d_get_v3(data, res->min_index_z).z
    );
    br_vec3_t bb_max = BR_VEC3(
        br_data_3d_get_v3(data, res->base.max_index_x).x,
        br_data_3d_get_v3(data, res->base.max_index_y).y,
        br_data_3d_get_v3(data, res->max_index_z).z
    );

    float min_x =  INFINITY;
    float max_x = -INFINITY;
    float min_y =  INFINITY;
    float max_y = -INFINITY;

    for (int i = 0; i < 8; ++i) {
        br_vec3_t corner = BR_VEC3(
            (i & 1) ? bb_max.x : bb_min.x,
            (i & 2) ? bb_max.y : bb_min.y,
            (i & 4) ? bb_max.z : bb_min.z
        );

        br_vec3_t ndc = br_vec3_transform_scale(corner, mvp);

        if (ndc.x == HUGE_VALF) return true;

        if (ndc.x >= -1.0f && ndc.x <= 1.0f &&
            ndc.y >= -1.0f && ndc.y <= 1.0f &&
            ndc.z >= -1.0f && ndc.z <= 1.0f) {
            return true;
        }

        if (ndc.x < min_x) min_x = ndc.x;
        if (ndc.x > max_x) max_x = ndc.x;
        if (ndc.y < min_y) min_y = ndc.y;
        if (ndc.y > max_y) max_y = ndc.y;
    }

    const float guard = 1.05f;
    bool visible = (max_x >= -guard && min_x <= guard &&
                    max_y >= -guard && min_y <= guard);
    return visible;
}

BR_TEST_ONLY static void br_resampling_debug_3d(br_resampling_t const* r, br_resampling_nodes_3d_t const* res, br_data_3d_t const* data) {
  br_vec3_t minx = br_data_3d_get_v3(data, res->base.min_index_x),
    miny = br_data_3d_get_v3(data, res->base.min_index_y),
    minz = br_data_3d_get_v3(data, res->min_index_z),
    maxx = br_data_3d_get_v3(data, res->base.max_index_x),
    maxy = br_data_3d_get_v3(data, res->base.max_index_y),
    maxz = br_data_3d_get_v3(data, res->max_index_z);

  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(minx.x, miny.y, minz.z), BR_VEC3(maxx.x, miny.y, minz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(minx.x, miny.y, minz.z), BR_VEC3(minx.x, maxy.y, minz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(minx.x, miny.y, minz.z), BR_VEC3(minx.x, miny.y, maxz.z));

  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(maxx.x, maxy.y, minz.z), BR_VEC3(minx.x, maxy.y, minz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(maxx.x, maxy.y, minz.z), BR_VEC3(maxx.x, miny.y, minz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(maxx.x, maxy.y, minz.z), BR_VEC3(maxx.x, maxy.y, maxz.z));


  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(minx.x, maxy.y, maxz.z), BR_VEC3(maxx.x, maxy.y, maxz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(minx.x, maxy.y, maxz.z), BR_VEC3(minx.x, miny.y, maxz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(minx.x, maxy.y, maxz.z), BR_VEC3(minx.x, maxy.y, minz.z));

  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(maxx.x, miny.y, maxz.z), BR_VEC3(minx.x, miny.y, maxz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(maxx.x, miny.y, maxz.z), BR_VEC3(maxx.x, maxy.y, maxz.z));
  br_mesh_3d_gen_line(&r->args_3d, BR_VEC3(maxx.x, miny.y, maxz.z), BR_VEC3(maxx.x, miny.y, minz.z));
}

static br_vec2_t br_resampling_nodes_3d_get_ratios(br_resampling_nodes_3d_t const* res, br_data_3d_t const* data, br_vec3_t look_dir) {
  br_vec3_t minx = br_data_3d_get_v3(data, res->base.min_index_x),
    miny         = br_data_3d_get_v3(data, res->base.min_index_y),
    minz         = br_data_3d_get_v3(data, res->min_index_z),
    maxx         = br_data_3d_get_v3(data, res->base.max_index_x),
    maxy         = br_data_3d_get_v3(data, res->base.max_index_y),
    maxz         = br_data_3d_get_v3(data, res->max_index_z);
  br_vec3_t m = BR_VEC3(minx.x, miny.y, minz.z);
  br_vec3_t M = BR_VEC3(maxx.x, maxy.y, maxz.z);
  br_vec3_t diff = br_vec3_sub(M, m);

  br_vec3_t rot_axis = br_vec3_normalize(br_vec3_cross(BR_VEC3(0, 0, 1.f), look_dir));
  float angle = br_vec3_angle(BR_VEC3(0, 0, 1.f), look_dir);
  br_vec3_t nA1 = br_vec3_abs(br_vec3_rot(BR_VEC3(diff.x, 0, 0), rot_axis, angle));
  br_vec3_t nA2 = br_vec3_abs(br_vec3_rot(BR_VEC3(0, diff.y, 0), rot_axis, angle));
  br_vec3_t nA3 = br_vec3_abs(br_vec3_rot(BR_VEC3(0, 0, diff.z), rot_axis, angle));
  br_vec3_t nA = BR_VEC3(max3(nA1.x, nA2.x, nA3.x), max3(nA1.y, nA2.y, nA3.y), 0);
  br_vec3_t curv = br_vec3_scale(res->curvature, 1.f/(float)res->base.len);
  br_vec3_t nC1 = br_vec3_abs(br_vec3_rot(BR_VEC3(curv.x, 0, 0), rot_axis, angle));
  br_vec3_t nC2 = br_vec3_abs(br_vec3_rot(BR_VEC3(0, curv.y, 0), rot_axis, angle));
  br_vec3_t nC3 = br_vec3_abs(br_vec3_rot(BR_VEC3(0, 0, curv.z), rot_axis, angle));
  br_vec3_t nC = br_vec3_add(nC1, br_vec3_add(nC2, nC3));
  return br_vec3_abs(br_vec3_mul(nC, nA)).xy;
}

