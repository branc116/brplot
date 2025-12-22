#if _WIN32
#  if defined(__GNUC__)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "gcc", "-I.", "-ggdb", "-o", binary_path, source_path
#  elif defined(__clang__)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "clang", "-I.", "-ggdb", "-o", binary_path, source_path
#  elif defined(_MSC_VER)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "cl.exe", "/I.", "-Zi", nob_temp_sprintf("/Fe:%s", (binary_path)), source_path
#  endif
#elif defined(__TINYC__)
#  define NOB_REBUILD_URSELF(binary_path, source_path) "tcc", "-I.", "-ggdb", "-o", binary_path, source_path, "-lm"
#else
#  define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-I.", "-ggdb", "-o", binary_path, source_path, "-lm"
#endif
#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "external/nob.h"

#define BR_NO_UNIT_TEST
#define BR_STR_IMPLEMENTATION
#include "src/br_str.h"
#include "src/br_da.h"
#include "include/brplot.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


#define COMMANDS(X) \
  X(build, "(default) Generate and build everything that is needed") \
  X(generate, "Generate additional files") \
  X(compile, "Only compile files, don't generate anything") \
  X(run, "Build and run brplot") \
  X(crun, "compile and run brplot") \
  X(debug, "Build and run brplot with gdb") \
  X(cdebug, "Compile (don't generate files) and run brplot with gdb") \
  X(amalgam, "Create amalgamation file that will be shipped to users") \
  X(install, "Install the files into prefix folder") \
  X(dist, "Create distribution zip") \
  X(publish, "Publish to the things..") \
  X(pip, "Create pip egg") \
  X(aur, "Publish to aur.") \
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

#define nob_cmd_run_cache(cmd, ...) nob_cmd_run(cmd, .ptrace_cache = &cache, ##__VA_ARGS__)
#define br_cmd_run(cmd, ...) nob_cmd_run(cmd, .ptrace_cache = NULL)

typedef struct {
  br_strv_t name;
  char alias;
  br_strv_t description;
  bool is_string;
  union {
    bool* is_set;
    const char** cstr_value;
  };
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
 "src/anim.c",
 "src/ui.c",
 "src/data.c",
 "src/mesh.c",
 "src/q.c",
 "src/read_input.c",
 "src/resampling.c",
 "src/shaders.c",
 "src/plotter.c",
 "src/plot.c",
 "src/permastate.c",
 "src/filesystem.c",
 "src/gui.c",
 "src/text_renderer.c",
 "src/data_generator.c",
 "src/platform2.c",
 "src/threads.c",
 "src/gl.c",
 "src/icons.c",
 "src/theme.c",
};

typedef enum platform_kind_t {
  p_linux,
  p_mac,
  p_wasm,
  p_windows,
  p_native =
#if defined(_WIN32)
    p_windows
#elif defined(__APPLE__)
    p_mac
#else
    p_linux
#endif
} platform_kind_t;

static const char* program_name;
static n_command do_command = n_default;
static bool is_debug = false;
static bool has_hotreload = false;
static bool is_headless = false;
static bool is_native = false;
static bool enable_asan = false;
static bool print_help = false;
static bool is_lib = false;
static bool is_rebuild = false;
static bool is_slib = false;
static bool is_wasm = false;
static bool is_win32 = false;
static bool do_dist = true;
static bool pip_skip_build = false;
static bool no_gen = false;
static bool disable_logs = false;
static bool is_pedantic = false;
static bool is_fatal_error = false;
static bool is_tracy = false;
static bool is_pg = false;
static bool is_help_subcommands = false;
static bool is_ignore_dirty = false;
static const char* g_install_prefix = NULL;

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

static Nob_Ptrace_Cache cache;

static void fill_command_flag_data(void) {
  command_flag_t debug_flag          = (command_flag_t) {.name = BR_STRL("debug"),      .alias = 'd',  .description = BR_STRL("Build debug version"),                                                                            .is_set = &is_debug};
  command_flag_t has_hotreload_flag  = (command_flag_t) {.name = BR_STRL("hot"),        .alias = 'H',  .description = BR_STRL("Build a version with hotreaload enabled"),                                                        .is_set = &has_hotreload};
  command_flag_t pedantic_flag       = (command_flag_t) {.name = BR_STRL("pedantic"),   .alias = 'p',  .description = BR_STRL("Turn on all warnings and treat warnings as errors"),                                              .is_set = &is_pedantic};
  command_flag_t fatal_error_flag    = (command_flag_t) {.name = BR_STRL("fatal-error"),.alias = 'F',  .description = BR_STRL("All compile errors are fatal"),                                                                   .is_set = &is_fatal_error};
  command_flag_t tracy_flag          = (command_flag_t) {.name = BR_STRL("tracy"),      .alias = 't',  .description = BR_STRL("Turn tracy profiler on"),                                                                         .is_set = &is_tracy};
  command_flag_t pg_flag             = (command_flag_t) {.name = BR_STRL("pg"),         .alias = '\0', .description = BR_STRL("Turn gpref profiler on"),                                                                         .is_set = &is_pg};
  command_flag_t native_flag         = (command_flag_t) {.name = BR_STRL("mnative"),    .alias = '\0', .description = BR_STRL("Mnative flag optimizations"),                                                                     .is_set = &is_native};
  command_flag_t headless_flag       = (command_flag_t) {.name = BR_STRL("headless"),   .alias = '\0', .description = BR_STRL("Create a build that will not spawn any windows"),                                                 .is_set = &is_headless};
  command_flag_t asan_flag           = (command_flag_t) {.name = BR_STRL("asan"),       .alias = 'a',  .description = BR_STRL("Enable address sanitizer"),                                                                       .is_set = &enable_asan};
  command_flag_t lib_flag            = (command_flag_t) {.name = BR_STRL("lib"),        .alias = 'l',  .description = BR_STRL("Build dynamic library"),                                                                          .is_set = &is_lib};
  command_flag_t wasm_flag           = (command_flag_t) {.name = BR_STRL("wasm"),       .alias = 'w',  .description = BR_STRL("Wasm version of brplot"),                                                                         .is_set = &is_wasm};
  command_flag_t win32_flag          = (command_flag_t) {.name = BR_STRL("win32"),      .alias = 'W',  .description = BR_STRL("Windows version of brplot"),                                                                      .is_set = &is_win32};
  command_flag_t force_rebuild       = (command_flag_t) {.name = BR_STRL("force"),      .alias = 'f',  .description = BR_STRL("Force rebuild everything"),                                                                       .is_set = &is_rebuild};
  command_flag_t slib_flag           = (command_flag_t) {.name = BR_STRL("slib"),       .alias = 's',  .description = BR_STRL("Build static library"),                                                                           .is_set = &is_slib};
  command_flag_t help_flag           = (command_flag_t) {.name = BR_STRL("help"),       .alias = 'h',  .description = BR_STRL("Print help"),                                                                                     .is_set = &print_help};
  command_flag_t help_sub_flag       = (command_flag_t) {.name = BR_STRL("help-sub"),   .alias = 's',  .description = BR_STRL("Print help for subcommands"),                                                                     .is_set = &is_help_subcommands};
  command_flag_t dist_flag           = (command_flag_t) {.name = BR_STRL("dist"),       .alias = 'D',  .description = BR_STRL("Also create dist ( Needed for pip package but maybe you wanna disable this because it's slow )"), .is_set = &do_dist};
  command_flag_t pip_skip_build_flag = (command_flag_t) {.name = BR_STRL("skip-build"), .alias = '\0', .description = BR_STRL("Don't do anything except call pip to create a package"),                                          .is_set = &pip_skip_build};
  command_flag_t unittest_no_gen     = (command_flag_t) {.name = BR_STRL("no-gen"),     .alias = 'n',  .description = BR_STRL("Don't generate files when running unittests"),                                                    .is_set = &no_gen};
  command_flag_t build_no_gen        = (command_flag_t) {.name = BR_STRL("no-gen"),     .alias = '\0', .description = BR_STRL("Don't generate files whild building"),                                                            .is_set = &no_gen};
  command_flag_t ignore_dirty        = (command_flag_t) {.name = BR_STRL("ignore-dirty"),.alias = '\0', .description = BR_STRL("Don't stop if the repo is dirty."),                                                              .is_set = &is_ignore_dirty};
  g_install_prefix = "/usr";
  command_flag_t install_prefix      = (command_flag_t) {.name = BR_STRL("prefix"),     .alias = '\0', .description = BR_STRL("Where to install the files"),                                                                     .is_string = true, .cstr_value = &g_install_prefix};
  br_da_push(command_flags[n_compile], debug_flag);
  br_da_push(command_flags[n_compile], has_hotreload_flag);
  br_da_push(command_flags[n_compile], pedantic_flag);
  br_da_push(command_flags[n_compile], fatal_error_flag);
  br_da_push(command_flags[n_compile], tracy_flag);
  br_da_push(command_flags[n_compile], pg_flag);
  br_da_push(command_flags[n_compile], native_flag);
  br_da_push(command_flags[n_compile], headless_flag);
  br_da_push(command_flags[n_compile], asan_flag);
  br_da_push(command_flags[n_compile], lib_flag);
  br_da_push(command_flags[n_compile], wasm_flag);
  br_da_push(command_flags[n_compile], win32_flag);
  br_da_push(command_flags[n_compile], slib_flag);
  br_da_push(command_flags[n_compile], force_rebuild);
  br_da_push(command_flags[n_help], help_flag);
  br_da_push(command_flags[n_help], help_sub_flag);
  br_da_push(command_flags[n_pip], dist_flag);
  br_da_push(command_flags[n_pip], pip_skip_build_flag);
  br_da_push(command_flags[n_unittests], unittest_no_gen);
  br_da_push(command_flags[n_build], build_no_gen);
  br_da_push(command_flags[n_install], install_prefix);
  br_da_push(command_flags[n_publish], ignore_dirty);

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

platform_kind_t get_target(void) {
  if (is_wasm) return p_wasm;
  else if (is_win32) return p_windows;
  return p_native;
}

const char* get_compiler(platform_kind_t target) {
  if (target == p_native) {
#if _WIN32
#  if defined(__GNUC__)
    return "gcc.exe";
#  elif defined(__clang__)
    return "clang.exe";
#  elif defined(_MSC_VER)
    return "cl.exe";
#  endif
#elif defined(__TINYC__)
    return "tcc";
#else
    return "cc";
#endif
  } else {
    switch (p_native) {
      case p_linux: {
        switch (target) {
          case p_linux: BR_UNREACHABLE("Handled by if statment"); break;
          case p_mac:   BR_TODO("Build for mac on linux."); break;
          case p_wasm:  return "emcc"; break;
          case p_windows:  return "x86_64-w64-mingw32-gcc"; break;
        }
      } break;
      case p_mac: BR_TODO("Cross compile from mac?"); break;
      case p_wasm: BR_TODO("Cross compile from wasm ? ? ?"); break;
      case p_windows: BR_TODO("Cross compile from windows?"); break;
    }
  }
}

bool is_msvc(const char* compiler) {
  return strcmp(compiler, "cl.exe") == 0;
}

typedef enum compile_output_kind_t {
  compile_output_obj,
  compile_output_exe,
  compile_output_slib,
  compile_output_dlib
} compile_output_kind_t;

const char* exe_ext(platform_kind_t target, const char* path) {
  switch (target) {
    case p_linux: return path;
    case p_wasm: return path;
    case p_mac: return nob_temp_sprintf("%s", path);
    case p_windows: return nob_temp_sprintf("%s.exe", path);
  }
}

const char* compiler_set_output(Nob_Cmd* cmd, const char* output_name, compile_output_kind_t kind, platform_kind_t target, const char* compiler) {
  if (is_msvc(compiler)) {
    BR_ASSERTF(target == p_windows, "Msvc compiler can only compile for windows...");
    switch (kind) {
      case compile_output_obj: {
        nob_cmd_append(cmd, nob_temp_sprintf("/Fo:%s.obj", output_name));
        return nob_temp_sprintf("%s.obj", output_name);
      } break;
      case compile_output_exe: {
        nob_cmd_append(cmd, "/Zi", "/DEBUG:FULL", nob_temp_sprintf("/Fe:%s.exe", output_name));
        return nob_temp_sprintf("%s.exe", output_name);
      } break;
      case compile_output_slib: BR_TODO("IDK slib on windows ?"); break;
      case compile_output_dlib: BR_TODO("IDK dlib on windows ?"); break;
    }
  } else {
    switch (target) {
      case p_linux: {
        switch (kind) {
          case compile_output_obj: {
            nob_cmd_append(cmd, "-c", "-o", nob_temp_sprintf("%s.o", output_name));
            return cmd->items[cmd->count - 1];
          } break;
          case compile_output_exe: {
            if (has_hotreload) nob_cmd_append(cmd, "-fpie", "-rdynamic");
            nob_cmd_append(cmd, "-o", output_name);
            const char* ret = cmd->items[cmd->count - 1];
            if (is_tracy) nob_cmd_append(cmd, "-l:libTracyClient.a", "-lstdc++");
            nob_cmd_append(cmd, "-ldl", "-lm", "-pthread");
            if (enable_asan) nob_cmd_append(cmd, SANITIZER_FLAGS);
            return ret;
          } break;
          case compile_output_slib: {
            nob_cmd_append(cmd, nob_temp_sprintf("%s.a", output_name));
            return cmd->items[cmd->count - 1];
          } break;
          case compile_output_dlib: {
            nob_cmd_append(cmd, "-shared", "-fPIC");
            nob_cmd_append(cmd, "-o", nob_temp_sprintf("%s.so", output_name));
            return cmd->items[cmd->count - 1];
          } break;
          default: BR_UNREACHABLE("output kind: %d", kind); break;
        }
      } break;
      case p_wasm: {
        switch (kind) {
          case compile_output_obj: {
            nob_cmd_append(cmd, "-c", "-o", nob_temp_sprintf("%s.o", output_name));
            return cmd->items[cmd->count - 1];
          } break;
          case compile_output_exe: BR_TODO("wasm exe"); break;
          case compile_output_slib: BR_TODO("wasm static lib"); break;
          case compile_output_dlib: {
            nob_cmd_append(cmd, "-sWASM_BIGINT", "-sALLOW_MEMORY_GROWTH", "-sUSE_GLFW=3", "-sUSE_WEBGL2=1",
                                          "-sGL_ENABLE_GET_PROC_ADDRESS",
                                          "-sCHECK_NULL_WRITES=0", "-sDISABLE_EXCEPTION_THROWING=1", "-sFILESYSTEM=0", "-sDYNAMIC_EXECUTION=0");
            nob_cmd_append(cmd, "-sMODULARIZE=1", "-sEXPORT_ES6=1");
            nob_cmd_append(cmd, "-o", nob_temp_sprintf("%s.js", output_name));
            return cmd->items[cmd->count - 1];
          } break;
          default: BR_UNREACHABLE("output kind: %d", kind); break;
        }
      } break;
      case p_mac: {
        switch (kind) {
          case compile_output_obj: nob_cmd_append(cmd, "-c", "-o", nob_temp_sprintf("%s.o", output_name)); break;
          case compile_output_exe: nob_cmd_append(cmd, "-o", output_name); break;
          case compile_output_slib: nob_cmd_append(cmd, "-o", nob_temp_sprintf("%s.a", output_name)); break;
          case compile_output_dlib: nob_cmd_append(cmd, "-o", nob_temp_sprintf("%s.dylib", output_name)); break;
          default: BR_UNREACHABLE("output kind: %d", kind);
        }
        return cmd->items[cmd->count - 1];
      } break;
      case p_windows: {
        switch (kind) {
          case compile_output_obj: {
            nob_cmd_append(cmd, "-c", "-DWIN32_LEAN_AND_MEAN");
            nob_cmd_append(cmd, "-o", nob_temp_sprintf("%s.o", output_name));
          } break;
          case compile_output_exe: nob_cmd_append(cmd, "-static", "-o", nob_temp_sprintf("%s.exe", output_name)); break;
          case compile_output_slib: BR_TODO("IDK slib on windows ?");
          case compile_output_dlib: BR_TODO("IDK dlib on windows ?");
          default: BR_UNREACHABLE("output kind: %d", kind);
        }
        return cmd->items[cmd->count - 1];
      } break;
    }
  }
}

void compiler_base_flags(Nob_Cmd* cmd, const char* compiler) {
  nob_cmd_append(cmd, compiler);
  if (is_msvc(compiler)) {
    nob_cmd_append(cmd, "/I.", "/Zi", "/D_CRT_SECURE_NO_WARNINGS=1");
  } else {
    nob_cmd_append(cmd, "-I.", "-g");
  }
}

const char* compiler_single_file2(compile_output_kind_t output_kind, platform_kind_t platform, const char* src_name, const char* output_file, Nob_Cmd additional_libs) {
  static Nob_Cmd cmd = { 0 };
  cmd.count = 0;
  const char* compiler = get_compiler(platform);
  compiler_base_flags(&cmd, compiler);
  nob_cmd_append(&cmd, src_name);
  const char* output = compiler_set_output(&cmd, output_file, output_kind, platform, compiler);
  nob_cmd_extend(&cmd, &additional_libs);
  if (false == nob_cmd_run_cache(&cmd)) return NULL;
  return output;
}

const char* compiler_single_file_exe2(platform_kind_t platform, const char* src_name, const char* output_file, Nob_Cmd additional_libs) {
  return compiler_single_file2(compile_output_exe, platform, src_name, output_file, additional_libs);
}

const char* compiler_single_file_exe(platform_kind_t platform, const char* src_name, const char* output_file) {
  return compiler_single_file_exe2(platform, src_name, output_file, (Nob_Cmd) {0});
}

const char* compiler_single_file_obj(platform_kind_t platform, const char* src_name, const char* output_file) {
  return compiler_single_file2(compile_output_obj, platform, src_name, output_file, (Nob_Cmd) {0});
}

static bool create_all_dirs(void) {
  if (false == nob_mkdir_if_not_exists("build"))      return false;
  if (false == nob_mkdir_if_not_exists("bin"))        return false;
  if (false == nob_mkdir_if_not_exists(".generated")) return false;
  return true;
}

static bool bake_font(void) {
  Nob_Cmd cmd = { 0 };
  const char* output = compiler_single_file_exe(p_native, "tools/font_bake.c", "bin/font_bake");
  if (NULL == output) return false;

  nob_cmd_append(&cmd, output, "./content/font.ttf", ".generated/default_font.h");
  if (false == nob_cmd_run_cache(&cmd)) return false;
  return true;
}

static bool generate_shaders(void) {
  const char* shaders_bake_bin = "bin/shaders_bake"EXE_EXT;
  const char* shaders_bake_source = "tools/shaders_bake.c";

  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, NOB_REBUILD_URSELF(shaders_bake_bin, shaders_bake_source));
  nob_cmd_append(&cmd, "src/filesystem.c");
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, shaders_bake_bin, "WEB", ".generated/shaders_web.h");
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, shaders_bake_bin, "LINUX", ".generated/shaders.h");
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

  // TODO: THis is not cached properly. Investigate...
  if (false == cache.was_last_cached) {
    nob_cmd_append(&cmd, gl_gen_bin, "--no-tracy");
    if (false == br_cmd_run(&cmd)) return false;
  }

  nob_cmd_free(cmd);
  return true;
}

static bool compile_standard_flags(Nob_Cmd* cmd, bool is_msvc) {
  nob_cmd_append(cmd, "-I.");
  if (is_headless) {
    nob_cmd_append(cmd, "-DBR_NO_X11", "-DHEADLESS");
  }
  if (is_wasm) nob_cmd_append(cmd, "-DGRAPHICS_API_OPENGL_ES3=1");
  if (is_tracy) nob_cmd_append(cmd, "-DTRACY_ENABLE=1");
  if (enable_asan) nob_cmd_append(cmd, SANITIZER_FLAGS);
  if (is_pg) nob_cmd_append(cmd, "-pg");
  if (is_native) nob_cmd_append(cmd, "-march=native", "-mtune=native");
  if (is_debug) {
    if (is_msvc) nob_cmd_append(cmd, "/Zi");
    else nob_cmd_append(cmd, "-ggdb");
    nob_cmd_append(cmd, "-DBR_DEBUG");
    if (has_hotreload) nob_cmd_append(cmd, "-fpie");
    else nob_cmd_append(cmd, "-DBR_HAS_HOTRELOAD=0");
  } else {
    if (is_msvc) nob_cmd_append(cmd, "/O2");
    else nob_cmd_append(cmd, "-O3", "-ggdb");
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
  if (is_fatal_error) {
    if (is_msvc) LOGF("Pedantic flags for msvc are not known");
    else nob_cmd_append(cmd, "-Wfatal-errors");
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
static bool compile_one(platform_kind_t target, Nob_Cmd* cmd, Nob_String_View source, Nob_Cmd* link_cmd, br_str_t* build_dir) {
  build_dir->len = 0;
  const char* compiler = get_compiler(target);

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
  if (is_win32)      br_str_push_literal(build_dir, ".win32");
  if (is_tracy)      br_str_push_literal(build_dir, ".tracy");
                     br_str_push_literal(build_dir, ".");
                     br_str_push_c_str  (build_dir, compiler);
  br_str_push_zero(build_dir);
  nob_cmd_append(cmd, compiler, "-c", source.data);
  compile_standard_flags(cmd, is_msvc(compiler));
  const char* obj = compiler_set_output(cmd, build_dir->str, compile_output_obj, target, compiler);

  nob_cmd_append(link_cmd, obj);

  return g_sike_compile || nob_cmd_run_cache(cmd);
}

static bool compile_and_link(platform_kind_t target, Nob_Cmd* cmd) {
  Nob_Cmd link_command = { 0 };
  const char* compiler = get_compiler(target);

  compile_output_kind_t out_kind;
  if (is_slib)     out_kind = compile_output_slib;
  else if (is_lib) out_kind = compile_output_dlib;
  else             out_kind = compile_output_exe;

  if (is_slib) nob_cmd_append(&link_command, "ar", "rcs");
  else         compiler_base_flags(&link_command, compiler);
  compiler_set_output(&link_command, "bin/brplot", out_kind, target, compiler);

  for (size_t i = 0; i < NOB_ARRAY_LEN(sources); ++i) {
    br_str_t build_dir = { 0 };
    if (false == compile_one(target, cmd, nob_sv_from_cstr(sources[i]), &link_command, &build_dir)) return false;
  }

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
    if (flag.is_string) {
      printf("%*s-%c <value>, --%.*s <value> - %.*s (%s)\n", depth*4, "", flag.alias, flag.name.len, flag.name.str, flag.description.len, flag.description.str, *flag.cstr_value);
    } else {
      printf("%*s-%c, --%.*s - %.*s (%s)\n", depth*4, "", flag.alias, flag.name.len, flag.name.str, flag.description.len, flag.description.str, *flag.is_set ? "ON" : "OFF");
    }
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

static bool apply_flag(n_command comm, char alias, bool turn_on, int argc, char** argv, int* i, bool visited[n_count]) {
  if (true == visited[comm]) return false;
  visited[comm] = true;

  bool found = false;
  command_flags_t fs = command_flags[comm];
  for (size_t j = 0; j < fs.len; ++j) {
    if (fs.arr[j].alias == alias) {
      if (fs.arr[j].is_string) {
        if (*i + 1 >= argc) {
          LOGE("Value for %c flag is not specified..", alias);
          print_usage();
          LOGF("Error while reading comand line arguments.");
        }
        *fs.arr[j].cstr_value = argv[++*i]; // TODO: This will fail if there are multiple flags with the same name
      } else {
        *fs.arr[j].is_set = turn_on;
      }
      found = true;
    }
  }

  for (size_t j = 0; j < command_deps[comm].len; ++j) {
    found |= apply_flag(command_deps[comm].arr[j], alias, turn_on, argc, argv, i, visited);
  }
  return found;
}

static bool apply_flag1(n_command comm, br_strv_t name, bool turn_on, int argc, char** argv, int* i, bool visited[n_count]) {
  if (true == visited[comm]) return false;
  visited[comm] = true;

  bool found = false;
  command_flags_t fs = command_flags[comm];
  for (size_t j = 0; j < fs.len; ++j) {
    if (br_strv_eq(fs.arr[j].name, name)) {
      if (fs.arr[j].is_string) {
        if (*i + 1 >= argc) {
          LOGE("Value for %.*s flag is not specified..", name.len, name.str);
          print_usage();
          LOGF("Error while reading comand line arguments.");
        }
        *fs.arr[j].cstr_value = argv[++*i]; // TODO: This will fail if there are multiple flags with the same name
      } else {
        *fs.arr[j].is_set = turn_on;
      }
      found = true;
      break;
    }
  }

  for (size_t j = 0; j < command_deps[comm].len; ++j) {
    found |= apply_flag1(command_deps[comm].arr[j], name, turn_on, argc, argv, i, visited);
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
      bool found = apply_flag1(comm, arg_name, turn_on, argc, argv, &i, (bool[n_count]) { 0 });
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
      bool found = apply_flag(comm, *cur, turn_on, argc, argv, &i, (bool[n_count]) { 0 });
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

bool nob_cmd_run_sync_str_and_reset(const char* temp_file_path, Nob_Cmd* cmd, br_str_t* str) {
  bool success = true;
  Nob_String_Builder sb = BR_STR_TO_SB(*str);

  if (false == nob_cmd_run_cache(cmd, .stdout_path = temp_file_path)) BR_ERROR("Failed to run the command");

  if (false == nob_read_entire_file(temp_file_path, &sb)) BR_ERROR("Failed to read temp file");

error:
  *str = BR_SB_TO_STR(sb);
  cmd->count = 0;

  return success;
}

static bool generate_version_file(br_str_t* out_hash) {
  bool success = true;
  Nob_Cmd cmd = { 0 };
  br_str_t value1 = { 0 };
  FILE* out_file = NULL;
  bool rebuild = false;

  nob_cmd_append(&cmd, "git", "branch", "--show-current");
  if (false == nob_cmd_run_sync_str_and_reset(".generated/temp1", &cmd, &value1)) BR_ERROR("Failed to run a command");
  if (false == cache.was_last_cached) rebuild = true;

  nob_cmd_append(&cmd, "git", "rev-parse", "HEAD");
  if (false == nob_cmd_run_sync_str_and_reset(".generated/temp2", &cmd, out_hash)) BR_ERROR("Failed to run a command");
  if (false == cache.was_last_cached) rebuild = true;

  if (rebuild) {
    LOGI("Rebuilding git version");
    if (NULL == (out_file = fopen(".generated/br_version.h", "wb"))) BR_ERROR("Failed to open file: %s", strerror(errno));
    fprintf(out_file, "#define BR_GIT_BRANCH \"%.*s\"\n", value1.len - 1, value1.str);
    fprintf(out_file, "#define BR_GIT_HASH \"%.*s\"\n", out_hash->len - 1, out_hash->str);
  }

  goto done;

error:
  success = false;

done:
  if (NULL != out_file) fclose(out_file);
  nob_cmd_free(cmd);
  br_str_free(value1);
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
  br_str_t out_hash = { 0 };
  if (false == generate_version_file(&out_hash)) return false;
  LOGI("Generate OK");
  return true;
}

static bool n_compile_do(void) {
  Nob_Cmd cmd = { 0 };
  bool ret = compile_and_link(get_target(), &cmd);
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
  nob_cmd_append(&cmd, exe_ext(get_target(), "bin/brplot"));
  br_cmd_run(&cmd);
  return true;
}

static bool n_crun_do(void) {
  if (false == n_compile_do()) return false;
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, exe_ext(get_target(), "bin/brplot"));
  br_cmd_run(&cmd);
  return true;
}

static void just_debug(void) {
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "gdb", "bin/brplot" EXE_EXT);
  br_cmd_run(&cmd);
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

  const char* output = compiler_single_file_exe(p_native, "tools/create_single_header_lib.c", "bin/cshl");
  if (NULL == output) return false;

  nob_cmd_append(&cmd, output, "include/brplot.h", ".generated/brplot.h");
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, output, "include/brplat.h", ".generated/brplat.h");
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_append(&cmd, output, "include/brui.h", ".generated/brui.h");
  if (false == nob_cmd_run_cache(&cmd)) return false;

  nob_cmd_free(cmd);
  return true;
}

typedef struct {
  const char* name;
  int section;
} doc_file_t;

static doc_file_t g_doc_files[] = {
  { "brplot", 1 }
};

static bool n_compress_docs_do(void) {
  LOGI("Compressing docs");

  Nob_Cmd cmd = { 0 };
  for (int i = 0; i < sizeof(g_doc_files)/sizeof(g_doc_files[0]); ++i) {
    nob_cmd_append(&cmd, "gzip");
    nob_cmd_append(&cmd, "-c");
    nob_cmd_append(&cmd, nob_temp_sprintf("content/docs/%s.%d.mdoc", g_doc_files[i].name, g_doc_files[i].section));
    if (false == nob_cmd_run_cache(&cmd, .stdout_path = nob_temp_sprintf(".generated/%s.%d.gz", g_doc_files[i].name, g_doc_files[i].section))) return false;
  }
  nob_cmd_free(cmd);
  LOGI("Everything compressed.");
  return true;
}

static bool n_install_do(void) {
  bool has_docs = n_compress_docs_do();
  if (false == has_docs) {
    LOGE("Couldn't compress the man pages. Installing without man pages.. :/");
  }
  if (false == n_amalgam_do()) return false;
  if (false == nob_mkdir_if_not_exists(g_install_prefix)) return false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/lib",                   g_install_prefix))) return false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/bin",                   g_install_prefix))) return false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/include",               g_install_prefix))) return false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/share",                 g_install_prefix))) return false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/share/licenses",        g_install_prefix))) return false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/share/licenses/brplot", g_install_prefix))) return false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/share/man",             g_install_prefix))) has_docs = false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/share/man/man1",        g_install_prefix))) has_docs = false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/share/man/man3",        g_install_prefix))) has_docs = false;
  if (false == nob_mkdir_if_not_exists(nob_temp_sprintf("%s/share/man/man7",        g_install_prefix))) has_docs = false;
  is_debug = false;
  is_slib = false;
  is_lib = false;
  if (false == n_compile_do()) return false;
  is_slib = true;
  if (false == n_compile_do()) return false;
  is_slib = false;
  is_lib = true;
  if (false == n_compile_do()) return false;
  if (false == nob_copy_file("bin/brplot" EXE_EXT,      nob_temp_sprintf("%s/bin/brplot"    EXE_EXT,         g_install_prefix))) return false;
  if (false == nob_copy_file("bin/brplot" SLIB_EXT,     nob_temp_sprintf("%s/lib/libbrplot" SLIB_EXT,        g_install_prefix))) return false;
  if (false == nob_copy_file("bin/brplot" LIB_EXT,      nob_temp_sprintf("%s/lib/libbrplot" LIB_EXT,         g_install_prefix))) return false;
  if (false == nob_copy_file(".generated/brplot.h",     nob_temp_sprintf("%s/include/brplot.h",              g_install_prefix))) return false;
  if (false == nob_copy_file(".generated/brui.h",       nob_temp_sprintf("%s/include/brui.h",                g_install_prefix))) return false;
  if (false == nob_copy_file(".generated/brplat.h",     nob_temp_sprintf("%s/include/brplat.h",              g_install_prefix))) return false;
  if (false == nob_copy_file(".generated/FULL_LICENSE", nob_temp_sprintf("%s/share/licenses/brplot/LICENSE", g_install_prefix))) return false;
  if (has_docs) {
    for (int i = 0; i < sizeof(g_doc_files)/sizeof(g_doc_files[0]); ++i) {
      char* infile = strdup(nob_temp_sprintf(".generated/%s.%d.gz", g_doc_files[i].name, g_doc_files[i].section));
      char* outfile = strdup(nob_temp_sprintf("%s/share/man/man%d/%s.%d.gz", g_install_prefix, g_doc_files[i].section, g_doc_files[i].name, g_doc_files[i].section));
      nob_copy_file(infile, outfile);
    }
  }
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
  g_install_prefix = ".generated/brplot-v"BR_VERSION_STR;
  n_install_do();
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "tar", "czf", ".generated/brplot-v" BR_VERSION_STR ".tar.gz", "-C", ".generated", "brplot-v" BR_VERSION_STR);
  if (false == nob_cmd_run(&cmd)) return false;
  nob_cmd_free(cmd);
  return true;
}

static bool n_publish_do(void) {
  disable_logs = true;
  is_debug = false;
  if (false == n_unittests_do()) return false;
  if (false == n_dist_do()) return false;
  if (false == n_pip_do()) return false;

  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "./tools/is_clean.sh");
  if (false == nob_cmd_run(&cmd) && false == is_ignore_dirty) LOGF("Can't publish because the repo is not clean.");
  nob_cmd_append(&cmd, "git", "tag", "v" BR_VERSION_STR);
  if (false == nob_cmd_run(&cmd)) LOGF("Can't publish because the version v" BR_VERSION_STR " is already publish. Increment the version in include/brplot.h:48");
  nob_cmd_append(&cmd, "gh", "release", "create", "v" BR_VERSION_STR, "--target", "master", "--notes", "Bump.", "--generate-notes", ".generated/brplot-v" BR_VERSION_STR ".tar.gz");
  if (false == nob_cmd_run(&cmd)) LOGF("Failed to publish..");
  nob_cmd_append(&cmd, "gh", "release", "upload", "v" BR_VERSION_STR, ".generated/brplot-v" BR_VERSION_STR "/include/brplot.h");
  if (false == nob_cmd_run(&cmd)) LOGF("Failed to publish..");
  if (false == n_aur_do()) return false;
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
    //is_rebuild = true;
    is_debug = false;
    disable_logs = true;
    if (false == n_dist_do()) return false;
    if (false == nob_mkdir_if_not_exists("packages/pip/src")) return false;
    if (false == nob_mkdir_if_not_exists("packages/pip/src/brplot")) return false;
    Nob_String_Builder pytoml = { 0 };
    if (false == nob_copy_file(".generated/brplot-v" BR_VERSION_STR "/share/licenses/brplot/LICENSE", "packages/pip/LICENSE")) return false;
    if (false == nob_copy_file("README.md", "packages/pip/README.md")) return false;
    if (false == nob_copy_file(".generated/brplot-v" BR_VERSION_STR "/include/brplot.h", "packages/pip/src/brplot/brplot.c")) return false;
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

static bool n_pip_upload_do(void) {

}

static bool n_aur_do(void) {
  Nob_Cmd cmd = { 0 };
  Nob_String_Builder aur = { 0 };
  br_str_t hash = { 0 };
  if (false == generate_version_file(&hash)) return false;

  if (false == nob_read_entire_file("packages/aur/PKGBUILD", &aur)) return false;
  br_str_t aur_br_str = { .str = aur.items, .len = aur.count, .cap = aur.capacity };

  if (false == br_str_replace_one1(&aur_br_str, BR_STRL("{VERSION}"), BR_STRL(BR_VERSION_STR))) return false;
  if (false == br_str_replace_one1(&aur_br_str, BR_STRL("{VERSION}"), BR_STRL(BR_VERSION_STR))) return false;
  if (false == br_str_replace_one1(&aur_br_str, BR_STRL("{HASH}"), br_str_as_view(hash))) return false;

  nob_set_current_dir("packages/aur");
  nob_cmd_append(&cmd, "git", "clone", "ssh://aur@aur.archlinux.org/brplot-git.git");
  nob_cmd_run(&cmd);
  nob_set_current_dir("brplot-git");
  nob_cmd_append(&cmd, "git", "pull", "ssh://aur@aur.archlinux.org/brplot-git.git");
  nob_cmd_run(&cmd);
  if (false == nob_write_entire_file("PKGBUILD", aur_br_str.str, aur_br_str.len)) return false;

  Nob_File_Paths children = { 0 };
  nob_read_entire_dir(".", &children);
  for (int i = 0; i < children.count; ++i) {
    if (NULL != strstr(children.items[i], ".zst")) {
      nob_delete_file(children.items[i]);
    }
  }
  nob_da_free(children);

  nob_cmd_append(&cmd, "namcap", "PKGBUILD");
  if (false == nob_cmd_run(&cmd)) return false;

  nob_cmd_append(&cmd, "makepkg", "-si");
  if (false == nob_cmd_run(&cmd)) return false;

  nob_cmd_append(&cmd, "namcap", "*.zst");
  if (false == nob_cmd_run(&cmd)) return false;

  nob_cmd_append(&cmd, "makepkg", "--printsrcinfo");
  if (false == nob_cmd_run(&cmd, .stdout_path = ".SRCINFO")) return false;

  nob_cmd_append(&cmd, "git", "add", "PKGBUILD", ".SRCINFO");
  if (false == nob_cmd_run(&cmd)) return false;

  nob_cmd_append(&cmd, "git", "commit", "-m", "Vesion bump.");
  if (false == nob_cmd_run(&cmd)) return false;

  nob_cmd_append(&cmd, "git", "push", "origin", "master");
  if (false == nob_cmd_run(&cmd)) return false;

  nob_set_current_dir("../../..");
  return true;
}

static bool n_unittests_do(void) {
  if (false == n_generate_do()) return false;

  Nob_Cmd cmd = { 0 };
  is_debug = true;
  is_headless = true;
  const char* compiler = get_compiler(get_target());
  Nob_Cmd additional = { 0 };

  const char* shl_obj = compiler_single_file_obj(get_target(), "tests/src_tests/shl.c", "build/shl_test");
  nob_cmd_append(&additional, shl_obj);

  if (get_target() != p_windows) nob_cmd_append(&additional, "-lm");

  static struct { char const *test_file, *out_bin; } test_programs[] = {
    { .test_file = "./tests/src_tests/ui.c", .out_bin  = "bin/ui" },
    { .test_file = "./tests/src_tests/data_generator.c", .out_bin  = "bin/data_generator" },
    { .test_file = "./tests/src_tests/resampling.c", .out_bin  = "bin/resampling" },
    { .test_file = "./tests/src_tests/read_input.c", .out_bin  = "bin/read_input" },
    { .test_file = "./tests/src_tests/filesystem.c", .out_bin  = "bin/memory" },
    { .test_file = "./tests/src_tests/memory.c", .out_bin  = "bin/memory" },
    { .test_file = "./tests/src_tests/str.c", .out_bin  = "bin/test_str" },
    { .test_file = "./tests/src_tests/free_list.c", .out_bin  = "bin/test_fl" },
    { .test_file = "./tests/src_tests/math.c", .out_bin  = "bin/test_math" },
    { .test_file = "./tests/src_tests/string_pool.c", .out_bin  = "bin/string_pool" },
    { .test_file = "./tests/src_tests/da.c", .out_bin  = "bin/da" },
  };

  for (size_t i = 0; i < BR_ARR_LEN(test_programs); ++i) {
    LOGI("-------------- START TESTS ------------------");
    LOGI("------------- %15s -> %15s ------------------", test_programs[i].test_file, test_programs[i].out_bin);
    const char* output = compiler_single_file_exe2(get_target(), test_programs[i].test_file, test_programs[i].out_bin, additional);
    nob_cmd_append(&cmd, output);
    if (false == br_cmd_run(&cmd)) return false;
    LOGI("-------------- END TESTS ------------------");
  }

  LOGI("Unit tests ok");
  return true;
}

static bool n_fuzztests_do(void) {
#define FUZZ_FLAGS "-print_final_stats=1", "-timeout=1", "-max_total_time=200", "-create_missing_dirs=1", ".generated/corpus_sp"
  Nob_Cmd cmd = { 0 };
  is_headless = true;
  is_debug = true;
  const char* compiler = "clang"EXE_EXT;
  const char* shl_obj = compiler_single_file_obj(get_target(), "tests/src_tests/shl.c", "build/shl_test");

//  nob_cmd_append(&cmd, "clang", "-fsanitize=fuzzer,address,leak,undefined", "-DBR_DISABLE_LOG", "-DFUZZ", "-o", "bin/fuzz_read_input", "tools/unity/brplot.c");
//  compile_standard_flags(&cmd);
//  if (false == nob_cmd_run_sync_and_reset(&cmd)) return false;
//  nob_cmd_append(&cmd, "./bin/fuzz_read_input", FUZZ_FLAGS);
//  if (false == nob_cmd_run_sync_and_reset(&cmd)) return false;

  nob_cmd_append(&cmd, compiler, "-fsanitize=fuzzer,address,leak,undefined", "-DFUZZ", "-o", "bin/fuzz_sp", "./tests/src_tests/string_pool.c", shl_obj);
  compile_standard_flags(&cmd, false);
  if (false == br_cmd_run(&cmd)) return false;
  nob_cmd_append(&cmd, "./bin/fuzz_sp", FUZZ_FLAGS);
  if (false == br_cmd_run(&cmd)) return false;
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
    if (false == compile_one(p_native, &compile_command, nob_sv_from_cstr(sources[i]), &link_command, &build_dir)) goto error;
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

int main(int argc, char** argv) {
  NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "external/nob.h");

  bool success = true;

  cache.file_path = "nob.cache";
  cache.no_absolute = true;

  fill_command_flag_data();
  program_name = argv[0];
  if (argc == 1) {
    do_command = n_default;
  } else {
    for (int i = 1; i < argc; ++i) {
      const char* command = argv[i];
      if (command[0] == '-') continue; // Skip flags
#define X(name, desc) if (0 == strcmp(command, #name)) { do_command = n_ ## name; } else
      COMMANDS(X)
#undef X
      /* else */ {}
    }
  }
  if (false == apply_flags(do_command, argc, argv)) return 1;
  if (print_help) {
    print_usage();
    return 0;
  }
#define X(name, desc) if (do_command == n_ ## name) { success = n_ ## name ## _do();  } else
  COMMANDS(X)
#undef X
  /*else*/ {
    LOGE("Bad number of arguments");
    print_usage();
    LOGF("Bad usage... Exiting");
  }

  //nob__ptrace_cache_print_makefile(cache);
  //nob__ptrace_cache_print_debug(cache);
  if (success) nob_ptrace_cache_finish(cache);
  return success ? 0 : 1;
}

void br_on_fatal_error() {
  LOGE("Fatal");
}
void brgui_push_log_line(const char* fmt, ...) {}
// On linux, mac, bsds
// cc -o nob -I. nob.c -lm
// ./nob
//
// On Windows:
// Download clang
// clang -o ./nob.exe -I. nob.c
// ./nob.exe
