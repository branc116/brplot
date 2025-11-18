#pragma once

#if defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

// --------------------------------------------- DEFINES ---------------------------------------------

void brgui_push_log_line(const char* fmt, ...);
#if !defined(BR_DISABLE_LOG)
#  define BR_LOG(SEVERITY, FMT, ...) do { \
  brgui_push_log_line(SEVERITY FMT, ##__VA_ARGS__); \
  fprintf(stderr, SEVERITY"[ " __FILE__ ":%d ] " FMT "\n", __LINE__, ##__VA_ARGS__); \
} while(0)

#  define LOGE(format, ...) do { \
    fprintf(stderr, "[ERROR][ " __FILE__ ":%d ] " format "\n", __LINE__, ##__VA_ARGS__); \
    BR_STACKTRACE(); \
} while(0)
#else
#  define BR_LOG(SEVERITY, FMT, ...)
#  define LOGE(...)
#endif

#define LOGI(format, ...) BR_LOG("[INFO]", format, ##__VA_ARGS__)
#define LOGW(format, ...) BR_LOG("[WARNING]", format, ##__VA_ARGS__)
#define BR_LOGI LOGI
#define BR_LOGW LOGW
#define BR_LOGE LOGE

extern void br_on_fatal_error(void);
#define LOGF(format, ...) do { \
  fprintf(stderr, "[FATAL][" __FILE__ ":%d] " format "\n", __LINE__, ##__VA_ARGS__); \
  br_on_fatal_error(); \
  BR_STACKTRACE(); \
  abort(); \
} while(0)

#define BR_UNREACHABLE(fmt, ...) do { \
  LOGE("Reached unreachable state: " fmt, ##__VA_ARGS__); \
  BR_ASSERTF(0, "Exiting"); \
  exit(1); \
} while(0)

#define BR_LOG_GL_ERROR(ERROR) do { \
  int __err = ERROR; \
  if (0 != __err) { \
    LOGE("GL Error: %d", __err); \
  } \
} while(0)

#define BR_TODO(fmt, ...) do { \
   BR_UNREACHABLE("TODO: " fmt, ##__VA_ARGS__); \
   LOGF("Exiting"); \
} while (0)


#define BR_ERROR(fmt, ...) do { \
  LOGE(fmt, ##__VA_ARGS__); \
  success = false; \
  goto error; \
} while(0)

#define BR_ERRORE(fmt, ...) BR_ERROR("[%s] " fmt, strerror(errno), ##__VA_ARGS__)

#if defined(BR_ASAN)
void __sanitizer_print_stack_trace(void);
#  define BR_STACKTRACE() __sanitizer_print_stack_trace()
#else
#  define BR_STACKTRACE()
#endif

#if defined(BRPLOT_IMPLEMENTATION)
#  define BR_LIB
#endif

#if defined(DEBUG) || defined(RELEASE) || defined(BR_RELEASE)
#  error "Please don't define DEBUG or RELEASE or BR_RELEASE macros"
#endif
#if !defined(BR_DEBUG)
#  define BR_RELEASE
#endif

#if !defined(BR_HAS_MEMORY)
#  if 0 && defined(BR_DEBUG)
#    define BR_HAS_MEMORY
#  endif
#endif

#if defined(BR_HAS_MEMORY)
#  define BR_MALLOC(SIZE) br_memory_malloc(false, SIZE, __FILE__, __LINE__)
#  define BR_CALLOC(N, SIZE) br_memory_malloc(true, ((N) * (SIZE)), __FILE__, __LINE__)
#  define BR_REALLOC(PTR, NEW_SIZE) br_memory_realloc(PTR, NEW_SIZE, __FILE__, __LINE__)
#  define BR_FREE(PTR) br_memory_free(PTR, __FILE__, __LINE__)
#else
#  define BR_MALLOC malloc
#  define BR_CALLOC calloc
#  define BR_REALLOC realloc
#  define BR_FREE free
#endif


#if defined(BR_RELEASE)
#  define BR_ASSERT(...)
#  define BR_ASSERTF(...)
#else
#  define BR_ASSERT(x) do { \
    if (!(x)) { \
       LOGE("ASSERT FAILED: `%s`", #x); \
       BR_BREAKPOINT(); \
       LOGF("Exiting"); \
    } \
  } while (0)
#  define BR_ASSERTF(x, fmt, ...) do { \
     if (!(x)) { \
       LOGE("ASSERT FAILED: `" #x "`" fmt, ##__VA_ARGS__); \
       BR_BREAKPOINT(); \
       LOGF("Exiting"); \
     } \
  } while (0)
#endif

#if defined(_MSC_VER)
#  if defined(__clang__)
#    define BR_WIN_CLANG
#  else
#    define BR_WIN_MSVC
#  endif
#endif

#if defined(BR_DEBUG) && !defined(BR_WIN_MSVC) && !defined(BR_NO_UNIT_TEST)
#  define BR_UNIT_TEST
#endif


#if defined(BR_DEBUG) && !defined(__linux__) && defined(UNIT_TEST)
   // IT don't work on windows....
#  undef UNIT_TEST
#endif

#if defined(BR_DEBUG)
#  if defined(_MSC_VER)
#    define BR_BREAKPOINT() __debugbreak()
#  elif defined(__TINYC__)
#    define BR_BREAKPOINT()
#  else
#    define BR_BREAKPOINT() __builtin_trap()
#  endif
#else
# define BR_BREAKPOINT()
#endif

#if !defined(BR_HAS_HOTRELOAD)
#  if defined(BR_DEBUG) && defined(__linux__) && !defined(LIB)
#    define BR_HAS_HOTRELOAD 1
#  else
#    define BR_HAS_HOTRELOAD 0
#  endif
#endif

#if defined(BR_DEBUG) && defined(__linux__)
#  define BR_HAS_SHADER_RELOAD 1
#else
#  define BR_HAS_SHADER_RELOAD 0
#endif

#if defined(_MSC_VER)
#  define ssize_t long long int
#  if defined(__clang__)
#    define TEST_ONLY __attribute__((__unused__))
#  else
#  define TEST_ONLY
#  endif
#else
#  define TEST_ONLY __attribute__((__unused__))
#endif

#define BR_CAT(A, B) BR_CAT2(A, B)
#define BR_CAT2(A, B) A##B
#if TRACY_ENABLE
#  define BR_PROFILE(NAME) TracyCZoneN(BR_CAT(br_profiler, __LINE__),  NAME, true); \
     for (int BR_CAT(profile_loop_start, __LINE__) = 1; BR_CAT(profile_loop_start, __LINE__) == 1; BR_CAT(profile_loop_start, __LINE__) = 0, TracyCZoneEnd(BR_CAT(br_profiler, __LINE__)))
#  define BR_PROFILE_START(NAME) TracyCFrameMarkStart(NAME)
#  define BR_PROFILE_END(NAME) TracyCFrameMarkEnd(NAME)
#  define BR_PROFILE_FRAME_MARK() TracyCFrameMark
#else
#  define BR_PROFILE(NAME)
#  define BR_PROFILE_START(NAME)
#  define BR_PROFILE_END(NAME)
#  define BR_PROFILE_FRAME_MARK()
#endif

#if defined(__GNUC__)
#  define BR_FALLTHROUGH        __attribute__ ((fallthrough))
#else
#  define BR_FALLTHROUGH
#endif

#if defined(__cplusplus) &&  __cplusplus >= 201103L
#  define BR_THREAD_LOCAL       thread_local
#elif defined(__TINYC__)
#  define BR_THREAD_LOCAL
#elif defined(_MSC_VER)
#  define BR_THREAD_LOCAL       __declspec(thread)
#elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#  define BR_THREAD_LOCAL       _Thread_local
#elif defined(__GNUC__) && __GNUC__ < 5
#  define BR_THREAD_LOCAL       __thread
#elif defined(__GNUC__)
#  define BR_THREAD_LOCAL       __thread
#endif

#if !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS // Windows bullshit
#endif
#if !defined(_GNU_SOURCE)
#  define _GNU_SOURCE // Linux bullshit
#endif

#if defined(__has_include)
#  define BR_HAS_INCLUDE(path) __has_include(path)
#else
#  define BR_HAS_INCLUDE(path)
#endif

#if defined(__EMSCRIPTEN__)
#  define BR_IS_SIZE_T_32_BIT
#endif

#define BR_ARR_LEN(ARR) (sizeof((ARR)) / sizeof((ARR)[0]))

#define BR_HAS_GL 1
#define BR_HAS_GLFW 1

#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#  if !defined(BR_NO_X11)
#    define BR_HAS_GLX 1
#    define BR_HAS_X11 1
#  endif
#  if defined(HEADLESS)
#  endif
#elif defined(__EMSCRIPTEN__)
#  define BR_GL_STATIC
#  define BR_GLFW_STATIC
#elif defined(_WIN32)
#  define BR_HAS_WIN32 1
#endif


typedef unsigned long long br_u64;
typedef signed long long br_i64;
typedef unsigned int br_u32;
typedef signed int br_i32;

