#include "plotter.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void *read_input_main_worker(void* gv) {
  int cur_str_len = 0;
  char cur_str[128];
  graph_values_t* gvt = gv;
  memset(cur_str, 0, sizeof(cur_str));
  while(1) {
    float value = 0;
    int group = 0;
    int c = getchar();
    if (c == -1) {
      printf("EOF detected, stop reading stdin\n");
      return NULL;
    } else if (c == 'e' || c == 'E' || c == '.' || c == ';' || (c >= '0' && c <= '9') || c == '-') {
      cur_str[cur_str_len++] = (char)c;
      if (cur_str_len + 1 < (int)sizeof(cur_str)) continue;
    } else if (cur_str_len == 0) {
      continue;
    }
    int ret = sscanf(cur_str, "%f;%d", &value, &group);
    memset(cur_str, 0, sizeof(cur_str));
    cur_str_len = 0;
    if (ret < 1) continue;
    while (!q_push(&gvt->commands, (q_command) {
          .type = q_command_push_point_y,
          .push_point_y = {
            .group = group,
            .y = value
          } })) fprintf(stderr, "Q full\n");
  }
  return NULL;
}

