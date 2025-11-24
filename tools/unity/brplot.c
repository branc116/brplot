#define _GNU_SOURCE
#include "include/brplot.h"
#if !defined(BR_INCLUDE_UNITY_BRUI_C)
#  define BR_INCLUDE_UNITY_BRUI_C
#  define BRUI_IMPLEMENTATION
#  define BRPLAT_IMPLEMENTATION
#  include "tools/unity/brui.c"
#endif
#if !defined(BR_INCLUDE_DATA_C)
#  define BR_INCLUDE_DATA_C
#  include "src/data.c"
#endif
#if !defined(BR_INCLUDE_DATA_GENERATOR_C)
#  define BR_INCLUDE_DATA_GENERATOR_C
#  include "src/data_generator.c"
#endif
#if !defined(BR_INCLUDE_GUI_C)
#  define BR_INCLUDE_GUI_C
#  include "src/gui.c"
#endif
#if !defined(BR_INCLUDE_PERMASTATE_C)
#  define BR_INCLUDE_PERMASTATE_C
#  include "src/permastate.c"
#endif
#if !defined(BR_INCLUDE_PLOT_C)
#  define BR_INCLUDE_PLOT_C
#  include "src/plot.c"
#endif
#if !defined(BR_INCLUDE_PLOTTER_C)
#  define BR_INCLUDE_PLOTTER_C
#  include "src/plotter.c"
#endif
#if !defined(BR_INCLUDE_Q_C)
#  define BR_INCLUDE_Q_C
#  include "src/q.c"
#endif
#if !defined(BR_INCLUDE_READ_INPUT_C)
#  define BR_INCLUDE_READ_INPUT_C
#  include "src/read_input.c"
#endif
#if !defined(BR_INCLUDE_MESH_C)
#  define BR_INCLUDE_MESH_C
#  include "src/mesh.c"
#endif
#if !defined(BR_INCLUDE_MAIN_C)
#  define BR_INCLUDE_MAIN_C
#  include "src/main.c"
#endif
#if !defined(BR_INCLUDE_RESAMPLING_C)
#  define BR_INCLUDE_RESAMPLING_C
#  include "src/resampling.c"
#endif

// cc -I. -o brploto tools/unity/brplot.c -lm

