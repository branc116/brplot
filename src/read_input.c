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
#define IS_VALID_NAME(c) ((c) >= 'a' && (c) <= 'z')
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

typedef struct {
  br_str_t ex;
  int group;
} extractor_t;

typedef struct {
  extractor_t* arr;
  size_t len;
  size_t cap;
} extractors_t;

input_token_t tokens[INPUT_TOKENS_COUNT];
size_t tokens_len = 0;

static extractors_t extractors = {0};
static bool should_push_eagre = true;

static void input_reduce_y(br_plot_t* gv) {
  if (should_push_eagre) q_push(&gv->commands, (q_command){.type = q_command_push_point_y, .push_point_y = { .group = 0, .y = tokens[0].value_f} });
}

static void input_reduce_xy(br_plot_t* gv) {
  if (should_push_eagre) q_push(&gv->commands, (q_command){.type = q_command_push_point_xy, .push_point_xy = { .group = 0, .x = tokens[0].value_f, .y = tokens[2].value_f} });
}

static void input_reduce_xygroup(br_plot_t* gv) {
  if (should_push_eagre) q_push(&gv->commands, (q_command){.type = q_command_push_point_xy, .push_point_xy = { .group = tokens[4].value_i, .x = tokens[0].value_f, .y = tokens[2].value_f } });
}

static void input_reduce_ygroup(br_plot_t* gv) {
  if (should_push_eagre) q_push(&gv->commands, (q_command){.type = q_command_push_point_y, .push_point_y = { .group = tokens[2].value_i, .y = tokens[0].value_f } });
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

static void extractors_push(extractors_t* exs, extractor_t ex) {
  if (exs->arr == NULL) {
    exs->arr = BR_CALLOC(8, sizeof(*exs->arr));
    exs->cap = 8;
    exs->len = 0;
  }
  if (exs->cap == exs->len) {
    size_t new_cap = exs->cap * 2;
    exs->arr = BR_REALLOC(exs->arr, new_cap * sizeof(*exs->arr));
  }
  exs->arr[exs->len++] = ex;
}

typedef enum {
  extractor_state_init,
  extractor_state_capture,
  extractor_state_escape
} extractor_state_t;

typedef enum {
  extractor_res_state_none = 0,
  extractor_res_state_unfinished = 1,
  extractor_res_state_x = 2,
  extractor_res_state_y = 4,
  extractor_res_state_xy = extractor_res_state_x | extractor_res_state_y
} extractor_res_state_t;

#define ERRORF(format, ...) fprintf(stderr, "[ERROR]" format "\n", __VA_ARGS__)
#define ERROR(format) fprintf(stderr, "[ERROR]" format "\n")

static bool extractor_is_valid(br_strv_t ex) {
  bool is_cap = false;
  bool x_cap = false, y_cap = false;
  bool is_wild = false;
  bool is_escape = false;
  bool was_last_extract = false;
  bool increment = false;
  for (size_t i = 0; (increment && i + 1 < ex.len) || (!increment && i < ex.len) ;) {
    if (increment) ++i;
    else increment = true;
    char e = ex.str[i];
    if (is_escape) {
      is_escape = false;
    }
    else if (is_wild) {
      if (e == '*') {
        ERRORF("[%lu] Capture is invalid, can't have wild char next to a wild char", i);
        return false;
      }
      is_wild = false;
      increment = false;
    }
    else if (is_cap) {
      if (e == 'x') {
        if (x_cap == true) {
          ERRORF("[%lu] Can't have multiple captures for X!", i);
          return false;
        }
        x_cap = true;
      } else if (e == 'y') {
        if (y_cap == true) {
          ERRORF("[%lu] Can't have multiple captures for Y!", i);
          return false;
        }
        y_cap = true;
      } else {
        ERRORF("[%lu] You can only capture x or y! You tryed to capture `%c`", i, e);
        return false;
      }
      was_last_extract = true;
      is_cap = false;
    } else if (was_last_extract) {
      if (e == '%') {
        ERRORF("[%lu] Can't have captures one next to the other. You must have delimiter between them!", i);
        return false;
      }
      was_last_extract = false;
      increment = false;
    } else if (e == '*') {
      is_wild = true;
    } else if (e == '\\') {
      is_escape = true;
    } else if (e == '%') {
      is_cap = true;
    }
  }
  if (is_cap) {
    ERROR("Extractor ends unfinished. You Are ending it inside of the capture grupe. Capture can only be '%%x' or '%%y'!");
    return false;
  }
  if (is_escape) {
    ERROR("Extractor ends unfinished. You Are ending it while escaping something!");
    return false;
  }
  if (is_wild) {
    ERROR("Extractor ends unfinished. You Are ending it with wild character.");
    return false;
  }
  if (x_cap == false && y_cap == false) {
    ERROR("Your extractor doesn't extract anything. You must specify eather `%%x` or `%%y` as values you want to extract.");
    return false;
  }
  return true;

  return true;
}

static int extractor_extract_number(br_strv_t view, float* outF) {
  bool is_neg = false, is_neg_whole = false;
  input_lex_state_t state = input_lex_state_init;
  int i = 0;
  long value_i = 0;
  float value_f = 0;
  int decimal = 0;
  bool read_next = false;

  while ((size_t)i < view.len) {
    if (read_next) ++i;
    else read_next = true;
    char c = view.str[i];
    switch (state) {
      case input_lex_state_init:
        if (c >= '0' && c <= '9') {
          state = input_lex_state_number;
          read_next = false;
        } else if (c == '.') {
          state = input_lex_state_number_decimal;
        } else if (c == '-') {
          state = input_lex_state_dash;
        } else return -1;
        break;
      case input_lex_state_dash:
        if (c >= '0' && c <= '9') {
          is_neg = true;
          is_neg_whole = true;
          state = input_lex_state_number;
          read_next = false;
        } else return -1;
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
          *outF = is_neg_whole ? (float)-value_i : (float)value_i;
          return i;
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
          *outF = is_neg_whole ? -value_f : value_f;
          return i;
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
          *outF = is_neg_whole ? -value_f : value_f;
          return i;
        }
        break;
      default:
        assert("Unhandled state while extracting number" && false);
        return -1;
    }
  }
  return -1;
}

static extractor_res_state_t extractor_extract(br_strv_t ex, br_strv_t view, float* x, float* y) {
  unsigned int ex_i = 0, view_i = 0;
  extractor_state_t state = extractor_state_init;
  extractor_res_state_t res = extractor_res_state_none;

  while (ex_i < ex.len && view_i < view.len) {
    char ex_c = ex.str[ex_i];
    char view_c = view.str[view_i];
    switch (state) {
      case extractor_state_init:
        switch (ex_c) {
          case '*':
            ex_i++;
            for (unsigned int j = view_i; j < view.len; ++j) {
              float tmp_x, tmp_y;
              extractor_res_state_t r = extractor_extract(br_strv_sub1(ex, ex_i), br_strv_sub1(view, j), &tmp_x, &tmp_y);
              if (extractor_res_state_none != r && extractor_res_state_unfinished != r) {
                if (r & extractor_res_state_x) *x = tmp_x;
                if (r & extractor_res_state_y) *y = tmp_y;
                return r | res;
              } else ++view_i;
            }
            break;
          case '\\':
            state = extractor_state_escape;
            ++ex_i;
            break;
          case '%':
            state = extractor_state_capture;
            ++ex_i;
            break;
          default:
            if (ex_c == view_c) {
              ++view_i;
              ++ex_i;
            } else return extractor_res_state_none;
            break;
        }
        break;
      case extractor_state_capture: {
        float temp_float = 0.f;
        int r = extractor_extract_number(br_strv_sub1(view, view_i), &temp_float);
        if (r > 0) {
          if (ex_c == 'x') {
            *x = temp_float;
            res |= extractor_res_state_x;
          } else if (ex_c == 'y') {
            *y = temp_float;
            res |= extractor_res_state_y;
          } else assert(false);
          ++ex_i;
          view_i += (unsigned int)r;
          state = extractor_state_init;
        } else return extractor_res_state_none;
      }break;
      case extractor_state_escape:
        if (ex_c == view_c) {
          ++ex_i;
          ++view_i;
          state = extractor_state_init;
        } else return extractor_res_state_none;
        break;
    }
  }
  return ex_i < ex.len ? extractor_res_state_unfinished : res;
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
  } else if (0 == strcmp("extract", tokens[1].name)) {
    if (extractor_is_valid(br_str_as_view(tokens[3].br_str))) {
      br_str_t s = br_str_copy(tokens[3].br_str);
      --s.len;
      extractors_push(&extractors, (extractor_t) { .ex =  s, .group = tokens[2].value_i });
      should_push_eagre = false;
    }
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
} static void input_tokens_reduce(br_plot_t* gv, bool force_reduce) {
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
  long value_i = 0;
  char name[MAX_NAME] = {0};
  size_t name_len = 0;
  bool is_neg_whole = false;
  bool is_neg = false;
  bool read_next = true;
  br_str_t cur_line = {0};

  input_lex_state_t state = input_lex_state_init;
  int c = -1;
  while (true) {
    if (read_next) {
#ifdef RELEASE
      c = getchar();
#else
      c = gv->getchar();
#endif
    if (extractors.len > 0) {
      if (c == '\n') {
        float x, y;
        for (size_t i = 0; i < extractors.len; ++i) {
          int group = extractors.arr[i].group;
          extractor_res_state_t r = extractor_extract(br_str_as_view(extractors.arr[i].ex), br_str_as_view(cur_line), &x, &y);
          if (r == extractor_res_state_x) {
            q_push(&gv->commands, (q_command) { .type = q_command_push_point_x, .push_point_x = { .x = x, .group = group }}); 
          } else if (r == extractor_res_state_y) {
            q_push(&gv->commands, (q_command) { .type = q_command_push_point_y, .push_point_y = { .y = y, .group = group }}); 
          } else if (r == extractor_res_state_xy) {
            q_push(&gv->commands, (q_command) { .type = q_command_push_point_xy, .push_point_xy = { .x = x, .y = y, .group = group }}); 
          }
          cur_line.len = 0;
        }
      } else {
        br_str_push_char(&cur_line, (char)c);
      }
      input_tokens_reduce(gv, true);
      state = input_lex_state_init;
    }
    } else read_next = true;

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
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = (float)value_i, .value_i = (int)value_i };
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
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = is_neg_whole ? -value_f : value_f, .value_i = (int)value_i };
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
          tokens[tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = is_neg_whole ? -value_f : value_f, .value_i = (int)value_i };
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

TEST_CASE(ExtractorsIsValid) {
  fprintf(stderr, "\n");
  TEST_EQUAL(true, extractor_is_valid((br_strv_from_c_str("abc%x"))));
  TEST_EQUAL(true, extractor_is_valid((br_strv_from_c_str("a%xbc\\\\"))));
  TEST_EQUAL(true, extractor_is_valid((br_strv_from_c_str("*%xabc"))));
  TEST_EQUAL(true, extractor_is_valid((br_strv_from_c_str("*\\%a%xabc"))));
  TEST_EQUAL(true, extractor_is_valid((br_strv_from_c_str("*\\\\%xabc"))));
  TEST_EQUAL(true, extractor_is_valid((br_strv_from_c_str("%y*%x"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("abc%a%x"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("abc%"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("abc\\"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("a**bc"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("a%xbc*"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("*\\%xabc"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("%y%x"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("%y*%y"))));
  TEST_EQUAL(false, extractor_is_valid((br_strv_from_c_str("%x*%x"))));
}

#define VALX(ex, v, x) do { \
  float rx; \
  TEST_EQUAL(true, extractor_is_valid(br_strv_from_c_str(ex))); \
  extractor_res_state_t r = extractor_extract(br_strv_from_c_str(ex), br_strv_from_c_str(v), &rx, NULL); \
  TEST_EQUAL(r, extractor_res_state_x); \
  TEST_EQUAL(rx, x); \
} while(false) \

#define VALXY(ex, v, x, y) do { \
  float rx, ry; \
  TEST_EQUAL(true, extractor_is_valid(br_strv_from_c_str(ex))); \
  extractor_res_state_t r = extractor_extract(br_strv_from_c_str(ex), br_strv_from_c_str(v), &rx, &ry); \
  TEST_EQUAL(r, extractor_res_state_xy); \
  TEST_EQUAL(rx, x); \
  TEST_EQUAL(ry, y); \
} while(false) \

TEST_CASE(Extractors) {
  fprintf(stderr, "\n");
  VALX("abc%x", "abc12", 12.f);
  VALX("abc%x", "abc-12", -12.f);
  VALX("a%xbc", "a12.2bc", 12.2f);
  VALX("*%xabc", "-------12e12abc", -12e12f);
  VALX("*-%xabc", "-12e12abc", 12e12f);
  VALX("*\\\\%xabc", "-------\\---\\\\12abc", 12.f);
  VALX("*\\%a%xabc", "---abs\\%a12e12---\\%a10e10abc", 10e10f);
  VALXY("%y*aa%x", "12a14aaaa13", 13.f, 12.f);
  VALXY("%y.%x", "12.13.14.15", 14.15f, 12.13f);
  //TEST_EQUAL(true, extractor_is_valid((br_strv_from_c_str("*\\\\%xabc"))));
}

#endif
