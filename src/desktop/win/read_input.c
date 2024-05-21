#define _WIN32_LEAN_AND_MEAN 1
#define NOGDI 1
#define NOUSER 1
#include <windows.h>
#include <processthreadsapi.h>

typedef struct br_plotter_t br_plotter_t;
static void* thandle;

void read_input_main_worker(br_plotter_t* br);
unsigned long read_input_indirect(void* gv) {
  read_input_main_worker((br_plotter_t*)gv);
  return 0;
}

int  read_input_read_next(void) {
  return getchar();
}

void read_input_start(br_plotter_t* gv) {
  thandle = CreateThread(NULL, 0, read_input_indirect, gv, 0, NULL);
}

void read_input_stop(void) {
  TerminateThread(thandle, 69);
}

