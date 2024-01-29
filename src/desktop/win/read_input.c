#include "src/br_plot.h"
#include <processthreadsapi.h>

static void* thandle;
void read_input_main_worker(br_plot_t* br);
unsigned long read_input_indirect(void* gv) {
  read_input_main_worker((br_plot_t*)gv);
  return 0;
}

int  read_input_read_next(void) {
  return getchar();
}

void read_input_start(br_plot_t* gv) {
  thandle = CreateThread(NULL, 0, read_input_indirect, gv, 0, NULL);
}

void read_input_stop(void) {
  TerminateThread(thandle, 69);
}

