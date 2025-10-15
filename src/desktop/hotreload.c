#include "src/br_plotter.h"

#include <bits/types/siginfo_t.h>
#include <dlfcn.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(BR_RELEASE)
#  error "RELEASE must not be defined"
#endif

#define BR_CC "clang"

static bool br_hotreload_compile(void) {
  pid_t a = fork();
  if (a == -1) {
    perror("Fork failed");
    return false;
  }
  if (a == 0) {
    static char* newargv[] = { BR_CC, "-I.", "-fpic", "--shared", "-g", "-o", "build/hot.o", "src/desktop/hot.c", NULL };
    execvp(BR_CC, newargv);
  } else {
    int wstat;
    waitpid(a, &wstat, WUNTRACED | WCONTINUED);
  }
  return true;
}
//TODO: Check This shit up
//#define RTLD_DEEPBIND  0x00008  /* Use deep binding.  */
//
///* If the following bit is set in the MODE argument to `dlopen',
//   the symbols of the loaded object and its dependencies are made
//   visible as if the object were linked directly into the program.  */
//#define RTLD_GLOBAL  0x00100

static void br_hotreload_link(br_hotreload_state_t* s) {
  s->handl = dlopen("build/hot.o", RTLD_NOW);
  if (s->handl == NULL) {
    const char* err = dlerror();
    LOGE("dlopen failed: `%s`", err ? err : "NULL");
    return;
  }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
  s->func_loop = (void (*)(br_plotter_t*))dlsym(s->handl, "br_hot_loop");
  s->func_loop_ui = (void (*)(br_plotter_t*))dlsym(s->handl, "br_hot_loop_ui");
  s->func_init = (void (*)(br_plotter_t*))dlsym(s->handl, "br_hot_init");
  LOGI("Hot opened init: %p, loop: %p", (void*)s->func_init, (void*)s->func_loop);
#pragma GCC diagnostic pop
  char* error = dlerror();
  if (error != NULL) {
    LOGE("Dlerror: %s", error);
    dlclose(s->handl);
    s->handl = NULL;
    s->func_loop = NULL;
    s->func_loop_ui = NULL;
    s->func_init = NULL;
  }
}

static void hot_reload_all(br_hotreload_state_t* state) {
  if (br_hotreload_compile()) {
    if (state->func_loop != NULL) {
      dlclose(state->handl);
      LOGI("dlclosed");
      state->handl = NULL;
      state->func_loop = NULL;
      state->func_loop_ui = NULL;
      state->func_init = NULL;
    }
    br_hotreload_link(state);
  }
}

static void* hot_reload_loop(void* s) {
  br_hotreload_state_t* state = (br_hotreload_state_t*)s;
  int fd = inotify_init();
  static uint32_t buff[512];
  int wd = inotify_add_watch(fd, "src/desktop", IN_MODIFY);
  if (wd < 0) {
    perror("INIT NOTIFY ERROR");
    return NULL;
  }
  LOGI("Starting to watch for hot path.");
  state->has_changed = true;
  while(true) {
    ssize_t len = read(fd, buff, sizeof(buff));
    LOGI("read %ld bytes", len);
    if (len == -1) perror("NOTIFY ERROR");
    if (len <= 0) continue;
    state->has_changed = true;
  }
  return NULL;
}

void br_hotreload_tick(br_hotreload_state_t* state) {
  if (state->has_changed) {
    LOGI("CHANGED");
    state->has_changed = false;
    hot_reload_all(state);
    if (NULL != state->func_init) state->func_init(state->plotter);
  }
  if (NULL != state->func_loop) state->func_loop(state->plotter);
}

void br_hotreload_tick_ui(br_hotreload_state_t* state) {
  if (NULL != state->func_loop_ui) state->func_loop_ui(state->plotter);
}

void br_hotreload_start(br_hotreload_state_t* state) {
  pthread_t thread2;
  pthread_attr_t attrs2;
  pthread_attr_init(&attrs2);
  if (pthread_create(&thread2, &attrs2, hot_reload_loop, state)) {
    LOGE("While creating thread: `%s`", strerror(errno));
  }
}
