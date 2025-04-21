#include "src/br_pp.h"
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"
#include "src/filesystem.c"
#include "src/br_da.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ARR_LEN(ARR) (sizeof((ARR)) / sizeof((ARR)[0]))

typedef enum {
  cshl_token_kind_not_include,
  cshl_token_kind_comment,
  cshl_token_kind_include,
  cshl_token_kind_include_system,
} cshl_token_kind_t;

typedef struct {
  cshl_token_kind_t kind;
  int line_start;
  br_strv_t str;
  br_strv_t include_path;
  br_strv_t file_name;
} cshl_token_t;

typedef struct {
  cshl_token_t* arr;
  size_t len, cap;
} cshl_tokens_t;

typedef struct {
  br_strv_t* arr;
  size_t len, cap;
} files_t;

const char* source_files[] = {
  "include/brplot.h"
};

bool has_visited(files_t all_visited, br_strv_t file) {
  for (size_t i = 0; i < all_visited.len; ++i) if (br_strv_eq(all_visited.arr[i], file)) return true;
  return false;
}

void cshl_get_includes(br_strv_t file_name, files_t* includes) {
  size_t len = 0;
  const char* source_c = br_fs_read(br_strv_to_scrach(file_name), &len);
  br_scrach_free();
  if (source_c == NULL) return;
  br_strv_t source = BR_STRV(source_c, len);
  while (source.len > 0) {
    br_strv_t pp = br_strv_splitr(source, '#');
    br_strv_t single_comment = br_strv_splitrs(source, BR_STRL("//"));
    br_strv_t multi_comment = br_strv_splitrs(source, BR_STRL("/*"));
    if (multi_comment.len >= pp.len && multi_comment.len >= single_comment.len) {
      source = br_strv_splitrs(multi_comment, BR_STRL("*/"));
    } else if (single_comment.len >= pp.len) {
      source = br_strv_splitr(multi_comment, '\n');
    } else {
      pp = br_strv_skip(pp, ' ');
      if (br_strv_starts_with(pp, BR_STRL("include"))) {
        int out_index = 0;
        br_strv_t path = br_strv_any_splitr(pp, 3, (char[]) { '"', '<', '\n' }, &out_index);
        if (out_index == 0) {
          path = br_strv_splitl(path, '"');
          br_da_push(*includes, path);
        } else {
          pp = path;
        }
      }
      source = pp;
    }
  }
}

#define USE_EXTERNAL true
bool cshl_get_tokens(br_strv_t file_name, files_t* all_visited, cshl_tokens_t* tokens, int depth, bool only_includes) {
  if (true == has_visited(*all_visited, file_name)) return true;
  LOGI("%*s%.*s", depth*2, "", file_name.len, file_name.str);
  br_da_push(*all_visited, file_name);

  size_t len = 0;
  const char* source_c = br_fs_read(br_strv_to_scrach(file_name), &len);
  br_scrach_free();
  if (source_c == NULL) return false;
  br_strv_t source = BR_STRV(source_c, len);

  const char* end = &source.str[source.len - 1];
  br_strv_t preq = source;
  int line = 1;
  while (source.len > 0) {
    br_strv_t pp = br_strv_splitr(source, '#');
    br_strv_t single_comment = br_strv_splitrs(source, BR_STRL("//"));
    br_strv_t multi_comment = br_strv_splitrs(source, BR_STRL("/*"));

    BR_ASSERTF(end == source.str + source.len - 1, "diff: %zd, source.len: %u", end - (source.str + source.len - 1), source.len);
    if (pp.len > single_comment.len && pp.len > multi_comment.len) {
      source = pp;
      uint32_t preq_len = preq.len - source.len - 1;
      br_strv_t full_directive = BR_STRV(source.str - 1, 0);
      source = br_strv_skip(source, ' ');
      if (br_strv_starts_with(source, BR_STRL("include"))) {
        source = br_str_sub1(source, 7);
        source = br_strv_skip(source, ' ');
        if (br_strv_starts_with(source, BR_STRL("\""))) {
          source = br_str_sub1(source, 1);
          br_strv_t name = br_strv_splitl(source, '\"');
          full_directive.len = &name.str[name.len] - full_directive.str + 1;
          if (preq_len > 0) {
            preq.len = preq_len;
            if (false == only_includes) {
              cshl_token_t pret = {
                .kind = cshl_token_kind_not_include,
                .line_start = line,
                .str = preq,
              };
              br_da_push(*tokens, pret);
            }
            line += br_strv_count(preq, '\n');
          }
          cshl_token_t include_token = {
            .kind = cshl_token_kind_include,
            .line_start = line,
            .str = full_directive,
            .include_path = name,
            .file_name = file_name
          };
          br_da_push(*tokens, include_token);

          if (false == cshl_get_tokens(name, all_visited, tokens, depth + 1, only_includes)) {
            LOGE("Failed to get tokens from : %.*s:%d", file_name.len, file_name.str, line);
            --tokens->len;
          }
          const char* next_preq_start = name.str + name.len + 1;
          preq = BR_STRV(next_preq_start, end - next_preq_start + 1);
          source = preq;
        } else if (br_strv_starts_with(source, BR_STRL("<"))) {
          source = br_str_sub1(source, 1);
          br_strv_t name = br_strv_splitl(source, '>');
          full_directive.len = &name.str[name.len] - full_directive.str + 1;
          if (preq_len > 0) {
            preq.len = preq_len;
            if (false == only_includes) {
              cshl_token_t pret = {
                .kind = cshl_token_kind_not_include,
                .line_start = line,
                .str = preq,
              };
              br_da_push(*tokens, pret);
            }
            line += br_strv_count(preq, '\n');
          }
          if (false == only_includes) {
            cshl_token_t include_token = {
              .kind = cshl_token_kind_include_system,
              .line_start = line,
              .str = full_directive,
              .include_path = name
            };
            br_da_push(*tokens, include_token);
            LOGI("%*s-%.*s", depth*2, "", name.len, name.str);
          }

          const char* next_preq_start = name.str + name.len + 1;
          preq = BR_STRV(next_preq_start, end - next_preq_start + 1);
          source = preq;
        }
      } else if (br_strv_starts_with(source, BR_STRL("pragma"))) {
        br_strv_t pragma = br_str_sub1(source, 6);
        pragma = br_strv_skip(pragma, ' ');
        if (br_strv_starts_with(pragma, BR_STRL("once"))) {
          source = br_str_sub1(pragma, 4);
          preq = source;
        }
      }
    } else if (single_comment.len > 0 || multi_comment.len > 0) {
      uint32_t com_len = 0;
      const char* com_start = NULL;
      if (single_comment.len > multi_comment.len) {
        br_strv_t com_end = br_strv_splitr(single_comment, '\n'); // If you are using backslash to end you comments \
                                                                     Fuck you!
        com_start = single_comment.str - 2;
        com_len = com_end.str - com_start;
      } else {
        br_strv_t com_end = br_strv_splitrs(multi_comment, BR_STRL("*/"));
        com_start = multi_comment.str - 2;
        com_len = com_end.str - com_start;
      }
      int preq_len = com_start - preq.str;
      if (preq_len > 0) {
        if (false == only_includes) {
          preq.len = (uint32_t)preq_len;
          cshl_token_t pret = {
            .kind = cshl_token_kind_not_include,
            .line_start = line,
            .str = preq,
          };
          br_da_push(*tokens, pret);
        }
        line += br_strv_count(preq, '\n');
      }
      cshl_token_t include_token = {
        .kind = cshl_token_kind_comment,
        .line_start = line,
        .str = BR_STRV(com_start, com_len),
      };
      if (false == only_includes) {
        br_da_push(*tokens, include_token);
      }
      line += br_strv_count(include_token.str, '\n');
      const char* next_preq_start = com_start + com_len;
      preq = BR_STRV(next_preq_start, end >= next_preq_start ? end - next_preq_start + 1 : 0);
      source = preq;
    } else break;
  }
  if (preq.str < end) {
    if (false == only_includes) {
      cshl_token_t pret = {
        .kind = cshl_token_kind_not_include,
        .line_start = line,
        .str = BR_STRV(preq.str, end - preq.str + 1),
      };
      br_da_push(*tokens, pret);
    }
  }
  return true;
}

void fill_to_visit(files_t* to_visit) {
  for (size_t i = 0; i < ARR_LEN(source_files); ++i) {
    br_da_push(*to_visit, br_strv_from_c_str(source_files[i]));
  }
}

int do_create_single_header_lib(void) {
  files_t to_visit = { 0 };
  files_t visited = { 0 };

  cshl_tokens_t tokens = { 0 };
  fill_to_visit(&to_visit);

  while (to_visit.len > 0) {
    cshl_get_tokens(to_visit.arr[to_visit.len - 1], &visited, &tokens, 0, false);
    --to_visit.len;
  }
  LOGI("Found %zu tokens, %zu files visited.", tokens.len, visited.len);
  const char* out_amalgam = ".generated/brplot.c";
  {
    FILE* amalgam_file = fopen(out_amalgam, "wb");
    for (size_t i = 0; i < tokens.len; ++i) {
      cshl_token_t token = tokens.arr[i];
      if (token.kind == cshl_token_kind_include) {
        fprintf(amalgam_file, "/* %.*s */\n", tokens.arr[i].str.len, tokens.arr[i].str.str);
      } else if (token.kind == cshl_token_kind_include_system) {
        fprintf(amalgam_file, "%.*s", tokens.arr[i].str.len, tokens.arr[i].str.str);
      } else {
        fprintf(amalgam_file, "%.*s", tokens.arr[i].str.len, tokens.arr[i].str.str);
      }
    }
    fclose(amalgam_file);
    LOGI("Generated: %s", out_amalgam);
  }
  {
    const char* out_dependencies = ".generated/brplot.c.d";
    FILE* dependencies = fopen(out_dependencies, "wb+");
    fprintf(dependencies, "%s: \\\n", out_amalgam);
    for (size_t i = 0; i < tokens.len; ++i) {
      cshl_token_t token = tokens.arr[i];
      if (token.kind == cshl_token_kind_include) {
        fprintf(dependencies, "\t%.*s \\\n", token.include_path.len, token.include_path.str);
      }
    }
    fprintf(dependencies, "\n");
    fclose(dependencies);
    LOGI("Generated: %s", out_dependencies);
  }
  return 0;
}

#if !defined(BR_CREATE_SINGLE_HEADER_LIB_NO_MAIN)
int main(void) {
  return do_create_single_header_lib();
}
#endif

// cc -DBR_DEBUG -Wall -Wextra -Wpedantic -g -I. -o bin/cshl tools/create_single_header_lib.c && ./bin/cshl
// clang -DBR_DEBUG -Wall -Wextra -Wpedantic -g -I. -o bin/cshl.exe tools/create_single_header_lib.c; ./bin/cshl.exe
