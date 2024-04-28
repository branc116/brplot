#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct br_plotter_t br_plotter_t;

void br_permastate_save(br_plotter_t* br);
bool br_permastate_load(br_plotter_t* br);

#ifdef __cplusplus
}
#endif
