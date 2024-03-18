#include "br_data.h"
#include "br_plot.h"
#ifdef __cplusplus
extern "C" {
#endif


struct br_plotter_t;
#ifdef IMGUI
#ifndef RELEASE
typedef struct {
  LOCK(lock)
  void (*func_loop)(struct br_plotter_t* gv);
  void (*func_init)(struct br_plotter_t* gv);
  bool is_init_called;
  void* handl;
} br_hotreload_state_t;
#endif
#endif

typedef struct br_plotter_t {
  br_datas_t groups;
  br_datas_3d_t groups_3d;
  br_plots_t plots;
  br_shaders_t shaders;

  // Any thread can write to this q, only render thread can pop
  q_commands commands;
#ifndef RELEASE
#ifdef IMGUI
#ifdef LINUX
  br_hotreload_state_t hot_state;
#endif
#endif
#endif
  Vector2 mouse_graph_pos;

  bool shaders_dirty;
  bool file_saver_inited;
  bool should_close;
} br_plotter_t;

BR_API br_plotter_t* br_plotter_malloc(void);
BR_API void br_plotter_init(br_plotter_t* br, float width, float height);
BR_API void br_plotter_resize(br_plotter_t* br, float width, float height);
BR_API br_datas_t* br_plotter_get_br_datas(br_plotter_t* br);
BR_API void br_plotter_set_bottom_left(br_plot_t* plot, float left, float bottom);
BR_API void br_plotter_set_top_right(br_plot_t* plot, float right, float top);
BR_API void br_plotter_focus_visible(br_plot_t* plot, br_datas_t groups);
void br_plotter_add_plot_2d(br_plotter_t* br);
void br_plotter_add_plot_3d(br_plotter_t* br);
void br_plotter_export(br_plotter_t const* br, char const* path);
void br_plotter_export_csv(br_plotter_t const* br, char const* path);
BR_API void br_plotter_free(br_plotter_t* br);
BR_API void br_plotter_draw(br_plotter_t* br);
BR_API void br_plotter_minimal(br_plotter_t* br);
BR_API void br_plotter_frame_end(br_plotter_t* br);

#ifdef __cplusplus
}
#endif

