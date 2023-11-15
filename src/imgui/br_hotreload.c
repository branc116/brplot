#include "dlfcn.h"
#include "errno.h"
#include "../br_plot.h"
#include "pthread.h"
#include "signal.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/inotify.h"
#include "sys/select.h"
#include "sys/syscall.h"
#include "unistd.h"
#include <bits/types/siginfo_t.h>
#include "poll.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define GCC "/bin/g++"

static bool br_hotreload_compile(void) {
  pid_t a = fork();
  if (a == -1) {
    perror("Fork failed");
    return false;
  }
  if (a == 0) {
    static char* newargv[] = { GCC, "-DLINUX", "-DPLATFORM_DESKTOP", "-fpic", "--shared", "-g", "-o", "build/br_hot.o", "src/imgui/br_hot.cpp", NULL };
    execvp(GCC, newargv);
  } else {
    int wstat;
    waitpid(a, &wstat, WUNTRACED | WCONTINUED);
  }
  return true;
}
//TODO: Check This shit up
//#define RTLD_DEEPBIND	0x00008	/* Use deep binding.  */
//
///* If the following bit is set in the MODE argument to `dlopen',
//   the symbols of the loaded object and its dependencies are made
//   visible as if the object were linked directly into the program.  */
//#define RTLD_GLOBAL	0x00100

void br_hotreload_link(br_hotreload_state_t* s) {
  s->handl = dlopen("build/br_hot.o", RTLD_GLOBAL |	RTLD_LAZY);
  if (s->handl == NULL) {
    fprintf(stderr, "%s\n", dlerror());
    return;
  }
  s->func_loop = (void (*)(br_plot_t*))dlsym(s->handl, "br_hot_loop");
  s->func_init = (void (*)(br_plot_t*))dlsym(s->handl, "br_hot_init");
  char* error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "%s\n", error);
    s->handl = NULL;
    s->func_loop = NULL;
    s->func_init = NULL;
    dlclose(s->handl);
  }
  s->is_init_called = false;
}

static void hot_reload_all(br_hotreload_state_t* state) {
  if (br_hotreload_compile()) {
    if (state->func_loop != NULL) {
      pthread_mutex_lock(&state->lock);
      dlclose(state->handl);
      state->handl = NULL;
      state->func_loop = NULL;
      state->func_init = NULL;
      state->is_init_called = false;
      pthread_mutex_unlock(&state->lock);
    }
    br_hotreload_link(state);
  }
}

static void* hot_reload_loop(void* s) {
  br_hotreload_state_t* state = (br_hotreload_state_t*)s;
  int fd = inotify_init();
  static uint32_t buff[512];
  int wd = inotify_add_watch(fd, "src", IN_MODIFY);
  if (wd < 0) {
    perror("INIT NOTIFY ERROR");
  }
  while(true) {
    ssize_t len = read(fd, buff, sizeof(buff));
    fprintf(stderr, "read %ld bytes\n", len);
    if (len == -1) {
      perror("NOTIFY ERROR");
    }
    if (len <= 0)
      continue;
    hot_reload_all(state);
  }
}

void br_hotreload_start(br_hotreload_state_t* state) {
  hot_reload_all(state);
  pthread_t thread2;
  pthread_attr_t attrs2;
  pthread_attr_init(&attrs2);
  if (pthread_create(&thread2, &attrs2, hot_reload_loop, state)) {
    fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
  }
}
