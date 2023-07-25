#include "../../plotter.h"
#include "pthread.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

void *read_input_main_worker(void* gv);

void read_input_main(graph_values_t* gv) {
  pthread_t thread1;
  pthread_attr_t attrs1;
  {
    pthread_attr_init(&attrs1);
    if (pthread_create(&thread1, &attrs1, read_input_main_worker, gv)) {
      fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
    }
  }
}

