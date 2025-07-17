#pragma once
#include "src/br_data.h"
#include "src/br_pp.h"
#include "src/br_str.h"

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
  q_command_push_point_xyz,
  q_command_pop,
  q_command_empty,
  q_command_clear,
  q_command_clear_all,
  q_command_screenshot,
  q_command_export,
  q_command_exportcsv,
  q_command_hide,
  q_command_show,
  q_command_set_name,
  q_command_focus,
  q_command_flush,
  q_command_new_data
} q_command_type;

typedef struct q_command {
  q_command_type type;
  union {
    double value;
    struct {
      int group;
      double x;
    } push_point_x;
    struct {
      int group;
      double y;
    } push_point_y;
    struct {
      int group;
      double x, y;
    } push_point_xy;
    struct {
      int group;
      double x, y, z;
    } push_point_xyz;
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
    struct {
      int data_id;
      br_data_kind_t kind;
    } new_data;
  };
} q_command;

typedef struct q_commands {
  volatile size_t read_index, write_index;
  volatile size_t capacity;
  q_command* commands;
  size_t write_wait;
} q_commands;

q_commands* q_malloc(void);
void q_free(q_commands* q);

// If you know that only one thread writes to q us this, else use q_push_safe.
bool q_push(q_commands* q, q_command command);
q_command q_pop(q_commands* q);
q_command q_peek(q_commands* q);

void handle_all_commands(br_plotter_t* br, q_commands* commands);

#ifdef __cplusplus
}
#endif
