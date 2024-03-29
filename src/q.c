#include "br_q.h"
#include "br_data.h"
#include "br_plotter.h"

#include "stdbool.h"
#include <stdlib.h>
#include <assert.h>

q_commands* q_malloc(void) {
  q_commands* q = BR_MALLOC(sizeof(*q));
  size_t cap = 1024 * 1024;
  q->commands = BR_MALLOC(cap * sizeof(q->commands[0]));
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
    q_command comm = q_pop(commands);
    switch (comm.type) {
      case q_command_none:          goto end;
      case q_command_push_point_x:  br_data_push_x(&br->groups, comm.push_point_x.x, comm.push_point_y.group); break;
      case q_command_push_point_y:  br_data_push_y(&br->groups, comm.push_point_y.y, comm.push_point_y.group); break;
      case q_command_push_point_xy: br_data_push_xy(&br->groups, comm.push_point_xy.x, comm.push_point_xy.y, comm.push_point_xy.group); break;
      case q_command_pop:           break; //TODO
      case q_command_clear:         br_data_clear(&br->groups, &br->plots, comm.clear.group); break;
      case q_command_clear_all:     br_datas_deinit(&br->groups); break;
      case q_command_screenshot:    br_plot_screenshot(&br->plots.arr[0], br->groups, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_export:        br_plotter_export(br, comm.path_arg.path);     free(comm.path_arg.path); break;
      case q_command_exportcsv:     br_plotter_export_csv(br, comm.path_arg.path); free(comm.path_arg.path); break;
      case q_command_hide:          br_data_get(&br->groups, comm.hide_show.group)->is_selected = false; break;
      case q_command_show:          br_data_get(&br->groups, comm.hide_show.group)->is_selected = true;  break;
      case q_command_set_name:      br_data_set_name(&br->groups, comm.set_quoted_str.group, comm.set_quoted_str.str);  break;
      case q_command_focus:         br_plotter_focus_visible(&br->plots.arr[0], br->groups); break;
      default:                      BR_ASSERT(false);
    }
  }
  end: return;
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
  if ((q->write_index + 1) % q->capacity == q->read_index) return false;
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

