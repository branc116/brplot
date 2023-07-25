#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "plotter.h"

#define MIN(x, y) (x) < (y) ? (x) : (y)
#define MAX(x, y) (x) > (y) ? (x) : (y)

typedef void (*quad_tree_visitor)(quad_tree_t*, void*);


static void quad_tree_extend_rect(Rectangle* rect, Vector2 point);
static void quad_tree_split(quad_tree_t* rect);
static void _quad_tree_draw_lines(quad_tree_t* qt);
//TODO: Point need not be passed as functon argument 
static int _quad_tree_add_point(quad_tree_t* root, size_t index);
static void quad_tree_draw_rects(quad_tree_t* qt);
static Vector2 quad_tree_mid_point(quad_tree_t const * qt, size_t n);
static void quad_tree_copy(quad_tree_t* dest, quad_tree_t const* src);
static void quad_tree_draw_rects_for_point(quad_tree_t* qt, Vector2 point);
static bool should_balance(quad_tree_t* t);
static size_t quad_tree_sort_groups(quad_tree_groups_t* groups, size_t len);
static void quad_tree_balance(quad_tree_t* dest, quad_tree_t const * qt, int depth);
static void quad_tree_print_info(quad_tree_t* qt, Vector2 x, int depth);
static int _quad_tree_print_dot(quad_tree_t* t, int* n);
static void quad_tree_check(quad_tree_t* rect);
static void quad_tree_visit(quad_tree_t* root, quad_tree_visitor enter, quad_tree_visitor exit, void* data);
static void help_rolling_average(Vector2* old_avg, Vector2 new_point, size_t new_count);
static Vector2 help_get_tangent(size_t index);

// A lot of functions in here are recursive, so passing a lot of stuff by arguments may not be optimal.
static int* _c;
static Rectangle _screen;
static Vector2 const * _all_points;
static size_t _all_points_count;
static smol_mesh_t* _line_mesh;
static smol_mesh_t* _quad_mesh;
static Color _color;
static int _debug;
static quad_tree_root_t* _root;

static bool balancing = false;

int quad_tree_draw(quad_tree_root_t* qt, Color color, Rectangle screen, smol_mesh_t* line_mesh, smol_mesh_t* quad_mesh, Vector2* all_points, size_t all_points_count, int debug) {
  int c = 0;
  _root = qt;
  _c = &c;
  _screen = screen;
  _screen.y -= _screen.height; // Transform `y positive down` to `y positive up`
  _all_points = all_points;
  _all_points_count = all_points_count;
  _line_mesh = line_mesh;
  _quad_mesh = quad_mesh;
  _color = color;
  _debug = debug;
  _quad_tree_draw_lines(qt->root);
  if (debug == 1 || debug == 2) quad_tree_draw_rects(qt->root);
  if (debug == 3 || debug == 4) quad_tree_draw_rects_for_point(qt->root, graph_mouse_position);
  if (debug && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    quad_tree_print_info(qt->root, graph_mouse_position, 0);
  }
  // Draw temp points
  for (size_t i = 0; i < qt->temp_length; ++i) {
    size_t index = qt->temp[i];
    size_t len = 0;
    if (index > 0) { --index; ++len; }
    if (index < all_points_count) ++len;
    smol_mesh_gen_line_strip(line_mesh, &all_points[index], len, color);
  }
  return c;
}

void quad_tree_print_dot(quad_tree_root_t* t) {
  int id = 0;
  printf("digraph L {\n");
  printf("node [shape=record fontname=Arial];\n");
  _quad_tree_print_dot(t->root, &id);
  printf("}\n");
  return;
}

quad_tree_root_t* quad_tree_malloc(void) {
  quad_tree_root_t* ret = malloc(sizeof(quad_tree_root_t));
  ret->temp_length = 0;
  ret->temp_mid_point = (Vector2){0};
  ret->root = NULL;
  return ret;
}

static void _quad_tree_free(quad_tree_t* qt) {
  if (qt->is_leaf) {
    free(qt->groups);
    return;
  }
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    _quad_tree_free(&qt->children[i]);
  }
  free(qt->children);
}

void quad_tree_free(quad_tree_root_t* qt) {
  _quad_tree_free(qt->root);
  free(qt->root);
  free(qt);
}

static quad_tree_t* quad_tree_malloc_children() {
  quad_tree_t* rect = malloc(QUAD_TREE_DIR_COUNT * sizeof(quad_tree_t));
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    rect[i].is_leaf = true;
    rect[i].count = 0;
    rect[i].groups_len = 0;
    rect[i].groups_cap = 0;
    rect[i].groups = NULL;
    rect[i].bounds = (Rectangle){0};
  }
  return rect;
}

static quad_tree_t* quad_tree_node_malloc(Vector2 split_point) {
    quad_tree_t* nr = malloc(sizeof(quad_tree_t));
    nr->is_leaf = false;
    nr->split_point = split_point;
    nr->children = quad_tree_malloc_children();
    nr->groups_len = 0;
    nr->groups_cap = 0;
    nr->mid_point = (Vector2){0};
    nr->count = 0;
    nr->tangent = (Vector2){0};
    nr->bounds = (Rectangle){0};
    return nr;
}

static void _quad_tree_root_womit(quad_tree_root_t* root) {
  for (size_t i = 0; i < root->temp_length; ++i) {
    _quad_tree_add_point(root->root, root->temp[i]);
  }
  root->temp_length = 0;
}

static void quad_tree_change_root(quad_tree_root_t* root) {
  Rectangle b = root->root->bounds;
  Vector2 p = root->temp_mid_point;
  Vector2 sp = {0};
  quad_tree_dir dir = 0;
  float down = p.y - b.y;
  float up = b.y + b.height - p.y;
  float left = p.x - b.x;
  float right = b.x + b.width - p.x;
  if (up < down) {
    dir = QUAD_TREE_DIR_DOWN;
    sp.y = nextafterf(b.y + 2*b.height, INFINITY);
  } else {
    dir = QUAD_TREE_DIR_UP;
    sp.y = nextafterf(b.y - b.height, -INFINITY);
  }
  if (right < left) {
    dir |= QUAD_TREE_DIR_LEFT;
    sp.x = nextafterf(b.x + 2*b.width, INFINITY);
  } else {
    dir |= QUAD_TREE_DIR_RIGHT;
    sp.x = nextafterf(b.x -b.width, -INFINITY);
  }
  quad_tree_t* new_root = quad_tree_node_malloc(sp);
  new_root->count = root->root->count;
  new_root->mid_point = root->root->mid_point;
  new_root->tangent = root->root->tangent;
  new_root->bounds = root->root->bounds;
  memcpy(&new_root->children[dir], root->root, sizeof(quad_tree_t));
  free(root->root);
  root->root = new_root;
  _quad_tree_root_womit(root);
  quad_tree_extend_rect(&new_root->bounds, sp);
}

bool quad_tree_add_point(quad_tree_root_t* root, Vector2 const * all_points, size_t index) {
  _all_points = all_points;
  _root = root;
  Vector2 p = all_points[index];
  assert(root->temp_length < QUAD_TREE_ROOT_TEMP_CAP);
  root->temp[root->temp_length++] = (size_t)index;
  float factor = (float)(root->temp_length - 1) / (float)(root->temp_length);
  root->temp_mid_point.x *= factor;
  root->temp_mid_point.y *= factor;
  root->temp_mid_point.x += p.x / (float)(root->temp_length);
  root->temp_mid_point.y += p.y / (float)(root->temp_length);
  if (root->temp_length < QUAD_TREE_ROOT_TEMP_CAP)
    return true;

  if (root->root == NULL) {
    root->root = quad_tree_node_malloc(root->temp_mid_point);
    _quad_tree_root_womit(root);
    return true;
  }
  if (CheckCollisionPointRec(root->temp_mid_point, root->root->bounds)) {
    _quad_tree_root_womit(root);
  } else {
    quad_tree_change_root(root);
  }
  return false;
}

static int _quad_tree_add_point(quad_tree_t* root, size_t index) {
  int ret = 0;
  Vector2 point = _all_points[index];
  // Check for NaN
  if (point.x != point.x) return false;
  if (point.y != point.y) return false;

  if (root->is_leaf) {
    bool is_ok = false;
    if (root->groups_cap == 0) {
      root->groups = malloc(sizeof(quad_tree_groups_t)*256);
      memset(root->groups, 0, sizeof(quad_tree_groups_t)*256); 
      root->groups_cap = 256;
    }
    for (size_t i = 0; i < root->groups_cap; ++i) {
      if (root->groups[i].length == 0) {
        root->groups[i].start_index = index;
        root->groups[i].length = 1;
        is_ok = true;
        ++root->groups_len;
        if (i > 0) root->groups_len -= quad_tree_sort_groups(root->groups, root->groups_len);
        break;
      } else if (root->groups[i].start_index == index - root->groups[i].length) {
        ++root->groups[i].length;
        is_ok = true;
        break;
      }
    }
    if (root->count == 0) {
      root->bounds = (Rectangle){.x = point.x, .y = point.y, .width = 0, .height = 0};
    } else {
      quad_tree_extend_rect(&root->bounds, point);
    }
    if (!is_ok) {
      size_t d = quad_tree_sort_groups(root->groups, root->groups_len);
      if (d) {
        root->groups_len -= d;
        printf("Successful groups sort %lu\n", d);
      } else {
        quad_tree_split(root);
        assert(0);
      }
      _quad_tree_add_point(root, index);
    } else {
      ++root->count;
      if (root->count > QUAD_TREE_SPLIT_COUNT)
        quad_tree_split(root);
    }
  } else {
    bool down = point.y < root->split_point.y, right = point.x > root->split_point.x;
    int node_index = (down << 1) | (right);
    if (root->count == 0) {
      root->bounds = (Rectangle){.x = point.x, .y = point.y, .height = 0, .width = 0};
    } 
    ++root->count;
    int child_should_balance = _quad_tree_add_point(&root->children[node_index], index);
    if (!balancing && _root->balanc_enable) {
      bool root_shuld_balance = should_balance(root);
      if (child_should_balance > 1 && root_shuld_balance) {
        balancing = true;
        quad_tree_t dest = {0};
        quad_tree_balance(&dest, root, 0);
        _quad_tree_free(root);
        memcpy(root, &dest, sizeof(quad_tree_t));
        ret = 0;
        balancing = false;
      } else {
        ret = root_shuld_balance ? 1 + child_should_balance : 0;
      }
    }
  }
  quad_tree_extend_rect(&root->bounds, point);
  help_rolling_average(&root->mid_point, point, root->count);
  help_rolling_average(&root->tangent, help_get_tangent(index), root->count);
  //assert(CheckCollisionPointRec(root->mid_point, root->bounds));
  //quad_tree_check(root);
  return ret;
}

static Vector2 help_get_tangent(size_t index) {
  Vector2 a, b;
  if (index == 0) {
    a = _all_points[index];
    b = _all_points[index + 1];
  } else {
    b = _all_points[index];
    a = _all_points[index - 1];
  }
  Vector2 tg = (Vector2){ b.x - a.x, b.y - a.y };
  float tg_norm = sqrtf(tg.x * tg.x + tg.y * tg.y);
  Vector2 tg_n = (Vector2){ tg.x / tg_norm, tg.y / tg_norm };
  if (tg_n.x == tg_n.x && tg_n.y == tg_n.y)
    return tg_n;
  return (Vector2){0};
}

static void help_rolling_average(Vector2* old_avg, Vector2 new_point, size_t new_count) {
  float factor = (float)(new_count - 1) / (float)(new_count);
  old_avg->x *= factor;
  old_avg->y *= factor;
  old_avg->x += new_point.x / (float)(new_count);
  old_avg->y += new_point.y / (float)(new_count);
}

static void quad_tree_check(quad_tree_t* rect) {
  size_t sum = 0;
  if (rect->is_leaf) {
    for (size_t i = 0; i < rect->groups_len; ++i) {
      sum += rect->groups[i].length;
      for (size_t j = 0; j < rect->groups[i].length; ++j) {
         assert(CheckCollisionPointRec(_all_points[rect->groups[i].start_index + j], rect->bounds));
      }
    }
    assert(sum == rect->count);
    return;
  }
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    if (rect->children[i].count == 0) continue;
    assert(rect->count >= rect->children[i].count);
    assert(rect->bounds.x <= rect->children[i].bounds.x);
    assert(rect->bounds.y <= rect->children[i].bounds.y);
    // quad_tree_print_info(rect, 0.0, 0) 
    //assert(rect->bounds.x + rect->bounds.width >= rect->children[i].bounds.x + rect->children[i].bounds.width );
    //assert(rect->bounds.y + rect->bounds.height >= rect->children[i].bounds.y + rect->children[i].bounds.height );
    //assert(rect->bounds.width >= rect->children[i].bounds.width);
    //assert(rect->bounds.height >= rect->children[i].bounds.height);
    quad_tree_check(&rect->children[i]);
    sum += rect->children[i].count;
  }
  assert(sum == rect->count);
}

static void quad_tree_split(quad_tree_t* rect) {
  assert(rect->is_leaf);
  Vector2 sp = quad_tree_mid_point(rect, rect->count);
  rect->is_leaf = false;
  rect->split_point = sp;
  rect->count = 0;
  rect->children = quad_tree_malloc_children();
  rect->bounds = (Rectangle){0};
  for (size_t i = 0; i < rect->groups_len; ++i) {
    for (size_t j = 0; j < rect->groups[i].length; ++j) {
      // b 324 if rect->groups[i].start_index + j == 998
      _quad_tree_add_point(rect, rect->groups[i].start_index + j);
    }
  }
  free(rect->groups);
  rect->groups_len = 0;
}

static void quad_tree_extend_rect(Rectangle* rect, Vector2 point) {
  //if (CheckCollisionPointRec(point, *rect)) return;
  float dx = point.x - rect->x;
  float dy = point.y - rect->y;
  if (dx > 0) {
    rect->width = MAX(dx, rect->width);
    while (rect->width + rect->x < point.x) rect->width = nextafterf(rect->width, INFINITY);
  } else if (dx < 0) {
    float old_reach = rect->width + rect->x;
    rect->x = point.x;
    rect->width -= dx;
    while (rect->width + rect->x < old_reach) rect->width = nextafterf(rect->width, INFINITY);
  }
  if (dy > 0) {
    rect->height = MAX(dy, rect->height);
    while (rect->height + rect->y < point.y) rect->height = nextafterf(rect->height, INFINITY);
  } else if (dy < 0) {
    float old_reach = rect->height + rect->y;
    rect->y = point.y;
    rect->height -= dy;
    while (rect->height + rect->y < old_reach) rect->height = nextafterf(rect->height, INFINITY);
  }
  // b quad_tree.c:352 if rect->width == 0.375137419 || rect->height == 0.375137419
  rect->width = rect->height = MAX(rect->width, rect->height);
}

static float quad_tree_calc_baddnes(quad_tree_t const * qt) {
  float badnes = (float)(MAX(MAX(qt->children[0].count, qt->children[1].count), MAX(qt->children[2].count, qt->children[3].count))) / (float)qt->count;
  return badnes;
}

static void quad_tree_balance(quad_tree_t* dest, quad_tree_t const * qt, int depth) {
  if (qt->is_leaf) {
    memcpy(dest, qt, sizeof(*dest));
    return;
  }
  //printf("BALANCING baddness=%f, count=%d, depth=%d\n", quad_tree_calc_baddnes(qt), qt->count, depth);
  ++_root->balanc_count;
  dest->is_leaf = false;
  dest->split_point = quad_tree_mid_point(qt, qt->count);
  dest->children = quad_tree_malloc_children();
  quad_tree_copy(dest, qt);
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    if (should_balance(&dest->children[i])) {
      quad_tree_t t = {0};
      quad_tree_balance(&t, &dest->children[i], depth + 1);
      _quad_tree_free(&dest->children[i]);
      memcpy(&dest->children[i], &t, sizeof(t));
    }
  }
  //quad_tree_check(dest, all_points);
  //printf("BALANCED baddness=%f, count=%d, depth=%d\n", quad_tree_calc_baddnes(dest), dest->count, depth);
}

static void quad_tree_copy(quad_tree_t* dest, quad_tree_t const * src) {
  size_t dest_start = dest->count;
  if (src->is_leaf) {
    for (size_t i = 0; i < src->groups_len; ++i) {
      if (src->groups[i].length == 0) break;
      for (size_t j = 0; j < src->groups[i].length; ++j) {
        _quad_tree_add_point(dest, src->groups[i].start_index + j);
      }
    }
  } else {
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      quad_tree_copy(dest, &src->children[i]);
    }
  }
  assert((dest->count - dest_start) == src->count);
  return;
}

static Vector2 quad_tree_mid_point(quad_tree_t const * qt, size_t n) {
  (void)n;
  float bx = qt->bounds.width/2, by = qt->bounds.height/2;
  float b = MAX(bx, by);
  //return (Vector2){ .x = b, .y = b };
  return (Vector2){ .x = qt->bounds.x + b, .y = qt->bounds.y + b };
//  Vector2 ret = {0};
//  if (qt->is_leaf) {
//    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
//      for (int j = 0; j < qt->groups[i].length; ++j) {
//        ret.x += _all_points[qt->groups[i].start_index + j].x / n;
//        ret.y += _all_points[qt->groups[i].start_index + j].y / n;
//      }
//    }
//    return ret;
//  }
//  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
//    Vector2 r = quad_tree_mid_point(&qt->node.children[i], n);
//    ret.x += r.x;
//    ret.y += r.y;
//  }
//  return ret;
}
static void indent(int depth) {
  for (int i = 0; i < depth; ++i) {
    printf("  ");
  }
}

static void quad_tree_print_info(quad_tree_t* qt, Vector2 x, int depth) {
  if (depth == 0) {
    printf("Info for point (%f, %f)\n", x.x, x.y);
  }
  //if (!CheckCollisionPointRec(x, qt->bounds)) return;
  if (qt->is_leaf) {
    size_t l = qt->groups_len;
    indent(depth);
    printf("Leaf: %ld groups, x=[%f, %f] (%f); y=[%f, %f] (%f)\n", l, qt->bounds.x, qt->bounds.x + qt->bounds.width, qt->bounds.width, qt->bounds.y, qt->bounds.y + qt->bounds.height, qt->bounds.height);
    for (size_t i = 0; i < l; ++i) {
      quad_tree_groups_t g = qt->groups[i];
      indent(depth + 1);
      printf("[%lu, %lu) (%lu) ", g.start_index, g.start_index + g.length, g.length);
      for (size_t j = 0; j < (MIN(3, g.length)); ++j) {
        Vector2 p = _all_points[g.start_index + j];
        printf("(%.3f, %.3f) ", p.x, p.y);
      }
      printf(g.length > 3 ? "...\n" : "\n");
    }
  } else {
    indent(depth);
    printf("Node: %ld points, x=[%f, %f] (%f); y=[%f, %f] (%f)\n", qt->count, qt->bounds.x, qt->bounds.x + qt->bounds.width, qt->bounds.width, qt->bounds.y, qt->bounds.y + qt->bounds.height, qt->bounds.height);
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      quad_tree_print_info(&qt->children[i], x, depth + 1);
    }
  }
}

static bool should_balance(quad_tree_t* t) {
  return !t->is_leaf && (int)t->count < _root->balanc_max_elements && (int)t->count > _root->balanc_min_elements && quad_tree_calc_baddnes(t) > _root->balanc_max_baddness;
}

static int quad_tree_sort_groups_cop(const quad_tree_groups_t* a, const quad_tree_groups_t* b) {
  if (a->length == 0) return 1;
  if (a->length == 0) return -1;
  return a->start_index < b->start_index;
}

static size_t quad_tree_sort_groups(quad_tree_groups_t* groups, size_t len) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
  qsort(groups, len, sizeof(quad_tree_groups_t), quad_tree_sort_groups_cop);
#pragma GCC diagnostic pop
  size_t c = 0;
  for (size_t i = 1; i < len; ++i) {
    size_t start = groups[i - 1].length;
    if (groups[i - 1].start_index <= groups[i].start_index + groups[i].length && groups[i].length > 0) {
      if (groups[i].start_index + groups[i].length < groups[i - 1].start_index + groups[i - 1].length)
        groups[i - 1].length += (groups[i - 1].start_index - groups[i].start_index);
      else
        groups[i - 1].length += (groups[i].length);
      groups[i - 1].start_index = groups[i].start_index;
      ++c;
      for (size_t j = i + 1; j < len; ++j) {
        groups[j - 1] = groups[j];
      }
      groups[len - 1] = (quad_tree_groups_t){0};
      --i;
      assert(start <= groups[i].length);
    }
  }
  return c;
}

static void quad_tree_draw_rects_for_point(quad_tree_t* qt, Vector2 point) {
  if (CheckCollisionPointRec(point, qt->bounds)) {
    if (qt->is_leaf || _debug == 4) {
      Vector2 vec[5] = { {.x = qt->bounds.x, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y} };
      smol_mesh_gen_line_strip(_line_mesh, vec, 5, _color);
    }
  }
  if (qt->is_leaf) return;
  for(int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_draw_rects_for_point(&qt->children[i], point); 
  }
}

static void quad_tree_draw_rects(quad_tree_t* qt) {
  if (!CheckCollisionRecs(_screen, qt->bounds)) return;
  if (qt->is_leaf) {
      Vector2 vec[5] = { {.x = qt->bounds.x, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y} };
      smol_mesh_gen_line_strip(_line_mesh, vec, 5, _color);
  } else {
    Vector2 vec[10] = {
      {.x = qt->bounds.x, .y = qt->bounds.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->bounds.x, .y = qt->bounds.y},
      {.x = qt->bounds.x, .y = qt->split_point.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->split_point.y},
      {.x = qt->split_point.x, .y = qt->split_point.y},
      {.x = qt->split_point.x, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->split_point.x, .y = qt->bounds.y}
    };
    smol_mesh_gen_line_strip(_line_mesh, vec, 10, _color);
  }
  if (qt->is_leaf) {
    return;
  }
  for(int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_draw_rects(&qt->children[i]); 
  }
}

static void _quad_tree_draw_lines(quad_tree_t* qt) {
  if (qt->count == 0) return;
  if (!CheckCollisionRecs(qt->bounds, _screen)) return;
  if (qt->count > 10 && qt->bounds.height * qt->bounds.width < 0.0001 * _screen.width * _screen.height) {
    smol_mesh_gen_quad(_quad_mesh, qt->bounds, qt->mid_point, qt->tangent, _color);
    return;
  }
  if (!qt->is_leaf) {
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      _quad_tree_draw_lines(&qt->children[i]);
    }
    return;
  }
  ++*_c;
  for (size_t i = 0; i < qt->groups_len; ++i) {
    Vector2 const * ps = &_all_points[qt->groups[i].start_index];
    size_t l = qt->groups[i].length;
    if (ps != _all_points) {
      ps -= 1;
      l += 1;
    }
    for (size_t j = 0; j < l; j+=PTOM_COUNT) {
      assert(l > j);
      size_t len = MIN(l - j, PTOM_COUNT);
      smol_mesh_gen_line_strip(_line_mesh, ps + j, len, _color);
    }
  }
}

static int _quad_tree_print_dot(quad_tree_t* t, int* n) {
  int id = ++*n;
  if (t->is_leaf) {
    printf(" id%d [label=\"%f,%f,%f,%f\\n%lu\"]\n", id, t->bounds.x, t->bounds.y, t->bounds.width, t->bounds.height, t->count);
    return id;
  } else {
    printf(" id%d [label=\"%f,%f,%f,%f\\l%f,%f\\l%lu\"]\n", id, t->bounds.x, t->bounds.y, t->bounds.width, t->bounds.height, t->split_point.x, t->split_point.y, t->count);
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      int c = _quad_tree_print_dot(&t->children[i], n);
      printf(" id%d -> id%d;\n", id, c);
    }
  }
  return id;
}

static quad_tree_visitor _visitor_enter, _visitor_exit;
static void * _visitor_user_data;
static void _quad_tree_visit(quad_tree_t* root) {
  if (_visitor_enter != NULL) _visitor_enter(root, _visitor_user_data);
  if (!root->is_leaf) {
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      _quad_tree_visit(&root->children[i]);
    }
  }
  if (_visitor_exit != NULL) _visitor_exit(root, _visitor_user_data);
}
static void quad_tree_visit(quad_tree_t* root, quad_tree_visitor enter, quad_tree_visitor exit, void* data) {
  _visitor_enter = enter;
  _visitor_exit = exit;
  _visitor_user_data = data;
  _quad_tree_visit(root);
}

typedef struct { int cur_depth, max_depth; } quad_tree_max_depth_data;
static void quad_tree_max_depth_visit_enter(quad_tree_t* node, quad_tree_max_depth_data* count) {
  (void)node;
  count->cur_depth++;
  if (count->cur_depth > count->max_depth) count->max_depth = count->cur_depth;
}
static void quad_tree_max_depth_visit_exit(quad_tree_t* node, quad_tree_max_depth_data* count) {
  (void)node;
  --count->cur_depth;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
int quad_tree_max_depth(quad_tree_root_t* r) {
  quad_tree_max_depth_data data;
  quad_tree_visit(r->root, quad_tree_max_depth_visit_enter, quad_tree_max_depth_visit_exit, &data);
  return data.max_depth;
}

static void quad_tree_count_nodes_visit_enter(quad_tree_t* node, int* count) {
  (void)node;
  ++*count;
}
int quad_tree_count_nodes(quad_tree_root_t* r) {
  int data = 0;
  quad_tree_visit(r->root, quad_tree_count_nodes_visit_enter, NULL, &data);
  return data;
}
typedef struct {float good_sum; int nodes_count; } quad_tree_average_goodness_data;
static void quad_tree_average_goodness_enter(quad_tree_t* qt, quad_tree_average_goodness_data* data) {
  if (qt->is_leaf) return;
  float goodness = 0;
  int optim = (int)qt->count / 4;
  for (int i = 0; i < 4; ++i) {
    goodness += (1.f - (float)abs(optim - (int)qt->children->count) / (float)optim) / 4.f;
  }
  //assert(goodness >= 0 && goodness <= 1);
  data->good_sum += goodness;
  ++data->nodes_count;
}

float quad_tree_average_goodness(quad_tree_root_t* r) {
  quad_tree_average_goodness_data data = {0};
  quad_tree_visit(r->root, quad_tree_average_goodness_enter, NULL, &data);
  return (float)data.good_sum/(float)data.nodes_count;
}
typedef struct {int cur_depth; long long sum;} quad_tree_average_depth_data;
static void quad_tree_average_depth_enter(quad_tree_t* r, quad_tree_average_depth_data* data) {
  ++data->cur_depth;
  if (r->is_leaf) {
    data->sum += (int)r->count * data->cur_depth;
  }
}
static void quad_tree_average_depth_exit(quad_tree_t* r, quad_tree_average_depth_data* data) {
  (void)r;
  --data->cur_depth;
}
float quad_tree_average_depth(quad_tree_root_t* r) {
  quad_tree_average_depth_data data = {0};
  quad_tree_visit(r->root, quad_tree_average_depth_enter, quad_tree_average_depth_exit, &data);
  return (float)data.sum/(float)r->root->count;
}

#pragma GCC diagnostic pop
