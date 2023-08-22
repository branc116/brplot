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

#define QUAD_TREE_SPLIT_COUNT 4


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
  size_t read_index, write_index;
  size_t capacity;
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
  size_t cur_len;
  // Max Number of points and only number of points.
  size_t capacity;
  size_t draw_calls, points_drawn;
} smol_mesh_t;

typedef struct {
  size_t start_index;
  size_t length;
} quad_tree_groups_t;
typedef struct bounding_box {
  float xmin, ymin, xmax, ymax;
} bb_t;

typedef enum {
  resampling_dir_null = 0ul,
  resampling_dir_left = 1ul,
  resampling_dir_right = 2ul,
  resampling_dir_up = 4ul,
  resampling_dir_down = 8ul
} resampling_dir;

typedef struct {
  bb_t bounds;
  size_t from, count;
  resampling_dir dir;
} resamping_interval_t;

typedef struct {
  Vector2 const ** points;
  resamping_interval_t* intervals;
  Vector2* temp_points;
  size_t intervals_count, intervals_cap;
  size_t resampling_count, raw_count;
} resampling_t;

typedef struct {
  size_t cap, len;
  int group_id;
  bool is_selected;
  Vector2* points;
  resampling_t* resampling;
} points_group_t;

typedef struct {
  union {
    Shader shaders[3];
    struct {
      Shader gridShader, linesShader, quadShader;
    };
  };
  int uResolution[3], uZoom[3], uOffset[3], uScreen[3];

  Rectangle graph_rect;
  Vector2 uvZoom, uvOffset, uvScreen, uvDelta;
  smol_mesh_t* lines_mesh;
  smol_mesh_t* quads_mesh;

  Font font;

  // TODO: This is too big, do something about it! 1MB
  // Only render thread can read or write to this array.
  points_group_t groups[GROUP_CAP];
  size_t groups_len;
  Color group_colors[GROUP_CAP];

  // Any thread can write to this q, only render thread can pop
  q_commands commands;

  bool shaders_dirty;
  bool follow;

} graph_values_t;


smol_mesh_t* smol_mesh_malloc(size_t capacity, Shader s);
void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, Vector2 mid_point, Vector2 tangent, Color color);
bool smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2 const * points, size_t len, Color color);
void smol_mesh_gen_bb(smol_mesh_t* mesh, bb_t bb, Color color);
void smol_mesh_draw(smol_mesh_t* mesh);
void smol_mesh_update(smol_mesh_t* mesh);
void smol_mesh_free(smol_mesh_t* mesh);

void points_group_push_y(points_group_t* pg_array, size_t* pg_array_len, size_t pg_array_cap, float y, int group);
void points_group_push_xy(points_group_t* pg_array, size_t* pg_array_len, size_t pg_array_cap, float x, float y, int group);
void points_group_clear(points_group_t* pg_array, size_t* pg_array_len, int group_id);
void points_group_clear_all(points_group_t* pg_array, size_t* pg_array_len);
void points_groups_draw(points_group_t* pg_array, size_t pg_len, smol_mesh_t* line_mesh, smol_mesh_t* quad_mesh, Color* colors, Rectangle rect);
void points_group_add_test_points(points_group_t* pg_array, size_t* pg_len, size_t pg_array_cap);

resampling_t* resampling_malloc(Vector2 const ** points);
void resampling_free(resampling_t* res);
size_t resampling_draw(resampling_t* res, Rectangle screen, smol_mesh_t* lines_mesh, smol_mesh_t* quad_mesh, Color color);
void resampling_add_point(resampling_t* res, size_t index);

extern Vector2 graph_mouse_position;
void graph_init(graph_values_t* gv, float width, float height);
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

static inline float maxf(float a, float b) {
  return a > b ? a : b;
}

static inline float minf(float a, float b) {
  return a > b ? b : a;
}

static inline float absf(float a) {
  return a > 0 ? a : -a;
}
static inline float signf(float a) {
  return a > 0.f ?  1.f :
         a < 0.f ? -1.f : 0.f;
}
static inline int signi(int a) {
  return a > 0 ?  1 :
         a < 0 ? -1 : 0;
}

#ifdef __cplusplus
}
#endif

