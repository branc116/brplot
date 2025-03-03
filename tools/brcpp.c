#include "src/br_str.h"
#include "src/str.c"
#include "src/filesystem.c"
#include "src/br_pp.h"
#include "src/br_da.h"

#include <stdio.h>
#include <stdlib.h>

#define TOKEN_KINDS(X) \
  X(tk_whitespace) \
  X(tk_hash) \
  X(tk_semi) \
  X(tk_popen) \
  X(tk_pclose) \
  X(tk_copen) \
  X(tk_cclose) \
  X(tk_string) \
  X(tk_ident) \
  X(tk_num) \

#define IS_IDEN_START(C) (((C) >= 'a' && (C) <= 'z') || ((C) >= 'A' && (C) <= 'Z') || ((C) == '_'))
#define IS_IDEN(C) (((C) >= 'a' && (C) <= 'z') || ((C) >= 'A' && (C) <= 'Z') || ((C) == '_') || ((C) >= '0' && (C) <= '9'))
#define IS_NUM(C) ((C) >= '0' && (C) <= '9')
#define IS_WS(C) ((C) == '\n' || (C) == ' ' || (C) == '\t')

typedef enum {
#define X(name) name,
  TOKEN_KINDS(X)
#undef X
} tk_t;

typedef struct {
  tk_t kind;
  br_strv_t str;
  int line_start;
  int offset;
  br_strv_t filename;
} token_t;

typedef struct {
  token_t* arr;
  size_t len, cap;
} tokens_t;

typedef struct {
  br_strv_t name;

  struct {
    br_strv_t arr;
    size_t len, cap;
  } arguments;
  tokens_t value;
} define_t;

typedef struct {
  define_t* arr;
  size_t len, cap;
} defines_t;

defines_t defines;

const char* tk_name(tk_t kind) {
  switch (kind) {
#define X(kind) case kind: return #kind;
    TOKEN_KINDS(X)
#undef X
  }
}

bool get_tokens(br_strv_t filename, tokens_t* tokens) {
  char* c_filename = br_strv_to_scrach(filename);
  size_t source_len = 0;
  char* c = br_fs_read("test.c", &source_len);
  if (c == NULL) return false;
  br_strv_t source = BR_STRV(c, source_len);
  int line = 1;
  int offset = 1;

  for (size_t i = 0; i < source_len;) {
    char c = source.str[i];
    if (IS_WS(c)) {
      int lines_to_add = 0;
      int new_offset = offset;
      int chars_to_add = 0;
      const char* start = &source.str[i];
      while (IS_WS(c)) {
        if (c == '\n') ++lines_to_add, ++chars_to_add, new_offset = 1;
        else ++new_offset, ++chars_to_add;
        c = source.str[++i];
      }
      token_t t = { 
        .str = BR_STRV(start, chars_to_add),
        .filename = filename,
        .kind = tk_whitespace,
        .line_start = line,
        .offset = offset
      };
      br_da_push(*tokens, t);
      line += lines_to_add;
      offset = new_offset;
    } else if (c == '#') {
      token_t t = { 
        .str = BR_STRV(&source.str[i++], 1),
        .filename = filename,
        .kind = tk_hash,
        .line_start = line,
        .offset = offset++
      };
      br_da_push(*tokens, t);
    } else if (c == ';') {
      token_t t = { 
        .str = BR_STRV(&source.str[i++], 1),
        .filename = filename,
        .kind = tk_semi,
        .line_start = line,
        .offset = offset++
      };
      br_da_push(*tokens, t);
    } else if (c == '(') {
      token_t t = { 
        .str = BR_STRV(&source.str[i++], 1),
        .filename = filename,
        .kind = tk_popen,
        .line_start = line,
        .offset = offset++
      };
      br_da_push(*tokens, t);
    } else if (c == ')') {
      token_t t = { 
        .str = BR_STRV(&source.str[i++], 1),
        .filename = filename,
        .kind = tk_pclose,
        .line_start = line,
        .offset = offset++
      };
      br_da_push(*tokens, t);
    } else if (c == '{') {
      token_t t = { 
        .str = BR_STRV(&source.str[i++], 1),
        .filename = filename,
        .kind = tk_copen,
        .line_start = line,
        .offset = offset++
      };
      br_da_push(*tokens, t);
    } else if (c == '}') {
      token_t t = { 
        .str = BR_STRV(&source.str[i++], 1),
        .filename = filename,
        .kind = tk_cclose,
        .line_start = line,
        .offset = offset++
      };
      br_da_push(*tokens, t);
    } else if (c == '"') {
      const char* start = &source.str[i];
      int offset_start = offset;
      ++offset;
      int chars_to_add = 1;
      bool last_backslash = false;
      c = source.str[++i];
      while (false == (last_backslash == false && c == '"')) {
        if (c == '\n') LOGF("[%.*s:%d,%d]Tokenizer: TODO: IMPLEMENT NEWLINE IN STRINGS! %d", filename.len, filename.str, line, offset, (int)c);
        last_backslash = c == '\\';
        c = source.str[++i];
        ++chars_to_add;
        ++offset;
      }
      token_t t = { 
        .str = BR_STRV(start, chars_to_add + 1),
        .filename = filename,
        .kind = tk_string,
        .line_start = line,
        .offset = offset_start
      };
      br_da_push(*tokens, t);
      ++i;
    } else if (IS_IDEN_START(c)) {
      int new_offset = offset;
      int chars_to_add = 0;
      const char* start = &source.str[i];
      while (IS_IDEN(c)) ++new_offset, ++chars_to_add, c = source.str[++i];
      token_t t = { 
        .str = BR_STRV(start, chars_to_add),
        .filename = filename,
        .kind = tk_ident,
        .line_start = line,
        .offset = offset
      };
      br_da_push(*tokens, t);
      offset = new_offset;
    } else if (IS_NUM(c)) {
      int new_offset = offset;
      int chars_to_add = 0;
      const char* start = &source.str[i];
      while (IS_NUM(c)) ++new_offset, ++chars_to_add, c = source.str[++i];
      token_t t = { 
        .str = BR_STRV(start, chars_to_add),
        .filename = filename,
        .kind = tk_num,
        .line_start = line,
        .offset = offset
      };
      br_da_push(*tokens, t);
      offset = new_offset;
    } else {
      LOGE("[%.*s:%d,%d]Tokenizer: unexpected character `%c`(%d)", filename.len, filename.str, line, offset, c, (int)c);
      return false;
    }
  }
  return true;
}

void define(FILE* out, tokens_t tokens, int* i) {
  token_t t = tokens.arr[*i];
  while ((t = tokens.arr[++*i]).kind == tk_whitespace) {
    fprintf(out, "%.*s", t.str.len, t.str.str);
  }
  fprintf(out, "%.*s", t.str.len, t.str.str);
  if (t.kind != tk_ident) LOGF("[%.*s:%d,%d]Emiter: Unexpected token (%s) in preprocessor", t.filename.len, t.filename.str, t.line_start, t.offset, tk_name(t.kind));
  br_strv_t name = t.str;
  int line = t.line_start;
  t = tokens.arr[++*i];
  fprintf(out, "%.*s", t.str.len, t.str.str);
  tokens_t define_tokens = { 0 };
  while (t.line_start == line) {
    br_da_push(define_tokens, t);
    t = tokens.arr[++*i];
    if (t.line_start != line) break;
    fprintf(out, "%.*s", t.str.len, t.str.str);
  }
  define_t d = {
    .name = name,
    .value = define_tokens
  };
  br_da_push(defines, d);
}

typedef enum {
  if_false,
  if_maybe,
  if_true,
} if_t;

if_t eval_pp(tokens_t tokens) {
  return if_true;
}

if_t if_directive(FILE* out, tokens_t tokens, int* i) {
  token_t t = tokens.arr[*i];
  while ((t = tokens.arr[++*i]).kind == tk_whitespace) {
    fprintf(out, "%.*s", t.str.len, t.str.str);
  }
  int line = t.line_start;
  tokens_t if_tokens = { 0 };
  while (t.line_start == line) {
    fprintf(out, "%.*s", t.str.len, t.str.str);
    br_da_push(if_tokens, t);
    t = tokens.arr[++*i];
  }
  if_t res = eval_pp(tokens);
  br_da_free(if_tokens);
  return res;
}

void include_directive(FILE* out, tokens_t tokens, int* i) {
  token_t t = tokens.arr[*i];
  t = tokens.arr[++*i];
  if (t.kind != tk_whitespace) LOGF("[%.*s:%d,%d]Emiter: Unexpected token (%s) in preprocessor", t.filename.len, t.filename.str, t.line_start, t.offset, tk_name(t.kind));
  fprintf(out, "%.*s", t.str.len, t.str.str);
  t = tokens.arr[++*i];
  if (t.kind != tk_string) LOGF("[%.*s:%d,%d]Emiter: Unexpected token (%s) in preprocessor", t.filename.len, t.filename.str, t.line_start, t.offset, tk_name(t.kind));
  fprintf(out, "%.*s", t.str.len, t.str.str);
  t = tokens.arr[++*i];
  if (t.kind != tk_whitespace) LOGF("[%.*s:%d,%d]Emiter: Unexpected token (%s) in preprocessor", t.filename.len, t.filename.str, t.line_start, t.offset, tk_name(t.kind));
}

bool emmit(FILE* out, tokens_t tokens) {
  struct {
    if_t* arr;
    size_t len, cap;
  } if_stack = { 0 };
#define TOP if_stack.arr[if_stack.len - 1]
  br_da_push(if_stack, if_true);

  for (int i = 0; i < tokens.len;) {
    token_t t = tokens.arr[i];
    if (TOP != if_false) {
      fprintf(out, "%.*s", t.str.len, t.str.str);
      if (t.kind == tk_hash) {
        while ((t = tokens.arr[++i]).kind == tk_whitespace) {
          fprintf(out, "%.*s", t.str.len, t.str.str);
        }
        fprintf(out, "%.*s", t.str.len, t.str.str);
        if (t.kind != tk_ident) LOGF("[%.*s:%d,%d]Emiter: Unexpected token (%s) in preprocessor", t.filename.len, t.filename.str, t.line_start, t.offset, tk_name(t.kind));
        if (br_strv_eq(t.str, BR_STRL("define"))) {
          define(out, tokens, &i);
        } else if (br_strv_eq(t.str, BR_STRL("if"))) {
          if_t res = if_directive(out, tokens, &i);
          br_da_push(if_stack, res);
        } else if (br_strv_eq(t.str, BR_STRL("include"))) {
          include_directive(out, tokens, &i);
        } else if (br_strv_eq(t.str, BR_STRL("else"))) {
          TOP = TOP == if_maybe ? if_maybe : if_false;
          ++i;
        } else LOGF("[%.*s:%d,%d]Emiter: Unexpected preprocessor directive `%.*s`", t.filename.len, t.filename.str, t.line_start, t.offset, t.str.len, t.str.str);
      } else {
        ++i;
      }
    } else {
      if (t.kind == tk_hash) {
        while ((t = tokens.arr[++i]).kind == tk_whitespace);
        if (br_strv_eq(t.str, BR_STRL("endif"))) {
          fprintf(out, "\n#endif");
          --if_stack.len;
          ++i;
        }
      } else {
        ++i;
      }
    }
  }
  return true;
}

int main(int argc, char** argv) {
  size_t s = 0;
  tokens_t tokens = { 0 };

  if (false == get_tokens(BR_STRL("test.c"), &tokens));
  emmit(stdout, tokens);
  return 0;
}
// cc -g -I. tools/brcpp.c -o bin/brcpp && bin/brcpp

