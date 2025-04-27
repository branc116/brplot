#include "src/br_math.h"
#include "src/br_resampling2.h"
#include "src/br_pp.h"
#include "src/br_data.h"
#include "src/br_plot.h"
#include "src/br_smol_mesh.h"
#include "src/br_da.h"
#include "src/br_shaders.h"
#include "src/br_tl.h"

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#define RESAMPLING_NODE_MAX_LEN 64

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
    lc->args.screen_size
  );

  const float min_dist = br_context.cull_min;
  if (fabsf(d.x) + fabsf(d.y) < min_dist) {
    lc->mid = p;
    return;
  }

  smol_mesh_gen_line(lc->args, lc->old, p);
  lc->mid = lc->old = p;
}

void br_line_culler_push_line_strip(br_vec2_t const* points, size_t n, br_line_culler_t* lc, br_vec2_t plot_size) {
  for (size_t i = 0; i < n; ++i) {
    br_line_culler_push_point(lc, points[i], plot_size);
  }
}

void br_line_culler_end(br_line_culler_t* lc) {
  if (false == br_vec2_eq(lc->old, lc->mid)) {
    smol_mesh_gen_line(lc->args, lc->old, lc->mid);
  }
  lc->has_old = false;
}

void br_line_culler_push_line_strip2(float const* xs, float const* ys, size_t n, br_line_culler_t* lc, br_vec2_t plot_size) {
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

static inline float max4(float a, float b, float c, float d) {
  return fmaxf(fmaxf(a, b), fmaxf(c, d));
}

static inline float max6(float a, float b, float c, float d, float e, float f) {
  return fmaxf(max4(a, b, c, d), fmaxf(e, f));
}

bool resampling2_nodes_2d_is_inside(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, br_extent_t rect) {
  if (res->base.len == 0) return false;
  float minx = xs[res->base.min_index_x], miny = ys[res->base.min_index_y], maxx = xs[res->base.max_index_x], maxy = ys[res->base.max_index_y];
  return !((miny > rect.y) || (maxy < rect.y - rect.height) || (minx > rect.x + rect.width) || (maxx < rect.x));
}

bool resampling2_nodes_2d_is_inside_3d(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mat) {
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

br_vec2_t resampling2_nodes_2d_get_ratios(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, float screen_width, float screen_height) {
  float xr = xs[res->base.max_index_x] - xs[res->base.min_index_x], yr = ys[res->base.max_index_y] - ys[res->base.min_index_y];
  return BR_VEC2(xr / screen_width, yr / screen_height);
}

br_vec2_t resampling2_nodes_2d_get_ratios_3d(resampling2_nodes_2d_t const* res, float const* xs, float const* ys, br_mat_t mvp) {
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

br_vec3_t br_data_3d_get_v3(br_data_3d_t const* data, uint32_t index) {
  return BR_VEC3(data->xs[index], data->ys[index], data->zs[index]);
}

bool resampling2_nodes_3d_is_inside(resampling2_nodes_3d_t const* res, br_data_3d_t const* data, br_mat_t mvp) {
  br_vec3_t minx = br_vec3_transform_scale(br_data_3d_get_v3(data, res->base.min_index_x), mvp),
          miny = br_vec3_transform_scale(br_data_3d_get_v3(data, res->base.min_index_y), mvp),
          minz = br_vec3_transform_scale(br_data_3d_get_v3(data, res->min_index_z), mvp),
          maxx = br_vec3_transform_scale(br_data_3d_get_v3(data, res->base.max_index_x), mvp),
          maxy = br_vec3_transform_scale(br_data_3d_get_v3(data, res->base.max_index_y), mvp),
          maxz = br_vec3_transform_scale(br_data_3d_get_v3(data, res->max_index_z), mvp);
  float my = min6(minx.y, miny.y, maxy.y, maxx.y, minz.y, maxz.y);
  float mx = min6(minx.x, miny.x, maxy.x, maxx.x, minz.x, maxz.x);
  float My = max6(minx.y, miny.y, maxy.y, maxx.y, minz.y, maxz.y);
  float Mx = max6(minx.x, miny.x, maxy.x, maxx.x, minz.x, maxz.x);
  float Mz = max6(minx.z, miny.z, maxy.z, maxx.z, minz.z, maxz.z);
  float quad_size = 2.1f;
  
  br_extent_t rect = BR_EXTENT(quad_size / -2, quad_size / -2, quad_size, quad_size);
  return Mz > 0.f && br_col_extents(rect, BR_EXTENT(mx, my, Mx - mx, My - my));
}

br_vec2_t resampling2_nodes_3d_get_ratios(resampling2_nodes_3d_t const* res, br_data_3d_t const* data, br_vec3_t eye, br_vec3_t look_dir) {
  br_vec3_t minx = br_data_3d_get_v3(data, res->base.min_index_x),
          miny = br_data_3d_get_v3(data, res->base.min_index_y),
          minz = br_data_3d_get_v3(data, res->min_index_z),
          maxx = br_data_3d_get_v3(data, res->base.max_index_x),
          maxy = br_data_3d_get_v3(data, res->base.max_index_y),
          maxz = br_data_3d_get_v3(data, res->max_index_z);
  br_vec3_t M = BR_VEC3(maxx.x, maxy.y, maxz.z);
  br_vec3_t m = BR_VEC3(minx.x, miny.y, minz.z);
  float dist = br_vec3_dist2(minx, eye);
  float pot_dist = br_vec3_dist2(miny, eye); if (pot_dist < dist) dist = pot_dist;
        pot_dist = br_vec3_dist2(minz, eye); if (pot_dist < dist) dist = pot_dist;
        pot_dist = br_vec3_dist2(maxx, eye); if (pot_dist < dist) dist = pot_dist;
        pot_dist = br_vec3_dist2(maxy, eye); if (pot_dist < dist) dist = pot_dist;
        pot_dist = br_vec3_dist2(maxz, eye); if (pot_dist < dist) dist = pot_dist;
  //dist = sqrtf(dist);

  br_vec3_t rot_axis = br_vec3_normalize(br_vec3_cross(BR_VEC3(0, 0, 1.f), look_dir));
  float angle = br_vec3_angle(BR_VEC3(0, 0, 1.f), look_dir);
  br_vec3_t nA = br_vec3_rot(br_vec3_sub(M, m), rot_axis, angle);
  br_vec3_t nC = br_vec3_rot(res->curvature, rot_axis, angle);
  br_vec3_t out = br_vec3_scale(br_vec3_pow(br_vec3_mul(nA, nC), 1.0f), 1/dist);
  return BR_VEC2(fabsf(out.x), fabsf(out.y));
}

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
  if (NULL == res) return;
  resampling2_nodes_deinit(res);
}

void resampling2_free(resampling2_t* r) {
  if (NULL == r) return;
  resampling2_nodes_deinit(r);
  BR_FREE(r);
}

void resampling2_add_point(resampling2_t* r, const br_data_t *pg, uint32_t index) {
  if (r->dd.len == 0) {
    switch (r->kind) {
      case br_data_kind_2d: { br_da_push((r->dd), (resampling2_nodes_2d_t){0}); break; }
      case br_data_kind_3d: { br_da_push((r->ddd), (resampling2_nodes_3d_t){0}); break; }
      default: BR_ASSERT(false);
    }
  }
  switch (r->kind) {
    case br_data_kind_2d: { resampling2_nodes_2d_push_point(&r->dd, 0, index, pg->dd.xs, pg->dd.ys); break; }
    case br_data_kind_3d: { resampling2_nodes_3d_push_point(&r->ddd, 0, index, pg->ddd.xs, pg->ddd.ys, pg->ddd.zs); break; }
    default: BR_ASSERT(false);
  }
}

void resampling2_reset(resampling2_t* res) {
  res->common.len = 0;
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
      resampling2_nodes_2d_t right = {0};
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
      if (false == resampling2_nodes_3d_push_point(nodes, node.base.child2, index, xs, ys, zs)) {
        return false;
      }
    }
    bool split = (node_index == 0 && node.base.len == RESAMPLING_NODE_MAX_LEN * powers[node.base.depth]) ||
      (node_index != 0 && node.base.len > RESAMPLING_NODE_MAX_LEN * powers[node.base.depth]);
    if (split) {
      resampling2_nodes_3d_t left = node;
      resampling2_nodes_3d_t right = {0};
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

static int size_t_cmp(void const* a, void const* b) {
  long long const* ap = a, *bp = b;
  return (int)(*ap - *bp);
}

static void resampling2_draw22(resampling2_nodes_2d_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_extent_t plot_extent) {
  BR_ASSERT(pg->kind == br_data_kind_2d);
  float const* xs = pg->dd.xs;
  float const* ys = pg->dd.ys;
  br_vec2_t plot_size = plot_extent.size.vec;
  resampling2_nodes_2d_t node = nodes->arr[index];
  if (false == resampling2_nodes_2d_is_inside(&node, xs, ys, plot_extent)) {
    br_line_culler_end(&pg->resampling->culler);
    return;
  }
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) {
    // This is the leaf node
    br_line_culler_push_line_strip2(&xs[node.base.index_start], &ys[node.base.index_start], node.base.len + (is_end ? 0 : 1), &pg->resampling->culler, plot_size);
    return;
  }
  br_vec2_t ratios = resampling2_nodes_2d_get_ratios(&node, xs, ys, plot_extent.width, plot_extent.height);
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
    //if (br_context.debug_bounds) smol_mesh_gen_bb(plot->dd.line_shader, bb_t{ ps[node.base.min_index_x].x, ps[node.base.min_index_y].y, ps[node.base.max_index_x].x, ps[node.base.max_index_y].y }, RAYWHITE);
    br_line_culler_push_line_strip(pss, 6, &pg->resampling->culler, plot_size);
  } else {

    //if (br_context.debug_bounds) smol_mesh_gen_bb(plot->dd.line_shader, bb_t{ ps[node.base.min_index_x].x, ps[node.base.min_index_y].y, ps[node.base.max_index_x].x, ps[node.base.max_index_y].y }, RAYWHITE);
    resampling2_draw22(nodes, node.base.child1, pg, plot_extent);
    resampling2_draw22(nodes, node.base.child2, pg, plot_extent);
  }
}

static void resampling2_draw32(resampling2_nodes_2d_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot, br_shaders_t const* shaders) {
  assert(plot->kind == br_plot_kind_3d);
  assert(pg->kind == br_data_kind_2d);
  //ZoneScopedN("resampling2_3d");
  float const* xs = pg->dd.xs;
  float const* ys = pg->dd.ys;
  resampling2_nodes_2d_t node = nodes->arr[index];
  br_mat_t mvp = shaders->line_3d->uvs.m_mvp_uv;
  if (false == resampling2_nodes_2d_is_inside_3d(&node, xs, ys, mvp)) return;
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) { // This is the leaf node
    smol_mesh_3d_gen_line_strip3(shaders->line_3d, &xs[node.base.index_start], &ys[node.base.index_start], node.base.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  br_vec2_t ratios = resampling2_nodes_2d_get_ratios_3d(&node, xs, ys, mvp);
  float rmin = fminf(ratios.x, ratios.y);
  if (rmin < (node.base.depth == 1 ? pg->resampling->something : pg->resampling->something2 )) {
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
    smol_mesh_3d_gen_line_strip2(shaders->line_3d, pss, 6, pg->color);
  } else {
    resampling2_draw32(nodes, node.base.child1, pg, plot, shaders);
    resampling2_draw32(nodes, node.base.child2, pg, plot, shaders);
  }
}

static void resampling2_draw33(resampling2_nodes_3d_allocator_t const* const nodes, size_t index, br_data_t const* const pg, br_plot_t* const plot, br_shaders_t* shaders) {
  assert(plot->kind == br_plot_kind_3d);
  assert(pg->kind == br_data_kind_3d);
  //ZoneScopedN("resampling2_3d");
  float const* xs = pg->ddd.xs;
  float const* ys = pg->ddd.ys;
  float const* zs = pg->ddd.zs;
  resampling2_nodes_3d_t node = nodes->arr[index];
  br_mat_t mvp = shaders->line_3d->uvs.m_mvp_uv;
  br_vec3_t eye = plot->ddd.eye;
  br_vec3_t target = plot->ddd.target;
  if (false == resampling2_nodes_3d_is_inside(&node, &pg->ddd, mvp)) return;
  bool is_end = pg->len == node.base.index_start + node.base.len;
  if (node.base.depth == 0) { // This is the leaf node
    size_t st = node.base.index_start;
    smol_mesh_3d_gen_line_strip1(shaders->line_3d, &xs[st], &ys[st], &zs[st], node.base.len + (is_end ? 0 : 1), pg->color);
    return;
  }
  br_vec2_t ratios = resampling2_nodes_3d_get_ratios(&node, &pg->ddd, eye, br_vec3_sub(target, eye));
  assert(ratios.x > 0);
  assert(ratios.y > 0);
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
      smol_mesh_3d_gen_line(shaders->line_3d,  BR_VEC3(xs[cur], ys[cur], zs[cur]),  BR_VEC3(xs[indexies[i]], ys[indexies[i]], zs[indexies[i]]), pg->color);
      cur = indexies[i];
    }
  } else {
    resampling2_draw33(nodes, node.base.child1, pg, plot, shaders);
    resampling2_draw33(nodes, node.base.child2, pg, plot, shaders);
  }
}

void br_resampling2_draw(resampling2_t* res, br_data_t const* pg, br_plot_t* plot, br_plot_data_t const* pd) {
  //ZoneScopedN("resampline2_draw0");

  double start = brtl_time();
  if (res->common.len == 0) return;
  switch (pg->kind) {
    case br_data_kind_2d: {
      switch (plot->kind) {
        case br_plot_kind_2d: {
          res->culler.args.screen_size = BR_VEC2I_TOF(plot->cur_extent.size.vec);
          res->culler.args.zoom = plot->dd.zoom;
          res->culler.args.offset = plot->dd.offset;
          res->culler.args.offset.x -= (float)pg->dd.rebase_x;
          res->culler.args.offset.y -= (float)pg->dd.rebase_y;
          res->culler.args.line_thickness = plot->dd.line_thickness * pd->thickness_multiplyer;

          brtl_shaders()->line->uvs.color_uv = BR_COLOR_TO4(pg->color).xyz;
          br_extent_t ex = BR_EXTENTI_TOF(plot->cur_extent);
          float aspect = ex.width/ex.height;
          br_extent_t plot_rect = BR_EXTENT(
            -aspect*plot->dd.zoom.x/2.f + plot->dd.offset.x - (float)pg->dd.rebase_x,
                    plot->dd.zoom.y/2.f + plot->dd.offset.y - (float)pg->dd.rebase_y,
            aspect*plot->dd.zoom.x,
            plot->dd.zoom.y);

          resampling2_draw22(&res->dd, 0, pg, plot_rect);
          br_line_culler_end(&res->culler);
          br_shader_line_draw(brtl_shaders()->line);
        } break;
        case br_plot_kind_3d: {
          br_vec3_t target = plot->ddd.target;
          target.x -= (float)pg->dd.rebase_x;
          target.y -= (float)pg->dd.rebase_y;
          br_vec3_t eye = plot->ddd.eye;
          eye.x -= (float)pg->dd.rebase_x;
          eye.y -= (float)pg->dd.rebase_y;
          br_extent_t const ex = BR_EXTENTI_TOF(plot->cur_extent);
          br_vec2_t re = (br_vec2_t) { .x = ex.width, .y = ex.height };
          br_mat_t per = br_mat_perspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
          br_mat_t look = br_mat_look_at(eye, target, plot->ddd.up);
          brtl_shaders()->line_3d_simple->uvs.m_mvp_uv = brtl_shaders()->line_3d->uvs.m_mvp_uv = br_mat_mul(look, per);
          brtl_shaders()->line_3d->uvs.eye_uv = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
          brtl_shaders()->line_3d->uvs.color_uv = BR_COLOR_TO4(pg->color).xyz;

          resampling2_draw32(&res->dd, 0, pg, plot, brtl_shaders());
          br_shader_line_3d_draw(brtl_shaders()->line_3d);
        } break;
      }
      break;
    }
    case br_data_kind_3d: {
      switch (plot->kind) {
        case br_plot_kind_2d: assert("Can't draw 3d data on 2d plot.." && 0);
        case br_plot_kind_3d: {
          br_vec3_t target = plot->ddd.target;
          target.x -= (float)pg->ddd.rebase_x;
          target.y -= (float)pg->ddd.rebase_y;
          target.z -= (float)pg->ddd.rebase_z;
          br_vec3_t eye = plot->ddd.eye;
          eye.x -= (float)pg->ddd.rebase_x;
          eye.y -= (float)pg->ddd.rebase_y;
          eye.z -= (float)pg->ddd.rebase_z;
          br_extent_t const ex = BR_EXTENTI_TOF(plot->cur_extent);
          br_vec2_t re = (br_vec2_t) { .x = ex.width, .y = ex.height };
          br_mat_t per = br_mat_perspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
          br_mat_t look = br_mat_look_at(eye, target, plot->ddd.up);
          brtl_shaders()->line_3d_simple->uvs.m_mvp_uv = brtl_shaders()->line_3d->uvs.m_mvp_uv = br_mat_mul(look, per);
          brtl_shaders()->line_3d->uvs.eye_uv = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
          brtl_shaders()->line_3d->uvs.color_uv = BR_COLOR_TO4(pg->color).xyz;

          resampling2_draw33(&res->ddd, 0, pg, plot, brtl_shaders()); break;
          br_shader_line_3d_draw(brtl_shaders()->line_3d);
        }
      }
      break;
    }
    default: BR_ASSERT(0);
  }
  res->render_time = brtl_time() - start;
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
    float mins = br_context.min_sampling;
    if (pg.arr[i].resampling->something < mins) pg.arr[i].resampling->something = mins;
    if (pg.arr[i].resampling->something2 < mins) pg.arr[i].resampling->something2 = mins;
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

#if defined(BR_UNIT_TEST)
#define PRINT_ALLOCS(prefix) \
  printf("\n%s ALLOCATIONS: %zu ( %zuKB ) | %lu (%zuKB)\n", prefix, \
      br_context.alloc_count, br_context.alloc_size >> 10, br_context.alloc_total_count, br_context.alloc_total_size >> 10);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "external/tests.h"
TEST_CASE(resampling) {
  float xs[] = { 0, 1, 2, 3 };
  float ys[] = { 1, 2, 4, 2 };
  br_data_t pg;
  memset(&pg, 0, sizeof(pg));
  pg.cap = 4;
  pg.len = 4;
  pg.dd.xs = xs;
  pg.dd.ys = ys;
  pg.resampling = NULL;
  resampling2_t* r = resampling2_malloc(br_data_kind_2d);
  for (int i = 0; i < 2*1024; ++i) resampling2_add_point(r, &pg, 3);
  resampling2_add_point(r, &pg, 3);
  for (int i = 0; i < 64*1024; ++i) resampling2_add_point(r, &pg, 3);
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
  resampling2_add_point(r, &pg, 3);
  resampling2_free(r);
}
#pragma GCC diagnostic pop
#endif
