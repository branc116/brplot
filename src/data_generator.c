#include "br_data_generator.h"
#include "br_data.h"
#include "br_permastate.h"
#include "br_da.h"
#include "br_pp.h"
#include "br_plotter.h"
#include "br_resampling2.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

inline static size_t min_s(size_t a, size_t b) { return a < b ? a : b; }
static size_t br_dagen_expr_add_to(br_datas_t datas, br_dagen_expr_t* expr, size_t offset, size_t n, float data[n]) {
  if (expr->kind == br_dagen_expr_kind_add) {
    size_t read = br_dagen_expr_add_to(datas, expr->operands.op1, offset, n, data);
    read = br_dagen_expr_add_to(datas, expr->operands.op1, offset, read, data);
    return read;
  }

  br_data_t* data_in = br_data_get1(datas, expr->group_id);
  if (data_in == NULL) return 0;
  float* fs = NULL;
  size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
  switch (expr->kind) {
    case br_dagen_expr_kind_reference_x: fs = &data_in->dd.xs[offset]; break;
    case br_dagen_expr_kind_reference_y: fs = &data_in->dd.ys[offset]; break;
    case br_dagen_expr_kind_reference_z: fs = &data_in->ddd.zs[offset]; break;
    default: BR_ASSERT(0);
  }
  for (size_t i = 0; i < real_n; ++i) {
    data[i] += fs[i];
  }
  return real_n;
}

static size_t br_dagen_expr_read_n(br_datas_t datas, br_dagen_expr_t* expr, size_t offset, size_t n, float data[n]) {
  if (expr->kind == br_dagen_expr_kind_add) {
    size_t read = br_dagen_expr_read_n(datas, expr->operands.op1, offset, n, data); 
    size_t added = br_dagen_expr_add_to(datas, expr, offset, read, data);
    return added;
  }

  br_data_t* data_in = br_data_get1(datas, expr->group_id);
  if (NULL == data_in) return 0;
  size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
  switch (expr->kind) {
    case br_dagen_expr_kind_reference_x: memcpy(data, &data_in->dd.xs[offset], real_n * sizeof(data[0])); break;
    case br_dagen_expr_kind_reference_y: memcpy(data, &data_in->dd.ys[offset], real_n * sizeof(data[0])); break;
    case br_dagen_expr_kind_reference_z: memcpy(data, &data_in->ddd.zs[offset], real_n * sizeof(data[0])); break;
    default: BR_ASSERT(0);
  }
  return real_n;
}

static size_t br_dagen_expr_len(br_datas_t datas, br_dagen_expr_t* expr) {
  switch (expr->kind) {
    case br_dagen_expr_kind_reference_x: 
    case br_dagen_expr_kind_reference_y: 
    case br_dagen_expr_kind_reference_z: 
    {
      br_data_t* data_in = br_data_get1(datas, expr->group_id);
      if (NULL == data_in) return 0;
      return data_in->len;
    }
    case br_dagen_expr_kind_add:
    {
      return min_s(br_dagen_expr_len(datas, expr->operands.op1), br_dagen_expr_len(datas, expr->operands.op2));
    }
    default: BR_ASSERT(0);
  }
}

void br_dagen_push_expr_xy(br_dagens_t* pg, br_datas_t* datas, br_dagen_expr_t x, br_dagen_expr_t y, int group) {
  if (NULL != br_data_get1(*datas, group)) {
    LOGI("Data with id %d already exists, will not create expr with the same id\n", group);
    return;
  }
  br_dagen_t dagen = {
    .kind = br_dagen_kind_expr,
    .data_kind = br_data_kind_2d,
    .state = br_dagen_state_inprogress,
    .group_id = group,
    .expr_2d = {
      .x_expr = x,
      .y_expr = y
    }
  };

  br_datas_create(datas, group, br_data_kind_2d);
  br_da_push(*pg, dagen);
}

bool br_dagen_push_file(br_dagens_t* dagens, br_data_t* temp_data, FILE* file) {
  if (file == NULL) goto error;
  br_save_state_command_t command = br_save_state_command_save_plots;
  size_t data_left = 0;

  if (1 != fread(&command, sizeof(command), 1, file)) goto error;
  switch (command) {
    case br_save_state_command_save_data_2d: temp_data->kind = br_data_kind_2d; break;
    case br_save_state_command_save_data_3d: temp_data->kind = br_data_kind_3d; break;
    default: goto error;
  }
  if (1 != fread(&temp_data->color, sizeof(temp_data->color), 1, file)) goto error;
  if (1 != fread(&temp_data->cap, sizeof(temp_data->cap), 1, file)) goto error;
  if (0 == temp_data->cap) {
    temp_data->cap = 8;
    switch (temp_data->kind) {
      case br_data_kind_2d: {
        temp_data->dd.xs = BR_MALLOC(sizeof(float) * 8);
        temp_data->dd.ys = BR_MALLOC(sizeof(float) * 8);
      } break;
      case br_data_kind_3d: {
        temp_data->ddd.xs = BR_MALLOC(sizeof(float) * 8);
        temp_data->ddd.ys = BR_MALLOC(sizeof(float) * 8);
        temp_data->ddd.zs = BR_MALLOC(sizeof(float) * 8);
      } break;
      default: BR_ASSERT(0);
    }
  } else {
    data_left = temp_data->cap;
    switch (temp_data->kind) {
      case br_data_kind_2d: {
        if (1 != fread(&temp_data->dd.bounding_box, sizeof(temp_data->dd.bounding_box), 1, file)) goto error;
        if (NULL == (temp_data->dd.xs = BR_MALLOC(sizeof(float) * temp_data->cap))) goto error;
        if (NULL == (temp_data->dd.ys = BR_MALLOC(sizeof(float) * temp_data->cap))) goto error;
      } break;
      case br_data_kind_3d: {
        if (1 != fread(&temp_data->ddd.bounding_box, sizeof(temp_data->ddd.bounding_box), 1, file)) goto error;
        if (NULL == (temp_data->ddd.xs = BR_MALLOC(sizeof(float) * temp_data->cap))) goto error;
        if (NULL == (temp_data->ddd.ys = BR_MALLOC(sizeof(float) * temp_data->cap))) goto error;
        if (NULL == (temp_data->ddd.zs = BR_MALLOC(sizeof(float) * temp_data->cap))) goto error;
      } break;
      default: BR_ASSERT(0);
    }
  }
  temp_data->resampling = resampling2_malloc(temp_data->kind);
  br_dagen_t new = {
    .data_kind = temp_data->kind,
    .state = br_dagen_state_inprogress,
    .group_id = temp_data->group_id,
    .file = {
      .file = file,
      .x_left = data_left,
      .y_left = data_left,
      .z_left = temp_data->kind == br_data_kind_2d ? 0 : data_left,
    }
  };
  br_da_push(*dagens, new);
  return true;

error:
  LOGEF("Failed to read plot file: %d(%s)\n", errno, strerror(errno));
  if (file != NULL) fclose(file);
  return false;
}

void br_dagen_handle(br_dagen_t* dagen, br_data_t* data, br_datas_t datas) {
  switch (dagen->kind) {
    case br_dagen_kind_file:
    {
      BR_ASSERT((data->kind == br_data_kind_2d) || (data->kind == br_data_kind_3d));
      size_t* left = &dagen->file.x_left;
      float* d = data->dd.xs;
      if (*left == 0) {
        left = &dagen->file.y_left;
        d = data->dd.ys;
      }
      if (*left == 0) {
        if (data->kind == br_data_kind_2d) {
          dagen->state = br_dagen_state_finished;
          fclose(dagen->file.file);
          return;
        }
        left = &dagen->file.z_left;
        d = data->ddd.zs;
      }
      size_t index = data->cap - *left;
      size_t read_n = 1024 < *left ? 1024 : *left;
      if (read_n != fread(&d[index], sizeof(d[index]), read_n, dagen->file.file)) goto error;
      *left -= read_n;
      if ((data->kind == br_data_kind_2d && d == data->dd.ys) || (data->kind == br_data_kind_3d && d == data->ddd.zs)) {
        data->len += read_n;
        for (size_t i = data->len - read_n; i < data->len; ++i) {
          resampling2_add_point(data->resampling, data, (uint32_t)i);
        }
        if (*left == 0) {
          dagen->state = br_dagen_state_finished;
          fclose(dagen->file.file);
        }
      }
      return;
error:
      LOGEF("Failed to read data for a plot %s: %d(%s)\n", br_str_to_c_str(data->name), errno, strerror(errno));
      fclose(dagen->file.file);
      dagen->state = br_dagen_state_failed;
    } break;
    case br_dagen_kind_expr:
    {
      switch(data->kind) {
        case br_data_kind_2d:
        {
          size_t read_index = data->len;
          size_t read_per_batch = 16*1024;
          size_t x_len = br_dagen_expr_len(datas, &dagen->expr_2d.x_expr);
          size_t y_len = br_dagen_expr_len(datas, &dagen->expr_2d.y_expr);
          size_t min_len = x_len < y_len ? x_len : y_len;
          if (data->cap < min_len) if (false == br_data_realloc(data, min_len)) {
            LOGE("Failed to alloc memory for generated plot\n");
            dagen->state = br_dagen_state_failed;
          }
          min_len -= read_index;
          if (min_len == 0) return;
          read_per_batch = min_s(read_per_batch, min_len);
          float* out_xs = &data->dd.xs[read_index];
          float* out_ys = &data->dd.ys[read_index];
          br_dagen_expr_read_n(datas, &dagen->expr_2d.x_expr, read_index, read_per_batch, out_xs);
          br_dagen_expr_read_n(datas, &dagen->expr_2d.y_expr, read_index, read_per_batch, out_ys);
          for (size_t i = 0; i < read_per_batch; ++i) {
            ++data->len;
            resampling2_add_point(data->resampling, data, (uint32_t)(read_index + i));
          }
        } break;
        default: LOGEF("Unsupported data kind: %d\n", data->kind); BR_ASSERT(0);
      }
    } break;
    default: BR_ASSERT(0);
  }
}

void br_dagens_handle(br_plotter_t* br, double until) {
  while (GetTime() < until) {
    for (size_t i = 0; i < br->dagens.len;) {
      br_dagen_t* cur = &br->dagens.arr[i]; 
      br_data_t* d = br_data_get1(br->groups, cur->group_id);
      if (NULL == cur || NULL == d) cur->state = br_dagen_state_failed;
      else br_dagen_handle(cur, d, br->groups);
      switch (cur->state) {
        case br_dagen_state_failed: {
          br_data_clear(&br->groups, &br->plots, d->group_id); 
          br_da_remove_at(br->dagens, i);
        } break;
        case br_dagen_state_finished: br_da_remove_at(br->dagens, i); break;
        case br_dagen_state_inprogress: ++i; break;
        default: BR_ASSERT(0);
      }
      if (GetTime() > until) return;
    }
  }
}
