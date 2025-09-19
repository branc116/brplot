#include "src/br_pp.h"
#include "src/br_plotter.h"
#include "src/br_str.h"
#include "src/br_plot.h"
#include "src/br_filesystem.h"
#include "src/br_resampling.h"
#include "src/br_permastate.h"
#include "src/br_data.h"
#include "src/br_data_generator.h"
#include "src/br_da.h"
#include "src/br_free_list.h"
#include "src/br_string_pool.h"
#include "src/br_ui.h"
#include "src/br_tl.h"
#include "src/br_memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#  include <unistd.h>
#endif

bool br_permastate_savef_plots(FILE* f, br_plots_t plots) {
  size_t plots_len                = (size_t)plots.len;
  br_save_state_command_t command = br_save_state_command_save_plots;
  uint32_t crc                    = 0;
  bool success                    = true;

  if (NULL == f)                                                         BR_ERROR("File is null: %s", strerror(errno));
  if (1 != fwrite(&command, sizeof(command), 1, f))                      BR_ERROR("Failed to write command: %s", strerror(errno));
  if (1 != fwrite(&plots_len, sizeof(plots_len), 1, f))                  BR_ERROR("Failed to write plots len: %s", strerror(errno));

  if (plots_len != fwrite(plots.arr, sizeof(*plots.arr), plots_len, f))  BR_ERROR("Failed to write plots: %s", strerror(errno));
  crc = br_fs_crc(plots.arr, sizeof(*plots.arr) * plots_len, 0);

  for (int i = 0; i < plots.len; ++i) {
    br_plot_t* plot = &plots.arr[i];
    br_plot_data_t* arr = plot->data_info.arr;
    int len = plot->data_info.len;
    if (1 != fwrite(&len, sizeof(len), 1, f))                            BR_ERROR("Failed to write data_info len i=%d: %s", i, strerror(errno));
    if (len == 0) continue;
    if (len != (int32_t)fwrite(arr, sizeof(*arr), (uint32_t)len, f))     BR_ERROR("Failed to write data_info i=%d: %s", i, strerror(errno));
    crc = br_fs_crc(arr, sizeof(*arr) * (size_t)len, crc);
  }
  if (1 != fwrite(&crc, sizeof(crc), 1, f))                              BR_ERROR("Failed to write crc %s", strerror(errno));
  goto end;

error:
  success = false;

end:
  return success;
}

bool br_permastate_save_plots(br_str_t path_folder, br_plots_t plots) {
  char buff[512]; buff[0]         = '\0';
  FILE* f                         = NULL;
  bool success                    = true;

  if (false == br_fs_cd(&path_folder, br_strv_from_literal("plots.br"))) BR_ERROR("Failed to navigate to a file");
  br_str_to_c_str1(path_folder, buff);
  f = fopen(buff, "wb");
  if (false == br_permastate_savef_plots(f, plots))                      BR_ERROR("Failed to save plots");
  goto done;

error:
  success = false;

done:
  if (NULL != f) fclose(f);
  br_str_free(path_folder);
  return success;
}

bool br_permastate_savef_data(FILE* file, br_dagens_t const* dagens, br_data_t* data) {
  br_save_state_command_t command = 0;
  bool success = true;

  if (br_data_is_generated(dagens, data->group_id)) return true;
  switch (data->kind) {
    case br_data_kind_2d: command = br_save_state_command_save_data_2d; break;
    case br_data_kind_3d: command = br_save_state_command_save_data_3d; break;
    default: BR_UNREACHABLE("Data kind %d", data->kind);
  }
  if (NULL == file)                                                                      BR_ERRORE("File is null");
  if (1 != fwrite(&command, sizeof(command), 1, file))                                   BR_ERRORE("Failed to write command");
  if (1 != fwrite(&data->color, sizeof(data->color), 1, file))                           BR_ERRORE("Failed to write color");
  if (1 != fwrite(&data->len, sizeof(data->len), 1, file))                               BR_ERRORE("Failed to write length");
  switch (data->kind) {
    case br_plot_kind_2d: {
      if (1 != fwrite(&data->dd.bounding_box, sizeof(data->dd.bounding_box), 1, file))   BR_ERRORE("Failed to write bounding box");
      if (1 != fwrite(&data->dd.rebase_x, sizeof(data->dd.rebase_x), 1, file))           BR_ERRORE("Failed to write rebase x");
      if (1 != fwrite(&data->dd.rebase_y, sizeof(data->dd.rebase_y), 1, file))           BR_ERRORE("Failed to write rebase y");
      if (0 != data->len) {
        if (data->len != fwrite(data->dd.xs, sizeof(*data->dd.xs), data->len, file))     BR_ERRORE("Failed to write xs");
        if (data->len != fwrite(data->dd.ys, sizeof(*data->dd.ys), data->len, file))     BR_ERRORE("Failed to write ys");
      }
    } break;
    case br_plot_kind_3d: {
      if (1 != fwrite(&data->ddd.bounding_box, sizeof(data->ddd.bounding_box), 1, file)) BR_ERRORE("Failed to write bounding box");
      if (1 != fwrite(&data->ddd.rebase_x, sizeof(data->ddd.rebase_x), 1, file))         BR_ERRORE("Failed to write rebase x");
      if (1 != fwrite(&data->ddd.rebase_y, sizeof(data->ddd.rebase_y), 1, file))         BR_ERRORE("Failed to write rebase y");
      if (1 != fwrite(&data->ddd.rebase_z, sizeof(data->ddd.rebase_z), 1, file))         BR_ERRORE("Failed to write rebase z");
      if (0 != data->len) {
        if (data->len != fwrite(data->ddd.xs, sizeof(*data->ddd.xs), data->len, file))   BR_ERRORE("Failed to write xs");
        if (data->len != fwrite(data->ddd.ys, sizeof(*data->ddd.ys), data->len, file))   BR_ERRORE("Failed to write ys");
        if (data->len != fwrite(data->ddd.zs, sizeof(*data->ddd.zs), data->len, file))   BR_ERRORE("Failed to write zs");
      }
    } break;
    default: BR_UNREACHABLE("Data kind %d", data->kind);
  }
  goto done;

error:
  success = false;

done:
  return success;
}

bool br_permastate_save_datas(br_str_t path_folder, br_dagens_t const* dagens, br_datas_t datas) {
  char buff[512]; buff[0] = '\0';
  FILE* file = NULL;
  bool success = true;

  for (size_t i = 0; i < datas.len; ++i) {
    br_data_t* data = &datas.arr[i];
    if (br_data_is_generated(dagens, data->group_id)) continue;
    if (false == br_fs_cd(&path_folder, br_strv_from_literal("data")))        BR_ERROR("Failed to change dir");
    if (false == br_str_push_int(&path_folder, data->group_id))               BR_ERROR("Failed to push int");
    if (false == br_str_push_strv(&path_folder, br_strv_from_literal(".br"))) BR_ERROR("Failed to push strv");
    br_str_to_c_str1(path_folder, buff);
    if (NULL == (file = fopen(buff, "wb")))                                   BR_ERRORE("Failed to open file %s", buff);
    if (false == br_permastate_savef_data(file, dagens, data))                BR_ERROR("Failed to write data %zu", i);
    fclose(file);
    file = NULL;
    buff[0] = '\0';
    br_fs_up_dir(&path_folder);
  }
  goto done;

error:
  success = false;

done:
  if (NULL != file) fclose(file);
  br_str_free(path_folder);
  return success;
}

static bool br_permastate_savef_plotter(FILE* file, br_plotter_t* br) {
  bool success = true;
  int fl_write_error = 0;
  br_save_state_command_t command = br_save_state_command_plotter;

  if (NULL == file)                                                          BR_ERRORE("File is NULL");
  if (1 != fwrite(&command, sizeof(command), 1, file))                       BR_ERRORE("Failed to write command");
  if (1 != fwrite(&br->groups.len, sizeof(br->groups.len), 1, file))         BR_ERRORE("Failed to write groups len");
  for (size_t i = 0; i < br->groups.len; ++i) {
    br_data_t* data = &br->groups.arr[i];
    if (1 != fwrite(&data->group_id, sizeof(data->group_id), 1, file))       BR_ERRORE("Failed to write group id");
    if (1 != fwrite(&data->name, sizeof(data->name), 1, file))               BR_ERRORE("Failed to write name id");
  }
  if (1 != fwrite(&br->ui, sizeof(br->ui), 1, file))                         BR_ERRORE("Failed to write UI.");
  brui_resizable_temp_delete_all();
  brfl_write(file, br->resizables, fl_write_error); if (fl_write_error != 0) BR_ERRORE("Failed to write resizables.");
  if (false == brsp_write(file, brtl_brsp()))                                BR_ERRORE("Failed to write string pool.");
  goto done;

error:
  success = false;

done:
  return success;
}

bool br_permastate_save_plotter(br_str_t path_folder, br_plotter_t* br) {
  char buff[512]; buff[0] = '\0';
  FILE* file = NULL;
  bool success = true;

  if (false == br_fs_cd(&path_folder, br_strv_from_literal("plotter.br"))) BR_ERROR("Failed to change directory");
  br_str_to_c_str1(path_folder, buff);
  if (NULL == (file = fopen(buff, "wb")))                                  BR_ERRORE("Failed to open file %s", buff);
  if (false == br_permastate_savef_plotter(file, br))                      BR_ERROR("Failed to save plotter");
  goto done;

error:
  success = false;

done:
  if (NULL != file) fclose(file);
  br_str_free(path_folder);
  return success;
}

void br_permastate_save(br_plotter_t* br) {
  char buff[512]               /* = uninitialized */;
  br_str_t path                   = {0};
  bool success                    = true;
  (void)success;

#if defined(FUZZ)
  goto error;
#endif
  if (false == br_fs_get_config_dir(&path))                                          BR_ERROR("Failed get config path");
  br_str_to_c_str1(path, buff);
  if (false == br_fs_mkdir(br_str_sub1(path, 0)))                                    BR_ERROR("Failed to create directory");
  if (false == br_permastate_save_plots(br_str_copy(path), br->plots))               BR_ERROR("Failed to save plots");
  if (false == br_permastate_save_datas(br_str_copy(path), &br->dagens, br->groups)) BR_ERROR("Failed to save data");
  if (false == br_permastate_save_plotter(br_str_copy(path), br))                    BR_ERROR("Failed to save plotter");
  goto done;

error:
  success = false;

done:
  br_str_free(path);
  return;
}

bool br_permastate_save_as(br_plotter_t* br, const char* path_to) {
  FILE* f           = NULL;
  bool success      = true;

  if (NULL == (f = fopen(path_to, "wb")))                                      BR_ERROR("Failed to open a file %s: %s", path_to, strerror(errno));
  if (false == br_permastate_savef_plots(f, br->plots))                        BR_ERROR("Failed to save plots");
  for (size_t i = 0; i < br->groups.len; ++i) {
    if (false == br_permastate_savef_data(f, &br->dagens, &br->groups.arr[i])) BR_ERROR("Failed to save data %zu", i);
  }
  if (false == br_permastate_savef_plotter(f, br))                             BR_ERROR("Failed to plotter");
  goto done;

error:
  success = false;

done:
  if (NULL != f) fclose(f);
  return success;
}

bool br_permastate_remove_pointers(br_plot_t* plot) {
  plot->data_info.cap = 0;
  plot->data_info.len = 0;
  plot->data_info.arr = NULL;
  return true;
}

bool br_permastate_load_plotter(FILE* file, br_plotter_t* br, br_data_descs_t* desc) {
  size_t datas_len = 0;
  size_t uis_read = 0;
  int fl_read_error = 0;
  int success = true;
  (void)success;

  if (1 != fread(&datas_len, sizeof(datas_len), 1, file))                 BR_ERROR("Failed to read number of datas");
  br_da_reserve(*desc, datas_len);
  if (datas_len != fread(desc->arr, sizeof(desc->arr[0]), datas_len, file)) BR_ERROR("Failed to read datas.");
  desc->len = datas_len;

  if (1 != (uis_read = fread(&br->ui, sizeof(br->ui), 1, file)))          BR_ERROR("Failed to read UI state.");
  brfl_read(file, br->resizables, fl_read_error); if (fl_read_error != 0) BR_ERROR("Failed to read resizables.");
  if (false == brsp_read(file, brtl_brsp()))                              BR_ERROR("Failed to read string pool.");
  if (0 != feof(file))                                                    BR_ERROR("Expected eof.");
  brsp_compress(brtl_brsp(), 1.3f, 16);
  return true;

error:
  if (br && br->resizables.arr) brfl_free(br->resizables);
  return false;
}

bool br_permastate_load_plots(FILE* file, br_plotter_t* br) {
  size_t plots_len        = 0;
  uint32_t read_crc       = 0;
  uint32_t calculated_crc = 0;
  br_plot_t* plots        = NULL;
  size_t read_plots       = 0;

  if (1 != fread(&plots_len, sizeof(plots_len), 1, file))                          goto error;
  if (plots_len != 0) {
    if (NULL == (plots = BR_MALLOC(sizeof(*plots) * plots_len)))                   goto error;
    if (plots_len != (read_plots = fread(plots, sizeof(*plots), plots_len, file))) goto error;
  }
  if (br->plots.arr) BR_FREE(br->plots.arr);
  br->plots.arr = plots;
  br->plots.len = br->plots.cap = (int)plots_len;
  if (plots_len != 0) calculated_crc = br_fs_crc(plots, sizeof(*plots) * (size_t)plots_len, 0);
  for (size_t i = 0; i < plots_len; ++i) {
    if (false == br_permastate_remove_pointers(&plots[i]))                         goto error;
  }
  for (size_t i = 0; i < plots_len; ++i) {
    br_plot_t* p = &plots[i];
    int len = 0;
    br_plot_data_t* arr = NULL;
    if (1 != fread(&len, sizeof(len), 1, file))                                    goto error;
    if (len == 0) continue;
    p->data_info.len = len;
    p->data_info.cap = len;
    arr = BR_MALLOC(sizeof(*arr) * (size_t)len);
    p->data_info.arr = arr;
    if ((uint32_t)len != fread(arr, sizeof(*arr), (size_t)len, file))              goto error;
    calculated_crc = br_fs_crc(arr, sizeof(*arr) * (size_t)len, calculated_crc);
  }
  if (plots_len != 0) {
    if (1 != fread(&read_crc, sizeof(read_crc), 1, file))                          goto error;
    if (calculated_crc != read_crc)                                                goto error;
  }
  return true;

error:
  if (0 == plots_len)                  LOGI("Failed to read a file: %d(%s)", errno, strerror(errno));
  else if (NULL == plots)              LOGI("Failed to allocated memory for array of %zu plots", plots_len);
  else if (read_plots != plots_len)    LOGI("Failed to read right amount of plots, wanted %zu, but got %zu", plots_len, read_plots);
  else if (calculated_crc != read_crc) LOGE("Crc check failed expected %u, but got %u", calculated_crc, read_crc);
  return false;
}

br_permastate_status_t br_permastate_load(br_plotter_t* br_initial) {
  char buff[512]               /* = uninitialized */;
  br_str_t path                   = {0};
  FILE* f                         = NULL;
  br_save_state_command_t command = br_save_state_command_save_plots;
  bool file_exists                = false;
  br_permastate_status_t status   = br_permastate_status_ok;
  br_data_descs_t descs           = {0};
  bool success                    = true;
  br_plotter_t loaded_br          = *br_initial;

#if defined(FUZZ)
  goto error;
#endif

  if (false == br_fs_get_config_dir(&path))                           BR_ERROR("Failed to get a config dir.");
  if (false == br_fs_cd(&path, BR_STRL("plotter.br")))                BR_ERROR("Failed to navigate to brplotter.br file.");
  {
    br_str_to_c_str1(path, buff);
    if (false == (file_exists = br_fs_exists(br_str_sub1(path, 0))))  BR_ERROR("plotter.br doesn't exist: %.*s: %s", path.len, path.str, strerror(errno));
    f = fopen(buff, "rb");
    if (NULL == f)                                                    BR_ERROR("Failed to open a file: %s", strerror(errno));
    if (1 != fread(&command, sizeof(command), 1, f))                  BR_ERROR("Failed to read a command: %s", strerror(errno));
    if (br_save_state_command_plotter != command)                     BR_ERROR("Bad command type: %d", command);
    if (false == br_permastate_load_plotter(f, &loaded_br, &descs))   BR_ERROR("Failed to load plotter");
    fclose(f);
    f = NULL;
  }

#if defined(BR_LIB)
  status = br_permastate_status_ui_loaded;
  goto end;
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
  if (ttyname(STDIN_FILENO) == NULL) {
    status = br_permastate_status_ui_loaded;
    goto end;
  }
#endif

  if (false == br_fs_up_dir(&path))                                   BR_ERROR("Failed to go up a dir");
  if (false == br_fs_cd(&path, br_strv_from_literal("plots.br")))     BR_ERROR("Failed to navigate to plots.br");
  {
    br_str_to_c_str1(path, buff);
    if (false == (file_exists = br_fs_exists(br_str_sub1(path, 0))))  BR_ERROR("File doesn't exist: %.*s", path.len, path.str);
    f = fopen(buff, "rb");
    if (NULL == f)                                                    BR_ERROR("Failed to open a file %.*s: %s", path.len, path.str, strerror(errno));
    if (1 != fread(&command, sizeof(command), 1, f))                  BR_ERROR("Failed to read a command: %s", strerror(errno));
    if (br_save_state_command_save_plots != command)                  BR_ERROR("Bad command type: %d", command);
    if (false == br_permastate_load_plots(f, &loaded_br))             BR_ERROR("Failed to load plots");
    fclose(f);
    f = NULL;
  }

  if (false == br_fs_up_dir(&path))                                   BR_ERROR("Failed to go up a dir");
  for (size_t i = 0; i < descs.len; ++i) {
    if (false == br_fs_cd(&path, br_strv_from_literal("data")))       BR_ERROR("Failed to navigate to data.br");
    if (false == br_str_push_int(&path, br_da_get(descs, i).group_id))BR_ERROR("Failed to navigate to data.br");
    if (false == br_str_push_literal(&path, ".br"))                   BR_ERROR("Failed to navigate to data.br");
    br_str_to_c_str1(path, buff);
    file_exists = br_fs_exists(br_str_as_view(path));
    if (false == file_exists || false == br_dagen_push_file(&loaded_br.dagens, &loaded_br.groups, &descs.arr[i], fopen(buff, "rb"))) {
      for (int j = 0; j < loaded_br.plots.len; ++j) {
        br_da_remove_feeld(loaded_br.plots.arr[j].data_info, group_id, descs.arr[i].group_id);
      }
    }
    if (false == br_fs_up_dir(&path))                                 BR_ERROR("Failed to go up a dir");
  }
  goto end;

error:
  status = br_permastate_status_failed;
  if (false == success) memset(&descs, 0, sizeof(descs));

end:
  if (NULL != path.str) br_str_free(path);
  if (NULL != f)        fclose(f);
  if (NULL != descs.arr)br_da_free(descs);
  if (true == success) *br_initial = loaded_br;
  return status;
}

