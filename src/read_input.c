#include "br_plot.h"
#include "src/misc/tests.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define MAX_REDUCE 6
#define MAX_NAME 64
#define REDUCTORS 9
#define INPUT_TOKENS_COUNT MAX_REDUCE
#define IS_VALID_NAME(C) ((c) >= 'a' && (c) <= 'z')
#define IS_VALID_PATH(c) (((c) >= '0' && (c) <= '1') || \
      ((c) >= 'a' && (c) <= 'z') || \
      ((c) >= 'a' && (c) <= 'z') || \
      (c) == '.' || \
      (c) == '/' || \
      (c) == '_')

typedef enum {
  input_token_any,
  input_token_number,
  input_token_name,
  input_token_quoted_string,
  input_token_path,
  input_token_comma,
  input_token_semicollon,
  input_token_dashdash
} input_token_kind_t;

typedef enum {
  input_lex_state_init,
  input_lex_state_number,
  input_lex_state_number_decimal,
  input_lex_state_number_exp,
  input_lex_state_number_reset,
  input_lex_state_name,
  input_lex_state_path,
  input_lex_state_dash,
  input_lex_state_quoted
} input_lex_state_t;

typedef struct {
  union {
    struct {
      float value_f;
      int value_i;
    };
    char name[MAX_NAME];
    br_str_t br_str;
  };
  input_token_kind_t kind;
} input_token_t;

typedef struct {
  input_token_kind_t kinds[MAX_REDUCE];
  void (*reduce_func)(br_plot_t* commands);
} input_reduce_t;

input_token_t tokens[INPUT_TOKENS_COUNT];
size_t tokens_len = 0;

static void input_reduce_y(br_plot_t* gv) {
  q_push(&gv->commands, (q_command){.type = q_command_push_point_y, .push_point_y = { .group = 0, .y = tokens[0].value_f} });
}

static void input_reduce_xy(br_plot_t* gv) {
  q_push(&gv->commands, (q_command){.type = q_command_push_point_xy, .push_point_xy = { .group = 0, .x = tokens[0].value_f, .y = tokens[2].value_f} });
}

static void input_reduce_xygroup(br_plot_t* gv) {
  q_push(&gv->commands, (q_command){.type = q_command_push_point_xy, .push_point_xy = { .group = tokens[4].value_i, .x = tokens[0].value_f, .y = tokens[2].value_f } });
}

static void input_reduce_ygroup(br_plot_t* gv) {
  q_push(&gv->commands, (q_command){.type = q_command_push_point_y, .push_point_y = { .group = tokens[2].value_i, .y = tokens[0].value_f } });
}

static void input_push_command_with_path(q_commands* commands, const char* path, q_command_type command) {
  size_t len = strlen(path);
  q_command cmd = {
    .type = command,
    .path_arg = { 
      .path = BR_MALLOC(len + 1) 
    }
  };
  memcpy(cmd.path_arg.path, path, len + 1);
  q_push(commands, cmd);
}

static void input_reduce_command(br_plot_t* gv) {
  (void)gv;
  if (0 == strcmp("zoomx", tokens[1].name)) {
    gv->uvZoom.x = tokens[2].value_f;
  } else if (0 == strcmp("zoomy", tokens[1].name)) {
    gv->uvZoom.y = tokens[2].value_f;
  } else if (0 == strcmp("zoom", tokens[1].name)) {
    gv->uvZoom.y = tokens[2].value_f;
    gv->uvZoom.x = tokens[2].value_f;
  } else if (0 == strcmp("offsetx", tokens[1].name)) {
    gv->uvOffset.x = tokens[2].value_f;
  } else if (0 == strcmp("offsety", tokens[1].name)) {
    gv->uvOffset.y = tokens[2].value_f;
  } else if (0 == strcmp("hide", tokens[1].name)) {
    q_push(&gv->commands, (q_command) { .type = q_command_hide, .hide_show = { .group = tokens[2].value_i } });
  } else if (0 == strcmp("show", tokens[1].name)) {
    q_push(&gv->commands, (q_command) { .type = q_command_show, .hide_show = { .group = tokens[2].value_i } });
  } else if (0 == strcmp("exportcsv", tokens[1].name)) {
    input_push_command_with_path(&gv->commands, tokens[2].name, q_command_exportcsv);
  } else if (0 == strcmp("export", tokens[1].name)) {
    input_push_command_with_path(&gv->commands, tokens[2].name, q_command_export);
  } else if (0 == strcmp("screenshot", tokens[1].name)) {
    input_push_command_with_path(&gv->commands, tokens[2].name, q_command_screenshot);
  } else if (0 == strcmp("exit", tokens[1].name)) {
    gv->should_close = true;
    exit(0);
  } else if (tokens[3].kind == input_token_quoted_string && 0 == strcmp("setname", tokens[1].name)) {
    q_command cmd = {
      .type = q_command_set_name,
      .set_quoted_str = {
        .group = tokens[2].value_i,
        .str = tokens[3].br_str
      }
    };
    q_push(&gv->commands, cmd);
    tokens[3].kind = input_token_any;
  } else
    printf("Execute %c%c%c...\n", tokens[1].name[0], tokens[1].name[1], tokens[1].name[2]);
}

input_reduce_t input_reductors_arr[REDUCTORS] = {
  {{input_token_number}, input_reduce_y},
  {{input_token_number, input_token_comma, input_token_number}, input_reduce_xy},
  {{input_token_number, input_token_comma, input_token_number, input_token_semicollon, input_token_number}, input_reduce_xygroup},
  {{input_token_number, input_token_semicollon, input_token_number}, input_reduce_ygroup},
  {{input_token_dashdash, input_token_name, input_token_number}, input_reduce_command},
  {{input_token_dashdash, input_token_name, input_token_path}, input_reduce_command},
  {{input_token_dashdash, input_token_name, input_token_name}, input_reduce_command},
  {{input_token_dashdash, input_token_name}, input_reduce_command},
  {{input_token_dashdash, input_token_name, input_token_number, input_token_quoted_string}, input_reduce_command},
};


static void input_tokens_pop(size_t n) {
  if (n == 0) return;
  for (size_t i = 0; i < n; ++i) {
    if (tokens[i].kind == input_token_quoted_string) {
      br_str_free(tokens[i].br_str);
    }
  }
  if (n >= INPUT_TOKENS_COUNT) {
    memset(tokens, 0, sizeof(tokens));
  }
  memmove(tokens, &tokens[n], sizeof(input_token_t) * (INPUT_TOKENS_COUNT - n));
  memset(&tokens[INPUT_TOKENS_COUNT - n], 0, sizeof(input_token_t) * n);
  tokens_len -= n;
}

static bool input_tokens_match_count(input_reduce_t* r, size_t* match_count) {
  size_t i = 0;
  for (; i < tokens_len && i < MAX_REDUCE && r->kinds[i] != input_token_any; ++i)
    if (tokens[i].kind != r->kinds[i]) return false;
  *match_count = i;
  return r->kinds[i] == input_token_any;
}

static bool input_tokens_can_next_be(input_token_kind_t kind) {
  bool bad_reducor[REDUCTORS] = {0};
  size_t j = 0;
  bool any = false;
  for (; j < tokens_len; ++j) {
    any = false;
    for (int i = 0; i < REDUCTORS; ++i) {
      if (bad_reducor[i] == false && input_reductors_arr[i].kinds[j] != tokens[j].kind) {
        bad_reducor[i] = true;
      } else {
        any = true;
      }
    }
    if (any == false) return false;
  }
  if (kind == input_token_any) {
    return any;
  }
  for (int i = 0; i < REDUCTORS; ++i) {
    if (bad_reducor[i] == false && input_reductors_arr[i].kinds[j] != kind) {
      bad_reducor[i] = true;
    } else {
      return true;
    }
  }
  return false;
}

static void input_tokens_reduce(br_plot_t* gv, bool force_reduce) {
  if (tokens_len == 0) return;
  int best = -1;
  size_t best_c = 0, match_c = 0;

  int best_reduce = -1;
  size_t best_reduce_count = 0;
  for (size_t i = 0; i < REDUCTORS; ++i) {
    size_t count = 0;
    bool can_reduce = input_tokens_match_count(&input_reductors_arr[i], &count);
    if (can_reduce) {
      if (count > best_reduce_count) {
        best_reduce_count = count;
        best_reduce = (int)i;
      }     }
    if (count > best_c) {
      best_c = count;
      match_c = 1;
      best = (int)i;
    } else if (count == best_c) {
      ++match_c;
    }
  }
  if (force_reduce == false) {
    if (best_c == 0) input_tokens_pop(1);
    else if ((match_c == 1 && best > -1 && input_reductors_arr[best].kinds[best_c] == input_token_any)) {
      input_reductors_arr[best].reduce_func(gv);
      input_tokens_pop((size_t)best_c);
    }
  } else {
    if (best_reduce_count == 0) input_tokens_pop(1);
    else if (best_reduce > -1) {
      input_reductors_arr[best_reduce].reduce_func(gv);
      input_tokens_pop((size_t)best_reduce_count);
    }
  }
}

static void lex(br_plot_t* gv) {
  float value_f = 0;
  int decimal = 0;
  int value_i = 0;
  char name[MAX_NAME] = {0};
  size_t name_len = 0;
  bool is_neg_whole = false;
  bool is_neg = false;
  bool read_next = true;

  input_lex_state_t state = input_lex_state_init;
  int c = -1;
  while (true) {
    if (read_next)
#ifdef RELEASE
      c = getchar();
#else
      c = gv->getchar();
#endif
    else read_next = true;
    if (c == -1) {
      input_tokens_reduce(gv, true);
      return;
    }
    switch (state) {
      case input_lex_state_init:
        input_tokens_reduce(gv, false);
        if (c == '-') {
          state = input_lex_state_dash;
        } else if (c >= '0' && c <= '9') {
          state = input_lex_state_number;
          read_next = false;
        } else if (c >= 'a' && c <= 'z' && input_tokens_can_next_be(input_token_name)) {
          state = input_lex_state_name;
          read_next = false;
        } else if (IS_VALID_PATH(c) && input_tokens_can_next_be(input_token_path)) {
          state = input_lex_state_path;
          read_next = false;
        } else if (c == '.') {
          state = input_lex_state_number_decimal;
        } else if (c == ',' && input_tokens_can_next_be(input_token_comma)) {
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_comma };
        } else if (c == ';' && input_tokens_can_next_be(input_token_semicollon)) {
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_semicollon };
        } else if (c == '"' && input_tokens_can_next_be(input_token_quoted_string)) {
          tokens[tokens_len] = (input_token_t) {
            .kind = input_token_quoted_string,
            .br_str = br_str_malloc(8),
          };
          state = input_lex_state_quoted;
        }
        break;
      case input_lex_state_dash:
        if (c == '-') {
          tokens[tokens_len++] = (input_token_t){ .kind = input_token_dashdash };
          state = input_lex_state_init;
        } else if (c >= '0' && c <= '9') {
          is_neg = true;
          is_neg_whole = true;
          state = input_lex_state_number;
          read_next = false;
        } else {
          state = input_lex_state_init;
        }
        break;
      case input_lex_state_number:
        if (c >= '0' && c <= '9') {
          value_i *= 10;
          value_i += c - '0';
        } else if (c == '.') {
          state = input_lex_state_number_decimal;
          value_f = (float)value_i;
          value_i = 0;
          is_neg = false;
        } else if (c == 'E' || c == 'e') {
          value_f = (float)value_i;
          value_i = 0;
          is_neg = false;
          decimal = 0;
          state = input_lex_state_number_exp;
        } else {
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = (float)value_i, .value_i = value_i };
          state = input_lex_state_number_reset;
          read_next = false;
        }
        break;
      case input_lex_state_number_decimal:
        if (c >= '0' && c <= '9') {
          value_i *= 10;
          value_i += c - '0';
          --decimal;
        } else if (c == 'E' || c == 'e') {
          value_f += (float)value_i * powf(10.f, (float)decimal);
          value_i = 0;
          decimal = 0;
          state = input_lex_state_number_exp;
        } else {
          value_f += (float)value_i * powf(10.f, (float)decimal);
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = is_neg_whole ? -value_f : value_f, .value_i = value_i };
          state = input_lex_state_number_reset;
          read_next = false;
        }
        break;
      case input_lex_state_number_exp:
        if (c >= '0' && c <= '9') {
          value_i *= 10;
          value_i += c - '0';
        } else if (c == '-') {
          is_neg = true;
        } else {
          value_f *= powf(10.f, is_neg ? (float)-value_i : (float)value_i);
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = is_neg_whole ? -value_f : value_f, .value_i = value_i };
          state = input_lex_state_number_reset;
          read_next = false;
        }
        break;
      case input_lex_state_number_reset:
        state = input_lex_state_init;
        read_next = false;
        value_f = 0.f;
        value_i = 0;
        decimal = 0;
        is_neg = false;
        is_neg_whole = false;
        break;
      case input_lex_state_name:
        if (name_len + 1 < MAX_NAME && c >= 'a' && c <= 'z') {
          name[name_len++] = (char)c;
        } else if ((name_len + 1 < MAX_NAME) && IS_VALID_PATH(c)) {
          read_next = false;
          state = input_lex_state_path;
        } else {
          memcpy(tokens[tokens_len].name, name, name_len);
          tokens[tokens_len].name[name_len] = (char)0;
          tokens[tokens_len++].kind = input_token_name;
          state = input_lex_state_init;
          name_len = 0;
          read_next = false;
        }
        break;
      case input_lex_state_path:
        if ((name_len + 1 < MAX_NAME) && IS_VALID_PATH(c)) {
          name[name_len++] = (char)c;
        } else {
          memcpy(tokens[tokens_len].name, name, name_len);
          tokens[tokens_len].name[name_len] = (char)0;
          tokens[tokens_len++].kind = input_token_path;
          state = input_lex_state_init;
          name_len = 0;
          read_next = false;
        }
        break;
      case input_lex_state_quoted:
        if (c == '"') {
          br_str_push_char(&tokens[tokens_len].br_str, (char)0);
          ++tokens_len;
          state = input_lex_state_init;
        } else br_str_push_char(&tokens[tokens_len].br_str, (char)c);
        break;
      default:
        assert(false);
    }
  }
}

void *read_input_main_worker(void* gv) {
  br_plot_t* gvt = gv;
  lex(gvt);
  return NULL;
}

#ifndef RELEASE
int test_str(void) {
  static size_t index = 0;
  const char str[] = "8.0,-16.0;1 -0.0078,16.0;1 \" \n \n 4.0;1\n\n\n\n\n\n 2.0 1;10;1;;;; 10e10 3e38 --test 1.2 --zoomx 10.0 1;12";
  if (index >= sizeof(str))
  {
    return -1;
  }
  return str[index++];
}

int test_str2(void) {
  static size_t index = 0;
  const char str[] = "--setname 1 \"hihi\" --setname 2 \"hihi hihi hihi hihi\"";
  if (index >= sizeof(str))
  {
    return -1;
  }
  return str[index++];
}

TEST_CASE(InputTests) {
  br_plot_t gvt;
  gvt.getchar = test_str;
  q_init(&gvt.commands);
  read_input_main_worker(&gvt);

  q_command c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_xy);
  TEST_EQUAL(c.push_point_xy.x, 8.f);
  TEST_EQUAL(c.push_point_xy.y, -16.f);
  TEST_EQUAL(c.push_point_xy.group, 1);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_xy);
  TEST_EQUAL(c.push_point_xy.x, -0.0078f);
  TEST_EQUAL(c.push_point_xy.y, 16.f);
  TEST_EQUAL(c.push_point_xy.group, 1);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 4.f);
  TEST_EQUAL(c.push_point_y.group, 1);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 2.f);
  TEST_EQUAL(c.push_point_y.group, 0);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 1.f);
  TEST_EQUAL(c.push_point_y.group, 10);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 1.f);
  TEST_EQUAL(c.push_point_y.group, 0);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 10e10f);
  TEST_EQUAL(c.push_point_y.group, 0);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 3e38f);
  TEST_EQUAL(c.push_point_y.group, 0);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_push_point_y);
  TEST_EQUAL(c.push_point_y.y, 1.f);
  TEST_EQUAL(c.push_point_y.group, 12);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_none);

  BR_FREE(gvt.commands.commands);
}

TEST_CASE(InputTests2) {
  br_plot_t gvt;
  gvt.getchar = test_str2;
  q_init(&gvt.commands);
  read_input_main_worker(&gvt);

  q_command c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_set_name);
  TEST_STREQUAL(c.set_quoted_str.str.str, "hihi");
  br_str_free(c.set_quoted_str.str);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_set_name);
  TEST_STREQUAL(c.set_quoted_str.str.str, "hihi hihi hihi hihi");
  br_str_free(c.set_quoted_str.str);

  c = q_pop(&gvt.commands);
  TEST_EQUAL(c.type, q_command_none);
  BR_FREE(gvt.commands.commands);
}
#endif
