#include "../../br_plot.h"
#include "pthread.h"
#include <bits/types/siginfo_t.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "signal.h"

void *read_input_main_worker(void* gv);

static pthread_t thread;
static void* indirection_function(void* gv);
static void regirter_interupt2(void);

void read_input_main(br_plot_t* gv) {
#ifndef RELEASE
  regirter_interupt2();
#endif
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
  (void)(sig); (void)si; (void)p;
  pthread_exit(NULL);
}

#ifndef RELEASE
static void int_handle2(int sig, siginfo_t* si, void* p) {
  (void)(sig); (void)si; (void)p;
}

static void regirter_interupt2(void) {
  struct sigaction act = { 0 };
  act.sa_sigaction = int_handle2;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &act, NULL);
}
#endif

static void regirter_interupt(void) {
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

