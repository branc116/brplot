#define _CRT_SECURE_NO_WARNINGS
#define BR_NO_UNIT_TEST
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"

#define BR_SHADER_BAKE_NO_MAIN
#include "tools/shaders_bake.c"

#define BR_PACK_ICONS_NO_MAIN
#include "tools/pack_icons.c"

#define BR_GL_GEN_NO_MAIN
#include "tools/gl_gen.c"

#define BR_CREATE_SINGLE_HEADER_LIB_NO_MAIN
#include "tools/create_single_header_lib.c"

#if _WIN32
#  if defined(__GNUC__)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "gcc", "-I.", "-ggdb", "-o", binary_path, source_path
#  elif defined(__clang__)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "clang", "-I.", "-ggdb", "-o", binary_path, source_path
#  elif defined(_MSC_VER)
#     define NOB_REBUILD_URSELF(binary_path, source_path) "cl.exe", "-I.", "-Zi", nob_temp_sprintf("/Fe:%s", (binary_path)), source_path
#  endif
#else
#  define NOB_REBUILD_URSELF(binary_path, source_path) "cc", "-I.", "-ggdb", "-o", binary_path, source_path, "-lm"
#endif
#define NOB_IMPLEMENTATION
#include "external/nob.h"

#include "include/brplot.h"

#define COMMANDS(X) \
  X(build, "(default) Generate and build everything that is needed") \
  X(generate, "Generate additional files") \
  X(compile, "Only compile files, don't generate anything") \
  X(run, "Build and run brplot") \
  X(debug, "Build and run brplot with gdb") \
  X(amalgam, "Create amalgamation file that will be shipped to users") \
  X(dist, "Create distribution zip") \
  X(pip, "Create pip egg") \
  X(unittests, "Run unit tests") \
  X(dot, "Create Dot file with file dependencies") \
  X(help, "Print help") \

#define SANITIZER_FLAGS "-fsanitize=address,undefined"

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

#define n_default n_build

typedef struct {
  n_command* arr;
  size_t len, cap;
} command_deps_t;

const char* sources[] = {
 "src/main.c",
 "src/ui.c",
 "src/data.c",
 "src/smol_mesh.c",
 "src/q.c",
 "src/read_input.c",
 "src/keybindings.c",
 "src/str.c",
 "src/resampling2.c",
 "src/graph_utils.c",
 "src/shaders.c",
 "src/plotter.c", \
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
 "src/string_pool.c",
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

#define compiler "clang"
static const char* program_name;
static n_command do_command = n_default;
static bool is_debug = false;
static bool is_headless = false;
static bool enable_asan = false;
static bool print_help = false;
static bool is_lib = false;
static bool is_rebuild = false;
static bool is_slib = false;
static bool do_dist = true;
static bool pip_skip_build = false;
static bool disable_logs = false;
static bool is_pedantic = false;
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

typedef struct {
  br_strv_t file_name;
  char* file_name_c;
  struct {
    int* arr;
    size_t len, cap;
  } includes;
} inc_graph_el;

typedef struct {
  inc_graph_el* arr;
  size_t len, cap;
} inc_graph_t;

inc_graph_t g_inc_graph;

static void inc_push(inc_graph_t* inc_graph, br_strv_t name) {
  inc_graph_el new_el = { .file_name = name, .file_name_c = br_strv_to_c_str(name), .includes = { 0 } };
  br_da_push(*inc_graph, new_el);
}

static size_t find_or_add_el(inc_graph_t* inc_graph, br_strv_t name) {
  for (size_t i = 0; i < inc_graph->len; ++i) {
    if (br_strv_eq(inc_graph->arr[i].file_name, name) == true) return i;
  }
  inc_push(inc_graph, name);
  return inc_graph->len - 1;
}

static bool inc_find(inc_graph_t* inc_graph, br_strv_t name, int* out_index) {
  for (size_t i = 0; i < inc_graph->len; ++i) {
    if (br_strv_eq(inc_graph->arr[i].file_name, name) == true) {
      *out_index = (int)i;
      return true;
    }
  }
  return false;
}

static void create_include_graph(void) {
  for (size_t i = 0; i < ARR_LEN(sources); ++i) {
    inc_push(&g_inc_graph, br_strv_from_c_str(sources[i]));
  }

  files_t includes = { 0 };
  for (size_t i = 0; i < g_inc_graph.len; ++i) {
    includes.len = 0;
    cshl_get_includes(g_inc_graph.arr[i].file_name, &includes);
    for (size_t j = 0; j < includes.len; ++j) {
      int index = (int)find_or_add_el(&g_inc_graph, includes.arr[j]);
      br_da_push(g_inc_graph.arr[i].includes, index);
    }
  }
}

typedef struct {
  char** arr;
  size_t len, cap;
} file_names_t;

typedef struct {
  int* arr;
  size_t len, cap;
} indexies_t;

static void all_deps(int index, file_names_t* file_names, indexies_t* indexies) {
  bool contains = false;
  br_da_contains(*indexies, index, contains);
  if (contains) return;
  br_da_push(*indexies, index);
  br_da_push(*file_names, g_inc_graph.arr[index].file_name_c);
  for (size_t i = 0; i < g_inc_graph.arr[index].includes.len; ++i) {
    all_deps(g_inc_graph.arr[index].includes.arr[i], file_names, indexies);
  }
}

static bool needs_rebuilding(br_strv_t input, br_strv_t output) {
  int index = 0;
  if (false == inc_find(&g_inc_graph, input, &index)) return true;
  file_names_t file_names = { 0 };
  indexies_t indexies = { 0 };
  all_deps(index, &file_names, &indexies);
  if (nob_needs_rebuild(br_strv_to_c_str(output), (const char**)file_names.arr, file_names.len) > 0) return true;
  return false;
}

static void fill_command_flag_data(void) {
  command_flag_t debug_flag = (command_flag_t) {.name = BR_STRL("debug"), .alias = 'd', .description = BR_STRL("Build debug version"), .is_set = &is_debug};
  command_flag_t pedantic_flag = (command_flag_t) {.name = BR_STRL("pedantic"), .alias = 'p', .description = BR_STRL("Turn on all warnings and treat warnings as errors"), .is_set = &is_pedantic};
  command_flag_t headless_flag = (command_flag_t) {.name = BR_STRL("headless"), .alias = '\0', .description = BR_STRL("Create a build that will not spawn any windows"), .is_set = &is_headless};
  command_flag_t asan_flag = (command_flag_t) {.name = BR_STRL("asan"), .alias = 'a', .description = BR_STRL("Enable address sanitizer"), .is_set = &enable_asan};
  command_flag_t lib_flag = (command_flag_t) {.name = BR_STRL("lib"), .alias = 'l', .description = BR_STRL("Build dynamic library"), .is_set = &is_lib};
  command_flag_t force_rebuild = (command_flag_t) {.name = BR_STRL("force"), .alias = 'f', .description = BR_STRL("Force rebuild everything"), .is_set = &is_rebuild};
  command_flag_t slib_flag = (command_flag_t) {.name = BR_STRL("slib"), .alias = 's', .description = BR_STRL("Build static library"), .is_set = &is_slib};
  command_flag_t help_flag = (command_flag_t) {.name = BR_STRL("help"), .alias = 'h', .description = BR_STRL("Print help"), .is_set = &print_help};
  command_flag_t dist_flag = (command_flag_t) {.name = BR_STRL("dist"), .alias = 'D', .description = BR_STRL("Also create dist ( Needed for pip package but maybe you wanna disable this because it's slow )"), .is_set = &do_dist};
  command_flag_t pip_skip_build_flag = (command_flag_t) {.name = BR_STRL("skip-build"), .alias = '\0', .description = BR_STRL("Don't do anything except call pip to create a package"), .is_set = &pip_skip_build};
  br_da_push(command_flags[n_compile], debug_flag);
  br_da_push(command_flags[n_compile], pedantic_flag);
  br_da_push(command_flags[n_compile], headless_flag);
  br_da_push(command_flags[n_compile], asan_flag);
  br_da_push(command_flags[n_compile], lib_flag);
  br_da_push(command_flags[n_compile], slib_flag);
  br_da_push(command_flags[n_compile], force_rebuild);
  br_da_push(command_flags[n_help], help_flag);
  br_da_push(command_flags[n_pip], dist_flag);
  br_da_push(command_flags[n_pip], pip_skip_build_flag);

  br_da_push(command_deps[n_debug], n_build);
  br_da_push(command_deps[n_run], n_build);
  br_da_push(command_deps[n_build], n_compile);
  br_da_push(command_deps[n_build], n_generate);
  br_da_push(command_deps[n_dist], n_build);
  br_da_push(command_deps[n_pip], n_dist);
  br_da_push(command_deps[n_unittests], n_build);
  for (int i = 0; i < n_count; ++i) {
    br_da_push(command_deps[i], n_help);
  }
}

static bool create_dirs(Nob_String_View sv) {
  Nob_String_View cur = sv;
  cur.count = 1;
  for (cur.count = 1; cur.count < sv.count; ++cur.count) {
    if (cur.data[cur.count] == '\\' || cur.data[cur.count] == '/') {
      if (false == nob_mkdir_if_not_exists(nob_temp_sv_to_cstr(cur))) return false;
    }
  }
  return true;
}

static bool create_all_dirs(void) {
  nob_mkdir_if_not_exists("build");
  nob_mkdir_if_not_exists("build/debug");
  nob_mkdir_if_not_exists("build/debug/exe");
  nob_mkdir_if_not_exists("build/debug/slib");
  nob_mkdir_if_not_exists("build/debug/lib");
  nob_mkdir_if_not_exists("build/release");
  nob_mkdir_if_not_exists("build/release/exe");
  nob_mkdir_if_not_exists("build/release/slib");
  nob_mkdir_if_not_exists("build/release/lib");
  nob_mkdir_if_not_exists("bin");
  nob_mkdir_if_not_exists(".generated");
  return true;
}

static bool bake_font(void) {
  const char* file_in = "content/font.ttf";
  const char* file_out = ".generated/default_font.h";
  Nob_String_Builder content = { 0 };

  LOGI("Generate: %s -> %s", file_in, file_out);
  if (false == nob_read_entire_file(file_in, &content)) return false;
  FILE* out = fopen(file_out, "wb");

  if (out == NULL) {
    fprintf(stderr, "[ERROR] Failed to open a file %s: %s\n", file_out, strerror(errno));
    return false;
  }
  fprintf(out, "const unsigned char br_font_data[] = {");
  for (size_t i = 0; i < content.count; ++i) {
    if (i % 30 == 0) fprintf(out, "\n  ");
    unsigned int tmp = (unsigned int)(unsigned char)content.items[i];
    fprintf(out, "0x%x, ", tmp);
  }
  fprintf(out, "\n};\n");
  fprintf(out, "const long long br_font_data_size = %zu;\n", content.count);
  fclose(out);
  return true;
}

static bool generate_shaders(void) {
  const char* out_name = ".generated/shaders.h";
  LOGI("Generate: src/shaders/*.[vs|fs] -> %s", out_name);
  FILE* f = fopen(out_name, "wb+");
  if (NULL == f) {
    fprintf(stderr, "[ERROR] Failed to open a file %s: %s\n", out_name, strerror(errno));
    return false;
  }
  programs_t programs = get_programs();
  for (size_t i = 0; i < programs.len; ++i) {
    get_tokens(&programs.arr[i].vertex);
    get_tokens(&programs.arr[i].fragment);
  }
  get_program_variables(programs);
  check_programs(programs);
  
  shader_output_kind_t out_kind = g_platform == tp_web ? shader_output_kind_web : shader_output_kind_desktop;
  for (size_t i = 0; i < programs.len; ++i) {
    embed_tokens(f, programs.arr[i].name, br_str_from_c_str("fs"), programs.arr[i].fragment.tokens, out_kind);
    embed_tokens(f, programs.arr[i].name, br_str_from_c_str("vs"), programs.arr[i].vertex.tokens, out_kind);
  }
  fclose(f);
  return true;
}

static bool pack_icons(void) {
  LOGI("Generate: content/*.png -> .generated/icons.h, .genereated/icons.c");
  if (0 == do_pack(false)) return true;
  return false;
}

static bool gl_gen(void) {
  LOGI("Generate: null -> .generated/gl.c");
  return do_gl_gen() == 0;
}

static bool compile_one(Nob_Cmd* cmd, Nob_String_View source, Nob_Cmd* link_cmd) {
  static file_names_t names = { 0 };
  static indexies_t indexies = { 0 };
  names.len = 0;
  indexies.len = 0;

  Nob_String_View file_name = source;
  nob_sv_chop_by_delim(&file_name, '/');
  br_str_t build_dir = { 0 };
  br_str_push_literal(&build_dir, "build");
  br_fs_cd(&build_dir, is_debug ? BR_STRL("debug") : BR_STRL("release"));
  if (is_slib) br_fs_cd(&build_dir, BR_STRL("slib"));
  else if (is_lib) br_fs_cd(&build_dir, BR_STRL("lib"));
  else br_fs_cd(&build_dir, BR_STRL("exe"));
  br_fs_cd(&build_dir, BR_STRV(file_name.data, (uint32_t)file_name.count));
  if (enable_asan) br_str_push_literal(&build_dir, ".asan");
  br_str_push_literal(&build_dir, ".o");
  br_str_push_char(&build_dir, '\0');
  nob_cmd_append(link_cmd, build_dir.str);

  if (false == is_rebuild) {
    int found_index = 0;
    bool found = inc_find(&g_inc_graph, BR_STRV(source.data, (uint32_t)source.count), &found_index);
    if (found) {
      all_deps(found_index, &names, &indexies);
      int rebuild_is_needed = nob_needs_rebuild(build_dir.str, (const char**)names.arr, names.len);
      if (false == rebuild_is_needed) {
        LOGI("Skipping rebuild for %.*s", (int)source.count, source.data);
        return true;
      }
    }
  }
  nob_cmd_append(cmd, compiler, "-I.", "-o", build_dir.str, "-c", source.data);
  if (is_headless) {
    nob_cmd_append(cmd, "-DBR_NO_X11", "-DBR_NO_WAYLAND", "-DHEADLESS");
  }
  if (enable_asan) nob_cmd_append(cmd, SANITIZER_FLAGS);
  if (is_debug) {
    nob_cmd_append(cmd, "-ggdb", "-DBR_DEBUG");
  } else {
    nob_cmd_append(cmd, "-O3");
  }
  if (disable_logs) nob_cmd_append(cmd, "-DBR_DISABLE_LOG");
  if (is_pedantic) {
#if defined(_MSC_VER)
    LOGF("Pedantic flags for msvc are not known");
#else
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Wconversion", "-Wshadow");
#endif
  }

  if (is_lib) {
    nob_cmd_append(cmd, "-DBR_LIB");
#if !defined(_WIN32)
    nob_cmd_append(cmd, "-fPIC");
#endif
  }
  return nob_cmd_run_sync_and_reset(cmd);
}

static bool compile_and_link(Nob_Cmd* cmd) {
  Nob_Cmd link_command = { 0 };
  nob_cmd_append(&link_command, compiler, "-ggdb");

  for (size_t i = 0; i < NOB_ARRAY_LEN(sources); ++i) {
    if (false == compile_one(cmd, nob_sv_from_cstr(sources[i]), &link_command)) return false;
  }

  if (is_slib) nob_cmd_append(&link_command, "-o", "bin/brplot" SLIB_EXT);
  else if (is_lib) nob_cmd_append(&link_command, "-shared", "-fPIC", "-o", "bin/brplot" LIB_EXT);
  else {
    nob_cmd_append(&link_command, "-o", "bin/brplot" EXE_EXT);
  }
  if (tp_linux == g_platform) {
    nob_cmd_append(&link_command, "-lm", "-pthread");
  }

  if (enable_asan) nob_cmd_append(&link_command, SANITIZER_FLAGS);
  bool ret = nob_cmd_run_sync_and_reset(&link_command);
  BR_ASSERT(ret);
  nob_cmd_free(link_command);
  return ret;
}

static void print_command_flags_usage(n_command command, bool visited[n_count]) {
  if (true == visited[command]) return;
  visited[command] = true;

  command_flags_t flags = command_flags[command];
  for (size_t i = 0; i < flags.len; ++i) {
    command_flag_t flag = flags.arr[i];
    printf("        -%c, --%.*s - %.*s (%s)\n", flag.alias, flag.name.len, flag.name.str, flag.description.len, flag.description.str, *flag.is_set ? "ON" : "OFF");
  }

  for (size_t i = 0; i < command_deps[command].len; ++i) {
    n_command dep = command_deps[command].arr[i];
    print_command_flags_usage(dep, visited);
  }
}

static void print_usage(void) {
  printf("Usage: %s [command]\n", program_name);
  printf("command - command to do\n");
#define X(name, desc) do { \
    bool visited[n_count] = { 0 }; \
    printf("  " #name " - " desc "\n"); \
    print_command_flags_usage(n_ ## name, visited); \
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
  LOGI("Founing: `%.*s`", name.len, name.str);

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

static bool n_generate_do(void) {
  if (false == create_all_dirs())  return false;
  if (false == bake_font())        return false;
  if (false == generate_shaders()) return false;
  if (false == pack_icons())       return false;
  if (false == gl_gen())           return false;
  LOGI("Generate OK");
  return true;
}

static bool n_compile_do(void) {
  Nob_Cmd cmd = { 0 };
  if (false == compile_and_link(&cmd)) return false;
  return true;
}

static bool n_build_do(void) {
  return n_generate_do() && n_compile_do();
}

static bool n_run_do(void) {
  if (false == n_build_do()) return false;
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "bin/brplot" EXE_EXT);
  nob_cmd_run_sync(cmd);
  return true;
}

static bool n_debug_do(void) {
  is_debug = true;
  if (false == n_build_do()) return false;
  Nob_Cmd cmd = { 0 };
  nob_cmd_append(&cmd, "gdb", "bin/brplot" EXE_EXT);
  nob_cmd_run_sync(cmd);
  return true;
}

static bool n_amalgam_do(void) {
  return do_create_single_header_lib() == 0;
}

static bool n_help_do(void) {
  print_usage();
  return true;
}

#define DIST "dist"
#define PREFIX DIST "/brplot"
#define PLIB PREFIX "/lib"
#define PBIN PREFIX "/bin"
#define PINC PREFIX "/include"
#define PSHARE PREFIX "/share"
#define PSHARE PREFIX "/share"

static bool replace_in_file(const char* file_name, br_strv_t to_replace, br_strv_t replace_with) {
  Nob_String_Builder nob_content = { 0 };
  if (false == nob_read_entire_file(file_name, &nob_content)) return false;
  FILE* sink = fopen(file_name, "wb+");
  br_strv_t content = BR_STRV(nob_content.items, (uint32_t)nob_content.count);

  for (uint32_t i = 0; i < content.len; ++i) {
    bool is_match = true;
    if (to_replace.len + i <= content.len) {
      for (uint32_t j = 0; j < to_replace.len; ++j) {
        if (content.str[i + j] != to_replace.str[j]) {
          is_match = false;
          break;
        }
      }
    } else is_match = false;
    if (is_match) {
      fprintf(sink, "%.*s", replace_with.len, replace_with.str);
      br_strv_t rest = br_str_sub1(content, i + to_replace.len);
      fprintf(sink, "%.*s", rest.len, rest.str);
      break;
    } else {
      fprintf(sink, "%c", content.str[i]);
    }
  }

  fclose(sink);
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
  if (false == nob_copy_file("LICENSE", PSHARE "/licenses/brplot/LICENSE")) return false;
  if (false == nob_copy_file("bin/brplot" EXE_EXT, PBIN "/brplot" EXE_EXT)) return false;
  if (false == nob_copy_file("bin/brplot" SLIB_EXT, PLIB "/brplot" SLIB_EXT)) return false;
  if (false == nob_copy_file("bin/brplot" LIB_EXT, PLIB "/brplot" LIB_EXT)) return false;
  if (false == nob_copy_file("include/brplot.h", PINC "/brplot.h")) return false;
  if (false == n_amalgam_do()) return false;
  if (false == nob_copy_file(".generated/brplot.c", PINC "brplot.c")) return false;
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
    if (false == nob_copy_file("dist/brplot/share/licenses/brplot/LICENSE", "packages/pip/LICENSE")) return false;
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
  return nob_cmd_run_sync_and_reset(&cmd);
}

static bool n_unittests_do(void) {
  Nob_Cmd cmd = { 0 };
  LOGI("Unittest");
  is_debug = true;
  is_headless = true;
  if (false == n_build_do()) return false;
  nob_cmd_append(&cmd, "./bin/brplot" EXE_EXT, "--unittest");

  if (false == nob_cmd_run_sync_and_reset(&cmd)) return false;
  LOGI("Unit tests ok");
  return true;
}

static bool n_dot_do(void) {
  printf("digraph {\n");
  for (uint32_t i = 0; i < g_inc_graph.len; ++i) {
    for (uint32_t j = 0; j < g_inc_graph.arr[i].includes.len; ++j) {
      int in = g_inc_graph.arr[i].includes.arr[j];
      printf("  \"%s\" -> \"%s\";\n", g_inc_graph.arr[i].file_name_c, g_inc_graph.arr[in].file_name_c);
    }
  }
  printf("}\n");
  return true;
}

void br_go_rebuild_yourself(int argc, char** argv) {
  inc_push(&g_inc_graph, BR_STRL(__FILE__));
  create_include_graph();
  file_names_t names = { 0 };
  indexies_t indexies = { 0 };
  all_deps(0, &names, &indexies);
  nob__go_rebuild_urself2(argc, argv, (const char**)names.arr, (int)names.len);
  br_da_free(names);
  br_da_free(indexies);
}


int main(int argc, char** argv) {
  br_go_rebuild_yourself(argc, argv);
  fill_command_flag_data();
  program_name = argv[0];
  if (argc == 1) {
    do_command = n_default;
  } else {
    bool command_found = false;
    for (int i = 1; i < argc; ++i) {
      const char* command = argv[i];
      if (command[0] == '-') continue; // Skip flags for now
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
#define X(name, desc) if (do_command == n_ ## name) { return n_ ## name ## _do() ? 0 : 1; } else
  COMMANDS(X)
#undef X
  /*else*/ {
    LOGE("Bad number of arguments");
    print_usage();
    LOGF("Bad usage... Exiting");
  }
  LOGI("Nob finsihed ok");
  return 0;
}
// On linux, mac, bsds
// cc -o nob -I. nob.c -lm
// ./nob
//
// On Windows:
// Download clang
// clang -o ./nob.exe -I. nob.c
// ./nob.exe
