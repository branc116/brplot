#include "src/br_q.h"
#include "src/br_data.h"
#include "src/br_plotter.h"
#include "src/br_pp.h"

#include <stdbool.h>
#include <stdlib.h>

#if defined(__linux__)
#  include <pthread.h>
#endif

q_commands* q_malloc(void) {
  q_commands* q = BR_MALLOC(sizeof(*q));
  size_t cap = 1024 * 1024;
  if (NULL == q) {
    LOGE("Failed to malloc q, exitting...\n");
    exit(1);
  }
  q->commands = BR_MALLOC(cap * sizeof(q->commands[0]));
  if (NULL == q->commands) {
    LOGE("Failed to malloc q commands, exitting...\n");
    exit(1);
  }
  q->capacity = cap;
  q->read_index = 0;
  q->write_index = 0;
#ifdef LOCK_T
  pthread_mutexattr_t attrs;
  pthread_mutexattr_init(&attrs);
  pthread_mutex_init(&q->push_mutex, &attrs);
#endif
  return q;
}

void q_free(q_commands* q) {
  q_command c = q_pop(q);
  while(c.type != q_command_none) {
    if (c.type == q_command_set_name) br_str_free(c.set_quoted_str.str);
    c = q_pop(q);
  }
  BR_FREE(q->commands);
  BR_FREE(q);
}

void handle_all_commands(br_plotter_t* br, q_commands* commands) {
  while (1) {
    q_command comm = q_peek(commands);
    switch (comm.type) {
      case q_command_push_point_x:  
      case q_command_push_point_y:  
      case q_command_push_point_xy: 
      case q_command_push_point_xyz: {
        for (size_t i = 0; i < br->dagens.len; ++i) {
          int group = comm.push_point_x.group;
          if (br->dagens.arr[i].group_id == group) {
            if (br->dagens.arr[i].kind == br_dagen_kind_file) return;
            else if (br->dagens.arr[i].kind == br_dagen_kind_expr) {
              LOGE("Trying to push points to data grenerated by experession id: %d\n", group);
            }
          }
        }
      }
      default: break;
    }
    comm = q_pop(commands);
    switch (comm.type) {
      case q_command_none:          return;
      case q_command_push_point_x:  br_data_push_x(&br->groups, comm.push_point_x.x, comm.push_point_y.group); break;
      case q_command_push_point_y:  br_data_push_y(&br->groups, comm.push_point_y.y, comm.push_point_y.group); break;
      case q_command_push_point_xy: br_data_push_xy(&br->groups, comm.push_point_xy.x, comm.push_point_xy.y, comm.push_point_xy.group); break;
      case q_command_push_point_xyz:br_data_push_xyz(&br->groups, comm.push_point_xyz.x, comm.push_point_xyz.y, comm.push_point_xyz.z, comm.push_point_xyz.group); break;
      case q_command_pop:           break; //TODO
      case q_command_empty: {
        br_data_t* d = br_data_get1(br->groups, comm.clear.group);
        if (NULL != d) br_data_empty(d);
      } break;
      case q_command_clear:         br_plotter_data_remove(br, comm.clear.group); break;
      case q_command_clear_all:     br_plotter_datas_deinit(br); break;
      case q_command_screenshot:    br_plot_screenshot(br->text, &br->plots.arr[0], &br->shaders, br->groups, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_export:        br_plotter_export(br, comm.path_arg.path);     free(comm.path_arg.path); break;
      case q_command_exportcsv:     br_plotter_export_csv(br, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_hide:          BR_TODO("Command hide not implemented");
      case q_command_show:          BR_TODO("Command show not implemented");
      case q_command_set_name:      br_data_set_name(&br->groups, comm.set_quoted_str.group, comm.set_quoted_str.str);  break;
      case q_command_flush:         return;
      case q_command_focus:         br_plots_focus_visible(br->plots, br->groups); break;
      case q_command_new_data:      br_datas_create(&br->groups, comm.new_data.data_id, comm.new_data.kind); break;
      default:                      BR_UNREACHABLE("Unknown command(%zu,%zu): %d\n", commands->read_index, commands->write_index, comm.type);
    }
  }
}

#ifdef LOCK_T
bool q_push_safe(q_commands* q, q_command command) {
  bool ret = false;
  pthread_mutex_lock(&q->push_mutex);
  ret = q_push(q, command);
  pthread_mutex_unlock(&q->push_mutex);
  return ret;
}
#endif

bool q_push(q_commands* q, q_command command) {
  while ((q->write_index + 1) % q->capacity == q->read_index) ++q->write_wait;
  q->commands[q->write_index] = command;
  q->write_index = (q->write_index + 1) % q->capacity;
  return true;
}

q_command q_pop(q_commands* q) {
  if (q->write_index == q->read_index) return (q_command) { .type = q_command_none };
  q_command ret = q->commands[q->read_index];
  q->read_index = (q->read_index + 1) % q->capacity;
  return ret;
}

q_command q_peek(q_commands* q) {
  if (q->write_index == q->read_index) return (q_command) { .type = q_command_none };
  return q->commands[q->read_index];
}
