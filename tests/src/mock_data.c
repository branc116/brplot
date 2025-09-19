#include "src/br_data.h"

void br_data_push_x(br_datas_t* pg, double x, int group) { (void)pg; (void)x; (void)group; BR_TODO("br_data_push_x"); }
void br_data_push_y(br_datas_t* pg, double x, int group) { (void)pg; (void)x; (void)group; BR_TODO("br_data_push_y"); }
void br_data_push_xy(br_datas_t* pg, double x, double y, int group) { (void)pg; (void)x; (void)y; (void)group; BR_TODO("br_data_push_xy"); }
void br_data_push_xyz(br_datas_t* pg, double x, double y, double z, int group) { (void)pg; (void)x; (void)y; (void)z; (void)group; BR_TODO("br_data_push_xyz"); }
br_data_t* br_data_get1(br_datas_t pg, int group) { (void)pg; (void)group; BR_TODO("br_data_get1"); }
void br_data_empty(br_data_t* pg) { (void)pg; BR_TODO("br_data_empty"); }
void br_plotter_data_remove(br_plotter_t* br, int group_id) { (void)br; (void)group_id; BR_TODO("br_plotter_data_remove"); }
void br_plotter_datas_deinit(br_plotter_t* br) { (void)br; BR_TODO("br_plotter_datas_deinit"); }
void br_plot_screenshot(br_text_renderer_t* tr, br_plot_t* br, br_shaders_t* shaders, br_datas_t groups, char const* path) {
  (void)tr; (void)br; (void)shaders; (void)groups; (void)path;
  BR_TODO("br_plot_screenshot"); }
void br_plotter_export(br_plotter_t const* br, char const* path) { (void)br; (void)path; BR_TODO("br_plotter_export"); }
void br_plotter_export_csv(br_plotter_t const* br, char const* path) { (void)br; (void)path; BR_TODO("br_plotter_export_csv"); }
void br_data_set_name(br_datas_t* pg_array, int group, br_str_t name) { (void)pg_array; (void)group; (void)name; BR_TODO("br_data_set_name"); }
bool br_dagens_add_expr_str(br_dagens_t* dagens, br_datas_t* datas, br_strv_t str, int group_id) { (void)dagens; (void)datas; (void)str; (void)group_id; BR_TODO("br_dagens_add_expr_str"); return true; }
void br_plots_focus_visible(br_plots_t plot, br_datas_t groups) { (void)plot; (void)groups; BR_TODO("br_plots_focus_visible"); }
br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind) { (void)datas; (void)group_id; (void)kind; BR_TODO("br_datas_create"); return NULL; }


