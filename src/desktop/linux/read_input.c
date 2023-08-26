#include "../../plotter.h"
#include "pthread.h"
#include <bits/types/siginfo_t.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "signal.h"

void *read_input_main_worker(void* gv);

static pthread_t thread;
static void* indirection_function(void* gv);

void read_input_main(graph_values_t* gv) {
  pthread_attr_t attrs1;
  pthread_attr_init(&attrs1);
  if (pthread_create(&thread, &attrs1, indirection_function, gv)) {
    fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
  }
}

void read_input_stop(void) {
  pthread_kill(thread, SIGALRM);
}

static void int_handle(int sig, siginfo_t* si, void* p) {
  pthread_t self = pthread_self();
  pthread_exit(NULL);
}

static void regirter_interupt() {
  pthread_t self = pthread_self();
  struct sigaction act = { 0 };
  act.sa_sigaction = int_handle;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGALRM, &act, NULL);
}

static void* indirection_function(void* gv) {
  regirter_interupt();
  read_input_main_worker(gv);
  return NULL;
}

