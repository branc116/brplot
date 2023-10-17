#pragma once
#include "raylib.h"
#include <stddef.h>
#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RELEASE

#ifdef PLATFORM_DESKTOP
#include "shaders.h"
#elif PLATFORM_WEB
#include "shaders_web.h"
#else
#error "Shaders for this platform arn't defined"
#endif

#endif

// This is the size of buffer used to transfer points from cpu to gpu.
#define PTOM_COUNT (1<<10)

//TODO: Do something with this...
#define GRAPH_LEFT_PAD 500

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
  int group_id;
  resamping_interval_t* intervals;
  Vector2* temp_points;
  size_t intervals_count, intervals_cap;
  size_t resampling_count, raw_count; //DEBUG STUFF
} resampling_t;

typedef struct {
  size_t cap, len;
  int group_id;
  bool is_selected;
  Color color;
  Vector2* points;
  resampling_t* resampling;
} points_group_t;

typedef struct {
  size_t cap, len;
  points_group_t* arr;
} points_groups_t;

#define CWD_MAX_SIZE 4096
#define FILE_EXTENSION_MAX_SIZE 16
#define CUR_NAME_MAX_SIZE 4096

typedef struct file_saver_s {
  Rectangle rect;
  char* cwd;
  char* file_extension;
  char* cur_name;
  void (*callback)(void* arg, bool success);
  void* arg;
  float font_size;
  float scroll_position;
  FilePathList paths;
  size_t selected_index;
} file_saver_t;

typedef enum {
  plotter_state_default,
  plotter_state_saving_file
} plotter_state_t;

typedef struct {
  plotter_state_t state;
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
  
  file_saver_t* fs;

  // Only render thread can read or write to this array.
  points_groups_t groups;

  // Any thread can write to this q, only render thread can pop
  q_commands commands;
#ifndef RELEASE
  int (*getchar)(void);
#endif

  bool shaders_dirty;
  bool follow;
  bool jump_around;
  bool file_saver_inited;
} graph_values_t;

typedef struct {
  Rectangle graph_rect;
  Vector2 mouse_screen_pos;
  Vector2 mouse_graph_pos;
  float font_scale;
  float recoil;
  bool mouse_inside_graph;
  bool debug_bounds;
  char buff[128];

} context_t;

extern context_t context;

file_saver_t* file_saver_malloc(const char* cwd, const char* file_exension, const char* default_filename, float font_size, void (*callback)(void*, bool), void* arg);
void file_saver_free(file_saver_t* fe);
void file_saver_draw(file_saver_t* fe);
char const* file_saver_get_full_path(file_saver_t* fe);

smol_mesh_t* smol_mesh_malloc(size_t capacity, Shader s);
void smol_mesh_gen_quad(smol_mesh_t* mesh, Rectangle rect, Vector2 mid_point, Vector2 tangent, Color color);
bool smol_mesh_gen_line_strip(smol_mesh_t* mesh, Vector2 const * points, size_t len, Color color);
void smol_mesh_gen_bb(smol_mesh_t* mesh, bb_t bb, Color color);
void smol_mesh_draw(smol_mesh_t* mesh);
void smol_mesh_update(smol_mesh_t* mesh);
void smol_mesh_free(smol_mesh_t* mesh);

points_group_t* points_group_get(points_groups_t* pg_array, int group);
void points_group_push_y(points_groups_t* pg, float y, int group);
void points_group_push_xy(points_groups_t* pg, float x, float y, int group);
void points_group_clear(points_groups_t* pg, int group_id);
// Only remove all points from a group, don't remove the group itself.
void points_group_empty(points_group_t* pg);
void points_group_export(points_group_t const* pg, FILE* file);
void points_group_export_csv(points_group_t const* pg, FILE* file);

void points_groups_draw(points_groups_t const* pg_array, smol_mesh_t* line_mesh, smol_mesh_t* quad_mesh, Rectangle rect);
void points_groups_add_test_points(points_groups_t* pg_array);
void points_groups_deinit(points_groups_t* pg_array);
// Only remove all points from all groups, don't remove groups themselfs.
void points_groups_empty(points_groups_t* pg_array);
void points_groups_export(points_groups_t const* pg_array, FILE* file);
void points_groups_export_csv(points_groups_t const* pg_array, FILE* file);

resampling_t* resampling_malloc(void);
void          resampling_free(resampling_t* res);
size_t        resampling_draw(resampling_t* res, points_group_t const* pg, Rectangle screen, smol_mesh_t* lines_mesh, smol_mesh_t* quad_mesh);
void          resampling_add_point(resampling_t* res, points_group_t const* pg, size_t index);

void graph_init(graph_values_t* gv, float width, float height);
void graph_screenshot(graph_values_t* gv, char const * path);
void graph_export(graph_values_t* gv, char const * path);
void graph_free(graph_values_t* gv);
void graph_draw(graph_values_t* gv);

#ifndef RELEASE
// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(graph_values_t* gv);
void zig_print_stacktrace(void);
#endif

void read_input_main(graph_values_t* ptr);
void read_input_stop(void);

void q_init(q_commands* q);
#ifdef LOCK_T
// If you know that only one thread writes to q use q_push, else use this.
bool q_push_safe(q_commands *q, q_command command);
#endif
// If you know that only one thread writes to q us this, else use q_push_safe.
bool q_push(q_commands* q, q_command command);
q_command q_pop(q_commands* q);

int     ui_draw_button(bool* is_pressed, float x, float y, float font_size, const char* str, ...);
void    ui_stack_buttons_init(Vector2 pos, float* scroll_position, float font_size);
void    ui_stack_set_size(Vector2 v);
int     ui_stack_buttons_add(bool* is_pressed, const char* str, ...);
Vector2 ui_stack_buttons_end(void);

void    help_trim_zeros(char * buff);
void    help_draw_text(const char *text, Vector2 pos, float fontSize, Color color);
Vector2 help_measure_text(const char* txt, float font_size);
void    help_draw_fps(int posX, int posY);
void    help_load_default_font(void);

static inline float maxf(float a, float b) {
  return a > b ? a : b;
}

static inline int maxi32(int a, int b) {
  return a > b ? a : b;
}

static inline size_t minui64(size_t a, size_t b) {
  return a < b ? a : b;
}

static inline size_t maxui64(size_t a, size_t b) {
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

