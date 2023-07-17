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

static void quad_tree_extend_rect(Rectangle* rect, Vector2 point);
static void quad_tree_split(quad_tree_t* rect, Vector2 const * all_points);
static void _quad_tree_draw_lines(quad_tree_t* qt);
static void quad_tree_draw_rects(quad_tree_t* qt, int debug);
static Vector2 quad_tree_mid_point(quad_tree_t const * qt, int n);
static void quad_tree_copy(quad_tree_t* dest, quad_tree_t const* src);
static void quad_tree_draw_rects_for_point(quad_tree_t* qt, Vector2 point, int debug);
static void quad_tree_check(quad_tree_t* rect, Vector2 const * all_points);
static bool should_balance(quad_tree_t* t);
static void quad_tree_extend_bb(Rectangle* bb, Vector2 point);

static int* _c;
static Rectangle _screen;
static Vector2 const * _all_points;
static int _all_points_count;
static Shader _shader;
static smol_mesh_t* _sm;

static bool balancing = false;

static float quad_tree_calc_baddnes(quad_tree_t const * qt) {
  float badnes = (MAX(MAX(qt->node.children[0].count, qt->node.children[1].count), MAX(qt->node.children[2].count, qt->node.children[3].count))) / (float)qt->count;
  return badnes;
}

void quad_tree_balance(quad_tree_t* dest, quad_tree_t const * qt, Vector2 const * all_points, int depth) {
  if (qt->is_leaf) {
    memcpy(dest, qt, sizeof(*dest));
    return;
  }
  balancing = true;
  printf("BALANCING baddness=%f, count=%d, depth=%d\n", quad_tree_calc_baddnes(qt), qt->count, depth);
  dest->is_leaf = false;
  _all_points = all_points;
  dest->node.split_point = quad_tree_mid_point(qt, qt->count);
  dest->node.children = malloc(QUAD_TREE_DIR_COUNT * sizeof(quad_tree_t));
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_init(&dest->node.children[i]);
  }
  quad_tree_copy(dest, qt);
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    if (should_balance(&dest->node.children[i])) {
      quad_tree_t t = {0};
      quad_tree_balance(&t, &dest->node.children[i], all_points, depth + 1);
      quad_tree_free(&dest->node.children[i]);
      memcpy(&dest->node.children[i], &t, sizeof(t));
    }
  }
  //quad_tree_check(dest, all_points);
  balancing = false;
  printf("BALANCED baddness=%f, count=%d, depth=%d\n", quad_tree_calc_baddnes(dest), dest->count, depth);
}

static void quad_tree_copy(quad_tree_t* dest, quad_tree_t const * src) {
  int dest_start = dest->count;
  if (src->is_leaf) {
    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
      if (src->groups[i].length == 0) break;
      for (int j = 0; j < src->groups[i].length; ++j) {
        quad_tree_add_point(dest, _all_points, _all_points[src->groups[i].start_index + j], src->groups[i].start_index + j);
      }
    }
  } else {
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      quad_tree_copy(dest, &src->node.children[i]);
    }
  }
  assert((dest->count - dest_start) == src->count);
  return;
}

static Vector2 quad_tree_mid_point(quad_tree_t const * qt, int n) {
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
static int quad_tree_groups_len(quad_tree_groups_t* g) {
  int i = 0;
  for (; i < QUAD_TREE_MAX_GROUPS && g[i].length > 0; ++i);
  return i;
}

static void quad_tree_print_info(quad_tree_t* qt, Vector2 x, int depth) {
  if (depth == 0) {
    printf("Info for point (%f, %f)\n", x.x, x.y);
  }
  //if (!CheckCollisionPointRec(x, qt->bounds)) return;
  if (qt->is_leaf) {
    int l = quad_tree_groups_len(qt->groups);
    indent(depth);
    printf("Leaf: %d groups, (%f, %f, %f, %f)\n", l, qt->bounds.x, qt->bounds.y, qt->bounds.width, qt->bounds.height);
    for (int i = 0; i < l; ++i) {
      indent(depth + 1);
      printf("%d - %d (%d)\n", qt->groups[i].start_index, qt->groups[i].start_index + qt->groups[i].length, qt->groups[i].length);
    }
  } else {
    indent(depth);
    printf("Node: %d points, (%f, %f, %f, %f)\n", qt->count, qt->bounds.x, qt->bounds.y, qt->bounds.width, qt->bounds.height);
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      quad_tree_print_info(&qt->node.children[i], x, depth + 1);
    }
  }
}

int quad_tree_draw_lines(quad_tree_t* qt, Rectangle screen, Vector2* all_points, int all_points_count, Shader shader, int debug) {
  int c = 0;
  _c = &c;
  _screen = screen;
  _screen.y -= _screen.height;
  _all_points = all_points;
  _all_points_count = all_points_count;
  _shader = shader;
  _sm = smol_mesh_get_temp();
  _quad_tree_draw_lines(qt);
  if (debug == 1 || debug == 2) quad_tree_draw_rects(qt, debug);
  if (debug == 3 || debug == 4) quad_tree_draw_rects_for_point(qt, graph_mouse_position, debug);
  if (debug && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    quad_tree_print_info(qt, graph_mouse_position, 0);
  }
  return c;
}

static void quad_tree_draw_rects_for_point(quad_tree_t* qt, Vector2 point, int debug) {
  if (CheckCollisionPointRec(point, qt->bounds)) {
    if (qt->is_leaf || debug == 4) {
      Vector2 vec[5] = { {.x = qt->bounds.x, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y} };
      smol_mesh_gen_line_strip(_sm, vec, 5, 0);
      smol_mesh_update(_sm);
      smol_mesh_draw_line_strip(_sm, _shader);
    }
  }
  if (qt->is_leaf) return;
  for(int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_draw_rects_for_point(&qt->node.children[i], point, debug); 
  }
}

static void quad_tree_draw_rects(quad_tree_t* qt, int debug) {
  if (!CheckCollisionRecs(_screen, qt->bb)) return;
  if (qt->is_leaf) {
      Vector2 vec[5] = { {.x = qt->bounds.x, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
        {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
        {.x = qt->bounds.x, .y = qt->bounds.y} };
      smol_mesh_gen_line_strip(_sm, vec, 5, 0);
      smol_mesh_update(_sm);
      smol_mesh_draw_line_strip(_sm, _shader);
  } else {
    Vector2 vec[10] = {
      {.x = qt->bounds.x, .y = qt->bounds.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->bounds.x, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->bounds.x, .y = qt->bounds.y},
      {.x = qt->bounds.x, .y = qt->node.split_point.y},
      {.x = qt->bounds.x + qt->bounds.width, .y = qt->node.split_point.y},
      {.x = qt->node.split_point.x, .y = qt->node.split_point.y},
      {.x = qt->node.split_point.x, .y = qt->bounds.y + qt->bounds.height},
      {.x = qt->node.split_point.x, .y = qt->bounds.y}
    };
    smol_mesh_gen_line_strip(_sm, vec, 10, 0);
    smol_mesh_update(_sm);
    smol_mesh_draw_line_strip(_sm, _shader);
  }
  if (qt->is_leaf) {
    return;
  }
  for(int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_draw_rects(&qt->node.children[i], debug); 
  }
}

static void _quad_tree_draw_lines(quad_tree_t* qt) {
  if (!CheckCollisionRecs(qt->bounds, _screen)) return;
  if (!qt->is_leaf) {
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      _quad_tree_draw_lines(&qt->node.children[i]);
    }
    return;
  }
  ++*_c;
  for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
    if (qt->groups->length == 0) return;
    Vector2 const * ps = &_all_points[qt->groups[i].start_index];
    int l = qt->groups[i].length;
    if (ps != _all_points) {
      ps -= 1;
      l += 1;
    }
    for (int j = 0; j < l; j+=PTOM_COUNT) {
      int len = MIN(l - j, PTOM_COUNT);
      if (smol_mesh_gen_line_strip(_sm, ps + j, len, 0)) {
        smol_mesh_update(_sm);
        smol_mesh_draw_line_strip(_sm, _shader);
      }
    }
  }
}

static int _quad_tree_print_dot(quad_tree_t* t, int* n) {
  int id = ++*n;
  if (t->is_leaf) {
    printf(" id%d [label=\"%f,%f,%f,%f\\n%d\"]\n", id, t->bounds.x, t->bounds.y, t->bounds.width, t->bounds.height, t->count);
    return id;
  } else {
    printf(" id%d [label=\"%f,%f,%f,%f\\l%f,%f\\l%d\"]\n", id, t->bounds.x, t->bounds.y, t->bounds.width, t->bounds.height, t->node.split_point.x, t->node.split_point.y, t->count);
    for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
      int c = _quad_tree_print_dot(&t->node.children[i], n);
      printf(" id%d -> id%d;\n", id, c);
    }
  }
  return id;
}

void quad_tree_print_dot(quad_tree_t* t) {
  int id = 0;
  printf("digraph L {\n");
  printf("node [shape=record fontname=Arial];\n");
  _quad_tree_print_dot(t, &id);
  printf("}\n");
  return;
}

void quad_tree_init(quad_tree_t* qt) {
  *qt = (quad_tree_t){ 0 };
  qt->is_leaf = true;
}

void quad_tree_free(quad_tree_t* qt) {
  if (qt->is_leaf) return;
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_free(&qt->node.children[i]);
  }
  free(qt->node.children);
}

static bool should_balance(quad_tree_t* t) {
  return !t->is_leaf && t->count < 32*QUAD_TREE_SPLIT_COUNT && t->count > 16*QUAD_TREE_SPLIT_COUNT && (MAX(MAX(t->node.children[0].count, t->node.children[1].count), MAX(t->node.children[2].count, t->node.children[3].count))) > t->count * 0.9;
}

int quad_tree_sort_groups_cop(const quad_tree_groups_t* a, const quad_tree_groups_t* b) {
  if (a->length == 0) return 1;
  if (a->length == 0) return -1;
  return -a->start_index > -b->start_index;
}

int quad_tree_sort_groups(quad_tree_groups_t* groups, int len) {
  qsort(groups, len, sizeof(quad_tree_groups_t), quad_tree_sort_groups_cop);
  int c = 0;
  for (int i = 1; i < len; ++i) {
    if (groups[i - 1].start_index <= groups[i].start_index + groups[i].length && groups[i].length > 0) {
      if (groups[i].start_index + groups[i].length < groups[i - 1].start_index + groups[i - 1].length)
        groups[i - 1].length += (groups[i - 1].start_index - groups[i].start_index);
      else
        groups[i - 1].length += (groups[i].length);
      groups[i - 1].start_index = groups[i].start_index;
      ++c;
      for (int j = i + 1; j < len; ++j) {
        groups[j - 1] = groups[j];
      }
      groups[len - 1] = (quad_tree_groups_t){0};
      --i;
    }
    assert(groups[i].length >= 0);
  }
  return c;
}

bool quad_tree_add_point(quad_tree_t* root, Vector2 const * all_points, Vector2 point, int index) {
  bool ret = false;
  // Check for NaN
  if (point.x != point.x) return false;
  if (point.y != point.y) return false;

  if (root->is_leaf) {
    bool is_ok = false;
    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
      if (root->groups[i].length == 0) {
        root->groups[i].start_index = index;
        root->groups[i].length = 1;
        is_ok = true;
        if (i > 0) quad_tree_sort_groups(root->groups, QUAD_TREE_MAX_GROUPS);
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
      quad_tree_extend_bb(&root->bb, point);
    }
    if (!is_ok) {
      int d = quad_tree_sort_groups(root->groups, QUAD_TREE_MAX_GROUPS);
      if (d) {
        printf("Successful groups sort %d\n", d);
      } else {
        assert(0);
      }
      quad_tree_add_point(root, all_points, point, index);
    } else {
      ++root->count;
      if (root->count > QUAD_TREE_SPLIT_COUNT)
        quad_tree_split(root, all_points);
    }
  } else {
    bool down = point.y < root->node.split_point.y, right = point.x > root->node.split_point.x;
    int node_index = (down << 1) | (right);
    if (root->count == 0) {
      root->bounds = (Rectangle){.x = point.x, .y = point.y, .height = 0, .width = 0};
    } else {
      quad_tree_extend_rect(&root->bounds, point);
      quad_tree_extend_bb(&root->bb, point);
    }
    ++root->count;
    bool child_should_balance = quad_tree_add_point(&root->node.children[node_index], all_points, point, index);
    if (!balancing) {
      bool root_shuld_balance = ret = should_balance(root);
      if (child_should_balance && root_shuld_balance) {
        quad_tree_t dest;
        quad_tree_init(&dest);
        quad_tree_balance(&dest, root, all_points, 0);
        quad_tree_free(root);
        memcpy(root, &dest, sizeof(quad_tree_t));
        ret = false;
      }
      ret = should_balance(root);
    }
  }
  //quad_tree_check(root, all_points);
  return ret;
}

static void quad_tree_check(quad_tree_t* rect, Vector2 const * all_points) {
  int sum = 0;
  if (rect->is_leaf) {
    for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
      sum += rect->groups[i].length;
      for (int j = 0; j < rect->groups[i].length; ++j) {
        assert(CheckCollisionPointRec(all_points[rect->groups[i].start_index + j], rect->bounds));
      }
    }
    assert(sum == rect->count);
    return;
  }
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    assert(rect->count >= rect->node.children[i].count);
    assert(rect->bounds.x <= rect->node.children[i].bounds.x || rect->node.children[i].count == 0);
    assert(rect->bounds.y <= rect->node.children[i].bounds.y || rect->node.children[i].count == 0);
    assert(rect->bounds.width >= rect->node.children[i].bounds.width);
    assert(rect->bounds.height >= rect->node.children[i].bounds.height);
    quad_tree_check(&rect->node.children[i], all_points);
    sum += rect->node.children[i].count;
  }
  assert(sum == rect->count);
}

static void quad_tree_split(quad_tree_t* rect, Vector2 const * all_points) {
  assert(rect->is_leaf);
  _all_points = all_points;
  Vector2 sp = quad_tree_mid_point(rect, rect->count);
  struct {
    int start;
    int length;
  } groups[QUAD_TREE_MAX_GROUPS];
  memcpy(groups, rect->groups, sizeof(rect->groups));
  rect->is_leaf = false;
  rect->node.split_point = sp;
  rect->node.children = malloc(QUAD_TREE_DIR_COUNT * sizeof(quad_tree_t));
  rect->count = 0;
  for (int i = 0; i < QUAD_TREE_DIR_COUNT; ++i) {
    quad_tree_init(&rect->node.children[i]);
  }
  for (int i = 0; i < QUAD_TREE_MAX_GROUPS; ++i) {
    for (int j = 0; j < groups[i].length; ++j) {
      quad_tree_add_point(rect, all_points, all_points[groups[i].start + j], groups[i].start + j);
    }
  }
}

static void quad_tree_extend_rect(Rectangle* rect, Vector2 point) {
  if (CheckCollisionPointRec(point, *rect)) return;
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
  rect->width = rect->height = MAX(rect->width, rect->height);
}

static void quad_tree_extend_bb(Rectangle* bb, Vector2 point) {
  if (CheckCollisionPointRec(point, *bb)) return;
  float dx = point.x - bb->x;
  float dy = point.y - bb->y;
  if (dx > 0) {
    bb->width = MAX(dx, bb->width);
    while (bb->width + bb->x < point.x) bb->width = nextafterf(bb->width, INFINITY);
  } else if (dx < 0) {
    float old_reach = bb->width + bb->x;
    bb->x = point.x;
    bb->width -= dx;
    while (bb->width + bb->x < old_reach) bb->width = nextafterf(bb->width, INFINITY);
  }
  if (dy > 0) {
    bb->height = MAX(dy, bb->height);
    while (bb->height + bb->y < point.y) bb->height = nextafterf(bb->height, INFINITY);
  } else if (dy < 0) {
    float old_reach = bb->height + bb->y;
    bb->y = point.y;
    bb->height -= dy;
    while (bb->height + bb->y < old_reach) bb->height = nextafterf(bb->height, INFINITY);
  }
}
