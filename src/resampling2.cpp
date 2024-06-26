#include "br_resampling2.h"
#include "br_pp.h"
#include "br_data.h"
#include "br_plot.h"
#include "br_smol_mesh.h"
#include "br_da.h"

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "raylib.h"
#define RAYMATH_STATIC_INLINE
#include "raymath.h"
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif
#include "tracy/Tracy.hpp"

#include <algorithm>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define RESAMPLING_NODE_MAX_LEN 128

struct resampling2_nodes_t {
  uint32_t index_start = 0, len = 0;
  size_t child1 = 0;
  size_t child2 = 0;
  uint32_t depth = 0;
  uint32_t min_index_x = 0, max_index_x = 0;
  uint32_t min_index_y = 0, max_index_y = 0;
};

template<typename T>
T my_max(T a, T b) {
  return std::max(a, b);
}
template<typename T>
T my_max(T a) {
  return a;
}
template<typename T, typename... Ts>
T my_max(T a, Ts... bs) {
  return std::max(a, my_max(bs...));
}
template<typename T>
T my_min(T a, T b) {
  return std::min(a, b);
}
template<typename T>
T my_min(T a) {
  return a;
}
template<typename T, typename... Ts>
T my_min(T a, Ts... bs) {
  return std::min(a, my_min(bs...));
}


struct resampling2_nodes_2d_t {
  resampling2_nodes_t base;

  bool is_inside(float const* xs, float const* ys, Rectangle rect) const {
    if (base.len == 0) return false;
    float minx = xs[base.min_index_x], miny = ys[base.min_index_y], maxx = xs[base.max_index_x], maxy = ys[base.max_index_y];
    return !((miny > rect.y) || (maxy < rect.y - rect.height) || (minx > rect.x + rect.width) || (maxx < rect.x));
  }

  bool is_inside_3d(float const* xs, float const* ys, Matrix mat) {
    if (base.len == 0) return false;
    Vector3 minx = Vector2TransformScale(CLITERAL(Vector2) { xs[base.min_index_x], ys[base.min_index_x] }, mat),
            miny = Vector2TransformScale(CLITERAL(Vector2) { xs[base.min_index_y], ys[base.min_index_y] }, mat),
            maxx = Vector2TransformScale(CLITERAL(Vector2) { xs[base.max_index_x], ys[base.max_index_x] }, mat),
            maxy = Vector2TransformScale(CLITERAL(Vector2) { xs[base.max_index_y], ys[base.max_index_y] }, mat);
    float mx = my_min(minx.x, miny.x, maxy.x, maxx.x);
    float Mx = my_max(minx.x, miny.x, maxy.x, maxx.x);
    float my = my_min(minx.y, miny.y, maxy.y, maxx.y);
    float My = my_max(minx.y, miny.y, maxy.y, maxx.y);
    float Mz = my_max(minx.z, miny.z, maxy.z, maxx.z);
    float quad_size = 2.1f;
    
    Rectangle rect = { quad_size / -2, quad_size / -2, quad_size, quad_size };
    return Mz > 0.f && CheckCollisionRecs(rect, Rectangle { mx, my, Mx - mx, My - my });
  }

  Vector2 get_ratios(float const* xs, float const* ys, float screen_width, float screen_height) const {
    float xr = xs[base.max_index_x] - xs[base.min_index_x], yr = ys[base.max_index_y] - ys[base.min_index_y];
    return {xr / screen_width, yr / screen_height};
  }

  Vector2 get_ratios_3d(float const* xs, float const* ys, Matrix mvp) const {
    Vector3 minx = Vector2TransformScale(CLITERAL(Vector2) { xs[base.min_index_x], ys[base.min_index_x] }, mvp),
            miny = Vector2TransformScale(CLITERAL(Vector2) { xs[base.min_index_y], ys[base.min_index_y] }, mvp),
            maxx = Vector2TransformScale(CLITERAL(Vector2) { xs[base.max_index_x], ys[base.max_index_x] }, mvp),
            maxy = Vector2TransformScale(CLITERAL(Vector2) { xs[base.max_index_y], ys[base.max_index_y] }, mvp);
    float mx = my_min(minx.x, miny.x, maxy.x, maxx.x);
    float my = my_min(minx.y, miny.y, maxy.y, maxx.y);
    float Mx = my_max(minx.x, miny.x, maxy.x, maxx.x);
    float My = my_max(minx.y, miny.y, maxy.y, maxx.y);
    return {(Mx - mx) / 2.f, (My - my) / 2.f};
  }
};


Vector3 br_data_3d_get_v3(br_data_3d_t const* data, uint32_t index) {
  return CLITERAL(Vector3) { data->xs[index], data->ys[index], data->zs[index] };
}

struct resampling2_nodes_3d_t {
  resampling2_nodes_t base;
  uint32_t min_index_z = 0, max_index_z = 0;

  bool is_inside(br_data_3d_t const* data, Matrix mvp) {
    Vector3 minx = Vector3TransformScale(br_data_3d_get_v3(data, base.min_index_x), mvp),
            miny = Vector3TransformScale(br_data_3d_get_v3(data, base.min_index_y), mvp),
            minz = Vector3TransformScale(br_data_3d_get_v3(data, min_index_z), mvp),
            maxx = Vector3TransformScale(br_data_3d_get_v3(data, base.max_index_x), mvp),
            maxy = Vector3TransformScale(br_data_3d_get_v3(data, base.max_index_y), mvp),
            maxz = Vector3TransformScale(br_data_3d_get_v3(data, max_index_z), mvp);
    float my = my_min(minx.y, miny.y, maxy.y, maxx.y, minz.y, maxz.y);
    float mx = my_min(minx.x, miny.x, maxy.x, maxx.x, minz.x, maxz.x);
    float My = my_max(minx.y, miny.y, maxy.y, maxx.y, minz.y, maxz.y);
    float Mx = my_max(minx.x, miny.x, maxy.x, maxx.x, minz.x, maxz.x);
    float Mz = my_max(minx.z, miny.z, maxy.z, maxx.z, minz.z, maxz.z);
    float quad_size = 2.1f;
    
    Rectangle rect = { quad_size / -2, quad_size / -2, quad_size, quad_size };
    return Mz > 0.f && CheckCollisionRecs(rect, Rectangle { mx, my, Mx - mx, My - my });
  }

  Vector2 get_ratios(br_data_3d_t const* data, Matrix mvp) const {
    Vector3 minx = Vector3TransformScale(br_data_3d_get_v3(data, base.min_index_x), mvp),
            miny = Vector3TransformScale(br_data_3d_get_v3(data, base.min_index_y), mvp),
            minz = Vector3TransformScale(br_data_3d_get_v3(data, min_index_z), mvp),
            maxx = Vector3TransformScale(br_data_3d_get_v3(data, base.max_index_x), mvp),
            maxy = Vector3TransformScale(br_data_3d_get_v3(data, base.max_index_y), mvp),
            maxz = Vector3TransformScale(br_data_3d_get_v3(data, max_index_z), mvp);
    float my = my_min(minx.y, miny.y, maxy.y, maxx.y, minz.y, maxz.y);
    float mx = my_min(minx.x, miny.x, maxy.x, maxx.x, minz.x, maxz.x);
    float My = my_max(minx.y, miny.y, maxy.y, maxx.y, minz.y, maxz.y);
    float Mx = my_max(minx.x, miny.x, maxy.x, maxx.x, minz.x, maxz.x);
    return {(Mx - mx) / 2.f, (My - my) / 2.f};
  }
};

template<typename arr_t>
struct resampling2_nodes_allocator_t {
  arr_t * arr = NULL;
  size_t len = 0, cap = 0;
};

using resampling2_nodes_2d_allocator_t = resampling2_nodes_allocator_t<resampling2_nodes_2d_t>;
using resampling2_nodes_3d_allocator_t = resampling2_nodes_allocator_t<resampling2_nodes_3d_t>;
typedef struct resampling2_t {
  br_data_kind_t kind;
  union {
    resampling2_nodes_2d_allocator_t dd;
    resampling2_nodes_3d_allocator_t ddd;
    resampling2_nodes_allocator_t<void> common;
  };
  double render_time = 0.0;
  float something = 0.02f;
  float something2 = 0.001f;
  uint32_t draw_count = 0;
} resampling2_t;

static uint32_t powers[32] = {0};
static uint32_t powers_base = 2;
void br_resampling2_construct(void) {
  powers[0] = 1;
  powers[1] = powers_base;
  for (int i = 2; i < 32; ++i) {
    powers[i] = powers[i - 1] * powers_base;
  }
}

static void resampling2_nodes_deinit(resampling2_t* nodes);
static bool resampling2_nodes_2d_push_point(resampling2_nodes_2d_allocator_t* nodes, size_t node_index, uint32_t index, float const* xs, float const* ys);
static bool resampling2_nodes_3d_push_point(resampling2_nodes_3d_allocator_t* nodes, size_t node_index, uint32_t index, float const* xs, float const* ys, float const* zs);

resampling2_t* resampling2_malloc(br_data_kind_t kind) {
  resampling2_t* r = (resampling2_t*)BR_CALLOC(1, sizeof(*r));
  if (r == NULL) return NULL;
  r->kind = kind;
  r->something = 0.02f;
  r->something2 = 0.001f;
  return r;
}

void resampling2_empty(resampling2_t* res) {
  if (nullptr == res) return;
  resampling2_nodes_deinit(res);
}

void resampling2_free(resampling2_t* r) {
  if (nullptr == r) return;
  resampling2_nodes_deinit(r);
  BR_FREE(r);
}

extern "C" void resampling2_add_point(resampling2_t* r, const br_data_t *pg, uint32_t index) {
  if (r->dd.len == 0) {
    switch (r->kind) {
      case br_data_kind_2d: { br_da_push((r->dd), resampling2_nodes_2d_t()); break; }
      case br_data_kind_3d: { br_da_push((r->ddd), resampling2_nodes_3d_t()); break; }
      default: BR_ASSERT(false);
    }
  }
  switch (r->kind) {
    case br_data_kind_2d: { resampling2_nodes_2d_push_point(&r->dd, 0, index, pg->dd.xs, pg->dd.ys); break; }
    case br_data_kind_3d: { resampling2_nodes_3d_push_point(&r->ddd, 0, index, pg->ddd.xs, pg->ddd.ys, pg->ddd.zs); break; }
    default: BR_ASSERT(false);
  }
}

static void resampling2_nodes_deinit(resampling2_t* nodes) {
  if (nodes == NULL) return;
  BR_FREE(nodes->common.arr);
  nodes->common.len = 0;
  nodes->common.cap = 0;
  nodes->common.arr = NULL;
}

static bool resampling2_nodes_2d_push_point(resampling2_nodes_2d_allocator_t* nodes, size_t node_index, uint32_t index, float const* xs, float const* ys) {
  resampling2_nodes_2d_t node = nodes->arr[node_index];
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
      if (false == resampling2_nodes_2d_push_point(nodes, node.base.child2, index, xs, ys)) {
        return false;
      }
    }
    bool split = (node_index == 0 && node.base.len == RESAMPLING_NODE_MAX_LEN * powers[node.base.depth]) ||
      (node_index != 0 && node.base.len > RESAMPLING_NODE_MAX_LEN * powers[node.base.depth]);
    if (split) {
      resampling2_nodes_2d_t left = node;
      resampling2_nodes_2d_t right = {};
      br_da_push(*nodes, left);
      node.base.child1 = nodes->len - 1;
      br_da_push(*nodes, right);
      node.base.child2 = nodes->len - 1;
      ++node.base.depth;
    }
  }
  nodes->arr[node_index] = node;
  return true;
}

static bool resampling2_nodes_3d_push_point(resampling2_nodes_3d_allocator_t* nodes, size_t node_index, uint32_t index,
    float const* xs, float const* ys, float const* zs) {
  resampling2_nodes_3d_t node = nodes->arr[node_index];
  ++node.base.len;
  if (node.base.len == 1) {
    node.base.index_start = 
    node.max_index_z = 
    node.min_index_z =
    node.base.max_index_y = 
    node.base.min_index_y =
    node.base.max_index_x = 
    node.base.min_index_x = index;
  } else {
    if (zs[index] < zs[node.min_index_z]) node.min_index_z = index;
    if (zs[index] > zs[node.max_index_z]) node.max_index_z = index;
    if (ys[index] < ys[node.base.min_index_y]) node.base.min_index_y = index;
    if (ys[index] > ys[node.base.max_index_y]) node.base.max_index_y = index;
    if (xs[index] < xs[node.base.min_index_x]) node.base.min_index_x = index;
    if (xs[index] > xs[node.base.max_index_x]) node.base.max_index_x = index;
    if (node.base.depth > 0) {
      if (false == resampling2_nodes_3d_push_point(nodes, node.base.child2, index, xs, ys, zs)) {
        return false;
      }
    }
    bool split = (node_index == 0 && node.base.len == RESAMPLING_NODE_MAX_LEN * powers[node.base.depth]) ||
      (node_index != 0 && node.base.len > RESAMPLING_NODE_MAX_LEN * powers[node.base.depth]);
    if (split) {
      resampling2_nodes_3d_t left = node;
      resampling2_nodes_3d_t right = {};
      br_da_push(*nodes, left);
      node.base.child1 = nodes->len - 1;
      br_da_push(*nodes, right);
      node.base.child2 = nodes->len - 1;
      ++node.base.depth;
    }
  }
  nodes->arr[node_index] = node;
  return true;
}


static void resampling2_draw(resampling2_nodes_2d_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  assert(plot->kind == br_plot_kind_2d);
  assert(pg->kind == br_data_kind_2d);
  ZoneScopedN("resampling2_2d");
  float const* xs = pg->dd.xs;
  float const* ys = pg->dd.ys;
  Rectangle rect = plot->dd.graph_rect;
  resampling2_nodes_2d_t node = nodes->arr[index];
  if (false == node.is_inside(xs, ys, rect)) return;
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) { // This is the leaf node
    smol_mesh_gen_line_strip2(plot->dd.line_shader, &xs[node.base.index_start], &ys[node.base.index_start], node.base.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  Vector2 ratios = node.get_ratios(xs, ys, rect.width, rect.height);
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
    std::sort(indexies, &indexies[6]);
    Vector2 pss[] = {
      {xs[indexies[0]], ys[indexies[0]]}, {xs[indexies[1]], ys[indexies[0]]},
      {xs[indexies[2]], ys[indexies[2]]}, {xs[indexies[3]], ys[indexies[3]]},
      {xs[indexies[4]], ys[indexies[4]]}, {xs[indexies[5]], ys[indexies[5]]},
    };
    //if (context.debug_bounds) smol_mesh_gen_bb(plot->dd.line_shader, bb_t{ ps[node.base.min_index_x].x, ps[node.base.min_index_y].y, ps[node.base.max_index_x].x, ps[node.base.max_index_y].y }, RAYWHITE);
    smol_mesh_gen_line_strip(plot->dd.line_shader, pss, 6, pg->color);
  } else {

    //if (context.debug_bounds) smol_mesh_gen_bb(plot->dd.line_shader, bb_t{ ps[node.base.min_index_x].x, ps[node.base.min_index_y].y, ps[node.base.max_index_x].x, ps[node.base.max_index_y].y }, RAYWHITE);
    resampling2_draw(nodes, node.base.child1, pg, plot);
    resampling2_draw(nodes, node.base.child2, pg, plot);
  }
}

static void resampling2_draw_3d(resampling2_nodes_2d_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  assert(plot->kind == br_plot_kind_3d);
  assert(pg->kind == br_data_kind_2d);
  ZoneScopedN("resampling2_3d");
  float const* xs = pg->dd.xs;
  float const* ys = pg->dd.ys;
  resampling2_nodes_2d_t node = nodes->arr[index];
  Matrix mvp = plot->ddd.line_shader->uvs.m_mvp_uv;
  if (false == node.is_inside_3d(xs, ys, mvp)) return;
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) { // This is the leaf node
    smol_mesh_3d_gen_line_strip3(plot->ddd.line_shader, &xs[node.base.index_start], &ys[node.base.index_start], node.base.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  Vector2 ratios = node.get_ratios_3d(xs, ys, mvp);
  float rmin = fminf(ratios.x, ratios.y);
  if (rmin < (node.base.depth == 1 ? 0.01f : 0.02f)) {
    size_t indexies[] = {
      node.base.index_start,
      node.base.min_index_x,
      node.base.min_index_y,
      node.base.max_index_x,
      node.base.max_index_y,
      node.base.index_start + node.base.len - (is_end ? 1 : 0)
    };
    std::sort(indexies, &indexies[6]);
    Vector2 pss[] = {
      { xs[indexies[0]], ys[indexies[0]] }, { xs[indexies[1]], ys[indexies[1]] },
      { xs[indexies[2]], ys[indexies[2]] }, { xs[indexies[3]], ys[indexies[3]] },
      { xs[indexies[4]], ys[indexies[4]] }, { xs[indexies[5]], ys[indexies[5]] },
    };
    smol_mesh_3d_gen_line_strip2(plot->ddd.line_shader, pss, 6, pg->color);
  } else {
    resampling2_draw_3d(nodes, node.base.child1, pg, plot);
    resampling2_draw_3d(nodes, node.base.child2, pg, plot);
  }
}

static void resampling2_3d_draw_3d(resampling2_nodes_3d_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot) {
  assert(plot->kind == br_plot_kind_3d);
  assert(pg->kind == br_data_kind_3d);
  ZoneScopedN("resampling2_3d");
  float const* xs = pg->ddd.xs;
  float const* ys = pg->ddd.ys;
  float const* zs = pg->ddd.zs;
  resampling2_nodes_3d_t node = nodes->arr[index];
  Matrix mvp = plot->ddd.line_shader->uvs.m_mvp_uv;
  if (false == node.is_inside(&pg->ddd, mvp)) return;
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) { // This is the leaf node
    size_t st = node.base.index_start;
    smol_mesh_3d_gen_line_strip1(plot->ddd.line_shader, &xs[st], &ys[st], &zs[st], node.base.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  Vector2 ratios = node.get_ratios(&pg->ddd, mvp);
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

    std::sort(indexies, &indexies[8]);
    size_t cur = indexies[0];
    for (size_t i = 1; i < 8; ++i) {
      if (cur == indexies[i]) continue;
      smol_mesh_3d_gen_line(plot->ddd.line_shader, CLITERAL(Vector3) { xs[cur], ys[cur], zs[cur] }, CLITERAL(Vector3) { xs[indexies[i]], ys[indexies[i]], zs[indexies[i]] }, pg->color);
      cur = indexies[i];
    }
  } else {
    resampling2_3d_draw_3d(nodes, node.base.child1, pg, plot);
    resampling2_3d_draw_3d(nodes, node.base.child2, pg, plot);
  }
}

void resampling2_draw(resampling2_t* res, br_data_t const* pg, br_plot_t* plot) {
  ZoneScopedN("resampline2_draw0");

  double start = GetTime();
  if (res->common.len == 0) return;
  switch (pg->kind) {
    case br_data_kind_2d: {
      switch (plot->kind) {
        case br_plot_kind_2d: resampling2_draw(&res->dd, 0, pg, plot); break;
        case br_plot_kind_3d: resampling2_draw_3d(&res->dd, 0, pg, plot); break;
      }
      break;
    }
    case br_data_kind_3d: {
      switch (plot->kind) {
        case br_plot_kind_2d: assert("Can't draw 3d data on 2d plot.." && 0);
        case br_plot_kind_3d: resampling2_3d_draw_3d(&res->ddd, 0, pg, plot); break;
      }
      break;
    }
    default: assert(0);
  }
  res->render_time = GetTime() - start;
  ++res->draw_count;
}

void resampling2_change_something(br_datas_t pg) {
  uint32_t draw_count = 0;
  for (size_t i = 0; i < pg.len; ++i) draw_count += pg.arr[i].resampling->draw_count;
  if (draw_count == 0) return;
  double target = 0.016 / (double)draw_count;
  for (size_t i = 0; i < pg.len; ++i) {
    if (pg.arr[i].resampling->draw_count == 0) continue;
    double delta = (pg.arr[i].resampling->render_time - target);
    double mul = 1.0 + 10.0f* (delta);

    pg.arr[i].resampling->something *= (float)mul;
    pg.arr[i].resampling->something2 *= (float)mul;
    if (pg.arr[i].resampling->something < 0.00001f) pg.arr[i].resampling->something = 0.0001f;
    if (pg.arr[i].resampling->something2 < 0.00001f) pg.arr[i].resampling->something2 = 0.0001f;
    pg.arr[i].resampling->draw_count = 0;
  }
}

double br_resampling2_get_draw_time(resampling2_t* res) {
  return res->render_time;
}

float br_resampling2_get_something(resampling2_t* res) {
  return res->something;
}

float br_resampling2_get_something2(resampling2_t* res) {
  return res->something2;
}

#ifndef _MSC_VER
#define PRINT_ALLOCS(prefix) \
  printf("\n%s ALLOCATIONS: %zu ( %zuKB ) | %lu (%zuKB)\n", prefix, \
      context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "misc/tests.h"
TEST_CASE(resampling) {
  float xs[] = { 0, 1, 2, 3};
  float ys[] = {  1, 2,  4, 2 };
  br_data_t pg;
  memset(&pg, 0, sizeof(pg));
  pg.cap = 4;
  pg.len = 4;
  pg.dd.xs = xs;
  pg.dd.ys = ys;
  pg.resampling = NULL;
  resampling2_t* r = resampling2_malloc(br_data_kind_2d);
  for (int i = 0; i < 2*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  for (int i = 0; i < 64*1024; ++i) resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}

#define N 2048
TEST_CASE(resampling2) {
  float xs[N];
  float ys[N];
  for (int i = 0; i < N; ++i) {
    xs[i] = sinf(3.14f / 4.f * (float)i);
    ys[i] = cosf(3.14f / 4.f * (float)i);
  }
  br_data_t pg;
  memset(&pg, 0, sizeof(pg));
  pg.cap = N;
  pg.len = N;
  pg.dd.xs = xs;
  pg.dd.ys = ys;
  pg.resampling = NULL;
  resampling2_t* r = resampling2_malloc(br_data_kind_2d);
  for (int i = 0; i < N; ++i) resampling2_add_point(r, &pg, (uint32_t)i);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_add_point(r, &pg, 3);
  printf("\nALLOCATIONS: %zu ( %zuKB ) | %zu (%zuKB)\n", context.alloc_count, context.alloc_size >> 10, context.alloc_total_count, context.alloc_total_size >> 10);
  resampling2_free(r);
}
#pragma GCC diagnostic pop
} // extern "C"
#endif
