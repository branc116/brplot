#pragma once
#include <stdbool.h>

typedef struct br_plotter_t br_plotter_t;

typedef enum {
  br_save_state_command_none = 0,
  // command       - br_save_state_command_t
  // N             - int
  // |for i in 0:N 
  // |   plot data - br_plot_t[N]
  // |_ CRC        - uint32_t
  br_save_state_command_save_plots,
  // command                   - br_save_state_command_t
  // number of points (len2)   - size_t
  //|points                    - Vector2[len2]
  //|_CRC                      - uint32_t
  br_save_state_command_save_data_2d,
  // command                   - br_save_state_command_t
  // number of points (len2)   - size_t
  //|points                    - Vector2[len2]
  //|_CRC                      - uint32_t
  br_save_state_command_save_data_3d,
  // command                   - br_save_state_command_t
  // number of datasets        - size_t
  // for dataset in datasets:
  //   dataset id              - size_t
  //   length of dataset name  - size_t
  //   dataset name            - char[len]
  // active plot index         - int
  br_save_state_command_plotter
  
} br_save_state_command_t;

typedef enum br_permastate_status_t {
  br_permastate_status_failed,
  br_permastate_status_ui_loaded,
  br_permastate_status_ok,
} br_permastate_status_t;


void                   br_permastate_save(br_plotter_t* br);
br_permastate_status_t br_permastate_load(br_plotter_t* br);

bool                   br_permastate_save_as(br_plotter_t* br, const char* path_to);

