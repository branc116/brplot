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

void br_dagen_push_expr_xy(br_dagens_t* pg, br_data_expr_t x, br_data_expr_t y, int group);

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
    temp_data->dd.points = BR_MALLOC(br_data_element_size(temp_data->kind) * 8);
  } else {
    data_left = temp_data->cap;
    switch (temp_data->kind) {
      case br_data_kind_2d: {
        if (1 != fread(&temp_data->dd.bounding_box, sizeof(temp_data->dd.bounding_box), 1, file)) goto error;
      } break;
      case br_data_kind_3d: {
        if (1 != fread(&temp_data->ddd.bounding_box, sizeof(temp_data->ddd.bounding_box), 1, file)) goto error;
      } break;
      default: BR_ASSERT(0);
    }
  }
  if (NULL == (temp_data->dd.points = BR_MALLOC(br_data_element_size(temp_data->kind) * temp_data->cap))) goto error;
  temp_data->resampling = resampling2_malloc(temp_data->kind);
  br_dagen_t new = {
    .data_kind = temp_data->kind,
    .state = br_dagen_state_inprogress,
    .group_id = temp_data->group_id,
    .file = {
      .file = file,
      .data_left = data_left,
    }
  };
  br_da_push(*dagens, new);
  return true;

error:
  LOGEF("Failed to read plot file: %d(%s)\n", errno, strerror(errno));
  if (file != NULL) fclose(file);
  return false;
}

void br_dagen_handle(br_dagen_t* dagen, br_data_t* data) {
  switch (dagen->kind) {
    case br_dagen_kind_file:
    {
      size_t read_n = 1024 < dagen->file.data_left ? 1024 : dagen->file.data_left;
      switch (data->kind) {
        case br_data_kind_2d: {
          if (read_n != fread(&data->dd.points[data->len], sizeof(data->dd.points[0]), read_n, dagen->file.file)) goto error;
        } break;
        case br_data_kind_3d: {
          if (read_n != fread(&data->ddd.points[data->len], sizeof(data->ddd.points[0]), read_n, dagen->file.file)) goto error;
        } break;
        default: BR_ASSERT(0);
      }
      data->len += read_n;
      dagen->file.data_left -= read_n;
      for (size_t i = data->len - read_n; i < data->len; ++i) {
        resampling2_add_point(data->resampling, data, (uint32_t)i);
      }
      if (dagen->file.data_left == 0) {
        dagen->state = br_dagen_state_finished;
        fclose(dagen->file.file);
      }
      return;
error:
      LOGEF("Failed to read data for a plot %s: %d(%s)\n", br_str_to_c_str(data->name), errno, strerror(errno));
      fclose(dagen->file.file);
      dagen->state = br_dagen_state_failed;
    } break;
    default: BR_ASSERT(0);
  }
}

void br_dagens_handle(br_plotter_t* br, double until) {
  while (GetTime() < until) {
    for (size_t i = 0; i < br->dagens.len;) {
      br_dagen_t* cur = &br->dagens.arr[i]; 
      br_data_t* d = br_data_get1(br->groups, cur->group_id);
      br_dagen_handle(cur, d);
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

#if 0
static size_t br_data_expr_get_len(br_datas_t datas, br_data_expr_t expr) {
  switch (expr.kind) {
    case br_data_expr_kind_reference_x:
    case br_data_expr_kind_reference_y:
    case br_data_expr_kind_reference_z:
      return br_data_get1(datas, expr.group_id)->len;
    default:
      assert(0);
  }
}

static float br_data_get_x_at(br_data_t const* data, size_t i) {
  switch(data->kind) {
    case br_data_kind_2d: return data->dd.points[i].x;
    case br_data_kind_3d: return data->ddd.points[i].x;
    default: assert(0);
  }
}

static float br_data_get_y_at(br_data_t const* data, size_t i) {
  switch(data->kind) {
    case br_data_kind_2d: return data->dd.points[i].y;
    case br_data_kind_3d: return data->ddd.points[i].y;
    default: assert(0);
  }
}

static float br_data_get_z_at(br_data_t const* data, size_t i) {
  switch(data->kind) {
    case br_data_kind_3d: return data->ddd.points[i].x;
    default: assert(0);
  }
}

static float br_data_expr_get_at(br_datas_t datas, br_data_expr_t expr, size_t i) {
  switch (expr.kind) {
    case br_data_expr_kind_reference_x: return br_data_get_x_at(br_data_get1(datas, expr.group_id), i);
    case br_data_expr_kind_reference_y: return br_data_get_y_at(br_data_get1(datas, expr.group_id), i);
    case br_data_expr_kind_reference_z: return br_data_get_z_at(br_data_get1(datas, expr.group_id), i);
    default: assert(0);
  }
}

//void br_data_calcuate_expressions(br_datas_t datas, double until) {
//  for (size_t i = 0; i < datas.len; ++i) {
//    br_data_t* data = &datas.arr[i];
//    switch (datas.arr[i].kind) {
//      case br_data_kind_2d:
//      case br_data_kind_3d:
//        continue;
//      case br_data_kind_expr_2d: {
//        size_t x_len = br_data_expr_get_len(datas, data->expr_2d.x_expr);
//        size_t y_len = br_data_expr_get_len(datas, data->expr_2d.y_expr);
//        size_t len = x_len < y_len ? x_len : y_len;
//        for (size_t i = data->len; i < len; ++i) {
//          if (i % 1024 && GetTime() > until) return;
//          Vector2 point = { br_data_expr_get_at(datas, data->expr_2d.x_expr, i), br_data_expr_get_at(datas, data->expr_2d.y_expr, i) };
//          br_data_push_point2(data, point);
//        }
//      } break;
//      default: assert(0);
//    }
//  }
//}
#endif
