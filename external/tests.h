// SPDX-License-Identifier: MIT
// BR: Stolen from: https://github.com/yshui/test.h
#pragma once

#include "src/br_pp.h"
#if defined(BR_UNIT_TEST)
#  if defined(_WIN32)
#    if !defined(WIN32_LEAN_AND_MEAN)
#      define WIN32_LEAN_AND_MEAN 1
#    endif
#    include <windows.h>
#  endif
#  include <stdbool.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>

#  if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__)
#    define USE_SYSCTL_FOR_ARGS 1
     // clang-format off
#    include <sys/types.h>
#    include <sys/sysctl.h>
     // clang-format on
#    include <unistd.h>        // getpid
#  endif

struct test_file_metadata;

struct test_failure {
  bool present;
  const char *message;
  const char *file;
  int line;
  bool owned;
};

struct test_case_metadata {
  void (*fn)(struct test_case_metadata *, struct test_file_metadata *);
  struct test_failure failure;
  const char *name;
  struct test_case_metadata *next;
};

struct test_file_metadata {
  bool registered;
  const char *name;
  struct test_file_metadata *next;
  struct test_case_metadata *tests;
};

extern struct test_file_metadata * test_file_head;
static struct test_file_metadata __test_h_file;

#define SET_FAILURE(_message, _owned)                        \
  metadata->failure = (struct test_failure) {                \
    .message = _message, .file = __FILE__, .line = __LINE__, \
    .present = true, .owned = _owned,                        \
  }

#define TEST_NEQUAL(a, b)                                    \
  do {                                                       \
    if ((a) == (b)) {                                        \
      SET_FAILURE(#a " == " #b, false);                      \
      int aa = (int)a, bb = (int)b;                          \
      fprintf(stderr, "%d != %d\n", aa, bb);                 \
      return;                                                \
    }                                                        \
  } while (0)

#define TEST_EQUAL(a, b)                                     \
  do {                                                       \
    if ((a) != (b)) {                                        \
      SET_FAILURE(#a " != " #b, false);                      \
      int aa = (int)a, bb = (int)b;                          \
      fprintf(stderr, "%d != %d\n", aa, bb);                   \
      return;                                                \
    }                                                        \
  } while (0)

#define TEST_EQUALF(a, b)                                    \
  do {                                                       \
    float aa = (float)(a), bb = (float)(b);               \
    if (fabsf(aa - bb) > 1e-5) { /* TODO: this is bullshit.. */ \
      SET_FAILURE(#a " != " #b, false);                      \
      LOGF("%f != %f ( " #a " != " #b " )", aa, bb);         \
      return;                                                \
    }                                                        \
  } while (0)

#define TEST_TRUE(a)                                         \
  do {                                                       \
    if (!(a)) {                                              \
      SET_FAILURE(#a " is not true", false);                 \
      return;                                                \
    }                                                        \
  } while (0)

#define TEST_STREQUAL(a, b)                       \
  do {                                            \
    if (strcmp(a, b) != 0) {                      \
      const char *part2 = " != " #b;              \
      size_t len = strlen(a) + strlen(part2) + 3; \
      char *buf = BR_MALLOC(len);                    \
      snprintf(buf, len, "\"%s\"%s", a, part2);   \
      SET_FAILURE(buf, true); \
      return;                 \
    }                         \
  } while (0)

#define TEST_STRNEQUAL(a, b, len)            \
  do {                                       \
    if (strncmp(a, b, len) != 0) {           \
      const char *part2 = " != " #b;         \
      size_t len2 = len + strlen(part2) + 3; \
      char *buf = BR_MALLOC(len2);              \
      snprintf(buf, len2, "\"%.*s\"%s", (int)len, a, part2); \
      SET_FAILURE(buf, true); \
      return;                 \
    }                         \
  } while (0)

#define TEST_CASE(_name)                                                            \
  static void __test_h_##_name(struct test_case_metadata *,                         \
                               struct test_file_metadata *);                        \
  static struct test_case_metadata __test_h_meta_##_name = {                        \
      __test_h_##_name,                                                       \
    {0}, \
      #_name, \
    NULL\
  };                                                                                \
  static void __attribute__((constructor(1001))) __test_h_##_name##_register(void) { \
    __test_h_meta_##_name.next = __test_h_file.tests;                               \
    __test_h_file.tests = &__test_h_meta_##_name;                                   \
    if (!__test_h_file.registered) {                                                \
      __test_h_file.name = __FILE__;                                                \
      __test_h_file.next = test_file_head;                                          \
      test_file_head = &__test_h_file;                                              \
      __test_h_file.registered = true;                                              \
    }                                                                               \
  }                                                                                 \
  static void __test_h_##_name(                                                     \
      struct test_case_metadata *metadata __attribute__((unused)),                  \
      struct test_file_metadata *file_metadata __attribute__((unused)))

/// Run defined tests, return true if all tests succeeds
/// @param[out] tests_run if not NULL, set to whether tests were run
static inline void __attribute__((constructor(1002))) run_tests(void) {
  bool should_run = false;
#if defined(_MSC_VER)
  char* cmdLine = GetCommandLine();
  printf("\nCommand line: %s\n", cmdLine);
  should_run = NULL != strstr(cmdLine, "--unittest");
#else
#ifdef USE_SYSCTL_FOR_ARGS
  int mib[] = {
    CTL_KERN,
#if defined(__NetBSD__) || defined(__OpenBSD__)
    KERN_PROC_ARGS,
    getpid(),
    KERN_PROC_ARGV,
#else
    KERN_PROC,
    KERN_PROC_ARGS,
    getpid(),
#endif
  };
  char *arg = NULL;
  size_t arglen;
  sysctl(mib, sizeof(mib) / sizeof(mib[0]), NULL, &arglen, NULL, 0);
  arg = BR_MALLOC(arglen);
  sysctl(mib, sizeof(mib) / sizeof(mib[0]), arg, &arglen, NULL, 0);
#else
  FILE *cmdlinef = fopen("/proc/self/cmdline", "r");
  char *arg = NULL;
  int arglen;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
  fscanf(cmdlinef, "%ms%n", &arg, &arglen);
#pragma GCC diagnostic pop
  fclose(cmdlinef);
#endif
  for (char *pos = arg; pos < arg + arglen; pos += strlen(pos) + 1) {
    if (strcmp(pos, "--unittest") == 0) {
      should_run = true;
      break;
    }
  }
  BR_FREE(arg);
#endif

  if (!should_run) {
    return;
  }

  struct test_file_metadata *i = test_file_head;
  int failed = 0, success = 0;
  while (i) {
    fprintf(stderr, "Running tests from %s:\n", i->name);
    struct test_case_metadata *j = i->tests;
    while (j) {
      fprintf(stderr, "\t%s ... ", j->name);
      j->failure.present = false;
      j->fn(j, i);
      if (j->failure.present) {
        fprintf(stderr, "failed (%s at %s:%d)\n", j->failure.message,
                j->failure.file, j->failure.line);
        if (j->failure.owned) {
          BR_FREE((char *)j->failure.message);
          j->failure.message = NULL;
        }
        failed++;
      } else {
        fprintf(stderr, "passed\n");
        success++;
      }
      j = j->next;
    }
    fprintf(stderr, "\n");
    i = i->next;
  }
  int total = failed + success;
  fprintf(stderr, "Test results: passed %d/%d, failed %d/%d\n", success, total,
          failed, total);
  exit(failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}


#if defined(BR_UNIT_TEST_IMPLEMENTATION)
struct test_file_metadata * test_file_head;
#endif

#else

#include <stdbool.h>

#define TEST_CASE(name) static void __attribute__((unused)) __test_h_##name(void)

#define TEST_NEQUAL(a, b)         \
  (void)(a),                      \
  (void)(b)
#define TEST_EQUAL(a, b)          \
  (void)(a),                      \
  (void)(b)
#define TEST_EQUALF(a, b)         \
  (void)(a),                      \
  (void)(b)
#define TEST_STREQUAL(a, b)       \
  (void)(a);                      \
  (void)(b)
#define TEST_STRNEQUAL(a, b, len) \
  (void)(a);                      \
  (void)(b);                      \
  (void)(len)
#define TEST_TRUE(a) (void)a      \

#endif
