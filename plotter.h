#pragma once
#include "raylib.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMOL_MESHES_CAP 1024

// This is the size of one group of points that will be drawn with one draw call.
// TODO: make this dynamic.
#define PTOM_COUNT (1<<10)

#define GROUP_CAP 32

//TODO: Do something with this...
#define GRAPH_LEFT_PAD 400

#define QUAD_TREE_MAX_GROUPS 128
#define QUAD_TREE_SPLIT_COUNT 256


#ifdef LINUX
#include "pthread.h"
#define LOCK_T pthread_mutex_t
#endif

#ifdef LOCK_T
#define LOCK(x) LOCK_T x;
#else
#define LOCK(x)
#endif


typedef enum {
  q_command_none,
  q_command_push_point_y,
  q_command_push_point_xy,
  q_command_pop,
  q_command_clear,
  q_command_clear_all
} q_command_type;

typedef struct {
  q_command_type type;
  union {
    struct {
      int group;
      float y;
    } push_point_y;
    struct {
      int group;
      float x, y;
    } push_point_xy;
    struct {
      int group;
    } pop;
    struct {
      int group;
    } clear;
  };
} q_command;

typedef struct {
  int read_index, write_index;
  int capacity;
  q_command* commands;
  LOCK(push_mutex)
} q_commands;

typedef struct {
  unsigned int vaoId;     // OpenGL Vertex Array Object id
  union {
    unsigned int vboId[3];
    struct {
      unsigned int vboIdVertex, vboIdNormal, vboIdColor;
    };
  };
  float* verticies;
  float* normals;
  float* colors;
  Shader active_shader;
  int cur_len;
  // Max Number of points and only number of points.
  int capacity;
  int draw_calls, points_drawn;
} smol_mesh_t;

enum {
  QUAD_TREE_UP_LEFT = 0,
  QUAD_TREE_UP_RIGHT,
  QUAD_TREE_DOWN_LEFT,
  QUAD_TREE_DOWN_RIGHT,
  QUAD_TREE_DIR_COUNT
}; 

typedef struct {
  int start_index;
  int length;
} quad_tree_groups_t;

typedef struct _quad_tree_s {
  Rectangle bounds;
  Rectangle bb;
  Vector2 average_tangent;
  int count;
  union {
    quad_tree_groups_t groups[QUAD_TREE_MAX_GROUPS];
    struct {
      Vector2 split_point;
      struct _quad_tree_s* children;
    } node;
  };
  bool is_leaf;
} quad_tree_t;


typedef struct {
  int cap, len;
  int group_id;
  bool is_selected, is_sorted;
  Vector2* points;
  quad_tree_t qt;
  int qt_expands;
} points_group_t;

typedef struct {
  union {
    Shader shaders[3];
    struct {
      Shader gridShader, linesShader, quadShader;
    };
  };
  int uResolution[3];
  int uZoom[3];
  int uOffset[3];
  int uScreen[3];

  Rectangle graph_rect;
  Vector2 uvZoom;
  Vector2 uvOffset;
  Vector2 uvScreen;
  Vector2 uvDelta;
  smol_mesh_t* lines_mesh;
  smol_mesh_t* quads_mesh;

  // TODO: This is too big, do something about it! 1MB
  // Only render thread can read or write to this array.
  points_group_t groups[GROUP_CAP];
  int groups_len;
  Color group_colors[GROUP_CAP];

  // Any thread can write to this q, only render thread can pop
  q_commands commands;

  int debug;

  bool shaders_dirty;
  bool follow;

} graph_values_t;


smol_mesh_t* smol_mesh_malloc(int capacity, Shader s);
void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, Vector2 tangent, Color color);
// Only render thread can access this functions.
bool smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2 const * points, int len, Color color);
// Only render thread can access this functions.
void smol_mesh_draw(smol_mesh_t* mesh);
// Only render thread can access this functions.
void smol_mesh_update(smol_mesh_t* mesh);
// Only render thread can access this functions.
void smol_mesh_free(smol_mesh_t* mesh);

// Only render thread can access this functions.
void points_group_push_y(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, float y, int group);
// Only render thread can access this functions.
void points_group_push_xy(points_group_t* pg_array, int* pg_array_len, int pg_array_cap, float x, float y, int group);
// Only render thread can access this functions.
void points_group_clear(points_group_t* pg_array, int* pg_array_len, int group_id);
// Only render thread can access this functions.
void points_group_clear_all(points_group_t* pg_array, int* pg_array_len);
// Only render thread can access this functions.
void points_groups_draw(points_group_t* pg_array, int pg_len, smol_mesh_t* line_mesh, smol_mesh_t* quad_mesh, Color* colors, Rectangle rect, int debug);
// Only render thread can access this functions.
void points_group_add_test_points(points_group_t* pg_array, int* pg_len, int pg_array_cap);

void quad_tree_init(quad_tree_t* qt);
bool quad_tree_add_point(quad_tree_t* root, Vector2 const * all_points, Vector2 point, int index);
int quad_tree_draw(quad_tree_t* qt, Color color, Rectangle screen, smol_mesh_t* line_mesh, smol_mesh_t* quad_mesh, Vector2* all_points, int all_points_count, int debug);
void quad_tree_print_dot(quad_tree_t* t);
// This will only free qt->node.children memory recursivley.
void quad_tree_free(quad_tree_t* qt);

extern Vector2 graph_mouse_position;
// Only render thread can access this functions.
void graph_init(graph_values_t* gv, float width, float height);
// Only render thread can access this functions.
void graph_draw(graph_values_t* gv);

#ifndef RELEASE
// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(graph_values_t* gv);
#endif

void read_input_main(graph_values_t* ptr);
void add_point_callback(graph_values_t* gv, float value, int group);

// Only render thread can access this functions.
void q_init(q_commands* q);
#ifdef LOCK_T
// If you know that only one thread writes to q use q_push, else use this.
bool q_push_safe(q_commands *q, q_command command);
#endif
// If you know that only one thread writes to q us this, else use q_push_safe.
bool q_push(q_commands* q, q_command command);
// Only render thread can access this functions.
q_command q_pop(q_commands* q);

#ifdef __cplusplus
}
#endif

