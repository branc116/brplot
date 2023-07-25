#include "plotter.h"

#include "stdbool.h"
#include <stdlib.h>

void q_init(q_commands* q) {
  size_t cap = 1024 * 1024;
  q->commands = malloc(cap * sizeof(q->commands[0]));
  q->capacity = cap;
  q->read_index = 0;
  q->write_index = 0;
#ifdef LOCK_T
  pthread_mutexattr_t attrs;
  pthread_mutexattr_init(&attrs);
  pthread_mutex_init(&q->push_mutex, &attrs);
#endif
}

#ifdef LOCK_T
bool q_push_safe(q_commands* q, q_command command) {
  bool ret = false;
  pthread_mutex_lock(&q->push_mutex);
  ret = q_push(q, command);
  pthread_mutex_unlock(&q->push_mutex);
  return ret;
}
#endif

bool q_push(q_commands* q, q_command command) {
  if ((q->write_index + 1 + q->capacity) % q->capacity == q->read_index) return false;
  q->commands[q->write_index] = command;
  q->write_index = (q->write_index + 1) % q->capacity;
  return true;
}

q_command q_pop(q_commands* q) {
  if (q->write_index == q->read_index) return (q_command) { .type = q_command_none };
  q_command ret = q->commands[q->read_index];
  q->read_index = (q->read_index + 1) % q->capacity;
  return ret;
}

