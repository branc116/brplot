#include "plotter.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "pthread.h"

static void *read_input_main_worker(void* gv);

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

static void *read_input_main_worker(void* gv) {
  int cur_str_len = 0;
  char cur_str[128];
  memset(cur_str, 0, sizeof(cur_str));
  while(1) {
    float value = 0;
    int group = 0;
    int c = getchar();
    if (c == -1) {
      printf("EOF detected, stop reading stdin\n");
      return NULL;
    } else if (c == 'e' || c == 'E' || c == '.' || c == ';' || (c >= '0' && c <= '9') || c == '-') {
      cur_str[cur_str_len++] = c;
      if (cur_str_len + 1 < (int)sizeof(cur_str)) continue;
    } else if (cur_str_len == 0) {
      continue;
    }
    int ret = sscanf(cur_str, "%f;%d", &value, &group);
    memset(cur_str, 0, sizeof(cur_str));
    cur_str_len = 0;
    if (ret < 1) continue;
    add_point_callback(gv, value, group);
  }
  return NULL;
}

