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

typedef struct br_data_save_t {
  // Both project and editor
  brsp_t sp;
  br_anims_t anims;
  bruirs_t resizables;

  // Only project
  br_plots_t plots;
  br_dagens_t dagens;
  br_datas_t datas;

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

  for (int i = 0; i < copy.datas.len; ++i) {
    br_data_t data = br_da_get(copy.datas, i);
    if (data.len == 0) continue;
    struct {
      float* arr;
      size_t len;
    } floats = { .len = data.len };
    LOGI("Start of data %d= %zd", i, ftell(file));
    switch (data.kind) {
      case br_plot_kind_2d: {
        floats.arr = data.dd.xs; br_da_write(file, floats);
        floats.arr = data.dd.ys; br_da_write(file, floats);
      } break;
      case br_plot_kind_3d: {
        floats.arr = data.ddd.xs; br_da_write(file, floats);
        floats.arr = data.ddd.ys; br_da_write(file, floats);
        floats.arr = data.ddd.zs; br_da_write(file, floats);
      } break;
      default: BR_UNREACHABLE("Data kind %d", data.kind);
    }
  }

error:
  brps_copy_deinit(copy);

  return success;
}

static bool brps_data_read(const char* file_path, long long* seek_pos, brps_copy_t* copy, int data_index) {
  size_t to_add = 0, len = 0;
  BR_FILE* fi = NULL;
  bool success = true;
  br_data_t* data = NULL;

  data = br_da_getp(copy->datas, data_index);
  len = data->len;
  if (len == 0) {
    if (false == br_data_malloc_axis(data, data->len)) BR_ERROR("Failed to alloc axis");
    return true;
  }
  if (false == br_data_malloc_axis(data, data->len)) BR_ERROR("Failed to alloc axis");
  fi = BR_FOPEN(file_path, "rb");
  fseek(fi, *seek_pos, SEEK_SET);
  if (false == br_dagen_push_file(&copy->dagens, data, fi)) BR_ERROR("Failed to push a file");

error:
  if (false == success) {
    br_data_free_axis(data);
  }
  to_add = 0;
  to_add += sizeof(size_t)+sizeof(float*)+sizeof(size_t)+sizeof(size_t); // HEADER
  to_add += sizeof(float)*len;                                           // DATA
  to_add += sizeof(size_t);                                              // FINAL_CHECK
  switch (data->kind) {
    case br_data_kind_2d: *seek_pos += to_add * 2; break;
    case br_data_kind_3d: *seek_pos += to_add * 3; break;
    default: BR_ERROR("Bad data kind %d", data->kind);
  }

  return success;
}

static bool brps_project_fread(BR_FILE* file, const char* file_path, br_plotter_t* br) {
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
  // |START....other stuff....data_header..data1|data2|data3...
  // data1 = [check: size_t, arr: float*, len: size_t, check: size_t, xs: float[len]][2 or 3 times]|
  long long end_of_data_header = ftell(file);
  long long seek_pos = end_of_data_header;

  for (int i = 0; i < copy.datas.len; ++i) {
    if (false == brps_data_read(file_path, &seek_pos, &copy, i)) {
      LOGE("Failed to push a file %d", i);
      brfl_remove(copy.datas, i);
      br_plots_remove_group(copy.plots, i);
    }
  }
  brps_plotter_load(br, &copy);
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_t* p = &br->plots.arr[i];
    br_extent_t ex = brui_resizable_cur_extent(p->extent_handle);
    p->texture_id = brgl_create_framebuffer((int)roundf(ex.width), (int)roundf(ex.height));
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

  if (false == brps_project_fread(file, file_name, br)) BR_ERROR("Failed to load the project..");

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

