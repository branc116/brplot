#include "src/br_pp.h"
#include "src/filesystem.c"
#include "src/str.c"
#include "src/br_da.h"

#include <stdio.h>
#include <stdlib.h>

#define ARR_LEN(ARR) (sizeof((ARR)) / sizeof((ARR)[0]))

typedef enum {
  token_kind_not_include,
  token_kind_comment,
  token_kind_include,
  token_kind_include_system,
} token_kind_t;

typedef struct {
  token_kind_t kind;
  int line_start;
  br_strv_t str;
  br_strv_t include_path;
} token_t;

typedef struct {
  token_t* arr;
  size_t len, cap;
} tokens_t;

typedef struct {
  br_strv_t* arr;
  size_t len, cap;
} files_t;

const char* source_files[] = {
  "include/brplot.h"
};

files_t to_visit = { 0 };
files_t visited = { 0 };

bool has_visited(br_strv_t file) {
  for (size_t i = 0; i < visited.len; ++i) if (br_strv_eq(visited.arr[i], file)) return true;
  return false;
}

#define USE_EXTERNAL true
bool get_tokens(br_strv_t file_name, tokens_t* tokens, int depth) {
  if (true == has_visited(file_name)) return true;
  LOGI("%*s%.*s", depth*2, "", file_name.len, file_name.str);
  br_da_push(visited, file_name);

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
        source = br_strv_sub1(source, 7);
        source = br_strv_skip(source, ' ');
        if (br_strv_starts_with(source, BR_STRL("\""))) {
          source = br_strv_sub1(source, 1);
          br_strv_t name = br_strv_splitl(source, '\"');
          full_directive.len = &name.str[name.len] - full_directive.str + 1;
          if (preq_len > 0) {
            preq.len = preq_len;
            token_t pret = {
              .kind = token_kind_not_include,
              .line_start = line,
              .str = preq,
            };
            br_da_push(*tokens, pret);
            line += br_strv_count(preq, '\n');
          }
          token_t include_token = {
            .kind = token_kind_include,
            .line_start = line,
            .str = full_directive,
            .include_path = name
          };
          br_da_push(*tokens, include_token);

          if (false == get_tokens(name, tokens, depth + 1)) {
            LOGE("Failed to get tokens from : %.*s:%d", file_name.len, file_name.str, line);
          }
          const char* next_preq_start = name.str + name.len + 1;
          preq = BR_STRV(next_preq_start, end - next_preq_start + 1);
          source = preq;
        } else if (br_strv_starts_with(source, BR_STRL("<"))) {
          source = br_strv_sub1(source, 1);
          br_strv_t name = br_strv_splitl(source, '>');
          full_directive.len = &name.str[name.len] - full_directive.str + 1;
          if (preq_len > 0) {
            preq.len = preq_len;
            token_t pret = {
              .kind = token_kind_not_include,
              .line_start = line,
              .str = preq,
            };
            br_da_push(*tokens, pret);
            line += br_strv_count(preq, '\n');
          }
          token_t include_token = {
            .kind = token_kind_include_system,
            .line_start = line,
            .str = full_directive,
            .include_path = name
          };
          br_da_push(*tokens, include_token);
          LOGI("%*s-%.*s", depth*2, "", name.len, name.str);

          const char* next_preq_start = name.str + name.len + 1;
          preq = BR_STRV(next_preq_start, end - next_preq_start + 1);
          source = preq;
        }
      } else if (br_strv_starts_with(source, BR_STRL("pragma"))) {
        br_strv_t pragma = br_strv_sub1(source, 6);
        pragma = br_strv_skip(pragma, ' ');
        if (br_strv_starts_with(pragma, BR_STRL("once"))) {
          source = br_strv_sub1(pragma, 4);
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
        preq.len = (uint32_t)preq_len;
        token_t pret = {
          .kind = token_kind_not_include,
          .line_start = line,
          .str = preq,
        };
        br_da_push(*tokens, pret);
        line += br_strv_count(preq, '\n');
      }
      token_t include_token = {
        .kind = token_kind_comment,
        .line_start = line,
        .str = BR_STRV(com_start, com_len),
      };
      line += br_strv_count(include_token.str, '\n');
      br_da_push(*tokens, include_token);
      const char* next_preq_start = com_start + com_len;
      preq = BR_STRV(next_preq_start, end >= next_preq_start ? end - next_preq_start + 1 : 0);
      source = preq;
    } else break;
  }
  if (preq.str < end) {
    token_t pret = {
      .kind = token_kind_not_include,
      .line_start = line,
      .str = BR_STRV(preq.str, end - preq.str + 1),
    };
    br_da_push(*tokens, pret);
  }
  return true;
}

void fill_to_visit(void) {
  for (size_t i = 0; i < ARR_LEN(source_files); ++i) {
    br_da_push(to_visit, br_strv_from_c_str(source_files[i]));
  }
}

int main(void) {
  tokens_t tokens = { 0 };
  fill_to_visit();

  while (to_visit.len > 0) {
    get_tokens(to_visit.arr[to_visit.len - 1], &tokens, 0);
    --to_visit.len;
  }
  LOGI("Found %zu tokens, %zu files visited.", tokens.len, visited.len);
  const char* out_file_name = ".generated/brplot.c";
  FILE* amalgam_file = fopen(out_file_name, "wb");
  for (size_t i = 0; i < tokens.len; ++i) {
    token_t token = tokens.arr[i];
    if (token.kind == token_kind_include) {
      fprintf(amalgam_file, "/* %.*s */\n", tokens.arr[i].str.len, tokens.arr[i].str.str);
    } else if (token.kind == token_kind_include_system) {
      fprintf(amalgam_file, "%.*s", tokens.arr[i].str.len, tokens.arr[i].str.str);
    } else {
      fprintf(amalgam_file, "%.*s", tokens.arr[i].str.len, tokens.arr[i].str.str);
    }
  }
  fclose(amalgam_file);
  LOGI("Generated: %s", out_file_name);
  return 0;
}

// cc -DBR_DEBUG -Wall -Wextra -Wpedantic -g -I. -o bin/cshl tools/create_single_header_lib.c && ./bin/cshl
// clang -DBR_DEBUG -Wall -Wextra -Wpedantic -g -I. -o bin/cshl.exe tools/create_single_header_lib.c; ./bin/cshl.exe
