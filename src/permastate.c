#include "br_plotter.h"
#include "br_str.h"
#include "br_plot.h"
#include "br_pp.h"
#include "br_filesystem.h"

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef enum {
  br_save_state_command_save_plot,
} br_save_state_command_t;

static bool br_permastate_get_home_dir(br_str_t* home) {
  return br_str_push_c_str(home, getenv("HOME"));
}

static bool br_permastate_get_config_dir(br_str_t* config) {
  if (false == br_permastate_get_home_dir(config)) return false;;
  return br_fs_cd(config, br_strv_from_literal(".config/brplot"));
}

static bool br_permastate_get_plots_state_file(br_str_t* plots_state) {
  if (false == br_permastate_get_config_dir(plots_state)) return false;
  return br_fs_cd(plots_state, br_strv_from_literal("plots.br"));
}

void br_permastate_save(br_plotter_t* br) {
  char buff[512]               /* = uninitialized */;
  br_str_t path                   = {0};
  FILE* f                         = NULL;
  size_t plots_len                = (size_t)br->plots.len;
  br_save_state_command_t command = br_save_state_command_save_plot;
  uint32_t crc                    = 0;

  if (false == br_permastate_get_plots_state_file(&path)) goto error;
  br_str_to_c_str1(path, buff);
  if (false == br_fs_up_dir(&path)) goto error;
  if (false == br_fs_mkdir(br_str_sub1(path, 0))) goto error;
  f = fopen(buff, "wb");
  if (NULL == f) goto error;
  if (1 != fwrite(&command, sizeof(command), 1, f)) goto error;
  if (1 != fwrite(&plots_len, sizeof(plots_len), 1, f)) goto error;
  if (plots_len != fwrite(br->plots.arr, sizeof(*br->plots.arr), plots_len, f)) goto error;
  crc = br_fs_crc(br->plots.arr, sizeof(*br->plots.arr) * plots_len);
  if (1 != fwrite(&crc, sizeof(crc), 1, f)) goto error;
  goto end;

error:
  if (path.str == NULL) LOGI("Failed to allocatate memory from the plots permastate path\n");
  else if (f == NULL)   LOGI("Failed to open a file %s: %d(%s)\n", buff, errno, strerror(errno));
  else                  LOGI("Failed to write to a file %s: %d(%s)\n", buff, errno, strerror(errno));

end:
  if (NULL != path.str) br_str_free(path);
  if (NULL != f) fclose(f);
  return;
}

void br_permastate_remove_pointers(br_plotter_t* br, br_plot_t* plot) {
  plot->groups_to_show.cap = 0;
  plot->groups_to_show.len = 0;
  plot->groups_to_show.arr = NULL;
  switch (plot->kind) {
    case br_plot_kind_2d:
      plot->dd.grid_shader = br->shaders.grid;
      plot->dd.line_shader = br->shaders.line;
      br->any_2d = true;
      break;
    case br_plot_kind_3d:
      plot->ddd.grid_shader = br->shaders.grid_3d;
      plot->ddd.line_shader = br->shaders.line_3d;
      plot->ddd.line_simple_shader = br->shaders.line_3d_simple;
      br->any_3d = true;
      break;
    default:
      assert(0);
  }
}

bool br_permastate_load(br_plotter_t* br) {
  char buff[512]               /* = uninitialized */;
  br_str_t path                   = {0};
  FILE* f                         = NULL;
  size_t plots_len                = 0;
  size_t read_plots               = 0;
  br_save_state_command_t command = br_save_state_command_save_plot;
  bool file_exists                = false;
  uint32_t read_crc               = 0;
  uint32_t calculated_crc         = 0;
  bool crc_ok                     = false;
  bool success                    = true;
  br_plot_t* plots                = NULL;

  if (false == br_permastate_get_plots_state_file(&path))                          goto error;
  br_str_to_c_str1(path, buff);
  if (false == (file_exists = br_fs_exists(br_str_sub1(path, 0))))                 goto error;
  f = fopen(buff, "rb");
  if (NULL == f)                                                                   goto error;
  if (1 != fread(&command, sizeof(command), 1, f))                                 goto error;
  switch (command) {
    case br_save_state_command_save_plot: {
      if (1 != fread(&plots_len, sizeof(plots_len), 1, f))                         goto error;
      plots = BR_MALLOC(sizeof(*plots) * plots_len);
      if (NULL == plots)                                                           goto error;
      if (plots_len != (read_plots = fread(plots, sizeof(*plots), plots_len, f)))  goto error;
      if (1 != fread(&read_crc, sizeof(read_crc), 1, f))                           goto error;
      calculated_crc = br_fs_crc(plots, sizeof(*plots) * plots_len);
      if (calculated_crc != read_crc)                                              goto error;
      BR_FREE(br->plots.arr);
      br->plots.arr = plots;
      br->plots.len = (int)plots_len;
      br->plots.cap = (int)plots_len;
      for (size_t i = 0; i < plots_len; ++i) {
        br_permastate_remove_pointers(br, &plots[i]);
      }
    } break;
    default: {
      LOGI("Unknown permastate command %d\n", command);
      goto error;
    }
  }
  goto end;

error:
  if (path.str == NULL)             LOGI("Failed to allocatate memory from the plots permastate path\n");
  else if (false == file_exists)    LOGI("Tried to open a file that doesn't exisit `%s`\n", buff);
  else if (f == NULL)               LOGI("Failed to open a file %s: %d(%s)\n", buff, errno, strerror(errno));
  else if (0 == plots_len)          LOGI("Failed to read a file %s: %d(%s)\n", buff, errno, strerror(errno));
  else if (NULL == plots)           LOGI("Failed to allocated memory for array of %lu plots\n", plots_len);
  else if (read_plots != plots_len) LOGI("Failed to read right amount of plots, wanted %lu, but got %lu\n", plots_len, read_plots);
  else if (calculated_crc != crc_ok)LOGI("Crc check failed expected %u, but got %u\n", calculated_crc, read_crc);
  success = false;

end:
  if (NULL != path.str) br_str_free(path);
  if (NULL != f) fclose(f);
  return success;
}

