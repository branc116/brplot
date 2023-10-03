#include "plotter.h"
#include "tests.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char* data;
  size_t len;
} string_view;

void *read_input_main_worker(void* gv) {
  int cur_str_len = 0;
  char cur_str[128];
  graph_values_t* gvt = gv;
  memset(cur_str, 0, sizeof(cur_str));

  float value_y = 0.f, value_x = 0.f;
  int group = 0;
  bool is_y_set = false, is_x_set = false;
  while(1) {
    int c = gvt->getchar();
    if (c == -1) {
      printf("EOF detected, stop reading stdin\n");
      return NULL;
    } else if (c == 'e' || c == 'E' || c == '.' || (c >= '0' && c <= '9') || c == '-') {
      cur_str[cur_str_len++] = (char)c;
      if (cur_str_len + 1 < (int)sizeof(cur_str)) continue;
    } else if (c == ',') {
      if (cur_str_len > 0) {
        if (is_x_set == false) {
          sscanf(cur_str, "%f", &value_x);
          memset(cur_str, 0, sizeof(cur_str));
          cur_str_len = 0;
          is_x_set = true;
          continue;
        } else {
          sscanf(cur_str, "%f", &value_y);
          memset(cur_str, 0, sizeof(cur_str));
          cur_str_len = 0;
        }
      } else {
        // PASS
      }
    } else if (c == ';') {
      if (cur_str_len > 0) {
        if (is_y_set == false) {
          sscanf(cur_str, "%f", &value_y);
          memset(cur_str, 0, sizeof(cur_str));
          cur_str_len = 0;
          is_y_set = true;
          continue;
        } else {
          sscanf(cur_str, "%d", &group);
        }
      } else {
        if (is_x_set == true) {
          is_y_set = true;
          is_x_set = false;
          value_y = value_y;
          continue;
        } else {
          // PASS
        }
      }
    } else if (cur_str_len > 0 && is_y_set) {
      sscanf(cur_str, "%d", &group);
    } else if (cur_str_len > 0) {
      sscanf(cur_str, "%f", &value_y);
      is_y_set = true;
    } else {
      continue;
    }
    if (is_y_set && is_x_set) {
      while (!q_push(&gvt->commands, (q_command) {
            .type = q_command_push_point_xy,
            .push_point_xy = {
              .group = group,
              .x = value_x,
              .y = value_y
            } })) {
        fprintf(stderr, "Q full\n");
        for (volatile int i = 0; i < 1000000; ++i); // SLEEP for some time... TODO: CHANGE THIS...
      }
    } else if (is_y_set && !is_x_set) {
      while (!q_push(&gvt->commands, (q_command) {
            .type = q_command_push_point_y,
            .push_point_y = {
              .group = group,
              .y = value_y
            } })) {
        fprintf(stderr, "Q full\n");
        for (volatile int i = 0; i < 1000000; ++i); // SLEEP for some time... TODO: CHANGE THIS...
      }
    } else {
      // PASS
    }
    is_x_set = is_y_set = false;
    value_y = value_x = 0.f;
    group = 0;
    memset(cur_str, 0, sizeof(cur_str));
    cur_str_len = 0;
  }
  return NULL;
}

int test_str() {
  static size_t index = 0;
  const char str[] = "8.0,16.0;1 4.0;1 2.0 1;10;1;;;;";
  if (index >= sizeof(str))
  {
    return -1;
  }
  return str[index++];
}

TEST_CASE(InputTests) {
  graph_values_t gvt;
  gvt.getchar = test_str;
  q_init(&gvt.commands);
  read_input_main_worker(&gvt);

  q_command c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_xy);
  TEST_EQUAL(c.push_point_xy.x, 8.f);
  TEST_EQUAL(c.push_point_xy.y, 16.f);
  TEST_EQUAL(c.push_point_xy.group, 1);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 4.f);
  TEST_EQUAL(c.push_point_y.group, 1);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 2.f);
  TEST_EQUAL(c.push_point_y.group, 0);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 1.f);
  TEST_EQUAL(c.push_point_y.group, 10);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 1.f);
  TEST_EQUAL(c.push_point_y.group, 0);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_none);

  free(gvt.commands.commands);
}

