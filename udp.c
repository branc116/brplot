#include "udp.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "sys/socket.h"
#include "netdb.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "pthread.h"

static void *udp_main_worker(void* gv);

void udp_main(void* gv) {
  pthread_t thread1;
  pthread_attr_t attrs1;
  {
    pthread_attr_init(&attrs1);
    if (pthread_create(&thread1, &attrs1, udp_main_worker, gv)) {
      fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
    }
  }
}

static void *udp_main_worker(void* gv) {
  printf("Attempting to bind on localhost:42069\n");
  char buffer[64] = {0};
  struct in_addr a;
  int e = inet_aton("127.0.0.1", &a);
  if (e < 0)
  {
    fprintf(stderr, "ERROR: %d:%s\n", errno, strerror(errno));
    assert(0);
  }
  struct sockaddr_in s_addr, c_addr;
  memset(&s_addr, 0, sizeof(s_addr));
  memset(&c_addr, 0, sizeof(c_addr));
  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(42069);
  s_addr.sin_addr = a;
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
  {
    fprintf(stderr, "ERROR: %d:%s\n", errno, strerror(errno));
    assert(0);
  }
  printf("SOCKET created with file descriptor: %d\n", fd);
  int ei = bind(fd, (const struct sockaddr *)(&s_addr), sizeof(s_addr));
  if (ei < 0)
  {
    fprintf(stderr, "ERROR: %d:%s\n", errno, strerror(errno));
    assert(0);
  }
  printf("BIND successful\n");
  while (true)
  {
    socklen_t soc_len = 0;
    ssize_t s = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)(&c_addr), &soc_len);

    if (s <= 0) continue;

    buffer[s] = 0;
    add_point_callback(gv, buffer, s);
  }
  printf("Killing listening thread\n");

  return NULL;
}
