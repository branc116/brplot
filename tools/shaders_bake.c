#define BR_SHADER_TOOL
#include "src/br_shaders.h"
#include "src/br_da.h"
#include "src/br_str.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define IS_SPECIAL_TOKEN(c) ((c) == '\n' || (c) == '\r' || (c) == '.' || (c) == ',' || (c) == '{' || (c) == '}' || (c) == ' ')
#define IS_NUMBER_TOKEN(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHA_TOKEN(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define FATAL(shader, line, offset, msg, ...) do { \
  fprintf(stderr, "|" __FILE__ ":%d||%s:%d:%d|ERROR: "msg"\n", __LINE__, br_str_to_c_str(shader->path), line, offset, __VA_ARGS__); \
  exit(1); \
} while(0)

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
  X(token_kind_lt) \
  X(token_kind_gt) \
  X(token_kind_or) \
  X(token_kind_question_mark) \
  X(token_kind_simic) \
  X(token_kind_plus) \
  X(token_kind_minus) \
  X(token_kind_times) \
  X(token_kind_div) \

typedef enum {
#define X(NAME) NAME,
TOKENS(X)
#undef X
} token_kind_t;

#define _TEX 20
typedef enum {
  variable_type_float = 1,
  variable_type_vec2 = 2,
  variable_type_vec3 = 3,
  variable_type_vec4 = 4,
  variable_type_mat4 = 16,
  variable_type_tex = 20
} variable_type_t;

typedef enum {
  variable_attr_in,
  variable_attr_out,
  variable_attr_uniform
} variable_attr_t;

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
  variable_attr_t attr;
  br_strv_t name;
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

  variables_t source_vars;
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

typedef enum {
  shader_output_kind_desktop,
  shader_output_kind_web
} shader_output_kind_t;

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
    variable_t var = { .attr = variable_attr_uniform, .name = br_strv_from_c_str(#NAME), .type = (variable_type_t)SIZE}; \
    br_da_push(variables_vertex, var); \
    br_da_push(variables_fragment, var); \
  }
# define X_B(NAME, SIZE) { \
    variable_t var = { .attr = variable_attr_in, .name = br_strv_from_c_str(#NAME), .type = (variable_type_t)SIZE}; \
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
        .variables = variables_fragment, \
        .source_vars = { 0 } \
      }, \
      .vertex = { \
        .path = br_str_from_c_str(NAME ## _vs), \
        .content = { 0 }, \
        .variables = variables_vertex, \
        .source_vars = { 0 } \
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
    ret.arr[i].fragment.content = read_entire_file(ret.arr[i].fragment.path);
    ret.arr[i].vertex.content = read_entire_file(ret.arr[i].vertex.path);
  }
  return ret;
}

variable_type_t get_variable_type(shader_t const* shader, size_t token_index) {
  token_t t = shader->tokens.arr[token_index];
  const char* s = br_strv_to_c_str(t.view);
  if (t.view.len == 4) {
    if (s[0] == 'v' && s[1] == 'e' && s[2] == 'c') {
      switch (s[3]) {
        case '2': return variable_type_vec2;
        case '3': return variable_type_vec3;
        case '4': return variable_type_vec4;
        default: FATAL(shader, t.line, t.start, "Unknown variable type: %s", s);
      }
    } else if (s[0] == 'm' && s[1] == 'a' && s[2] == 't') {
      switch (s[3]) {
        case '4': return variable_type_mat4;
        default: FATAL(shader, t.line, t.start, "Unknown variable type: %s", s);
      }
    } 
  } else if (strcmp(s, "float") == 0) return variable_type_float;
  else if (strcmp(s, "sampler2D") == 0) return variable_type_tex;
  FATAL(shader, t.line, t.start, "Unknown variable type: %s", s);
}

void get_shader_variables(shader_t* shader) {
  int depth_curly = 0;
  for (size_t i = 0; i < shader->tokens.len; ++i) {
    token_t t = shader->tokens.arr[i];
    char* v = br_strv_to_c_str(t.view);
    bool any = true;
    variable_attr_t attr;
    if (t.kind != token_kind_identifier) continue;
    if (0 == strcmp(v, "in")) attr = variable_attr_in;
    else if (0 == strcmp(v, "out")) attr = variable_attr_out;
    else if (0 == strcmp(v, "uniform")) attr = variable_attr_uniform;
    else any = false;
    if (any) {
      variable_type_t type = get_variable_type(shader, i + 1);
      br_strv_t name = shader->tokens.arr[i + 2].view;
      variable_t var = {
        .attr = attr,
        .name = name,
        .type = type
      };
      br_da_push(shader->source_vars, var);
    }
    BR_FREE(v);
  }
}

void get_program_variables(programs_t programs) {
  for (size_t i = 0; i < programs.len; ++i) {
    get_shader_variables(&programs.arr[i].vertex);
    get_shader_variables(&programs.arr[i].fragment);
  }
}

void embed_tokens(FILE* out, br_str_t name, br_str_t name_postfix, tokens_t tokens, shader_output_kind_t kind) {
  fprintf(out, "#define %s_%s \"", br_str_to_c_str(name), br_str_to_c_str(name_postfix));
  switch (kind) {
    case shader_output_kind_desktop: fprintf(out, "#version 330\\n"); break;
    case shader_output_kind_web: fprintf(out, "#version 300 es\\n"); break;
    default: fprintf(stderr, "ERROR: Bad output kind: %d\n", kind);
  }
  fprintf(out, "\" \\\n\"");
  bool was_last_iden = false;
  for (size_t i = 3; i < tokens.len; ++i) {
    token_t t = tokens.arr[i];
    if (t.kind == token_kind_preprocess) {
      fprintf(out, "\\n\"\n\"");
      for (; i < tokens.len && t.line == tokens.arr[i].line; ++i) {
        fprintf(out, "%s ", br_strv_to_c_str(tokens.arr[i].view));
      }
      fprintf(out, "\\n\" \\\n\"\n");
      was_last_iden = false;
      --i;
    } else {
      bool is_iden = t.kind == token_kind_identifier;
      if (was_last_iden && is_iden) fprintf(out, " ");
      fprintf(out, "%s", br_strv_to_c_str(t.view));
      was_last_iden = is_iden;
    }
  }
  fprintf(out, "\"\n\n");
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
    } else if (cur == '<') {
      token_t token = init_token(token_kind_lt, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, token);
    } else if (cur == '>') {
      token_t token = init_token(token_kind_gt, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, token);
    } else if (cur == '|') {
      token_t token = init_token(token_kind_or, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, token);
    } else if (cur == '?') {
      token_t token = init_token(token_kind_question_mark, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, token);
    } else if (cur == ':') {
      token_t token = init_token(token_kind_simic, line, offset, &shader->content.str[i]);
      ++offset;
      br_da_push(shader->tokens, token);
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
          FATAL(shader, line, offset, "Multiline comments aren't implemented %d", 69);
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
      FATAL(shader, line, offset, "Unknown start of token: `%c`(%d)", cur, cur);
    }
  }
}

#define BAD_TOKEN(SHADER, EXPECTED, ACTUAL) \
  FATAL(shader, token.line, token.start, "Expected token %s but got %s", \
    token_kind_to_str(EXPECTED), token_kind_to_str(ACTUAL)) \

#define EXPECT_TOKEN_K(SHADER, TOKEN_INDEX, KIND) \
  do { \
    token_t token = SHADER->tokens.arr[TOKEN_INDEX]; \
    if (token.kind != KIND) { \
      BAD_TOKEN(SHADER, KIND, token.kind); \
    } \
  } while(0);

#define EXPECT_TOKEN_KS(SHADER, TOKEN_INDEX, KIND, VALUE) \
  do { \
    token_t token = SHADER->tokens.arr[TOKEN_INDEX]; \
    char* cur_val = br_strv_to_c_str(token.view); \
    if (SHADER->tokens.arr[TOKEN_INDEX].kind != KIND || 0 != strcmp(VALUE, cur_val)) { \
      FATAL(SHADER, token.line, token.start, "Expected token %s[%s] but got %s[%s]", \
          token_kind_to_str(KIND), VALUE, token_kind_to_str(token.kind), br_strv_to_c_str(token.view)); \
    } \
    BR_FREE(cur_val); \
  } while(0);

#define ALL_CHECKS(X) \
  X(check_version) \
  X(check_numbers) \
  X(check_used_variables) \
  X(check_set_precision)

void check_numbers(shader_t const* shader) {
  for (size_t i = 3; i < shader->tokens.len; ++i) {
    if (shader->tokens.arr[i].kind == token_kind_number) {
      token_t t1 = shader->tokens.arr[i + 1];
      if (t1.kind == token_kind_identifier && t1.view.str[0] == 'e') continue;
      if (t1.kind == token_kind_dot) {
      } else {
        EXPECT_TOKEN_K(shader, i + 1, token_kind_dot);
      }
      token_t t2 = shader->tokens.arr[i + 2];
      if (t2.kind == token_kind_number) i+=2;
      else ++i;
    }
  }
}

void check_version(shader_t const* shader) {
  if (shader->tokens.len <= 3) {
    FATAL(shader, 0, 0, "Expected shader to have more than 2 tokens, found only %zu.\n", shader->tokens.len);
  }
  EXPECT_TOKEN_K(shader, 0, token_kind_preprocess);
  EXPECT_TOKEN_KS(shader, 1, token_kind_identifier, "version");
  EXPECT_TOKEN_KS(shader, 2, token_kind_number, "330");
}

void check_used_variables(shader_t const* shader) {
  for (size_t i = 0; i < shader->source_vars.len; ++i) {
    int n = 0;
    br_strv_t name = shader->source_vars.arr[i].name;
    for (size_t j = 0; j < shader->tokens.len; ++j) {
      if (shader->tokens.arr[j].kind != token_kind_identifier) continue;
      if (!br_strv_eq(shader->tokens.arr[j].view, name)) continue;
      if (++n == 2) break;
    }
    if (n < 2) FATAL(shader, 0, 0, "Unused variable `%s`", br_strv_to_c_str(name));
  }
}

void check_set_precision(shader_t const* shader) {
  for (size_t i = 0; i < shader->tokens.len; ++i) {
    //precision mediump float;
    token_t t = shader->tokens.arr[i];
    if (strcmp("precision", br_strv_to_c_str(t.view))) continue;
    t = shader->tokens.arr[i + 1];
    if (strcmp("mediump", br_strv_to_c_str(t.view))) {
      FATAL(shader, shader->tokens.arr[i].line, shader->tokens.arr[i].start, "Bad precision `%s`, only supported is mediump",
          br_strv_to_c_str(t.view));
    }
    t = shader->tokens.arr[i + 2];
    if (strcmp("float", br_strv_to_c_str(t.view))) {
      FATAL(shader, shader->tokens.arr[i].line, shader->tokens.arr[i].start, "WTF is precision mediump %s ???",
          br_strv_to_c_str(t.view));
    }
    return;
  }
  FATAL(shader, 0, 0, "precision mediump float missing!%d", 69);
}

void check_programs(programs_t programs) {
  for (size_t i = 0; i < programs.len; ++i) {
#define X(name) name(&programs.arr[i].vertex); \
    name(&programs.arr[i].fragment);
    ALL_CHECKS(X)
#undef X
    shader_t* vs = &programs.arr[i].vertex;
    shader_t* fs = &programs.arr[i].fragment;
    variables_t vars = vs->variables;
    for (size_t j = 0; j < vars.len; ++j) {
      variable_t v = vars.arr[j];
      if (v.attr == variable_attr_in) {
        variables_t srcvs = vs->source_vars;
        for (size_t k = 0; k < srcvs.len; ++k) {
          variable_t srcv = srcvs.arr[k];
          if (false == br_strv_eq(srcv.name, v.name)) continue;
          if (srcv.type != v.type) {
            FATAL(vs, 0, 0, "U said `%s` is of type %d but it's of type %d...", br_strv_to_c_str(v.name), v.type, srcv.type);
          }
          if (srcv.attr == v.attr) goto ok;
          FATAL(vs, 0, 0, "U said `%s` is `in` but it's not...", br_strv_to_c_str(v.name));
        }
        FATAL(vs, 0, 0, "U said `%s` vertex buffer object of type %d should be in this shader, but it's not...", br_strv_to_c_str(v.name), v.type);
      } else if (v.attr == variable_attr_uniform) {
        variables_t srcvs = vs->source_vars;
        for (size_t k = 0; k < srcvs.len; ++k) {
          variable_t srcv = srcvs.arr[k];
          if (false == br_strv_eq(srcv.name, v.name)) continue;
          if (srcv.type != v.type) {
            FATAL(vs, 0, 0, "U said `%s` is of type %d but it's of type %d...", br_strv_to_c_str(v.name), v.type, srcv.type);
          }
          if (srcv.attr == v.attr) goto ok;
          FATAL(vs, 0, 0, "U said `%s` is `uniform` but it's not...", br_strv_to_c_str(v.name));
        }
        srcvs = fs->source_vars;
        for (size_t k = 0; k < srcvs.len; ++k) {
          variable_t srcv = srcvs.arr[k];
          if (false == br_strv_eq(srcv.name, v.name)) continue;
          if (srcv.type != v.type) {
            FATAL(fs, 0, 0, "U said `%s` is of type %d but it's of type %d...", br_strv_to_c_str(v.name), v.type, srcv.type);
          }
          if (srcv.attr == v.attr) goto ok;
          FATAL(fs, 0, 0, "U said `%s` is `uniform` but it's not...", br_strv_to_c_str(v.name));
        }
        FATAL(vs, 0, 0, "U said `%s` uniform of type %d should be in this shader, but it's not...", br_strv_to_c_str(v.name), v.type);
      }
      ok:;
    }
    for (size_t j = 0; j < fs->source_vars.len; ++j) {
      variable_t var = fs->source_vars.arr[j];
      if (var.attr == variable_attr_uniform) {
        for (size_t k = 0; k < vars.len; ++k) {
          if (br_strv_eq(vars.arr[k].name, var.name)) goto ok1;
        }
        FATAL(fs, 0, 0, "Shader requires `%s` but it's not provided", br_strv_to_c_str(var.name));
        ok1:;
      }
    }
    for (size_t j = 0; j < vs->source_vars.len; ++j) {
      variable_t var = vs->source_vars.arr[j];
      if (var.attr != variable_attr_out) {
        for (size_t k = 0; k < vars.len; ++k) {
          if (br_strv_eq(vars.arr[k].name, var.name)) goto ok2;
        }
        FATAL(vs, 0, 0, "Shader requires `%s` but it's not provided", br_strv_to_c_str(var.name));
        ok2:;
      }
    }
  }
}

void exit_usage(const char* name) {
  fprintf(stderr, "ERROR: Bad input parametes\n"
          "Usage: %s WEB|LINUX|WINDOWS\n", name);
  exit(1);
}

int main(int argc, char const * const* argv) {
  if (argc < 2 || argc > 3) {
    exit_usage(argv[0]);
  }
  shader_output_kind_t output = shader_output_kind_desktop;
  if (0 == strcmp("WEB", argv[1])) output = shader_output_kind_web;
  FILE* f = stdout;
  if (argc == 3) {
    f = fopen(argv[2], "w");
    if (f == NULL) {
      fprintf(stderr, "Error opening a file %s: %d:%s\n", argv[2], errno, strerror(errno));
      exit(1);
    }
  }

  programs_t programs = get_programs();
  for (size_t i = 0; i < programs.len; ++i) {
    get_tokens(&programs.arr[i].vertex);
    get_tokens(&programs.arr[i].fragment);
  }
  get_program_variables(programs);
  check_programs(programs);
  
  for (size_t i = 0; i < programs.len; ++i) {
    embed_tokens(f, programs.arr[i].name, br_str_from_c_str("fs"), programs.arr[i].fragment.tokens, output);
    embed_tokens(f, programs.arr[i].name, br_str_from_c_str("vs"), programs.arr[i].vertex.tokens, output);
  }
  return 0;
}

