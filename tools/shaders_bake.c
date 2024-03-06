#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "src/br_shaders.h"
#include "src/br_plot.h"
#include "src/br_da.h"

#define grid_fs "src/desktop/shaders/grid.fs"
#define grid_vs "src/desktop/shaders/grid.vs"
#define line_fs "src/desktop/shaders/line.fs"
#define line_vs "src/desktop/shaders/line.vs"
#define quad_fs "src/desktop/shaders/quad.fs"
#define quad_vs "src/desktop/shaders/quad.vs"

#define grid_3d_fs "src/desktop/shaders/grid_3d.fs"
#define grid_3d_vs "src/desktop/shaders/grid_3d.vs"
#define line_3d_fs "src/desktop/shaders/line_3d.fs"
#define line_3d_vs "src/desktop/shaders/line_3d.vs"
#define line_3d_simple_fs "src/desktop/shaders/line_3d_simple.fs"
#define line_3d_simple_vs "src/desktop/shaders/line_3d_simple.vs"

#define IS_SPECIAL_TOKEN(c) ((c) == '\n' || (c) == '\r' || (c) == '.' || (c) == ',' || (c) == '{' || (c) == '}' || (c) == ' ')
#define IS_NUMBER_TOKEN(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHA_TOKEN(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

#define TOKENS(X) \
  X(token_kind_preprocess) \
  X(token_kind_number) \
  X(token_kind_identifier) \
  X(token_kind_semicolon) \
  X(token_kind_open_paren) \
  X(token_kind_close_paren) \
  X(token_kind_open_curly) \
  X(token_kind_close_curly) \
  X(token_kind_comma) \
  X(token_kind_dot) \
  X(token_kind_equals) \
  X(token_kind_equals_equals) \
  X(token_kind_plus) \
  X(token_kind_minus) \
  X(token_kind_times) \
  X(token_kind_div) \

typedef enum {
#define X(NAME) NAME,
TOKENS(X)
#undef X
} token_kind_t;

typedef enum {
  variable_type_float = 1,
  variable_type_vec2 = 2,
  variable_type_vec3 = 3,
  variable_type_vec4 = 4,
  variable_type_mat4 = 16,
} variable_type_t;

typedef struct {
  token_kind_t kind;
  int line, start;
  br_strv_t view;
} token_t;

typedef struct {
  token_t* arr;
  size_t len, cap;
} tokens_t;

typedef struct {
  bool is_uniform;
  br_str_t name;
  variable_type_t type;
} variable_t;

typedef struct {
  variable_t* arr;
  size_t len, cap;
} variables_t;

typedef struct {
  br_str_t path;
  br_str_t content;
  variables_t variables;
  tokens_t tokens;
} shader_t;

typedef struct {
  shader_t vertex;
  shader_t fragment;
  br_str_t name;
} program_t;

typedef struct {
  program_t* arr;
  size_t len, cap;
} programs_t;

typedef struct {
  shader_t* arr;
  size_t len, cap;
} shaders_t;

const char* token_kind_to_str(token_kind_t kind) {
  switch (kind) {
#define X(NAME) case NAME: return #NAME;
TOKENS(X)
#undef X
    default: return "unknown";
  }
}

br_str_t read_entire_file(br_str_t path) {
  br_str_t ret = { 0 };
  FILE* file = fopen(br_str_to_c_str(path), "r");
  if (file == NULL) {
    fprintf(stderr, "Error opening file %s: %d:%s\n", br_str_to_c_str(path), errno, strerror(errno));
    exit(1);
  }
  int cur;
  while ((cur = getc(file)) != EOF) br_str_push_char(&ret, (char)cur);
  return ret;
}

programs_t get_programs(void) {
  programs_t ret = { 0 };
# define X_U(NAME, SIZE) { \
    variable_t var = { .is_uniform = true, .name = br_str_from_c_str(#NAME), .type = (variable_type_t)SIZE}; \
    br_da_push(variables_vertex, var); \
    br_da_push(variables_fragment, var); \
  }
# define X_B(NAME, SIZE) { \
    variable_t var = { .is_uniform = false, .name = br_str_from_c_str(#NAME), .type = (variable_type_t)SIZE}; \
    br_da_push(variables_vertex, var); \
  }
# define X(NAME, CAP, VERT, BUFF) do { \
    variables_t variables_vertex = { 0 }; \
    variables_t variables_fragment = { 0 }; \
    VERT \
    BUFF \
    program_t p = { \
      .fragment = { \
        .path = br_str_from_c_str(NAME ## _fs), \
        .content = { 0 }, \
        .variables = { 0 } \
      }, \
      .vertex = { \
        .path = br_str_from_c_str(NAME ## _vs), \
        .content = { 0 }, \
        .variables = { 0 } \
      }, \
      .name = br_str_from_c_str(#NAME), \
    }; \
    br_da_push(ret, p); \
  } while(0);
  BR_ALL_SHADERS(X, X_U, X_B);
# undef X
# undef X_B
# undef X_U
  for (size_t i = 0; i < ret.len; ++i) {
    ret.arr[i].fragment.content = read_entire_file(ret.arr[i].vertex.path);
    ret.arr[i].vertex.content = read_entire_file(ret.arr[i].vertex.path);
  }
  return ret;
}

void embed(br_str_t name, br_str_t name_postfix, br_str_t content) {
  printf("#define %s_%s \"", br_str_to_c_str(name), br_str_to_c_str(name_postfix));
  size_t l = content.len;
  for (int j = 0; j < l; ++j) {
    char cur = content.str[j];
    if (cur == '\n') printf("\\n\" \\\n\"");
    else if (cur == '\r') continue;
    else printf("%c", cur);
  }
}

token_t init_token(token_kind_t kind, int line, int offset, const char* start) {
  return (token_t) {
    .kind = kind,
    .line = line,
    .start = offset,
    .view = { .str = start, .len = 1 }
  };
}

void get_tokens(shader_t* shader) {
  size_t i = 0;
  int line = 1;
  int offset = 0;
  bool increment = false;
  while (true) {
    if (increment) ++i;
    else increment = true;
    if (i >= shader->content.len) break;
    char cur = shader->content.str[i];

    if (cur == '#') {
      token_t preproc = init_token(token_kind_preprocess, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, preproc);
    } else if (IS_ALPHA_TOKEN(cur)) {
      token_t ident = init_token(token_kind_identifier, line, offset, &shader->content.str[i]);
      char cur = shader->content.str[i];
      int len = 0;
      do {
        cur = shader->content.str[i + ++len];
      } while ((IS_ALPHA_TOKEN(cur) || IS_NUMBER_TOKEN(cur) || cur == '_') && i + len < shader->content.len);
      ident.view.len = len;
      i += len;
      offset += len;
      increment = false;
      br_da_push(shader->tokens, ident);
    } else if (IS_NUMBER_TOKEN(cur)) {
      token_t number = init_token(token_kind_number, line, offset, &shader->content.str[i]);
      char cur = shader->content.str[i];
      int len = 0;
      do {
        cur = shader->content.str[i + ++len];
      } while ((IS_NUMBER_TOKEN(cur)) && i + len < shader->content.len);
      number.view.len = len;
      i += len;
      offset += len;
      increment = false;
      br_da_push(shader->tokens, number);
    } else if (cur == ';') {
      token_t semi = init_token(token_kind_semicolon, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, semi);
    } else if (cur == '(') {
      token_t paren = init_token(token_kind_open_paren, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, paren);
    } else if (cur == ')') {
      token_t paren = init_token(token_kind_close_paren, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, paren);
    } else if (cur == '{') {
      token_t paren = init_token(token_kind_open_curly, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, paren);
    } else if (cur == '}') {
      token_t paren = init_token(token_kind_close_curly, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, paren);
    } else if (cur == '=') {
      if (i + 1 >= shader->content.len ||  shader->content.str[i + 1] != '=') {
        token_t equals = init_token(token_kind_equals, line, offset, &shader->content.str[i]);
        ++offset;
        br_da_push(shader->tokens, equals);
      } else {
        token_t equals = init_token(token_kind_equals_equals, line, offset, &shader->content.str[i]);
        equals.view.len = 2;
        offset += 2;
        ++i;
        br_da_push(shader->tokens, equals);
      }
    } else if (cur == ',') {
      token_t comma = init_token(token_kind_comma, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, comma);
    } else if (cur == '.') {
      token_t dot = init_token(token_kind_dot, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, dot);
    } else if (cur == '-') {
      token_t minus = init_token(token_kind_minus, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, minus);
    } else if (cur == '+') {
      token_t plus = init_token(token_kind_plus, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, plus);
    } else if (cur == '*') {
      token_t times = init_token(token_kind_times, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, times);
    } else if (cur == '/') {
      if (i + 1 >= shader->content.len) {
        token_t div = init_token(token_kind_div, line, offset, &shader->content.str[i]);
        ++offset;
        br_da_push(shader->tokens, div);
      } else {
        char next = shader->content.str[i + 1];
        if (next == '/') {
          char cur = shader->content.str[i];
          int len = 0;
          do {
            cur = shader->content.str[i + ++len];
          } while (cur != '\n' && i + len < shader->content.len);
          i += len;
          ++line;
          offset = 0;
        } else if (next == '*') {
          fprintf(stderr, "|%s:%d:%d|ERROR: Multiline comment not implemented",
              br_str_to_c_str(shader->path), line, offset);
          assert(false);
        } else {
          token_t div = init_token(token_kind_div, line, offset, &shader->content.str[i]);
          ++offset;
          br_da_push(shader->tokens, div);
        }
      }
    } else if (cur == '\n') {
      ++line;
      offset = 0;
    } else if (cur == ' ') {
      ++offset;
    } else {
      fprintf(stderr, "|%s:%d:%d|ERROR: unknown start of token: `%c`(%d)", br_str_to_c_str(shader->path), line, offset, cur, cur);
      assert(false);
    }
  }
}

int main(void) {
  programs_t programs = get_programs();
  for (size_t i = 0; i < programs.len; ++i) {
    get_tokens(&programs.arr[i].vertex);
    get_tokens(&programs.arr[i].fragment);
  }
  {
    tokens_t tokens = programs.arr[0].vertex.tokens;
    for (int i = 0; i < tokens.len; ++i) {
      printf("%s<%s>\n", token_kind_to_str(tokens.arr[i].kind), br_strv_to_c_str(tokens.arr[i].view));
    }
  }
  {
    tokens_t tokens = programs.arr[0].fragment.tokens;
    for (int i = 0; i < tokens.len; ++i) {
      printf("%s<%s>\n", token_kind_to_str(tokens.arr[i].kind), br_strv_to_c_str(tokens.arr[i].view));
    }
  }
  return 0;
  
  for (size_t i = 0; i < programs.len; ++i) {
    embed(programs.arr[i].name, br_str_from_c_str("fs"), programs.arr[i].fragment.content);
    embed(programs.arr[i].name, br_str_from_c_str("vs"), programs.arr[i].vertex.content);
  }
  return 0;
}

