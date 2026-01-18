#pragma once
#include "src/br_pp.h"

typedef struct br_plotter_t br_plotter_t;

bool brps_project_write(br_plotter_t* br, char const* file_name);
bool brps_project_read( br_plotter_t* br, char const* file_name);

bool brps_editor_write(br_plotter_t const* br, char const* file_name);
bool brps_editor_read( br_plotter_t      * br, char const* file_name);
