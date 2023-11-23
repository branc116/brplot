#pragma once
#include "raylib.h"
#include <stddef.h>
#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RELEASE

#ifdef PLATFORM_DESKTOP
#include "src/misc/shaders.h"
#elif PLATFORM_WEB
#include "src/misc/shaders_web.h"
#else
#error "Shaders for this platform arn't defined"
#endif

#else
#include "src/misc/shaders_dbg.h"

#ifndef LINUX
#ifdef UNIT_TEST
#undef UNIT_TEST
// IT don't work on windows....
#endif
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

#define BR_MALLOC br_malloc
#define BR_CALLOC br_calloc
#define BR_REALLOC br_realloc
#define BR_FREE br_free
#define BR_IMGUI_MALLOC br_imgui_malloc
#define BR_IMGUI_FREE br_imgui_free

typedef enum {
  q_command_none,
  q_command_push_point_y,
  q_command_push_point_xy,
  q_command_pop,
  q_command_clear,
  q_command_clear_all,
  q_command_screenshot,
  q_command_export,
  q_command_exportcsv,
  q_command_hide,
  q_command_show,
} q_command_type;

extern char q_command_path[];

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
    struct {
      // Should be freed by UI thread
      char* path;
    } path_arg;
    struct {
      int group;
    } hide_show;
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
  int group_id;
  Color color;
  size_t cap, len;
  Vector2* points;
  resampling_t* resampling;
  bool is_selected;
} points_group_t;

typedef struct {
  size_t cap, len;
  points_group_t* arr;
} points_groups_t;

typedef struct {
  int uResolution;
  int uZoom;
  int uOffset;
  int uScreen;
  Shader shader;
#ifndef RELEASE
  const char* vs_file_name;
  const char* fs_file_name;
#endif
} br_shader_t;

struct br_plot_t;
#ifdef IMGUI
#ifndef RELEASE
typedef struct {
  LOCK(lock)
  void (*func_loop)(struct br_plot_t* gv);
  void (*func_init)(struct br_plot_t* gv);
  bool is_init_called;
  void* handl;
} br_hotreload_state_t;
#endif
#endif

typedef struct br_plot_t {
  union {
    br_shader_t shaders[3];
    struct {
      br_shader_t gridShader, linesShader, quadShader;
    };
  };

  // graph_rect is in the graph coordinates.
  //            That is if you zoom in and out, graph_rect will change.
  // graph_screen_rect is in the screen coordinates.
  //                   That is if you resize the whole plot, or move the plot around the screen this value will change.
  Rectangle graph_rect, graph_screen_rect;
  Vector2 uvZoom, uvOffset, uvScreen, uvDelta;
  smol_mesh_t* lines_mesh;
  smol_mesh_t* quads_mesh;

  // Only render thread can read or write to this array.
  points_groups_t groups;

  // Any thread can write to this q, only render thread can pop
  q_commands commands;
#ifndef RELEASE
  int (*getchar)(void);
#ifdef IMGUI
  br_hotreload_state_t hot_state;
#endif
#endif
  Vector2 mouse_graph_pos;

  float recoil;
  bool shaders_dirty;
  bool follow;
  bool jump_around;
  bool file_saver_inited;
  bool mouse_inside_graph;
  bool should_close;
} br_plot_t;

typedef struct {
  Vector2 mouse_screen_pos;
  float font_scale;
  bool debug_bounds;
  size_t alloc_size, alloc_count, alloc_total_size, alloc_total_count, alloc_max_size, alloc_max_count, free_of_unknown_memory;
  char buff[128];
} context_t;


extern context_t context;

void* br_malloc(size_t size);
void* br_realloc(void *old, size_t newS);
void  br_free(void* p);
void* br_imgui_malloc(size_t size, void* user_data);
void  br_imgui_free(void* p, void* user_data);

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

void graph_init(br_plot_t* br, float width, float height);
void graph_screenshot(br_plot_t* br, char const* path);
void graph_export(br_plot_t const* br, char const* path);
void graph_export_csv(br_plot_t const* br, char const* path);

void graph_free(br_plot_t* br);
void graph_draw(br_plot_t* br);
void graph_frame_end(br_plot_t* br);

void br_keybinding_handle_keys(br_plot_t* br);

#ifndef RELEASE
// Start watching shaders folder for changes and
// mark gv->shader_dirty flag to true if there were any change to shaders.
void start_refreshing_shaders(br_plot_t* br);
#endif

void read_input_main(br_plot_t* ptr);
void read_input_stop(void);

void q_init(q_commands* q);
#ifdef LOCK_T
// If you know that only one thread writes to q use q_push, else use this.
bool q_push_safe(q_commands *q, q_command command);
#endif
// If you know that only one thread writes to q us this, else use q_push_safe.
bool q_push(q_commands* q, q_command command);
q_command q_pop(q_commands* q);

#ifndef IMGUI
int     ui_draw_button(bool* is_pressed, float x, float y, float font_size, const char* str, ...);
void    ui_stack_buttons_init(Vector2 pos, float* scroll_position, float font_size);
void    ui_stack_set_size(Vector2 v);
int     ui_stack_buttons_add(bool* is_pressed, const char* str, ...);
Vector2 ui_stack_buttons_end(void);
#endif

void    help_trim_zeros(char * buff);
void    help_draw_text(const char *text, Vector2 pos, float fontSize, Color color);
Vector2 help_measure_text(const char* txt, float font_size);
void    help_draw_fps(int posX, int posY);
void    help_load_default_font(void);
void    help_resampling_dir_to_str(char* buff, resampling_dir r);

#ifdef IMGUI
#ifndef RELEASE
void br_hotreload_start(br_hotreload_state_t* s);
#endif
#endif

#ifdef IMGUI
#ifndef file_saver2_t
#define file_saver2_t void
#endif

typedef enum {
  file_saver_state_exploring,
  file_saver_state_accept,
  file_saver_state_cancle
} file_saver_state_t;

file_saver_state_t hot_file_explorer(file_saver2_t* fs);
#endif

#ifdef __cplusplus
}
#endif

