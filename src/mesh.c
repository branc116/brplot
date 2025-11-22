#include "src/br_shaders.h"
#include "src/br_mesh.h"
#include "src/br_plot.h"
#include "src/br_math.h"
#include "src/br_theme.h"

static BR_THREAD_LOCAL struct {
  br_shaders_t* shaders;
  bool* debug;
  br_theme_t* theme;
} br_mesh_state;

void br_mesh_construct(br_shaders_t* shaders, bool* debug, br_theme_t* theme) {
  br_mesh_state.shaders = shaders;
  br_mesh_state.debug = debug;
  br_mesh_state.theme = theme;
}

void br_mesh_gen_bb(br_mesh_line_t args, br_bb_t bb) {
  float xmi = bb.min_x, ymi = bb.min_y , xma = bb.max_x, yma = bb.max_y;
  br_vec2_t v[5] = {
    BR_VEC2(xmi, ymi),
    BR_VEC2(xma, ymi),
    BR_VEC2(xma, yma),
    BR_VEC2(xmi, yma),
    BR_VEC2(xmi, ymi),
  };

  br_mesh_gen_line_strip(args, v, 5);
}

void br_mesh_gen_point(br_mesh_line_t args, br_vec2_t point) {
  br_vec2d_t size = BR_VEC2D(args.zoom.x * .01f, args.zoom.y * .01f);
  br_mesh_gen_bb(args, (br_bb_t) {
      .min_x = (float)(point.x - size.x / 2),
      .min_y = (float)(point.y - size.y / 2),
      .max_x = (float)(point.x + size.x / 2),
      .max_y = (float)(point.y + size.y / 2),
  });
}

void br_mesh_gen_point1(br_mesh_line_t args, br_vec2_t point, br_vec2_t size) {
  br_mesh_gen_bb(args, (br_bb_t) {
      .min_x = point.x - size.x / 2,
      .min_y = point.y - size.y / 2,
      .max_x = point.x + size.x / 2,
      .max_y = point.y + size.y / 2,
  });
}

void br_mesh_gen_line(br_mesh_line_t* args, br_vec2_t startPos, br_vec2_t endPos) {
  br_vec2_t const delta = br_vec2_sub(endPos, startPos);

  br_vec2d_t strip[2] = {
    BR_VEC2D(startPos.x, startPos.y),
    BR_VEC2D(endPos.x, endPos.y),
  };

  float thick = args->line_thickness;
  br_vec2d_t zoom = args->zoom;
  br_vec2d_t screen = args->screen_size;
  br_vec2d_t offset = args->offset;

  br_vec2d_t normal = br_vec2d_normalize(br_vec2d_mul(BR_VEC2D(delta.y, -delta.x), zoom));
  br_vec2d_t dif = br_vec2d_scale(normal, thick);
  double eps = DBL_EPSILON*10e8;
  br_vec2d_t dif0 = br_vec2d_max(br_vec2d_scale(zoom, 0.1), br_vec2d_scale(strip[0], eps));
  br_vec2d_t dif1 = br_vec2d_max(br_vec2d_scale(zoom, 0.1), br_vec2d_scale(strip[1], eps));
  br_vec2d_t poss[4] = {
    br_vec2d_add(strip[0], br_vec2d_mul(dif, dif0)),
    br_vec2d_add(strip[0], br_vec2d_mul(br_vec2d_scale(dif, -1.0), dif0)),
    br_vec2d_add(strip[1], br_vec2d_mul(br_vec2d_scale(dif, -1.0), dif1)),
    br_vec2d_add(strip[1], br_vec2d_mul(dif, dif1)),
  };
  br_vec2d_t fact = br_vec2d_div(screen, BR_VEC2D(screen.y, screen.y));
  fact = br_vec2d_mul(fact, zoom);
  for (int i = 0; i < 4; ++i) {
    poss[i] = br_vec2d_sub(poss[i], offset);
    poss[i] = br_vec2d_div(poss[i], fact);
    poss[i] = br_vec2d_scale(poss[i], 2.f);
  }

  br_vec2_t cur[2] = {BR_VEC2((float)poss[0].x, (float)poss[0].y), BR_VEC2((float)poss[1].x, (float)poss[1].y) };
  br_vec2_t mid = br_vec2_add(br_vec2_scale(cur[0], 0.5f), br_vec2_scale(cur[1], 0.5f));

  br_shader_line_push_quad(br_mesh_state.shaders->line, (br_shader_line_el_t[4]) {
      { .pos_delta = BR_VEC4((float)poss[0].x, (float)poss[0].y, 0.98f,  1.f) },
      { .pos_delta = BR_VEC4((float)poss[1].x, (float)poss[1].y, 0.98f, -1.f) },
      { .pos_delta = BR_VEC4((float)poss[2].x, (float)poss[2].y, 0.98f, -1.f) },
      { .pos_delta = BR_VEC4((float)poss[3].x, (float)poss[3].y, 0.98f,  1.f) },
  });

  if (args->prev[0].x != 0.f || args->prev[1].x != 0.f || args->prev[0].y != 0.f || args->prev[1].y != 0.f) {
    if (false == br_vec2_ccv(mid, args->prev[1], cur[0])) {
      br_shader_line_push_tri(br_mesh_state.shaders->line, (br_shader_line_el_t[3]) {
          { .pos_delta = BR_VEC4(mid.x, mid.y, 0.98f,  0.f) },
          { .pos_delta = BR_VEC4(args->prev[1].x, args->prev[1].y, 0.98f, 1.f) },
          { .pos_delta = BR_VEC4(cur[0].x, cur[0].y, 0.98f, 1.f) },
      });
    }
    if (true == br_vec2_ccv(mid, args->prev[0], cur[1])) {
      br_shader_line_push_tri(br_mesh_state.shaders->line, (br_shader_line_el_t[3]) {
          { .pos_delta = BR_VEC4(mid.x, mid.y, 0.98f,  0.f) },
          { .pos_delta = BR_VEC4(cur[1].x, cur[1].y, 0.98f, 1.f) },
          { .pos_delta = BR_VEC4(args->prev[0].x, args->prev[0].y, 0.98f, 1.f) },
      });
    }
  }

  args->prev[0] = BR_VEC2((float)poss[2].x, (float)poss[2].y);
  args->prev[1] = BR_VEC2((float)poss[3].x, (float)poss[3].y);

  if (*br_mesh_state.debug) {
    *br_mesh_state.debug = false;
    br_mesh_gen_point(*args, startPos);
    br_mesh_gen_point(*args, endPos);
    *br_mesh_state.debug = true;
  }
}

void br_mesh_gen_line_strip(br_mesh_line_t args, br_vec2_t const * points, size_t len) {
  for (size_t v = 0; v < (len - 1); ++v) br_mesh_gen_line(&args, points[v], points[v + 1]);
}

void br_mesh_gen_line_strip2(br_mesh_line_t args, float const* xs, float const* ys, size_t len) {
  for (size_t v = 0; v < (len - 1); ++v)
    br_mesh_gen_line(&args, BR_VEC2(xs[v], ys[v]), BR_VEC2(xs[v+1], ys[v+1]));
}


void br_mesh_3d_gen_line_simple(br_shader_line_3d_simple_t* shader, br_vec3_t p1, br_vec3_t p2, br_color_t color, br_vec3_t eye) {
  // TODO: This actually looks quite good. Maybe use this in the future
  (void)color;
  br_vec3_t diff = br_vec3_sub(p2, p1);
  float dist1 = 0.01f * br_vec3_dist(eye, p1);
  float dist2 = 0.01f * br_vec3_dist(eye, p2);
  dist1 = dist2 = (dist1 + dist2) * .5f;
  br_vec3_t right1 = br_vec3_normalize(br_vec3_cross(br_vec3_sub(eye, p1), diff));
  br_vec3_t right2 = br_vec3_normalize(br_vec3_cross(br_vec3_sub(eye, p2), diff));
  br_vec3_t bl = br_vec3_scale(right1, -dist1);
  br_vec3_t br = br_vec3_scale(right1, dist1);
  br_vec3_t tl = br_vec3_scale(right2, -dist1);
  br_vec3_t tr = br_vec3_scale(right2, dist1);
  br_shader_line_3d_simple_push_quad(shader, (br_shader_line_3d_simple_el_t[4]) {
    { .vertexPosition = br_vec3_add(p1, bl), .vertexNormal = bl },
    { .vertexPosition = br_vec3_add(p1, br), .vertexNormal = br },
    { .vertexPosition = br_vec3_add(p2, tr), .vertexNormal = br },
    { .vertexPosition = br_vec3_add(p2, tl), .vertexNormal = tl },
  });
}

void br_mesh_3d_gen_line(const br_mesh_line_3d_t* args, br_vec3_t p1, br_vec3_t p2) {
  br_shader_line_3d_t* ls = br_mesh_state.shaders->line_3d;
  float const line_3d_size = args->line_thickness;
  br_vec3_t diff  = br_vec3_normalize(br_vec3_sub(p2, p1));
  br_vec3_t norm = br_vec3_normalize(br_vec3_perpendicular(diff));
  float dist1 = 0.1f * br_vec3_dist(ls->uvs.eye_uv, p1);
  float dist2 = 0.1f * br_vec3_dist(ls->uvs.eye_uv, p2);
  if (false == br_vec3_ccv(p1, norm, p2)) norm = br_vec3_scale(norm, -1.f);
#define BR_MESH_3D_N 4
  static float angle = 3.14159265f * 2 / (float)BR_MESH_3D_N;
  float angle_sin = sinf(angle);
  float angle_cos = cosf(angle);
  for (int k = 0; k <= BR_MESH_3D_N; ++k) {
    br_vec3_t next = br_vec3_rot2(norm, diff, angle_sin, angle_cos);
    br_shader_line_3d_push_quad(ls, (br_shader_line_3d_el_t[4]) {
        { .vertexPosition = br_vec3_add(p1, br_vec3_scale(norm, line_3d_size*dist1)), .vertexNormal = norm },
        { .vertexPosition = br_vec3_add(p1, br_vec3_scale(next, line_3d_size*dist1)), .vertexNormal = next },
        { .vertexPosition = br_vec3_add(p2, br_vec3_scale(next, line_3d_size*dist2)), .vertexNormal = next },
        { .vertexPosition = br_vec3_add(p2, br_vec3_scale(norm, line_3d_size*dist2)), .vertexNormal = norm }
    });
    norm = next;
  }
}

void br_mesh_3d_gen_line_strip(br_mesh_line_3d_t args, br_vec3_t const* ps, size_t len) {
  for (size_t i = 0; i < len - 1; ++i) br_mesh_3d_gen_line(&args, ps[i], ps[i + 1]);
}

void br_mesh_3d_gen_line_strip1(br_mesh_line_3d_t args, float const* xs, float const* ys, float const* zs, size_t len) {
  for (size_t i = 0; i < len - 1; ++i) br_mesh_3d_gen_line(&args, BR_VEC3(xs[i], ys[i], zs[i]), BR_VEC3(xs[i + 1], ys[i + 1], zs[i + 1]));
}

void br_mesh_3d_gen_line_strip2(br_mesh_line_3d_t args, br_vec2_t const* ps, size_t len) {
  for (size_t i = 0; i < len - 1; ++i) br_mesh_3d_gen_line(&args,
      BR_VEC3(ps[i].x, ps[i].y, 0),
      BR_VEC3(ps[i+1].x, ps[i+1].y, 0));
}

void br_mesh_3d_gen_line_strip3(br_mesh_line_3d_t args, float const* xs, float const* ys, size_t len) {
  for (size_t i = 0; i < len - 1; ++i) br_mesh_3d_gen_line(&args,
      BR_VEC3(xs[i], ys[i], 0),
      BR_VEC3(xs[i+1], ys[i+1], 0));
}

br_extent_t brui_resizable_cur_extent(int resizable_handle);

typedef struct {
  float l, r;
  float thick;
} br_mesh_line_thick;
int get_points(int n, br_mesh_line_thick* arr, double to, double from, double extent, double line_thickness) {
  double diff = fabs(to - from);
  double real_exp = log10(diff / 2.f);
  double exp = floor(real_exp);
  double base = pow(10.0, exp);
  double start = floor(from / base) * base;
  n = br_i_min(diff / base + 3, n);
  double thickness_base = 2.0 - n / 20.0;
  if (thickness_base < 1.0) thickness_base = 1.0;
  for (int i = 0; i < n; ++i) {
    double cur = start + i*base;
    double thick = thickness_base;
    double higher = fabs(cur / (base * 10));
    if (fabs(higher - round(higher)) < 0.01) thick = 5.0;

    double from_start = cur - from;
    double uv = 2.0 * from_start / diff - 1.0;
    double thick_real = 14.0/extent * line_thickness;
    double l = uv - thick_real;
    double r = uv + thick_real;
    arr[i] = (br_mesh_line_thick) { .l = (float)l, .r = (float)r, .thick = (float)thick };
  }
  return n;
}

br_vec3_t get_zero_z(br_vec3_t p1, br_vec4_t p2) {
  p2 = br_vec4_scale(p2, 1.f/p2.w);
  br_vec3_t k = br_vec3_sub(p1, p2.xyz);
  br_vec3_t d = br_vec3_sub(p1, k);
  float t0 = -d.z/k.z;
  br_vec3_t zeroz = br_vec3_add(br_vec3_scale(k, t0), d);
  return zeroz;
}

static void br_mesh_grid_3d_line_draw2(br_vec3_t p1, br_vec3_t p2, br_vec3_t eye, br_vec3_t color, float far_plane, br_vec3_t normal) {
  br_vec3_t diff = br_vec3_sub(p2, p1);
  float dist1 = br_vec3_dist(eye, p1);
  float dist2 = br_vec3_dist(eye, p2);
  float thick1 =  dist1 * 0.002f;
  float thick2 =  dist2 * 0.002f;
  br_vec3_t r1 = br_vec3_normalize(br_vec3_cross(br_vec3_sub(eye, p1), diff));
  br_vec3_t r2 = br_vec3_normalize(br_vec3_cross(br_vec3_sub(eye, p2), diff));
  br_vec3_t bl = br_vec3_add(br_vec3_scale(r1, -thick1), p1);
  br_vec3_t br = br_vec3_add(br_vec3_scale(r1, thick1), p1);
  br_vec3_t tr = br_vec3_add(br_vec3_scale(r2, thick2), p2);
  br_vec3_t tl = br_vec3_add(br_vec3_scale(r2, -thick2), p2);
  br_shader_grid_3d_push_quad(br_mesh_state.shaders->grid_3d, (br_shader_grid_3d_el_t[4]) {
      { .pos = BR_VEC4_31(bl, -1), .normal = normal, .color = color },
      { .pos = BR_VEC4_31(br,  1), .normal = normal, .color = color },
      { .pos = BR_VEC4_31(tr,  1), .normal = normal, .color = color },
      { .pos = BR_VEC4_31(tl, -1), .normal = normal, .color = color },
  });
}

static void br_mesh_grid_3d_line_draw(br_vec3_t p1, br_vec3_t p2, br_vec3_t eye, br_vec3_t color, float far_plane, br_vec3_t normal) {
  br_vec3_t diff = br_vec3_sub(p2, p1);
  float diff_sq = br_vec3_dot(diff, diff);
  if (diff_sq < 1e-6f) return;  // Degenerate line
  br_vec3_t to_eye_p1 = br_vec3_sub(eye, p1);
  float t = br_vec3_dot(to_eye_p1, diff) / diff_sq;
  t = fmaxf(0.0f, fminf(1.0f, t));
  br_vec3_t closest = br_vec3_add(p1, br_vec3_scale(diff, t));

  br_mesh_grid_3d_line_draw2(p1, closest, eye, color, far_plane, normal);
  br_mesh_grid_3d_line_draw2(closest, p2, eye, color, far_plane, normal);
}

static void br_mesh_grid_3d_draw(br_vec3_t normal, br_vec3_t up, br_size_t size, br_vec3_t eye, br_vec3_t origin, br_vec2_t base, br_vec3_t color) {
  br_vec3_t right = br_vec3_normalize(br_vec3_cross(normal, up));
  br_vec3_t start = origin;
  br_vec2_t n = br_vec2_div(size.vec, base);
  start = br_vec3_sub(start, br_vec3_scale(up, size.height/2));
  start = br_vec3_sub(start, br_vec3_scale(right, size.width/2));
  for (int i = 1; i < (int)n.x; ++i) {
    br_vec3_t p1 = br_vec3_add(start, br_vec3_scale(right, i*base.x));
    br_vec3_t p2 = br_vec3_add(p1, br_vec3_scale(up, size.height));
    br_mesh_grid_3d_line_draw(p1, p2, eye, color, 1000.f, normal);
  }
  for (int i = 1; i < (int)n.y; ++i) {
    br_vec3_t p1 = br_vec3_add(start, br_vec3_scale(up, i*base.y));
    br_vec3_t p2 = br_vec3_add(p1, br_vec3_scale(right, size.width));
    br_mesh_grid_3d_line_draw(p1, p2, eye, color, 1000.f, normal);
  }
}

// TODO: This should be split to _gen and _draw
void br_mesh_grid_draw(br_plot_t* plot, br_theme_t* theme) {
  // TODO 2D/3D
#define BR_MESH_LINE_THICK_N 32
  br_mesh_line_thick line_thicks[BR_MESH_LINE_THICK_N];
  br_mesh_line_thick line_thicks2[BR_MESH_LINE_THICK_N];
  br_extent_t ex = brui_resizable_cur_extent(plot->extent_handle);
  switch (plot->kind) {
    case br_plot_kind_2d: {
      BR_PROFILE("grid_draw_2d") {
        br_mesh_state.shaders->grid->uvs.bg_color_uv = BR_COLOR_TO4(theme->colors.plot_bg);
        br_mesh_state.shaders->grid->uvs.lines_color_uv = BR_COLOR_TO4(theme->colors.grid_lines);
        br_vec2d_t from = br_plot2d_to_plot(plot, BR_VEC2(ex.x, ex.y + ex.height), ex);
        br_vec2d_t to = br_plot2d_to_plot(plot, BR_VEC2(ex.x + ex.width, ex.y), ex);
        {
          int n = get_points(BR_MESH_LINE_THICK_N, line_thicks, to.x, from.x, ex.width, plot->dd.grid_line_thickness);
          for (int i = 0; i < n; ++i) {
            float thick = line_thicks[i].thick;
            float l = line_thicks[i].l;
            float r = line_thicks[i].r;
            br_shader_grid_push_quad(br_mesh_state.shaders->grid, (br_shader_grid_el_t[4]) {
                { .vert = BR_VEC3(l,  1.f, -1.f/thick) },
                { .vert = BR_VEC3(r,  1.f,  1.f/thick) },
                { .vert = BR_VEC3(r, -1.f,  1.f/thick) },
                { .vert = BR_VEC3(l, -1.f, -1.f/thick) },
            });
          }
        }
        {
          int n = get_points(BR_MESH_LINE_THICK_N, line_thicks, to.y, from.y, ex.height, plot->dd.grid_line_thickness);
          for (int i = 0; i < n; ++i) {
            float thick = line_thicks[i].thick;
            float d = line_thicks[i].l;
            float u = line_thicks[i].r;
            br_shader_grid_push_quad(br_mesh_state.shaders->grid, (br_shader_grid_el_t[4]) {
                { .vert = BR_VEC3(-1.f, u, -1.f/thick) },
                { .vert = BR_VEC3( 1.f, u, -1.f/thick) },
                { .vert = BR_VEC3( 1.f, d,  1.f/thick) },
                { .vert = BR_VEC3(-1.f, d,  1.f/thick) },
            });
          }
        }
      }
    } break;
    case br_plot_kind_3d: {
      BR_PROFILE("grid_draw_3d") {
        br_vec2_t re = (br_vec2_t) { .x = ex.width, .y = ex.height };
        br_mat_t per = br_mat_perspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
        br_mat_t look = br_mat_look_at(plot->ddd.eye, plot->ddd.target, plot->ddd.up);
        br_mat_t mvp = br_mat_mul(look, per);
        br_mesh_state.shaders->grid_3d->uvs.m_mvp_uv = mvp;
        br_mesh_state.shaders->grid_3d->uvs.bg_color_uv = BR_COLOR_TO4(br_mesh_state.theme->colors.plot_bg).xyz;
        br_mesh_state.shaders->grid_3d->uvs.eye_uv = plot->ddd.eye;
        br_mesh_state.shaders->grid_3d->uvs.target_uv = plot->ddd.target;
        br_vec3_t color = BR_COLOR_TO4(br_mesh_state.theme->colors.grid_lines).xyz;
        static float angle = 0;
        //angle += 0.01;
        br_mesh_grid_3d_draw(br_vec3_rot(BR_VEC3(0, 0, 1), BR_VEC3(1, 0, 0), angle), br_vec3_rot(BR_VEC3(0, 1, 0), BR_VEC3(1, 0, 0), angle), BR_SIZE(100, 100), plot->ddd.eye, BR_VEC3(0, 0, 0), BR_VEC2(10, 10), color);
        br_mesh_grid_3d_draw(br_vec3_rot(BR_VEC3(0, 1, 0), BR_VEC3(1, 0, 0), angle), br_vec3_rot(BR_VEC3(0, 0, 1), BR_VEC3(1, 0, 0), angle), BR_SIZE(100, 100), plot->ddd.eye, BR_VEC3(0, 0, 0), BR_VEC2(10, 10), color);
        br_mesh_grid_3d_draw(br_vec3_rot(BR_VEC3(1, 0, 0), BR_VEC3(1, 0, 0), angle), br_vec3_rot(BR_VEC3(0, 1, 0), BR_VEC3(1, 0, 0), angle), BR_SIZE(100, 100), plot->ddd.eye, BR_VEC3(0, 0, 0), BR_VEC2(10, 10), color);
      }
    } break;
    default: BR_UNREACHABLE("plot kind: %d", plot->kind);
  }
}

