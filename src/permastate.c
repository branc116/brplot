#include "src/br_plotter.h"
#include "src/br_str.h"
#include "src/br_plot.h"
#include "src/br_pp.h"
#include "src/br_filesystem.h"
#include "src/br_resampling2.h"
#include "src/br_permastate.h"
#include "src/br_data.h"
#include "src/br_data_generator.h"
#include "src/br_da.h"
#include "src/br_ui.h"

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

bool br_permastate_save_plots(br_str_t path_folder, br_plots_t plots) {
  char buff[512]; buff[0]         = '\0';
  FILE* f                         = NULL;
  size_t plots_len                = (size_t)plots.len;
  br_save_state_command_t command = br_save_state_command_save_plots;
  uint32_t crc                    = 0;
  bool success                    = true;

  if (false == br_fs_cd(&path_folder, br_strv_from_literal("plots.br"))) goto error;
  br_str_to_c_str1(path_folder, buff);
  f = fopen(buff, "wb");
  if (NULL == f)                                                         goto error;
  if (1 != fwrite(&command, sizeof(command), 1, f))                      goto error;
  if (1 != fwrite(&plots_len, sizeof(plots_len), 1, f))                  goto error;
  if (plots_len != fwrite(plots.arr, sizeof(*plots.arr), plots_len, f))  goto error;
  crc = br_fs_crc(plots.arr, sizeof(*plots.arr) * plots_len, 0);

  for (int i = 0; i < plots.len; ++i) {
    br_plot_t* plot = &plots.arr[i];
    int* arr = plot->groups_to_show.arr;
    int len = plot->groups_to_show.len;
    if (1 != fwrite(&len, sizeof(len), 1, f))                            goto error;
    if (len == 0) continue;
    if (len != (int32_t)fwrite(arr, sizeof(*arr), (uint32_t)len, f))    goto error;
    crc = br_fs_crc(arr, sizeof(*arr) * (size_t)len, crc);
  }
  if (1 != fwrite(&crc, sizeof(crc), 1, f))                              goto error;
  goto end;

error:
  if ('\0' == buff[0]) LOGI("Failed to allocate memory for plots path\n");
  else if (NULL == f) LOGI("Failed to open a file %s: %d(%s)", buff, errno, strerror(errno));
  else LOGI("Failed to write to file %s: %d(%s)", buff, errno, strerror(errno));
  success = false;

end:
  if (NULL != f) fclose(f);
  br_str_free(path_folder);
  return success;
}

bool br_permastate_save_datas(br_str_t path_folder, br_dagens_t const* dagens, br_datas_t datas) {
  char buff[512]; buff[0] = '\0';
  FILE* file = NULL;
  br_save_state_command_t command;
  bool success = true;
  uint32_t crc = 0;

  for (size_t i = 0; i < datas.len; ++i) {
    br_data_t* data = &datas.arr[i];
    if (br_data_is_generated(dagens, data->group_id)) continue;
    switch (data->kind) {
      case br_data_kind_2d: command = br_save_state_command_save_data_2d; break;
      case br_data_kind_3d: command = br_save_state_command_save_data_3d; break;
      default: BR_ASSERT(0);
    }
    if (false == br_fs_cd(&path_folder, br_strv_from_literal("data")))                     goto error;
    if (false == br_str_push_int(&path_folder, data->group_id))                            goto error;
    if (false == br_str_push_br_strv(&path_folder, br_strv_from_literal(".br")))           goto error;
    br_str_to_c_str1(path_folder, buff);
    if (NULL == (file = fopen(buff, "wb")))                                                goto error;
    if (1 != fwrite(&command, sizeof(command), 1, file))                                   goto error;
    if (1 != fwrite(&data->color, sizeof(data->color), 1, file))                           goto error;
    if (1 != fwrite(&data->len, sizeof(data->len), 1, file))                               goto error;
    switch (data->kind) {
      case br_plot_kind_2d: {
        if (1 != fwrite(&data->dd.bounding_box, sizeof(data->dd.bounding_box), 1, file))   goto error;
        if (0 != data->len) {
          if (data->len != fwrite(data->dd.xs, sizeof(*data->dd.xs), data->len, file))     goto error;
          if (data->len != fwrite(data->dd.ys, sizeof(*data->dd.ys), data->len, file))     goto error;
        }
        if (1 != fwrite(&crc, sizeof(crc), 1, file))                                       goto error;
      } break;
      case br_plot_kind_3d: {
        if (1 != fwrite(&data->ddd.bounding_box, sizeof(data->ddd.bounding_box), 1, file)) goto error;
        if (0 != data->len) {
          if (data->len != fwrite(data->ddd.xs, sizeof(*data->ddd.xs), data->len, file))   goto error;
          if (data->len != fwrite(data->ddd.ys, sizeof(*data->ddd.ys), data->len, file))   goto error;
          if (data->len != fwrite(data->ddd.zs, sizeof(*data->ddd.zs), data->len, file))   goto error;
        }
      } break;
      default: BR_ASSERT(0);
    }
    fclose(file);
    file = NULL;
    buff[0] = '\0';
    br_fs_up_dir(&path_folder);
  }
  goto end;

error:
  if (buff[0] == '\0')  LOGI("Failed to allocatate memory from the plots permastate path\n");
  else if (file == NULL)LOGI("Failed to open a file %s: %d(%s)\n", buff, errno, strerror(errno));
  else                  LOGI("Failed to write to a file %s: %d(%s)\n", buff, errno, strerror(errno));
  success = false;

end:
  if (NULL != file) fclose(file);
  br_str_free(path_folder);
  return success;
}

bool br_permastate_save_plotter(br_str_t path_folder, br_plotter_t* br) {
  char buff[512]; buff[0] = '\0';
  FILE* file = NULL;
  br_save_state_command_t command = br_save_state_command_plotter;
  bool success = true;

  if (false == br_fs_cd(&path_folder, br_strv_from_literal("plotter.br")))                       goto error;
  br_str_to_c_str1(path_folder, buff);
  if (NULL == (file = fopen(buff, "wb")))                                                        goto error;
  if (1 != fwrite(&command, sizeof(command), 1, file))                                           goto error;
  if (1 != fwrite(&br->groups.len, sizeof(br->groups.len), 1, file))                             goto error;
  for (size_t i = 0; i < br->groups.len; ++i) {
    br_data_t* data = &br->groups.arr[i];
    if (1 != fwrite(&data->group_id, sizeof(data->group_id), 1, file))                           goto error;
    if (1 != fwrite(&data->name.len, sizeof(data->name.len), 1, file))                           goto error;
    if (data->name.len != fwrite(data->name.str, sizeof(*data->name.str), data->name.len, file)) goto error;
  }
  if (1 != fwrite(&br->active_plot_index, sizeof(br->active_plot_index), 1, file))               goto error;
  goto end;

error:
  if (buff[0] == '\0')  LOGI("Failed to allocatate memory from the plots permastate path\n");
  else if (file == NULL)LOGI("Failed to open a file %s: %d(%s)\n", buff, errno, strerror(errno));
  else                  LOGI("Failed to write to a file %s: %d(%s)\n", buff, errno, strerror(errno));
  success = false;

end:
  if (NULL != file) fclose(file);
  br_str_free(path_folder);
  return success;
}

void br_permastate_save(br_plotter_t* br) {
  char buff[512]               /* = uninitialized */;
  br_str_t path                   = {0};
  for (size_t i = 0; i < br->dagens.len; ++i) {
    if (br->dagens.arr[i].kind == br_dagen_kind_file) return;
  }

  if (false == br_fs_get_config_dir(&path))                                          goto error;
  br_str_to_c_str1(path, buff);
  if (false == br_fs_mkdir(br_str_sub1(path, 0)))                                    goto error;
  if (false == br_permastate_save_plots(br_str_copy(path), br->plots))               goto error;
  if (false == br_permastate_save_datas(br_str_copy(path), &br->dagens, br->groups)) goto error;
  if (false == br_permastate_save_plotter(br_str_copy(path), br))                    goto error;
  goto end;

error:
  if (path.str == NULL) LOGI("Failed to allocatate memory from the plots permastate path\n");
  LOGI("Failed to save state to permastate: %d(%s)\n", errno, strerror(errno));

end:
  br_str_free(path);
  return;
}

void br_permastate_remove_pointers(br_plotter_t* br, br_plot_t* plot) {
  plot->groups_to_show.cap = 0;
  plot->groups_to_show.len = 0;
  plot->groups_to_show.arr = NULL;
  switch (plot->kind) {
    case br_plot_kind_2d: br->any_2d = true; break;
    case br_plot_kind_3d: br->any_3d = true; break;
    default: assert(0);
  }
}

bool br_permastate_load_plotter(FILE* file, br_plotter_t* br, br_data_descs_t* desc) {
  size_t datas_len = 0;
  if (1 != fread(&datas_len, sizeof(datas_len), 1, file))                        goto error;
  if (datas_len == 0) return true;
  for (size_t i = 0; i < datas_len; ++i) {
    int id = 0;
    uint32_t len = 0;
    char* str = NULL;
    if (1 != fread(&id, sizeof(id), 1, file))                                    goto error;
    if (1 != fread(&len, sizeof(len), 1, file))                                  goto error; 
    if (len != 0) {
      if (NULL == (str = BR_MALLOC(len * sizeof(*str))))                         goto error;
      if (len != fread(str, sizeof(*str), len, file))                            goto error;
    }
    br_data_desc_t d = { .group_id = id, .name = { .str = str, .len = len, .cap = len } };
    br_da_push(*desc, d);
  }
  if (1 != fread(&br->active_plot_index, sizeof(br->active_plot_index), 1, file)) goto error;
  return true;
  
error:
  LOGI("Failed to read plotter %d(%s)\n", errno, strerror(errno));
  return false;
}

bool br_permastate_load_plots(FILE* file, br_plotter_t* br) {
  size_t plots_len                = 0;
  uint32_t read_crc               = 0;
  uint32_t calculated_crc         = 0;
  br_plot_t* plots                = NULL;
  size_t read_plots               = 0;

  if (1 != fread(&plots_len, sizeof(plots_len), 1, file))                        goto error;
  if (NULL == (plots = BR_MALLOC(sizeof(*plots) * plots_len)))                   goto error;
  if (plots_len != (read_plots = fread(plots, sizeof(*plots), plots_len, file))) goto error;
  BR_FREE(br->plots.arr);
  br->plots.arr = plots;
  br->plots.len = br->plots.cap = (int)plots_len;
  calculated_crc = br_fs_crc(plots, sizeof(*plots) * (size_t)plots_len, 0);
  for (size_t i = 0; i < plots_len; ++i) {
    br_permastate_remove_pointers(br, &plots[i]);
  }
  for (size_t i = 0; i < plots_len; ++i) {
    br_plot_t* p = &plots[i];
    int len = 0;
    int* arr = NULL;
    if (1 != fread(&len, sizeof(len), 1, file))                                  goto error;
    if (len == 0) continue;
    p->groups_to_show.len = len;
    p->groups_to_show.cap = len;
    arr = BR_MALLOC(sizeof(*arr) * (size_t)len);
    p->groups_to_show.arr = arr;
    if ((uint32_t)len != fread(arr, sizeof(*arr), (size_t)len, file))            goto error;
    calculated_crc = br_fs_crc(arr, sizeof(*arr) * (size_t)len, calculated_crc);
  }
  if (1 != fread(&read_crc, sizeof(read_crc), 1, file))                          goto error;
  if (calculated_crc != read_crc)                                                goto error;

  for (size_t i = 0; i < plots_len; ++i) {
    plots[i].extent_handle = brui_resizable_new(plots[i].cur_extent, 0);
    plots[i].menu_extent_handle = brui_resizable_new(BR_EXTENTI(0, 0, 300, plots[i].cur_extent.height), plots[i].extent_handle);
  }

  return true;

error:
  if (0 == plots_len)                  LOGI("Failed to read a file: %d(%s)\n", errno, strerror(errno));
  else if (NULL == plots)              LOGI("Failed to allocated memory for array of %zu plots\n", plots_len);
  else if (read_plots != plots_len)    LOGI("Failed to read right amount of plots, wanted %zu, but got %zu\n", plots_len, read_plots);
  else if (calculated_crc != read_crc) LOGE("Crc check failed expected %u, but got %u\n", calculated_crc, read_crc);
  return false;
}

bool br_permastate_load(br_plotter_t* br) {
  char buff[512]               /* = uninitialized */;
  br_str_t path                   = {0};
  FILE* f                         = NULL;
  br_save_state_command_t command = br_save_state_command_save_plots;
  bool file_exists                = false;
  bool success                    = true;
  br_data_descs_t descs           = {0};

  if (false == br_fs_get_config_dir(&path))                           goto error;
  if (false == br_fs_cd(&path, br_strv_from_literal("plotter.br")))   goto error;
  {
    br_str_to_c_str1(path, buff);
    if (false == (file_exists = br_fs_exists(br_str_sub1(path, 0))))  goto error;
    f = fopen(buff, "rb");
    if (NULL == f)                                                    goto error;
    if (1 != fread(&command, sizeof(command), 1, f))                  goto error;
    if (br_save_state_command_plotter != command)                     goto error;
    if (false == br_permastate_load_plotter(f, br, &descs))           goto error;
    fclose(f);
    f = NULL;
  }

  if (false == br_fs_up_dir(&path))                                   goto error;
  if (false == br_fs_cd(&path, br_strv_from_literal("plots.br")))     goto error;
  {
    br_str_to_c_str1(path, buff);
    if (false == (file_exists = br_fs_exists(br_str_sub1(path, 0))))  goto error;
    f = fopen(buff, "rb");
    if (NULL == f)                                                    goto error;
    if (1 != fread(&command, sizeof(command), 1, f))                  goto error;
    if (br_save_state_command_save_plots != command)                  goto error;
    if (false == br_permastate_load_plots(f, br))                     goto error;
    fclose(f);
    f = NULL;
  }

  if (false == br_fs_up_dir(&path))                                   goto error;
  for (size_t i = 0; i < descs.len; ++i) {
    if (false == br_fs_cd(&path, br_strv_from_literal("data")))       goto error;
    if (false == br_str_push_int(&path, descs.arr[i].group_id))       goto error;
    if (false == br_str_push_literal(&path, ".br"))                   goto error;
    br_str_to_c_str1(path, buff);
    file_exists = br_fs_exists(br_str_as_view(path));
    if (false == file_exists || false == br_dagen_push_file(&br->dagens, &br->groups, &descs.arr[i], fopen(buff, "rb"))) {
      for (int j = 0; j < br->plots.len; ++j) {
        br_da_remove(br->plots.arr[j].groups_to_show, descs.arr[i].group_id);
      }
    }
    if (false == br_fs_up_dir(&path))                                 goto error;
  }
  goto end;

error:
  if (path.str == NULL)          LOGI("Failed to allocatate memory from the plots permastate path\n");
  else if (false == file_exists) LOGI("Tried to open a file that doesn't exisit `%s`\n", buff);
  else if (f == NULL)            LOGI("Failed to open a file %s: %d(%s)\n", buff, errno, strerror(errno));
  else                           LOGI("Failed loading permastate %d(%s)\n", errno, strerror(errno));
  success = false;

end:
  if (NULL != path.str) br_str_free(path);
  if (NULL != f)        fclose(f);
  for (size_t i = 0; i < descs.len; ++i) {
    br_str_t name = descs.arr[i].name;
    if (NULL != name.str) br_str_free(name);
  }
  br_da_free(descs);
  return success;
}

