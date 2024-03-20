#pragma once
#include "br_pp.h"
#include "br_str.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_plotter_t br_plotter_t;

typedef enum {
  q_command_none,
  q_command_set_zoom_x,
  q_command_set_zoom_y,
  q_command_set_offset_x,
  q_command_set_offset_y,
  q_command_push_point_x,
  q_command_push_point_y,
  q_command_push_point_xy,
  q_command_pop,
  q_command_clear,
  q_command_clear_all,
  q_command_screenshot,
  q_command_export,
  q_command_exportcsv,
  q_command_hide,
  q_command_show,
  q_command_set_name,
  q_command_focus
} q_command_type;

typedef struct q_command {
  q_command_type type;
  union {
    float value;
    struct {
      float x;
      int group;
    } push_point_x;
    struct {
      float y;
      int group;
    } push_point_y;
    struct {
      int group;
      float x, y;
    } push_point_xy;
    struct {
      int group;
    } pop;
    struct {
      int group;
    } clear;
    struct {
      // Should be freed by UI thread
      char* path;
    } path_arg;
    struct {
      int group;
    } hide_show;
    struct {
      int group;
      // Should be freed by UI thread
      br_str_t str;
    } set_quoted_str;
  };
} q_command;

typedef struct q_commands {
  size_t read_index, write_index;
  size_t capacity;
  q_command* commands;
  LOCK(push_mutex)
} q_commands;

q_commands* q_malloc(void);
void q_free(q_commands* q);

#ifdef LOCK_T
// If you know that only one thread writes to q use q_push, else use this.
bool q_push_safe(q_commands* q, q_command command);
#endif
// If you know that only one thread writes to q us this, else use q_push_safe.
bool q_push(q_commands* q, q_command command);
q_command q_pop(q_commands* q);

void handle_all_commands(br_plotter_t* br, q_commands* commands);

#ifdef __cplusplus
}
#endif
