#include "src/br_permastate.h"
#include "src/br_plotter.h"
#include "include/br_str_header.h"
#include "src/br_plot.h"
#include "src/br_filesystem.h"
#include "src/br_resampling.h"
#include "src/br_data.h"
#include "src/br_data_generator.h"
#include "src/br_da.h"
#include "include/br_free_list_header.h"
#include "src/br_ui.h"
#include "src/br_memory.h"
#include "src/br_series.h"

typedef struct br_data_save_t {
  // Both project and editor
  brsp_t sp;
  br_anims_t anims;
  bruirs_t resizables;

  // Only project
  br_plots_t plots;
  br_dagens_t dagens;
  br_datas_t datas;
  br_serieses_t serieses;

  // Only editor
  br_plotter_ui_t ui;
  br_theme_t theme;
} brps_copy_t;

static void brps_copy_deinit(brps_copy_t copy) {
  for (int i = 0; i < copy.plots.len; ++i) {
    br_da_free(copy.plots.arr[i].data_info);
  }
  br_da_free(copy.plots);
  brsp_free(&copy.sp);
  br_anims_delete(&copy.anims);
  brfl_free(copy.resizables);
  br_da_free(copy.dagens);
  brfl_free(copy.datas);
  brfl_free(copy.serieses);
}

static int brps_string_save(br_plotter_t const* br, brps_copy_t* save, int string_handle) {
  if (string_handle == 0) return 0;
  br_strv_t str = brsp_get(br->uiw.sp, string_handle);
  int new_handle = brsp_push(&save->sp, str);
  return new_handle;
}
static int brps_string_load(br_plotter_t* br, brps_copy_t const* save, int string_handle) {
  if (string_handle == 0) return 0;
  br_strv_t str = brsp_get(save->sp, string_handle);
  int new_handle = brsp_push(&br->uiw.sp, str);
  return new_handle;
}

static int brps_anim_save(br_plotter_t const* br, brps_copy_t* save, int anim_handle) {
  br_anim_t old = br_da_get(br->uiw.anims.all, anim_handle);
  if (old.kind == br_anim_vec3) {
    if (old.is_slerp) {
      old.vec3.slerp_origin = brps_anim_save(br, save, old.vec3.slerp_origin);
    }
  }
  int ret_handle = brfl_push(save->anims.all, old);
  if (old.is_alive) {
    brfl_push(save->anims.alive, ret_handle);
  }
  return ret_handle;
}
static int brps_anim_load(br_plotter_t* br, brps_copy_t const* save, int anim_handle) {
  br_anim_t old = br_da_get(save->anims.all, anim_handle);
  if (old.kind == br_anim_vec3) {
    if (old.is_slerp) {
      old.vec3.slerp_origin = brps_anim_load(br, save, old.vec3.slerp_origin);
    }
  }
  int ret_handle = brfl_push(br->uiw.anims.all, old);
  if (old.is_alive) {
    brfl_push(br->uiw.anims.alive, ret_handle);
  }
  return ret_handle;
}

static int brps_resizable_save(br_plotter_t const* br, brps_copy_t* save, int res_handle, int new_parent) {
  brui_resizable_t res = br_da_get(br->uiw.resizables, res_handle);
  brui_resizable_t res_copy = res;
  res_copy.parent = new_parent;
  res_copy.title_id = brps_string_save(br, save, res.title_id);
  res_copy.title_height_ah = brps_anim_save(br, save, res.title_height_ah);
  res_copy.scroll_offset_percent_ah = brps_anim_save(br, save, res.scroll_offset_percent_ah);
  res_copy.cur_extent_ah = brps_anim_save(br, save, res.cur_extent_ah);
  return brfl_push(save->resizables, res_copy);
}
static int brps_resizable_load(br_plotter_t* br, brps_copy_t const* save, int res_handle, int new_parent) {
  brui_resizable_t res = br_da_get(save->resizables, res_handle);
  brui_resizable_t res_copy = res;
  res_copy.parent = new_parent;
  res_copy.title_id = brps_string_load(br, save, res.title_id);
  res_copy.title_height_ah = brps_anim_load(br, save, res.title_height_ah);
  res_copy.scroll_offset_percent_ah = brps_anim_load(br, save, res.scroll_offset_percent_ah);
  res_copy.cur_extent_ah = brps_anim_load(br, save, res.cur_extent_ah);
  return brfl_push(br->uiw.resizables, res_copy);
}

static void brps_plot_save(br_plotter_t const* br, brps_copy_t* save, int plot_id) {
  br_plot_t plot = br_da_get(br->plots, plot_id);
  if (plot.is_deleted) return;
  br_plot_t copy_plot = plot;
  copy_plot.data_info.arr = NULL;
  copy_plot.data_info.len = 0;
  copy_plot.data_info.cap = 0;
  br_da_copy(copy_plot.data_info, plot.data_info);
  for (int i = 0; i < copy_plot.data_info.len; ++i) {
    br_plot_data_t* data = br_da_getp(copy_plot.data_info, i);
    data->thickness_multiplyer_ah = brps_anim_save(br, save, data->thickness_multiplyer_ah);
  }
  // TODO: Parent of plot can be something other than root..
  copy_plot.extent_handle = brps_resizable_save(br, save, plot.extent_handle, 0);
  copy_plot.menu_extent_handle = brps_resizable_save(br, save, plot.menu_extent_handle, copy_plot.extent_handle);
  copy_plot.legend_extent_handle = brps_resizable_save(br, save, plot.legend_extent_handle, copy_plot.extent_handle);
  switch (plot.kind) {
    case br_plot_kind_2d: {
      copy_plot.dd.zoom_ah = brps_anim_save(br, save, plot.dd.zoom_ah);
      copy_plot.dd.offset_ah = brps_anim_save(br, save, plot.dd.offset_ah);
    } break;
    case br_plot_kind_3d: {
      copy_plot.ddd.eye_ah = brps_anim_save(br, save, plot.ddd.eye_ah);
      copy_plot.ddd.target_ah = brps_anim_save(br, save, plot.ddd.target_ah);
    } break;
    default: BR_UNREACHABLE("plot.kind: %d", plot.kind);
  }
  br_da_push_t(int, save->plots, copy_plot);
}
static void brps_plot_load(br_plotter_t* br, brps_copy_t const* save, int plot_id) {
  br_plot_t plot = br_da_get(save->plots, plot_id);
  if (plot.is_deleted) return;
  br_plot_t copy_plot = plot;
  copy_plot.data_info.arr = NULL;
  copy_plot.data_info.len = 0;
  copy_plot.data_info.cap = 0;
  br_da_copy(copy_plot.data_info, plot.data_info);
  for (int i = 0; i < copy_plot.data_info.len; ++i) {
    br_plot_data_t* data = br_da_getp(copy_plot.data_info, i);
    data->thickness_multiplyer_ah = brps_anim_load(br, save, data->thickness_multiplyer_ah);
  }
  // TODO: Parent of plot can be something other than root..
  copy_plot.extent_handle = brps_resizable_load(br, save, plot.extent_handle, 0);
  copy_plot.menu_extent_handle = brps_resizable_load(br, save, plot.menu_extent_handle, copy_plot.extent_handle);
  copy_plot.legend_extent_handle = brps_resizable_load(br, save, plot.legend_extent_handle, copy_plot.extent_handle);
  switch (plot.kind) {
    case br_plot_kind_2d: {
      copy_plot.dd.zoom_ah = brps_anim_load(br, save, plot.dd.zoom_ah);
      copy_plot.dd.offset_ah = brps_anim_load(br, save, plot.dd.offset_ah);
    } break;
    case br_plot_kind_3d: {
      copy_plot.ddd.eye_ah = brps_anim_load(br, save, plot.ddd.eye_ah);
      copy_plot.ddd.target_ah = brps_anim_load(br, save, plot.ddd.target_ah);
    } break;
    default: BR_UNREACHABLE("plot.kind: %d", plot.kind);
  }
  br_da_push_t(int, br->plots, copy_plot);
}

static int brps_series_save(br_plotter_t const* br, brps_copy_t* save, int series_handle) {
  br_series_t s =  br_da_get(br->serieses, series_handle);
  return brfl_push(save->serieses, s);
}
static int brps_series_load(br_plotter_t* br, brps_copy_t const* save, int series_handle) {
  br_series_t s =  br_da_get(save->serieses, series_handle);
  return brfl_push(br->serieses, s);
}

static void brps_plotter_save(br_plotter_t const* br, brps_copy_t* save) {
  for (int i = 0; i < br->plots.len; ++i) {
    brps_plot_save(br, save, i);
  }

  brfl_foreach(i, br->groups) {
    br_data_t data = br_da_get(br->groups, i);
    if (br_data_is_generated(&br->dagens, data.group_id)) continue;
    br_data_t data_copy = data;
    data_copy.name = brps_string_save(br, save, data.name);
    data_copy.resampling = NULL;
    for (int j = 0; j < data.serieses_len; ++j) {
      data_copy.series_handles[j] = brps_series_save(br, save, data.series_handles[j]);
    }
    brfl_push(save->datas, data_copy);
  }

  for (size_t i = 0; i < br->dagens.len; ++i) {
    br_dagen_t dag = br_da_get(br->dagens, i);
    dag.state = br_dagen_state_inprogress;
    if (dag.kind == br_dagen_kind_file) continue;
    br_da_push(save->dagens, dag);
  }
}
static void brps_plotter_load(br_plotter_t* br, brps_copy_t const* save) {
  for (int i = 0; i < save->plots.len; ++i) {
    brps_plot_load(br, save, i);
  }

  brfl_foreach(i, save->datas) {
    br_data_t data = br_da_get(save->datas, i);
    BR_ASSERT(!br_data_is_generated(&save->dagens, data.group_id));
    br_data_t data_copy = data;
    data_copy.name = brps_string_load(br, save, data.name);
    for (int j = 0; j < data.serieses_len; ++j) {
      data_copy.series_handles[j] = brps_series_load(br, save, data.series_handles[j]);
    }
    brfl_push(br->groups, data_copy);
  }

  for (size_t i = 0; i < save->dagens.len; ++i) {
    br_dagen_t dag = br_da_get(save->dagens, i);
    dag.state = br_dagen_state_inprogress;
    //BR_ASSERT(dag.kind != br_dagen_kind_file);
    br_da_push(br->dagens, dag);
  }
}

static bool brps_project_fwrite(BR_FILE* file, br_plotter_t const* br) {
  bool success = true;
  int err = 0;
  brps_copy_t copy = { 0 };
  brps_plotter_save(br, &copy);

  br_da_write(file, copy.plots);
  for (int i = 0; i < copy.plots.len; ++i) {
    br_da_write(file, copy.plots.arr[i].data_info);
  }
  brsp_write(file, &copy.sp);
  br_anim_save(file, &copy.anims);

  brfl_write(file, copy.resizables, err);
  if (err) BR_ERRORE("Failed to write resizables");

  br_da_write(file, copy.dagens);

  brfl_write(file, copy.datas, err);
  if (err) BR_ERRORE("Failed to write datas");

  if (false == br_serieses_write(file, copy.serieses)) BR_ERROR("Failed to write serieses");;

error:
  brps_copy_deinit(copy);

  return success;
}

static bool brps_project_fread(BR_FILE* file, br_plotter_t* br) {
  bool success = true;
  int err = 0;
  brps_copy_t copy = { 0 };

  br_da_read(file, copy.plots);
  for (int i = 0; i < copy.plots.len; ++i) {
    copy.plots.arr[i].data_info.arr = NULL;
  }
  for (int i = 0; i < copy.plots.len; ++i) {
    br_da_read(file, copy.plots.arr[i].data_info);
  }
  brsp_read(file, &copy.sp);
  br_anim_load(file, &copy.anims);
  brfl_read(file, copy.resizables, err);
  if (err) BR_ERRORE("Failed to read resizables");

  br_da_read(file, copy.dagens);
  brfl_read(file, copy.datas, err);
  LOGI("datas len: %d", copy.datas.free_len);
  brfl_foreach(i, copy.datas) {
    LOGI("data %d=%d", i, copy.datas.arr[i].group_id);
  }

  // TODO: Maybe just some series failed to read. Handle that case...
  if (false == br_serieses_read(file, &copy.serieses)) BR_ERROR("Failed to write serieses");;

  brfl_foreach(i, copy.datas) {
    br_data_t* data = br_da_getp(copy.datas, i);
    br_series_t s = br_da_get(copy.serieses, data->series_handles[0]);
    size_t len = br_series_len(s);
    br_dagen_push_file(&copy.dagens, data, len);
  }

  brps_plotter_load(br, &copy);
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_t* p = &br->plots.arr[i];
    br_extent_t ex = brui_resizable_cur_extent(p->extent_handle);
    p->texture_id = brgl_create_framebuffer((int)roundf(ex.width), (int)roundf(ex.height));
  }

  brfl_foreach(i, br->groups) {
    br_data_t* data = br_da_getp(br->groups, i);
    data->resampling = br_resampling_malloc(data->kind);
    BR_ASSERTF(data->resampling, "Failed to allocate resampling structure for data with id %d", data->group_id);
  }

error:
  brps_copy_deinit(copy);

  return success;
}

static const char* brps_editor_default_file_name(br_str_t* str) {
  if (false == br_fs_get_config_dir(str)) return NULL;
  if (false == br_fs_cd(str, BR_STRL("editor.brplot"))) return NULL;
  br_str_push_zero(str);
  return str->str;
}

bool brps_project_write(br_plotter_t* br, const char* file_name) {
  BR_FILE* file = NULL;
  bool success = true;
  if (file_name == NULL) {
    file_name = "project.brplot";
    br->ui.project_path_id = brsp_push(&br->uiw.sp, br_strv_from_c_str(file_name));
  }

  file = BR_FOPEN(file_name, "wb");
  if (file == NULL) BR_ERRORE("Failed to open a file `%s`", file_name);

  if (false == brps_project_fwrite(file, br)) BR_ERRORE("Failed to save the project..");

error:
  if (file != NULL) BR_FCLOSE(file);
  return success;
}

bool brps_project_read(br_plotter_t* br, const char* file_name) {
  BR_FILE* file = NULL;
  bool success = true;

  file = BR_FOPEN(file_name, "rb");
  if (file == NULL) BR_ERRORE("Failed to open a file `%s`", file_name);

  if (false == brps_project_fread(file, br)) BR_ERROR("Failed to load the project..");

error:
  if (file != NULL) BR_FCLOSE(file);
  return success;
}

bool brps_editor_write(br_plotter_t const* br, char const* file_name) {
  BR_FILE* file = NULL;
  bool success = true;
  int err = 0;
  brps_copy_t copy = { 0 };
  br_str_t tmp = { 0 };

  if (file_name == NULL) {
    file_name = brps_editor_default_file_name(&tmp);
    if (file_name == NULL) BR_ERROR("Failed to get editor file path");
  }
  BR_FS_OPEN(file, file_name, "wb");

  copy.ui = br->ui;
  copy.theme = br->uiw.theme;
  copy.ui.menu_extent_handle = brps_resizable_save(br, &copy, copy.ui.menu_extent_handle, 0);
  copy.ui.csv_file_opened = brps_string_save(br, &copy, copy.ui.csv_file_opened);
  copy.ui.font_path_id = brps_string_save(br, &copy, copy.ui.font_path_id);
  copy.ui.project_path_id = brps_string_save(br, &copy, copy.ui.project_path_id);

  brsp_write(file, &copy.sp);
  br_anim_save(file, &copy.anims);
  brfl_write(file, copy.resizables, err);
  if (err) BR_ERRORE("Failed to write resizables");
  BR_FS_WRITE1(file, copy.theme);
  BR_FS_WRITE1(file, copy.ui);
  LOGI("Wrote %zd bytes to %s", ftell(file), file_name);

error:
  if (file != NULL) BR_FCLOSE(file);
  brps_copy_deinit(copy);
  br_str_free(tmp);
  return success;
}

bool brps_editor_read(br_plotter_t* br, char const* file_name) {
  BR_FILE* file = NULL;
  bool success = true;
  int err = 0;
  brps_copy_t copy = { 0 };
  br_str_t tmp = { 0 };

  if (file_name == NULL) {
    file_name = brps_editor_default_file_name(&tmp);
    if (file_name == NULL) BR_ERROR("Failed to get editor file path");
  }
  BR_FS_OPEN(file, file_name, "rb");

  brsp_read(file, &copy.sp);
  br_anim_load(file, &copy.anims);
  brfl_read(file, copy.resizables, err);
  if (err) BR_ERRORE("Failed to read resizables");
  BR_FS_READ1(file, copy.theme);
  BR_FS_READ1(file, copy.ui);
  LOGI("Read %zd bytes from %s", ftell(file), file_name);
  BR_FS_EOF(file);

  br->ui = copy.ui;
  br->uiw.theme = copy.theme;
  br->ui.menu_extent_handle = brps_resizable_load(br, &copy, copy.ui.menu_extent_handle, 0);
  br->ui.csv_file_opened = brps_string_load(br, &copy, copy.ui.csv_file_opened);
  br->ui.font_path_id = brps_string_load(br, &copy, copy.ui.font_path_id);
  br->ui.project_path_id = brps_string_load(br, &copy, copy.ui.project_path_id);

error:
  if (file != NULL) BR_FCLOSE(file);
  brps_copy_deinit(copy);
  br_str_free(tmp);
  return success;
}

