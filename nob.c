#define _CRT_SECURE_NO_WARNINGS
#define BR_NO_UNIT_TEST
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"
#include "src/br_da.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#if _WIN32
#  if defined(__GNUC__)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "gcc", "-I.", "-ggdb", "-o", binary_path, source_path
#  elif defined(__clang__)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "clang", "-I.", "-ggdb", "-o", binary_path, source_path
#  elif defined(_MSC_VER)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "cl.exe", "-I.", "-Zi", nob_temp_sprintf("/Fe:%s", (binary_path)), source_path
#  endif
#elif defined(__TINYC__)
#  define NOB_REBUILD_URSELF(binary_path, source_path) "tcc", "-I.", "-ggdb", "-o", binary_path, source_path, "-lm"
#else
#  define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-I.", "-ggdb", "-o", binary_path, source_path, "-lm"
#endif
#define NOB_IMPLEMENTATION
#include "external/nob.h"

#define COMMANDS(X) \
  X(build, "(default) Generate and build everything that is needed") \
  X(generate, "Generate additional files") \
  X(compile, "Only compile files, don't generate anything") \
  X(run, "Build and run brplot") \
  X(crun, "compile and run brplot") \
  X(debug, "Build and run brplot with gdb") \
  X(cdebug, "Compile (don't generate files) and run brplot with gdb") \
  X(amalgam, "Create amalgamation file that will be shipped to users") \
  X(dist, "Create distribution zip") \
  X(pip, "Create pip egg") \
  X(unittests, "Run unit tests") \
  X(fuzztests, "Run fuzz tests") \
  X(compile_commands, "Create compile_commands.json file") \
  X(help, "Print help") \

#define SANITIZER_FLAGS "-fsanitize=address,undefined", "-DBR_ASAN"

#define STR(A) #A
#define CAT3(A, B, C) STR(A) "." STR(B) "." STR(C)
#define BR_VERSION_STR CAT3(BR_MAJOR_VERSION, BR_MINOR_VERSION, BR_PATCH_VERSION)

#if defined(_WIN32)
#  define EXE_EXT ".exe"
#  define SLIB_EXT ".dll"
#  define LIB_EXT ".dll"
#else
#  define EXE_EXT ""
#  define SLIB_EXT ".a"
#  define LIB_EXT ".so"
#endif

#define DIST "dist"
#define PREFIX DIST "/brplot-" BR_VERSION_STR
#define PLIB PREFIX "/lib"
#define PBIN PREFIX "/bin"
#define PINC PREFIX "/include"
#define PSHARE PREFIX "/share"

#define nob_cmd_run_cache(cmd) nob_cmd_run(cmd, .strace_cache = &cache)

typedef enum target_platform_t {
  tp_linux,
  tp_windows,
  tp_web
} target_platform_t;

typedef struct {
  br_strv_t name;
  char alias;
  br_strv_t description;
  bool* is_set;
} command_flag_t;

typedef struct {
  command_flag_t* arr;
  size_t len, cap;
} command_flags_t;

typedef enum n_command {
#define X(name, desc) n_ ## name,
  COMMANDS(X)
#undef X
  n_count,
  n_default = n_build,
} n_command;

typedef struct {
  char const* name;
  char const* desc;
  n_command type;
} command_t;

command_t g_commands[n_count] = {
#define X(NAME, DESC) { .name = #NAME, .desc = #DESC, .type = n_ ## NAME },
  COMMANDS(X)
#undef X
};

#define n_default n_build

typedef struct {
  n_command* arr;
  size_t len, cap;
} command_deps_t;

const char* sources[] = {
 "external/shl_impls.c",
 "src/main.c",
 "src/ui.c",
 "src/data.c",
 "src/smol_mesh.c",
 "src/q.c",
 "src/read_input.c",
 "src/keybindings.c",
 "src/resampling2.c",
 "src/graph_utils.c",
 "src/shaders.c",
 "src/plotter.c",
 "src/plot.c",
 "src/permastate.c",
 "src/filesystem.c",
 "src/gui.c",
 "src/text_renderer.c",
 "src/data_generator.c",
 "src/platform.c",
 "src/threads.c",
 "src/gl.c",
 "src/icons.c",
 "src/theme.c",
 "src/free_list.c",
 "src/string_pool.c"
};

static const target_platform_t g_platform =
#if defined(_WIN32)
  tp_windows;
#else
  tp_linux;
#endif
//static const char* out_name =
//#if defined(_WIN32)
//  "brplot.exe";
//#else
//  "brplot";
//#endif

#if _WIN32
#  if defined(__GNUC__)
static const char* compiler = "gcc";
#  elif defined(__clang__)
static const char* compiler = "clang";
#  elif defined(_MSC_VER)
static const char* compiler = "cl.exe";
#  endif
#elif defined(__TINYC__)
static const char* compiler = "tcc";
#else
static const char* compiler = "cc";
#endif
static const char* program_name;
static n_command do_command = n_default;
static bool is_debug = false;
static bool has_hotreload = false;
static bool is_headless = false;
static bool enable_asan = false;
static bool print_help = false;
static bool is_lib = false;
static bool is_rebuild = false;
static bool is_slib = false;
static bool is_wasm = false;
static bool do_dist = true;
static bool pip_skip_build = false;
static bool no_gen = false;
static bool disable_logs = false;
static bool is_pedantic = false;
static bool is_tracy = false;
static bool is_help_subcommands = false;
#if !defined(__APPLE__)
static bool is_macos = false;
#else
static bool is_macos = true;
#endif

#define X(name, desc) [n_ ## name] = { 0 },
static command_flags_t command_flags[] = {
COMMANDS(X)
};
static command_deps_t command_deps[] = {
COMMANDS(X)
};
#undef X

#define X(name, desc) static bool n_ ## name ## _do(void);
COMMANDS(X)
#undef X

static Nob_Strace_Cache cache;

static void fill_command_flag_data(void) {
  command_flag_t debug_flag          = (command_flag_t) {.name = BR_STRL("debug"),      .alias = 'd',  .description = BR_STRL("Build debug version"),                                                                            .is_set = &is_debug};
  command_flag_t has_hotreload_flag  = (command_flag_t) {.name = BR_STRL("hot"),        .alias = 'H',  .description = BR_STRL("Build a version with hotreaload enabled"),                                                        .is_set = &has_hotreload};
  command_flag_t pedantic_flag       = (command_flag_t) {.name = BR_STRL("pedantic"),   .alias = 'p',  .description = BR_STRL("Turn on all warnings and treat warnings as errors"),                                              .is_set = &is_pedantic};
  command_flag_t tracy_flag          = (command_flag_t) {.name = BR_STRL("tracy"),      .alias = 't',  .description = BR_STRL("Turn tracy profiler on"),                                                                         .is_set = &is_tracy};
  command_flag_t headless_flag       = (command_flag_t) {.name = BR_STRL("headless"),   .alias = '\0', .description = BR_STRL("Create a build that will not spawn any windows"),                                                 .is_set = &is_headless};
  command_flag_t asan_flag           = (command_flag_t) {.name = BR_STRL("asan"),       .alias = 'a',  .description = BR_STRL("Enable address sanitizer"),                                                                       .is_set = &enable_asan};
  command_flag_t lib_flag            = (command_flag_t) {.name = BR_STRL("lib"),        .alias = 'l',  .description = BR_STRL("Build dynamic library"),                                                                          .is_set = &is_lib};
  command_flag_t wasm_flag           = (command_flag_t) {.name = BR_STRL("wasm"),       .alias = 'w',  .description = BR_STRL("Wasm version of brplot"),                                                                         .is_set = &is_wasm};
  command_flag_t force_rebuild       = (command_flag_t) {.name = BR_STRL("force"),      .alias = 'f',  .description = BR_STRL("Force rebuild everything"),                                                                       .is_set = &is_rebuild};
  command_flag_t slib_flag           = (command_flag_t) {.name = BR_STRL("slib"),       .alias = 's',  .description = BR_STRL("Build static library"),                                                                           .is_set = &is_slib};
  command_flag_t help_flag           = (command_flag_t) {.name = BR_STRL("help"),       .alias = 'h',  .description = BR_STRL("Print help"),                                                                                     .is_set = &print_help};
  command_flag_t help_sub_flag       = (command_flag_t) {.name = BR_STRL("help-sub"),   .alias = 's',  .description = BR_STRL("Print help for subcommands"),                                                                     .is_set = &is_help_subcommands};
  command_flag_t dist_flag           = (command_flag_t) {.name = BR_STRL("dist"),       .alias = 'D',  .description = BR_STRL("Also create dist ( Needed for pip package but maybe you wanna disable this because it's slow )"), .is_set = &do_dist};
  command_flag_t pip_skip_build_flag = (command_flag_t) {.name = BR_STRL("skip-build"), .alias = '\0', .description = BR_STRL("Don't do anything except call pip to create a package"),                                          .is_set = &pip_skip_build};
  command_flag_t unittest_no_gen     = (command_flag_t) {.name = BR_STRL("no-gen"),     .alias = 'n',  .description = BR_STRL("Don't generate files when running unittests"),                                                    .is_set = &no_gen};
  command_flag_t build_no_gen        = (command_flag_t) {.name = BR_STRL("no-gen"),     .alias = '\0', .description = BR_STRL("Don't generate files whild building"),                                                            .is_set = &no_gen};
  br_da_push(command_flags[n_compile], debug_flag);
  br_da_push(command_flags[n_compile], has_hotreload_flag);
  br_da_push(command_flags[n_compile], pedantic_flag);
  br_da_push(command_flags[n_compile], tracy_flag);
  br_da_push(command_flags[n_compile], headless_flag);
  br_da_push(command_flags[n_compile], asan_flag);
  br_da_push(command_flags[n_compile], lib_flag);
  br_da_push(command_flags[n_compile], wasm_flag);
  br_da_push(command_flags[n_compile], slib_flag);
  br_da_push(command_flags[n_compile], force_rebuild);
  br_da_push(command_flags[n_help], help_flag);
  br_da_push(command_flags[n_help], help_sub_flag);
  br_da_push(command_flags[n_pip], dist_flag);
  br_da_push(command_flags[n_pip], pip_skip_build_flag);
  br_da_push(command_flags[n_unittests], unittest_no_gen);
  br_da_push(command_flags[n_build], build_no_gen);

  br_da_push(command_deps[n_debug], n_build);
  br_da_push(command_deps[n_cdebug], n_compile);
  br_da_push(command_deps[n_run], n_build);
  br_da_push(command_deps[n_crun], n_compile);
  br_da_push(command_deps[n_build], n_compile);
  br_da_push(command_deps[n_build], n_generate);
  br_da_push(command_deps[n_dist], n_build);
  br_da_push(command_deps[n_pip], n_dist);
  br_da_push(command_deps[n_unittests], n_build);
  br_da_push(command_deps[n_compile_commands], n_compile);
  for (int i = 0; i < n_count; ++i) {
    br_da_push(command_deps[i], n_help);
  }
}

static bool create_all_dirs(void) {
  if (false == nob_mkdir_if_not_exists("build"))      return false;
  if (false == nob_mkdir_if_not_exists("bin"))        return false;
  if (false == nob_mkdir_if_not_exists(".generated")) return false;
  return true;
}

static bool bake_font(void) {
  const char* file_in = "content/font.ttf";
  const char* file_out = ".generated/default_font.h";
  const char* font_bake_bin = "bin/font_bake"EXE_EXT;
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, NOB_REBUILD_URSELF(font_bake_bin, "tools/font_bake.c"));
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, font_bake_bin, "./content/font.ttf", ".generated/default_font.h");
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_free(cmd);
  return true;
}

static bool generate_shaders(void) {
  const char* shaders_bake_bin = "bin/shaders_bake"EXE_EXT;
  const char* shaders_header = is_wasm ? ".generated/shaders_web.h" : ".generated/shaders.h";
  const char* shaders_bake_source = "tools/shaders_bake.c";

  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, NOB_REBUILD_URSELF(shaders_bake_bin, shaders_bake_source));
  nob_cmd_append(&cmd, "src/filesystem.c");
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, shaders_bake_bin, is_wasm ? "WEB" : "LINUX", shaders_header);
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_free(cmd);
  return true;
}

static bool pack_icons(void) {
  const char* pack_icons_bin = "bin/pack_icons"EXE_EXT;
  const char* pack_icons_src = "tools/pack_icons.c";
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, NOB_REBUILD_URSELF(pack_icons_bin, pack_icons_src));
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, pack_icons_bin);
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_free(cmd);
  return true;
}

static bool gl_gen(void) {
  const char* gl_gen_bin = "bin/gl_gen"EXE_EXT;
  const char* gl_gen_src = "tools/gl_gen.c";
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, NOB_REBUILD_URSELF(gl_gen_bin, gl_gen_src));
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, gl_gen_bin);
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_free(cmd);
  return true;
}

static bool compile_standard_flags(Nob_Cmd* cmd) {
  nob_cmd_append(cmd, "-I.");
  if (is_headless) {
    nob_cmd_append(cmd, "-DBR_NO_X11", "-DBR_NO_WAYLAND", "-DHEADLESS");
  }
  if (is_wasm) nob_cmd_append(cmd, "-DGRAPHICS_API_OPENGL_ES3=1");
  if (is_tracy) nob_cmd_append(cmd, "-DTRACY_ENABLE=1");
  if (enable_asan) nob_cmd_append(cmd, SANITIZER_FLAGS);
  if (is_debug) {
    nob_cmd_append(cmd, "-ggdb", "-DBR_DEBUG");
    if (has_hotreload) nob_cmd_append(cmd, "-fpic", "-fpie");
    else nob_cmd_append(cmd, "-DBR_HAS_HOTRELOAD=0");
  } else {
    nob_cmd_append(cmd, "-O3", "-ggdb");
  }
  if (disable_logs) nob_cmd_append(cmd, "-DBR_DISABLE_LOG");
  if (is_pedantic) {
#if defined(__clang__)
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow");
    nob_cmd_append(cmd, "-Wno-gnu-binary-literal");
#elif defined(_MSC_VER)
    LOGF("Pedantic flags for msvc are not known");
#else
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wshadow");
#endif
  }

  if (is_lib) {
    nob_cmd_append(cmd, "-DBR_LIB");
#if !defined(_WIN32)
    nob_cmd_append(cmd, "-fPIC");
#endif
  } else if (is_slib) {
    nob_cmd_append(cmd, "-DBR_LIB");
  }
  return true;
}

bool g_sike_compile = false;
static bool compile_one(Nob_Cmd* cmd, Nob_String_View source, Nob_Cmd* link_cmd, br_str_t* build_dir) {
  build_dir->len = 0;

  Nob_String_View file_name = source;
  nob_sv_chop_by_delim(&file_name, '/');
                     br_str_push_literal(build_dir, "build/");
                     br_str_push_strv   (build_dir, BR_STRV(file_name.data, (uint32_t)file_name.count));
  if (is_debug)      br_str_push_literal(build_dir, ".debug");
  if (is_slib)       br_str_push_literal(build_dir, ".slib");
  else if (is_lib)   br_str_push_literal(build_dir, ".lib");
  else               br_str_push_literal(build_dir, ".exe");
  if (is_headless)   br_str_push_literal(build_dir, ".headless");
  if (enable_asan)   br_str_push_literal(build_dir, ".asan");
  if (has_hotreload) br_str_push_literal(build_dir, ".hot");
  if (is_wasm)       br_str_push_literal(build_dir, ".wasm");
  if (is_tracy)      br_str_push_literal(build_dir, ".tracy");
                     br_str_push_literal(build_dir, ".");
                     br_str_push_c_str  (build_dir, compiler);
                     br_str_push_literal(build_dir, ".o");
  br_str_push_zero(build_dir);
  nob_cmd_append(link_cmd, build_dir->str);

  nob_cmd_append(cmd, compiler, "-o", build_dir->str, "-c", source.data);
  compile_standard_flags(cmd);
  if (is_macos) {
    if (0 == strcmp(source.data, "src/platform.c")) {
      nob_cmd_append(cmd, "-ObjC");
    }
  }
  return g_sike_compile || nob_cmd_run_cache(cmd);
}

static bool compile_and_link(Nob_Cmd* cmd) {
  Nob_Cmd link_command = { 0 };
  if (is_slib) nob_cmd_append(&link_command, "ar", "rcs", "bin/brplot" SLIB_EXT);
  else nob_cmd_append(&link_command, compiler, "-ggdb");

  for (size_t i = 0; i < NOB_ARRAY_LEN(sources); ++i) {
    br_str_t build_dir = { 0 };
    if (false == compile_one(cmd, nob_sv_from_cstr(sources[i]), &link_command, &build_dir)) return false;
  }

  if (is_macos) {
    nob_cmd_append(&link_command, "-framework", "Foundation");
    nob_cmd_append(&link_command, "-framework", "CoreServices");
    nob_cmd_append(&link_command, "-framework", "CoreGraphics");
    nob_cmd_append(&link_command, "-framework", "AppKit");
    nob_cmd_append(&link_command, "-framework", "IOKit");
  }
  if (is_wasm) {
    nob_cmd_append(&link_command, "-sWASM_BIGINT", "-sALLOW_MEMORY_GROWTH", "-sUSE_GLFW=3", "-sUSE_WEBGL2=1",
        "-sGL_ENABLE_GET_PROC_ADDRESS", "--shell-file=src/web/minshell.html",
        "-sCHECK_NULL_WRITES=0", "-sDISABLE_EXCEPTION_THROWING=1", "-sFILESYSTEM=0", "-sDYNAMIC_EXECUTION=0");
    if (is_lib) {
      nob_cmd_append(&link_command, "-sMODULARIZE=1", "-sEXPORT_ES6=1");
      nob_cmd_append(&link_command, "-o", "bin/brplot_lib.js");
    } else {
      BR_TODO("Implement wasm exe");
    }
  } else {
    if (is_tracy) nob_cmd_append(&link_command, "-l:libTracyClient.a", "-lstdc++");
    if (is_slib);
    else if (is_lib) nob_cmd_append(&link_command, "-shared", "-fPIC", "-o", "bin/brplot" LIB_EXT);
    else {
      if (has_hotreload) {
        nob_cmd_append(&link_command, "-fpic", "-fpie", "-rdynamic", "-ldl");
      }
      nob_cmd_append(&link_command, "-o", "bin/brplot" EXE_EXT);
      if (tp_linux == g_platform) {
        nob_cmd_append(&link_command, "-lm", "-pthread");
      }
    }
  }

  if (false == is_slib && enable_asan) nob_cmd_append(&link_command, SANITIZER_FLAGS);
  bool ret = nob_cmd_run_cache(&link_command);
  nob_cmd_free(link_command);
  return ret;
}

static void print_command_flags_usage(n_command command, int depth, bool visited[n_count]) {
  if (true == visited[command]) return;
  visited[command] = true;

  command_flags_t flags = command_flags[command];
  for (size_t i = 0; i < flags.len; ++i) {
    command_flag_t flag = flags.arr[i];
    printf("%*s-%c, --%.*s - %.*s (%s)\n", depth*4, "", flag.alias, flag.name.len, flag.name.str, flag.description.len, flag.description.str, *flag.is_set ? "ON" : "OFF");
  }

  for (size_t i = 0; i < command_deps[command].len; ++i) {
    n_command dep = command_deps[command].arr[i];
    if (false == visited[dep]) goto rec;
  }
  return;

rec:
  if (is_help_subcommands) {
    for (size_t i = 0; i < command_deps[command].len; ++i) {
      n_command dep = command_deps[command].arr[i];
      if (true == visited[dep]) continue;
      printf("%*s%s:\n", depth*4, "", g_commands[dep].name);
      print_command_flags_usage(dep, depth + 1, visited);
    }
  }
}

static void print_usage(void) {
  printf("Usage: %s [command]\n", program_name);
  printf("command - command to do\n");
#define X(name, desc) do { \
    bool visited[n_count] = { 0 }; \
    printf("  " #name " - " desc "\n"); \
    print_command_flags_usage(n_ ## name, 2, visited); \
  } while (0);
  COMMANDS(X)
#undef X
}

static bool apply_flag(n_command comm, char alias, bool turn_on, bool visited[n_count]) {
  if (true == visited[comm]) return false;
  visited[comm] = true;

  bool found = false;
  command_flags_t fs = command_flags[comm];
  for (size_t j = 0; j < fs.len; ++j) {
    if (fs.arr[j].alias == alias) {
      *fs.arr[j].is_set = turn_on;
      found = true;
      break;
    }
  }

  for (size_t i = 0; i < command_deps[comm].len; ++i) {
    found |= apply_flag(command_deps[comm].arr[i], alias, turn_on, visited);
  }
  return found;
}

static bool apply_flag1(n_command comm, br_strv_t name, bool turn_on, bool visited[n_count]) {
  if (true == visited[comm]) return false;
  visited[comm] = true;

  bool found = false;
  command_flags_t fs = command_flags[comm];
  for (size_t j = 0; j < fs.len; ++j) {
    if (br_strv_eq(fs.arr[j].name, name)) {
      *fs.arr[j].is_set = turn_on;
      found = true;
      break;
    }
  }

  for (size_t i = 0; i < command_deps[comm].len; ++i) {
    found |= apply_flag1(command_deps[comm].arr[i], name, turn_on, visited);
  }
  return found;
}

static bool apply_flags(n_command comm, int argc, char** argv) {
  for (int i = 0; i < argc; ++i) {
    const char* arg = argv[i];
    const char* cur = &arg[1];
    bool turn_on = true;
    if (arg[0] != '-') continue;
    if (arg[1] == '-') {
      br_strv_t arg_name = BR_STRV(&arg[2], (uint32_t)(strlen(arg) - 2));
      if (arg[2] == '!') {
        turn_on = false;
        arg_name = br_str_sub1(arg_name, 1);
      }
      bool found = apply_flag1(comm, arg_name, turn_on, (bool[n_count]) { 0 });
      if (false == found) {
        LOGE("Bad flag `%s`", arg);
        print_usage();
        return false;
      }
      continue;
    } else if (arg[1] == '!') {
      turn_on = false;
      cur = &arg[2];
    }
    while (*cur) {
      bool found = apply_flag(comm, *cur, turn_on, (bool[n_count]) { 0 });
      if (false == found) {
        LOGE("Bad flag `%s`", arg);
        print_usage();
        return false;
      }
      ++cur;
    }
  }
  return true;
}

static bool cat_files(const char* in_paths[], int n, const char* out_path) {
  static BR_THREAD_LOCAL br_str_t msg = {0};
  FILE* in_file = NULL;
  FILE* out_file = NULL;

  int i = 0;
  unsigned long read_len = 0;
  unsigned long write_len = 0;
  bool success = true;
  enum { buff_cap = 1024 };
  unsigned char* buff[buff_cap];

  if (false == is_rebuild) {
    if (false == nob_needs_rebuild(out_path, in_paths, (size_t)n)) {
      LOGI("Cat `%s`: SKIPPED", out_path);
      return true;
    }
  }

  msg.len = 0;
  br_str_push_literal(&msg, "cat: [");

  out_file = fopen(out_path, "wb");
  if (NULL == out_file) goto error;

  for (; i < n; ++i) {
    in_file = fopen(in_paths[i], "rb");
    if (NULL == in_file) goto error;

    while (0 != (read_len = fread(buff, 1, buff_cap, in_file))) {
      if (read_len != (write_len = fwrite(buff, 1, read_len, out_file))) goto error;
    }

    br_str_push_c_str(&msg, in_paths[i]);
    if (i + 1 < n) br_str_push_literal(&msg, ", ");

    fclose(in_file);
    in_file = NULL;
  }
  goto done;

error:
  if (NULL == out_file) LOGE("Failed to open output file `%s`: %s", out_path, strerror(errno));
  else if (i < n) {
    if (NULL == in_file) LOGE("Failed to open input file `%s`: %s", in_paths[i], strerror(errno));
    else if (write_len != read_len) LOGE("Read and write length don't match %lu != %lu: %s", read_len, write_len, strerror(errno));
    else LOGE("Unknown error: %s", strerror(errno));
  }
  else LOGE("Unknown error 2: %s", strerror(errno));
  success = false;

done:
  if (out_file) fclose(out_file);
  if (in_file) fclose(in_file);
  if (success) {
    br_str_push_literal(&msg, "] -> ");
    br_str_push_c_str(&msg, out_path);
    br_str_push_literal(&msg, " OK");
  } else {
    br_str_push_literal(&msg, " FAILED");
  }
  LOGI("%.*s", msg.len, msg.str);
  return success;
}

static bool embed_file_as_string(const char* in_path, const char* variable_name) {
  static BR_THREAD_LOCAL br_str_t out_name = { 0 };
  FILE* out_file = NULL;
  FILE* in_file = NULL;
  unsigned long read_len = 0;
  unsigned char cur_char = '\0';
  bool bad_char_flag = false;
  int line_n = 1;
  int char_n = 1;
  int read_bin_n = 0;
  enum { buff_cap = 1024 };
  unsigned char buff[buff_cap];
  bool success = true;

  out_name.len = 0;
  br_str_push_literal(&out_name, "./.generated/");
  br_str_push_c_str(&out_name, variable_name);
  br_str_push_literal(&out_name, ".c\0");

  if (false == is_rebuild) {
    if (false == nob_needs_rebuild(out_name.str, (const char*[]){ in_path }, 1)) {
      LOGI("Generate `%s`: SKIPPED", out_name.str);
      return true;
    }
  }


  if (NULL == (in_file = fopen(in_path, "rb"))) goto error;
  if (NULL == (out_file = fopen(out_name.str, "wb"))) goto error;

  fprintf(out_file, "#if !defined(BR_STRLC)\n"
                    "#  define BR_STRLC(STR) { .str = STR, .len = sizeof(STR) - 1 }\n"
                    "#endif\n");
  fprintf(out_file, "const br_strv_t %s[] = {\n  BR_STRLC(\"", variable_name);
  while ((read_len = fread(buff, 1, buff_cap, in_file))) {
    for (unsigned long i = 0; i < read_len; ++i) {
      cur_char = buff[i];
      if (read_bin_n) {
        if (1 != fwrite((unsigned char[]){ cur_char }, 1, 1, out_file)) goto error;
        --read_bin_n;
      } else if ('"' == cur_char) {
        fprintf(out_file, "\\\"");
      } else if (isprint(cur_char)) {
        fprintf(out_file, "%c", buff[i]);
      } else if ((cur_char & 0b11100000) == 0b11000000) {
        if (1 != fwrite((unsigned char[]){ cur_char }, 1, 1, out_file)) goto error;
        read_bin_n = 1;
      } else if ((cur_char & 0b11110000) == 0b11100000) {
        if (1 != fwrite((unsigned char[]){ cur_char }, 1, 1, out_file)) goto error;
        read_bin_n = 2;
      } else if ((cur_char & 0b11111000) == 0b11110000) {
        if (1 != fwrite((unsigned char[]){ cur_char }, 1, 1, out_file)) goto error;
        read_bin_n = 3;
      } else if (cur_char == '\n') {
        fprintf(out_file, "\"),\n  BR_STRLC(\"");
        ++line_n;
        char_n = 0;
      } else if (cur_char == '\r') {
      } else {
        bad_char_flag = true;
        goto error;
      }
      ++char_n;
    }
  }
  fprintf(out_file, "\")\n};\n");
  fprintf(out_file, "int %s_lines = %d;\n\n", variable_name, line_n);
  goto done;

error:
  if (NULL == in_file) LOGE("Failed to open input file `%s`: %s", in_path, strerror(errno));
  else if (NULL == out_file) LOGE("Failed to open output file `%s`: %s", out_name.str, strerror(errno));
  else if (bad_char_flag) LOGE("Unexpected character found in input file `%s:%d:%d`: %c(%d[0x%x])", in_path, line_n, char_n, cur_char, cur_char, cur_char);
  success = false;

done:
  if (NULL != out_file) fclose(out_file);
  if (NULL != in_file) fclose(in_file);
  if (success) LOGI("Generate %s: OK", out_name.str);
  return success;
}

#define BR_STR_TO_SB(STR) (Nob_String_Builder) { .items = (STR).str, .count = (STR).len, .capacity = (STR).cap }
#define BR_SB_TO_STR(STR) (br_str_t) { .str = (STR).items, .len = (STR).count, .cap = (STR).capacity }

bool nob_cmd_run_sync_str_and_reset(Nob_Cmd* cmd, br_str_t* str) {
  bool success = true;
  br_strv_t temp_file = BR_STRL(".generated/temp_file");
  Nob_String_Builder sb = BR_STR_TO_SB(*str);

  Nob_Cmd tmp_cmd = { 0 };
  const char* run_cmd_bin = "bin/redirect_output";
  const char* run_cmd_src = "tools/redirect_output.c";

  nob_cmd_append(&tmp_cmd, NOB_REBUILD_URSELF(run_cmd_bin, run_cmd_src));
  if (false == nob_cmd_run_cache(&tmp_cmd)) return false;

  nob_cmd_append(&tmp_cmd, run_cmd_bin, temp_file.str);
  for (size_t i = 0; i < cmd->count; ++i) {
    nob_cmd_append(&tmp_cmd, cmd->items[i]);
  }
  if (false == nob_cmd_run_cache(&tmp_cmd)) BR_ERROR("Failed to run the command");

  if (false == nob_read_entire_file(temp_file.str, &sb)) BR_ERROR("Failed to read temp file");

error:
  nob_cmd_free(tmp_cmd);
  *str = BR_SB_TO_STR(sb);
  cmd->count = 0;

  return success;
}

static bool generate_version_file(void) {
  bool success = true;
  Nob_Cmd cmd = { 0 };
  br_str_t value1 = { 0 };
  br_str_t value2 = { 0 };
  FILE* out_file = NULL;
  bool rebuild = false;

  nob_cmd_append(&cmd, "git", "branch", "--show-current");
  if (false == nob_cmd_run_sync_str_and_reset(&cmd, &value1)) BR_ERROR("Failed to run a command");
  if (false == cache.was_last_cached) rebuild = true;

  nob_cmd_append(&cmd, "git", "rev-parse", "HEAD");
  if (false == nob_cmd_run_sync_str_and_reset(&cmd, &value2)) BR_ERROR("Failed to run a command");
  if (false == cache.was_last_cached) rebuild = true;

  if (rebuild) {
    LOGI("Rebuilding git version");
    if (NULL == (out_file = fopen(".generated/br_version.h", "wb"))) BR_ERROR("Failed to open file: %s", strerror(errno));
    fprintf(out_file, "#define BR_GIT_BRANCH \"%.*s\"\n", value1.len - 1, value1.str);
    fprintf(out_file, "#define BR_GIT_HASH \"%.*s\"\n", value2.len - 1, value2.str);
  }

  goto done;

error:
  success = false;

done:
  if (NULL != out_file) fclose(out_file);
  nob_cmd_free(cmd);
  br_str_free(value1);
  br_str_free(value2);
  return success;
}

static bool n_generate_do(void) {
  if (false == create_all_dirs())  return false;
  if (false == bake_font())        return false;
  if (false == generate_shaders()) return false;
  if (false == pack_icons())       return false;
  if (false == gl_gen())           return false;
  if (false == cat_files((const char*[]){"LICENSE", "./external/LICENCES"}, 2, ".generated/FULL_LICENSE")) return false;
  if (false == embed_file_as_string(".generated/FULL_LICENSE", "br_license")) return false;
  if (false == generate_version_file()) return false;
  LOGI("Generate OK");
  return true;
}

static bool n_compile_do(void) {
  Nob_Cmd cmd = { 0 };
  bool ret = compile_and_link(&cmd);
  nob_cmd_free(cmd);
  return ret;
}

static bool n_build_do(void) {
  if (false == no_gen && false == n_generate_do()) return false;
  return n_compile_do();
}

static bool n_run_do(void) {
  if (false == n_build_do()) return false;
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "bin/brplot" EXE_EXT);
  nob_cmd_run(&cmd);
  return true;
}

static bool n_crun_do(void) {
  if (false == n_compile_do()) return false;
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "bin/brplot" EXE_EXT);
  nob_cmd_run(&cmd);
  return true;
}

static void just_debug(void) {
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "gdb", "bin/brplot" EXE_EXT);
  nob_cmd_run(&cmd);
}

static bool n_cdebug_do(void) {
  is_debug = true;
  if (false == n_compile_do()) return false;
  just_debug();
  return true;
}

static bool n_debug_do(void) {
  is_debug = true;
  if (false == n_build_do()) return false;
  just_debug();
  return true;
}

static bool n_amalgam_do(void) {
  Nob_Cmd cmd = { 0 };
  if (false == n_generate_do()) return false;

  const char* run_cmd_bin = "build/cshl" EXE_EXT;
  const char* run_cmd_src = "tools/create_single_header_lib.c";
  nob_cmd_append(&cmd, NOB_REBUILD_URSELF(run_cmd_bin, run_cmd_src));
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, run_cmd_bin);
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_free(cmd);
  return true;
}

static bool n_help_do(void) {
  print_usage();
  return true;
}

static bool write_entire_file(const char* file_name, br_strv_t content) {
  FILE* out_toml_file = fopen(file_name, "wb+");
  if (NULL == out_toml_file) {
    LOGE("Failed to open file `%s`: %s", file_name, strerror(errno));
    return false;
  }
  fprintf(out_toml_file, "%.*s", content.len, content.str);
  fclose(out_toml_file);
  LOGI("Written %d bytes into `%s`.", content.len, file_name);
  return true;
}

static bool n_dist_do(void) {
  if (false == n_generate_do()) return false;
  if (false == nob_mkdir_if_not_exists(DIST)) return false;
  if (false == nob_mkdir_if_not_exists(PREFIX)) return false;
  if (false == nob_mkdir_if_not_exists(PLIB)) return false;
  if (false == nob_mkdir_if_not_exists(PBIN)) return false;
  if (false == nob_mkdir_if_not_exists(PINC)) return false;
  if (false == nob_mkdir_if_not_exists(PSHARE)) return false;
  if (false == nob_mkdir_if_not_exists(PSHARE "/licenses")) return false;
  if (false == nob_mkdir_if_not_exists(PSHARE "/licenses/brplot")) return false;
  is_debug = false;
  is_slib = false;
  is_lib = false;
  if (false == n_compile_do()) return false;
  is_slib = true;
  if (false == n_compile_do()) return false;
  is_slib = false;
  is_lib = true;
  if (false == n_compile_do()) return false;
  if (false == nob_copy_file("bin/brplot" EXE_EXT, PBIN "/brplot" EXE_EXT)) return false;
  if (false == nob_copy_file("bin/brplot" SLIB_EXT, PLIB "/brplot" SLIB_EXT)) return false;
  if (false == nob_copy_file("bin/brplot" LIB_EXT, PLIB "/brplot" LIB_EXT)) return false;
  if (false == nob_copy_file("include/brplot.h", PINC "/brplot.h")) return false;
  if (false == n_amalgam_do()) return false;
  if (false == nob_copy_file(".generated/brplot.c", PINC "/brplot.c")) return false;
  if (false == nob_copy_file(".generated/FULL_LICENSE", PSHARE "/licenses/brplot/LICENSE")) return false;

  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "tar", "czf", "brplot-" BR_VERSION_STR ".tar.gz", "-C", DIST, "brplot-" BR_VERSION_STR);
  if (false == nob_cmd_run_cache(&cmd)) return false;
  nob_cmd_free(cmd);


  return true;
}

static bool build_no_get_next(const char* file_name, int* build_no) {
  FILE* build_no_file = fopen(file_name, "rb");
  if (NULL != build_no_file) {
    fscanf(build_no_file, "%d", build_no);
    ++*build_no;
    fclose(build_no_file);
    LOGI("Next Build No in `%s`: %d", file_name, *build_no);
    return true;
  }
  *build_no = 0;
  return false;
}

static bool build_no_set(const char* file_name, int build_no) {
  FILE* build_no_file = fopen(file_name, "wb+");
  if (NULL != build_no_file) {
    fprintf(build_no_file, "%d\n", build_no);
    fclose(build_no_file);
    LOGI("Build No written to `%s`: %d", file_name, build_no);
    return true;
  }
  LOGE("Failed to get Build No in file `%s`: %s", file_name, strerror(errno));
  return false;
}

static bool n_pip_do(void) {
  if (false == pip_skip_build) {
    is_rebuild = true;
    is_debug = false;
    disable_logs = true;
    if (false == n_dist_do()) return false;
    if (false == nob_mkdir_if_not_exists("packages/pip/src")) return false;
    if (false == nob_mkdir_if_not_exists("packages/pip/src/brplot")) return false;
    Nob_String_Builder pytoml = { 0 };
    if (false == nob_copy_file(PSHARE "/licenses/brplot/LICENSE", "packages/pip/LICENSE")) return false;
    if (false == nob_copy_file("README.md", "packages/pip/README.md")) return false;
    if (false == nob_copy_file(".generated/brplot.c", "packages/pip/src/brplot/brplot.c")) return false;
    if (false == nob_read_entire_file("packages/pip/pyproject.toml.in", &pytoml)) return false;
    br_str_t out_toml = { .str = pytoml.items, .len = (uint32_t)pytoml.count, .cap = (uint32_t)pytoml.capacity };
    br_str_t build_no_str = { 0 };
    int build_no = 0;

    const char* build_no_file_name = "packages/pip/buildno";
    build_no_get_next(build_no_file_name, &build_no);
    br_str_push_int(&build_no_str, build_no);
    if (false == br_str_replace_one1(&out_toml, BR_STRL("{VERSION}"), BR_STRL(BR_VERSION_STR))) return false;
    if (false == br_str_replace_one1(&out_toml, BR_STRL("{BUILDNO}"), br_str_as_view(build_no_str))) return false;
    if (false == write_entire_file("packages/pip/pyproject.toml", br_str_as_view(out_toml))) return false;
    if (false == build_no_set(build_no_file_name, build_no)) return false;
  }
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "python", "-m", "build", "-s", "packages/pip");
  return nob_cmd_run_cache(&cmd);
}

static bool n_unittests_do(void) {
  Nob_Cmd cmd = { 0 };
  is_debug = true;
  is_headless = true;
  if (no_gen) {
    if (false == n_compile_do()) return false;
  } else {
    if (false == n_build_do()) return false;
  }
  nob_cmd_append(&cmd, "./bin/brplot" EXE_EXT, "--unittest");

  if (false == nob_cmd_run(&cmd)) return false;

  static struct { char const *test_file, *out_bin; } test_programs[] = {
    { .test_file = "./tests/src/str.c", .out_bin  = "bin/test_str" EXE_EXT },
    { .test_file = "./tests/src/free_list.c", .out_bin  = "bin/test_fl" EXE_EXT },
    { .test_file = "./tests/src/math.c", .out_bin  = "bin/test_math" EXE_EXT },
    { .test_file = "./tests/src/string_pool.c", .out_bin  = "bin/string_pool" EXE_EXT },
    { .test_file = "./tests/src/da.c", .out_bin  = "bin/da" EXE_EXT },
  };

  for (size_t i = 0; i < BR_ARR_LEN(test_programs); ++i) {
    LOGI("-------------- START TESTS ------------------");
    LOGI("------------- %15s -> %15s ------------------", test_programs[i].test_file, test_programs[i].out_bin);
    cmd.count = 0;
    nob_cmd_append(&cmd, compiler, "-o", test_programs[i].out_bin, test_programs[i].test_file);
    compile_standard_flags(&cmd);
    if (tp_linux == g_platform) {
      nob_cmd_append(&cmd, "-lm", "-pthread");
    }
    if (false == nob_cmd_run(&cmd)) return false;
    nob_cmd_append(&cmd, test_programs[i].out_bin, "--unittest");
    if (false == nob_cmd_run(&cmd)) return false;
    LOGI("-------------- END TESTS ------------------");
  }

  LOGI("Unit tests ok");
  return true;
}

static bool n_fuzztests_do(void) {
#define FUZZ_FLAGS "-print_final_stats=1", "-timeout=1", "-max_total_time=200", "-create_missing_dirs=1", ".generated/corpus_sp"
  Nob_Cmd cmd = { 0 };
  is_headless = true;
  compiler = "clang";

//  nob_cmd_append(&cmd, "clang", "-fsanitize=fuzzer,address,leak,undefined", "-DBR_DISABLE_LOG", "-DFUZZ", "-o", "bin/fuzz_read_input", "tools/unity/brplot.c");
//  compile_standard_flags(&cmd);
//  if (false == nob_cmd_run_sync_and_reset(&cmd)) return false;
//  nob_cmd_append(&cmd, "./bin/fuzz_read_input", FUZZ_FLAGS);
//  if (false == nob_cmd_run_sync_and_reset(&cmd)) return false;

  nob_cmd_append(&cmd, compiler, "-fsanitize=fuzzer,address,leak,undefined", "-DFUZZ", "-o", "bin/fuzz_sp", "./tests/src/string_pool.c");
  compile_standard_flags(&cmd);
  if (false == nob_cmd_run(&cmd)) return false;
  nob_cmd_append(&cmd, "./bin/fuzz_sp", FUZZ_FLAGS);
  if (false == nob_cmd_run(&cmd)) return false;
  return true;
}
// ./bin/fuzz_sp -mutation_graph_file=1 -print_full_coverage=1 -print_final_stats=1 -print_coverage=1 -max_total_time=10 -create_missing_dirs=1 .generated/corpus_sp
//

static bool n_compile_commands_do(void) {
  const char * cwd = nob_get_current_dir_temp();
  bool success = true;
  const char* file_name = "compile_commands.json";
  FILE* f = fopen(file_name, "wb+");
  if (NULL == f) goto error;
  fprintf(f, "[\n");
  Nob_Cmd compile_command = { 0 };
  Nob_Cmd link_command = { 0 };
  g_sike_compile = true;
  is_rebuild = true;
  is_pedantic = true;
  is_debug = true;
  for (int i = 0; i < BR_ARR_LEN(sources); ++i) {
    br_str_t build_dir = { 0 };
    LOGI("Index: %d: %s", i, sources[i]);
    fprintf(f, "  {\n    ");
    compile_command.count = link_command.count = 0;
    if (false == compile_one(&compile_command, nob_sv_from_cstr(sources[i]), &link_command, &build_dir)) goto error;
    fprintf(f, "\"directory\": \"%s\",\n    ", cwd);
    fprintf(f, "\"command\": \"");
    for (int j = 0; j < compile_command.count; ++j) {
      fprintf(f, "%s ", compile_command.items[j]);
    }
    fprintf(f, "\",\n    ");
    fprintf(f, "\"file\": \"%s\"\n", sources[i]);
    fprintf(f, "  }");
    if (i + 1 < BR_ARR_LEN(sources)) fprintf(f, ",\n");
    else fprintf(f, "\n");
  }
  fprintf(f, "]");
  goto done;

error:
  if (NULL != f) LOGE("Failed to write to file %s: %s", file_name, strerror(errno));
  else           LOGE("Failed to open file %s: %s", file_name, strerror(errno));
  success = false;

done:
  if (NULL != f) fclose(f);
  if (compile_command.items) nob_cmd_free(compile_command);
  if (link_command.items) nob_cmd_free(link_command);
  return success;
}

void print_nodes(Nob_Strace_Cache* cache, Nob_Strace_Cache_Node node, br_str_t* str) {
  int old_len = str->len;
  br_str_push_c_str(str, node.argv);
  br_str_push_char(str, ' ');

  if (node.children.count == 0) {
    nob_sb_appendf(&cache->temp_sb, "%.*s\n", str->len, str->str);
  } else {
    for (size_t i = 0; i < node.children.count; ++i) {
      print_nodes(cache, cache->nodes.items[node.children.items[i]], str);
    }
  }
  str->len = old_len;
}

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "external/nob.h");

  bool success = true;

  cache.file_path = "nob.cache";
  nob_strace_cache_read(&cache);


  fill_command_flag_data();
  program_name = argv[0];
  if (argc == 1) {
    do_command = n_default;
  } else {
    bool command_found = false;
    for (int i = 1; i < argc; ++i) {
      const char* command = argv[i];
      if (command[0] == '-') continue; // Skip flags
      if (command_found == true) {
        LOGE("Unknown command: `%s`", command);
        print_usage();
        return 1;
      }
#define X(name, desc) if (0 == strcmp(command, #name)) do_command = n_ ## name; else
      COMMANDS(X)
#undef X
      /* else */ {
        LOGE("Bad argument: %s", command);
        print_usage();
        return 1;
      }
      command_found = true;
    }
  }
  if (false == apply_flags(do_command, argc, argv)) return 1;
  if (print_help) {
    print_usage();
    return 0;
  }
  if (is_wasm) compiler = "emcc";
#define X(name, desc) if (do_command == n_ ## name) { success = n_ ## name ## _do();  } else
  COMMANDS(X)
#undef X
  /*else*/ {
    LOGE("Bad number of arguments");
    print_usage();
    LOGF("Bad usage... Exiting");
  }

  if (success) nob_strace_cache_finish(cache);
  return success ? 0 : 1;
}

void br_on_fatal_error() {
  LOGE("Fatal");
}
// On linux, mac, bsds
// cc -o nob -I. nob.c -lm
// ./nob
//
// On Windows:
// Download clang
// clang -o ./nob.exe -I. nob.c
// ./nob.exe
