#include "src/br_plot.h"

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <assert.h>

static int br_pipes[2];
static pthread_t thread;
// TODO: fprintf -> LOG
static void* indirection_function(void* gv);

void read_input_start(br_plotter_t* gv) {
  pthread_attr_t attrs1;
  pthread_attr_init(&attrs1);
  if (pthread_create(&thread, &attrs1, indirection_function, gv)) {
    fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
  }
}

void read_input_stop(void) {
  write(br_pipes[1], "", 0);
  close(br_pipes[1]);
  close(STDIN_FILENO);
  /* Wait for thread to exit, or timeout after 64ms */
  struct pollfd fds[] = { { .fd = br_pipes[0], .events = POLLHUP | 32 } };
  fprintf(stderr, "Exit pool returned %d\n", poll(fds, 1,  64));
  pthread_join(thread, NULL);
}

int read_input_read_next(void) {
  struct pollfd fds[] = { { .fd = STDIN_FILENO, .events = POLLIN | POLLHUP | 32 }, { .fd = br_pipes[0], .events = POLLIN | POLLHUP | 32} };
  do {
    unsigned char c;
    if (poll(fds, 2, -1) <= 0) fprintf(stderr, "Failed to pool %d:%s\n", errno, strerror(errno));

    if (POLLIN & fds[0].revents) {
      read(STDIN_FILENO, &c, 1);
      return (int)c;
    } else if (POLLHUP & fds[0].revents) {
      fprintf(stderr, "Got POOLHUP(%d) on stdin, Stopping read_input\n", fds[0].revents);
      close(br_pipes[0]);
      return -1;
    }

    if (POLLIN & fds[1].revents) {
      if (read(br_pipes[0], &c, 1) == 0) {
        fprintf(stderr, "Rcvd 0 on br_pipe, Stoping read_input\n");
        close(br_pipes[0]);
        return -1;
      }
    } else if ((32 | POLLHUP) & fds[1].revents) {
      fprintf(stderr, "Got POOLHUP(%d) on br_pipe, Stopping read_input\n", fds[1].revents);
      close(br_pipes[0]);
      return -1;
    }
    else if (fds[1].revents != 0) assert((0 && "Unknown event mask"));
  } while(true);
}

static void* indirection_function(void* gv) {
  pipe(br_pipes);
  read_input_main_worker(gv);
  return NULL;
}
