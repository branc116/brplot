#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_q.h"

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#  include "src/desktop/linux/read_input.c"
#elif defined(_WIN32) || defined(__CYGWIN__)
#  include "src/desktop/win/read_input.c"
#elif defined(__EMSCRIPTEN__)
#  include "src/web/read_input.c"
#else
#  error "Unsupported platform"
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define MAX_REDUCE 6
#define MAX_NAME 64
#define REDUCTORS 10
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

typedef struct input_token_t {
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

struct lex_state_s;

typedef struct {
  input_token_kind_t kinds[MAX_REDUCE];
  void (*reduce_func)(br_plotter_t* commands, struct lex_state_s* s);
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

typedef struct lex_state_s {
  float value_f;
  int decimal;
  long value_i;
  char name[MAX_NAME];
  size_t name_len;
  bool is_neg_whole;
  bool is_neg;
  bool read_next;
  br_str_t cur_line;
  input_lex_state_t state;
  int c;

  input_token_t tokens[INPUT_TOKENS_COUNT];
  size_t tokens_len;
  extractors_t extractors;
  bool should_push_eagre;
  input_reduce_t reductors[REDUCTORS];
} lex_state_t;

static void input_reduce_y(br_plotter_t* gv, lex_state_t* s) {
  if (s->should_push_eagre) q_push(gv->commands, (q_command){.type = q_command_push_point_y, .push_point_y = { .group = 0, .y = s->tokens[0].value_f} });
}

static void input_reduce_xy(br_plotter_t* gv, lex_state_t* s) {
  if (s->should_push_eagre) q_push(gv->commands, (q_command){.type = q_command_push_point_xy, .push_point_xy = { .group = 0, .x = s->tokens[0].value_f, .y = s->tokens[2].value_f} });
}

static void input_reduce_xyz(br_plotter_t* gv, lex_state_t* s) {
  if (s->should_push_eagre) q_push(gv->commands, (q_command){.type = q_command_push_point_xyz, 
      .push_point_xyz = { 
        .group = 0,
        .x = s->tokens[0].value_f,
        .y = s->tokens[2].value_f,
        .z = s->tokens[4].value_f
  }});
}

static void input_reduce_xygroup(br_plotter_t* gv, lex_state_t* s) {
  if (s->should_push_eagre) q_push(gv->commands, (q_command){.type = q_command_push_point_xy, .push_point_xy = { .group = s->tokens[4].value_i, .x = s->tokens[0].value_f, .y = s->tokens[2].value_f } });
}

static void input_reduce_ygroup(br_plotter_t* gv, lex_state_t* s) {
  if (s->should_push_eagre) q_push(gv->commands, (q_command){.type = q_command_push_point_y, .push_point_y = { .group = s->tokens[2].value_i, .y = s->tokens[0].value_f } });
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
        ERRORF("[%zu] Capture is invalid, can't have wild char next to a wild char", i);
        return false;
      }
      is_wild = false;
      increment = false;
    }
    else if (is_cap) {
      if (e == 'x') {
        if (x_cap == true) {
          ERRORF("[%zu] Can't have multiple captures for X!", i);
          return false;
        }
        x_cap = true;
      } else if (e == 'y') {
        if (y_cap == true) {
          ERRORF("[%zu] Can't have multiple captures for Y!", i);
          return false;
        }
        y_cap = true;
      } else {
        ERRORF("[%zu] You can only capture x or y! You tryed to capture `%c`", i, e);
        return false;
      }
      was_last_extract = true;
      is_cap = false;
    } else if (was_last_extract) {
      if (e == '%') {
        ERRORF("[%zu] Can't have captures one next to the other. You must have delimiter between them!", i);
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

  while ((size_t)(i + (read_next ? 1 : 0)) < view.len) {
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
  switch (state) {
    case input_lex_state_init:
      return -1;
    case input_lex_state_number:
      *outF = is_neg_whole ? (float)-value_i : (float)value_i;
      return (int)view.len;
    case input_lex_state_number_decimal:
      value_f += (float)value_i * powf(10.f, (float)decimal);
      *outF = is_neg_whole ? -value_f : value_f;
      return (int)view.len;
    case input_lex_state_number_exp:
      value_f *= powf(10.f, is_neg ? (float)-value_i : (float)value_i);
      *outF = is_neg_whole ? -value_f : value_f;
      return (int)view.len;
    default:
      assert("Unhandled state while extracting number" && false);
      return -1;
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

static void input_reduce_command(br_plotter_t* gv, lex_state_t* s) {
  (void)gv;
  if (0 == strcmp("zoomx", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_set_zoom_x, .value = s->tokens[2].value_f});
  } else if (0 == strcmp("focus", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_focus });
  } else if (0 == strcmp("zoomy", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_set_zoom_y, .value = s->tokens[2].value_f});
  } else if (0 == strcmp("zoom", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_set_zoom_x, .value = s->tokens[2].value_f});
    q_push(gv->commands, (q_command) { .type = q_command_set_zoom_y, .value = s->tokens[2].value_f});
  } else if (0 == strcmp("offsetx", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_set_offset_x, .value = s->tokens[2].value_f});
  } else if (0 == strcmp("offsety", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_set_offset_y, .value = s->tokens[2].value_f});
  } else if (0 == strcmp("hide", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_hide, .hide_show = { .group = s->tokens[2].value_i } });
  } else if (0 == strcmp("show", s->tokens[1].name)) {
    q_push(gv->commands, (q_command) { .type = q_command_show, .hide_show = { .group = s->tokens[2].value_i } });
  } else if (0 == strcmp("exportcsv", s->tokens[1].name)) {
    input_push_command_with_path(gv->commands, s->tokens[2].name, q_command_exportcsv);
  } else if (0 == strcmp("export", s->tokens[1].name)) {
    input_push_command_with_path(gv->commands, s->tokens[2].name, q_command_export);
  } else if (0 == strcmp("screenshot", s->tokens[1].name)) {
    input_push_command_with_path(gv->commands, s->tokens[2].name, q_command_screenshot);
  } else if (0 == strcmp("exit", s->tokens[1].name)) {
    gv->should_close = true;
    exit(0);
  } else if (0 == strcmp("extract", s->tokens[1].name)) {
    if (extractor_is_valid(br_str_as_view(s->tokens[3].br_str))) {
      br_str_t str = br_str_copy(s->tokens[3].br_str);
      --str.len;
      extractors_push(&s->extractors, (extractor_t) { .ex =  str, .group = s->tokens[2].value_i });
      s->should_push_eagre = false;
    }
  } else if (s->tokens[3].kind == input_token_quoted_string && 0 == strcmp("setname", s->tokens[1].name)) {
    q_command cmd = {
      .type = q_command_set_name,
      .set_quoted_str = {
        .group = s->tokens[2].value_i,
        .str = s->tokens[3].br_str
      }
    };
    q_push(gv->commands, cmd);
    s->tokens[3].kind = input_token_any;
  } else
    printf("Execute %c%c%c...\n", s->tokens[1].name[0], s->tokens[1].name[1], s->tokens[1].name[2]);
}

static void input_tokens_pop(lex_state_t* s, size_t n) {
  if (n == 0) return;
  for (size_t i = 0; i < n; ++i) {
    if (s->tokens[i].kind == input_token_quoted_string) {
      br_str_free(s->tokens[i].br_str);
    }
  }
  if (n >= INPUT_TOKENS_COUNT) {
    memset(s->tokens, 0, sizeof(s->tokens));
  }
  memmove(s->tokens, &s->tokens[n], sizeof(input_token_t) * (INPUT_TOKENS_COUNT - n));
  memset(&s->tokens[INPUT_TOKENS_COUNT - n], 0, sizeof(input_token_t) * n);
  s->tokens_len -= n;
}

static bool input_tokens_match_count(const lex_state_t* s, input_reduce_t* r, size_t* match_count) {
  size_t i = 0;
  for (; i < s->tokens_len && i < MAX_REDUCE && r->kinds[i] != input_token_any; ++i)
    if (s->tokens[i].kind != r->kinds[i]) return false;
  *match_count = i;
  return r->kinds[i] == input_token_any;
}

static bool input_tokens_can_next_be(const lex_state_t* s, input_token_kind_t kind) {
  bool bad_reducor[REDUCTORS] = {0};
  size_t j = 0;
  bool any = false;
  for (; j < s->tokens_len; ++j) {
    any = false;
    for (int i = 0; i < REDUCTORS; ++i) {
      if (bad_reducor[i] == false && s->reductors[i].kinds[j] != s->tokens[j].kind) {
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
    if (bad_reducor[i] == false && s->reductors[i].kinds[j] != kind) {
      bad_reducor[i] = true;
    } else {
      return true;
    }
  }
  return false;
}

static void input_tokens_reduce(br_plotter_t* gv, lex_state_t* s, bool force_reduce) {
  if (s->tokens_len == 0) return;
  int best = -1;
  size_t best_c = 0, match_c = 0;

  int best_reduce = -1;
  size_t best_reduce_count = 0;
  for (size_t i = 0; i < REDUCTORS; ++i) {
    size_t count = 0;
    bool can_reduce = input_tokens_match_count(s, &s->reductors[i], &count);
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
    if (best_c == 0) input_tokens_pop(s, 1);
    else if ((match_c == 1 && best > -1 && s->reductors[best].kinds[best_c] == input_token_any)) {
      s->reductors[best].reduce_func(gv, s);
      input_tokens_pop(s, (size_t)best_c);
    }
  } else {
    if (best_reduce_count == 0) input_tokens_pop(s, 1);
    else if (best_reduce > -1) {
      s->reductors[best_reduce].reduce_func(gv, s);
      input_tokens_pop(s, (size_t)best_reduce_count);
    }
  }
}

static void lex_state_init(lex_state_t* lex_state) {
  *lex_state = (lex_state_t){ 0,
   .reductors = {
    {{input_token_number}, input_reduce_y},
    {{input_token_number, input_token_comma, input_token_number}, input_reduce_xy},
    {{input_token_number, input_token_comma, input_token_number, input_token_comma, input_token_number}, input_reduce_xyz},
    {{input_token_number, input_token_comma, input_token_number, input_token_semicollon, input_token_number}, input_reduce_xygroup},
    {{input_token_number, input_token_semicollon, input_token_number}, input_reduce_ygroup},
    {{input_token_dashdash, input_token_name, input_token_number}, input_reduce_command},
    {{input_token_dashdash, input_token_name, input_token_path}, input_reduce_command},
    {{input_token_dashdash, input_token_name, input_token_name}, input_reduce_command},
    {{input_token_dashdash, input_token_name}, input_reduce_command},
    {{input_token_dashdash, input_token_name, input_token_number, input_token_quoted_string}, input_reduce_command},
  }};
  lex_state->read_next = true;
  lex_state->c = -1;
  lex_state->should_push_eagre = true;
}

static void lex_state_deinit(lex_state_t* s) {
  for (size_t i = 0; i < s->tokens_len; ++i) {
    if (s->tokens[i].kind == input_token_quoted_string) {
      br_str_free(s->tokens[i].br_str);
    }
  }
  s->tokens_len = 0;
}

static void lex_step(br_plotter_t* br, lex_state_t* s) {
  switch (s->state) {
    case input_lex_state_init:
      input_tokens_reduce(br, s, false);
      if (s->c == '-') {
        s->state = input_lex_state_dash;
      } else if (s->c >= '0' && s->c <= '9') {
        s->state = input_lex_state_number;
        s->read_next = false;
      } else if (s->c >= 'a' && s->c <= 'z' && input_tokens_can_next_be(s, input_token_name)) {
        s->state = input_lex_state_name;
        s->read_next = false;
      } else if (IS_VALID_PATH(s->c) && input_tokens_can_next_be(s, input_token_path)) {
        s->state = input_lex_state_path;
        s->read_next = false;
      } else if (s->c == '.') {
        s->state = input_lex_state_number_decimal;
      } else if (s->c == ',' && input_tokens_can_next_be(s, input_token_comma)) {
        s->tokens[s->tokens_len++] = (input_token_t) { .kind = input_token_comma };
      } else if (s->c == ';' && input_tokens_can_next_be(s, input_token_semicollon)) {
        s->tokens[s->tokens_len++] = (input_token_t) { .kind = input_token_semicollon };
      } else if (s->c == '"' && input_tokens_can_next_be(s, input_token_quoted_string)) {
        s->tokens[s->tokens_len] = (input_token_t) {
          .kind = input_token_quoted_string,
          .br_str = br_str_malloc(8),
        };
        s->state = input_lex_state_quoted;
      }
      break;
    case input_lex_state_dash:
      if (s->c == '-') {
        s->tokens[s->tokens_len++] = (input_token_t){ .kind = input_token_dashdash };
        s->state = input_lex_state_init;
      } else if (s->c >= '0' && s->c <= '9') {
        s->is_neg = true;
        s->is_neg_whole = true;
        s->state = input_lex_state_number;
        s->read_next = false;
      } else {
        s->state = input_lex_state_init;
      }
      break;
    case input_lex_state_number:
      if (s->c >= '0' && s->c <= '9') {
        s->value_i *= 10;
        s->value_i += s->c - '0';
      } else if (s->c == '.') {
        s->state = input_lex_state_number_decimal;
        s->value_f = (float)s->value_i;
        s->value_i = 0;
        s->is_neg = false;
      } else if (s->c == 'E' || s->c == 'e') {
        s->value_f = (float)s->value_i;
        s->value_i = 0;
        s->is_neg = false;
        s->decimal = 0;
        s->state = input_lex_state_number_exp;
      } else {
        s->tokens[s->tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = (float)s->value_i, .value_i = (int)s->value_i };
        s->state = input_lex_state_number_reset;
        s->read_next = false;
      }
      break;
    case input_lex_state_number_decimal:
      if (s->c >= '0' && s->c <= '9') {
        s->value_i *= 10;
        s->value_i += s->c - '0';
        --s->decimal;
      } else if (s->c == 'E' || s->c == 'e') {
        s->value_f += (float)s->value_i * powf(10.f, (float)s->decimal);
        s->value_i = 0;
        s->decimal = 0;
        s->state = input_lex_state_number_exp;
      } else {
        s->value_f += (float)s->value_i * powf(10.f, (float)s->decimal);
        s->tokens[s->tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = s->is_neg_whole ? -s->value_f : s->value_f, .value_i = (int)s->value_i };
        s->state = input_lex_state_number_reset;
        s->read_next = false;
      }
      break;
    case input_lex_state_number_exp:
      if (s->c >= '0' && s->c <= '9') {
        s->value_i *= 10;
        s->value_i += s->c - '0';
      } else if (s->c == '-') {
        s->is_neg = true;
      } else {
        s->value_f *= powf(10.f, s->is_neg ? (float)-s->value_i : (float)s->value_i);
        s->tokens[s->tokens_len++] = (input_token_t) { .kind = input_token_number, .value_f = s->is_neg_whole ? -s->value_f : s->value_f, .value_i = (int)s->value_i };
        s->state = input_lex_state_number_reset;
        s->read_next = false;
      }
      break;
    case input_lex_state_number_reset:
      s->state = input_lex_state_init;
      s->read_next = false;
      s->value_f = 0.f;
      s->value_i = 0;
      s->decimal = 0;
      s->is_neg = false;
      s->is_neg_whole = false;
      break;
    case input_lex_state_name:
      if (s->name_len + 1 < MAX_NAME && s->c >= 'a' && s->c <= 'z') {
        s->name[s->name_len++] = (char)s->c;
      } else if ((s->name_len + 1 < MAX_NAME) && IS_VALID_PATH(s->c)) {
        s->read_next = false;
        s->state = input_lex_state_path;
      } else {
        memcpy(s->tokens[s->tokens_len].name, s->name, s->name_len);
        s->tokens[s->tokens_len].name[s->name_len] = (char)0;
        s->tokens[s->tokens_len++].kind = input_token_name;
        s->state = input_lex_state_init;
        s->name_len = 0;
        s->read_next = false;
      }
      break;
    case input_lex_state_path:
      if ((s->name_len + 1 < MAX_NAME) && IS_VALID_PATH(s->c)) {
        s->name[s->name_len++] = (char)s->c;
      } else {
        memcpy(s->tokens[s->tokens_len].name, s->name, s->name_len);
        s->tokens[s->tokens_len].name[s->name_len] = (char)0;
        s->tokens[s->tokens_len++].kind = input_token_path;
        s->state = input_lex_state_init;
        s->name_len = 0;
        s->read_next = false;
      }
      break;
    case input_lex_state_quoted:
      if (s->c == '"' || s->c == '\0' || s->c == -1) {
        br_str_push_char(&s->tokens[s->tokens_len].br_str, (char)0);
        ++s->tokens_len;
        s->state = input_lex_state_init;
      } else br_str_push_char(&s->tokens[s->tokens_len].br_str, (char)s->c);
      break;
    default:
      assert(false);
  }
}

static void lex_step_extractor(br_plotter_t* br, lex_state_t* s) {
  if (s->extractors.len > 0) {
    if (s->c == '\n') {
      float x, y;
      for (size_t i = 0; i < s->extractors.len; ++i) {
        int group = s->extractors.arr[i].group;
        extractor_res_state_t r = extractor_extract(br_str_as_view(s->extractors.arr[i].ex), br_str_as_view(s->cur_line), &x, &y);
        if (r == extractor_res_state_x) {
          q_push(br->commands, (q_command) { .type = q_command_push_point_x, .push_point_x = { .x = x, .group = group }});
        } else if (r == extractor_res_state_y) {
          q_push(br->commands, (q_command) { .type = q_command_push_point_y, .push_point_y = { .y = y, .group = group }});
        } else if (r == extractor_res_state_xy) {
          q_push(br->commands, (q_command) { .type = q_command_push_point_xy, .push_point_xy = { .x = x, .y = y, .group = group }});
        }
      }
      s->cur_line.len = 0;
    } else {
      br_str_push_char(&s->cur_line, (char)s->c);
    }
  }
}

static void lex(br_plotter_t* br) {
  lex_state_t s;
  lex_state_init(&s);
  while (true) {
    if (s.read_next) {
      s.c = read_input_read_next();
      if (s.c == -1) {
        input_tokens_reduce(br, &s, true);
        ERROR("Exiting read_input thread");
        break;
      }
      lex_step_extractor(br, &s);
    } else s.read_next = true;
    lex_step(br, &s);
  }
  s.c = 0;
  do {
    lex_step(br, &s);
    input_tokens_reduce(br, &s, true);
  } while (s.tokens_len > 0);
  lex_state_deinit(&s);
  q_push(br->commands, (q_command) { .type = q_command_focus });
}

void read_input_main_worker(br_plotter_t* gv) {
  lex(gv);
}

#if !defined(_MSC_VER) && defined(BR_DEBUG)
#include "external/tests.h"

#if defined(FUZZ)
#include "src/br_data_generator.h"
#include "src/br_plotter.h"
#include "src/br_gui_internal.h"
#include "src/br_help.h"
int LLVMFuzzerTestOneInput(const char *str, size_t str_len) {
  lex_state_t s;
  br_plotter_t* br = br_plotter_malloc();
  br_plotter_init(br, true);
  br->shaders = br_shaders_malloc();
  br->text = br_text_renderer_malloc(1024, 1024, br_font_data, &br->shaders.font);
  for (size_t i = 0; i < str_len;) {
    if (s.read_next) {
      s.c = str[i++];
      lex_step_extractor(br, &s);
    } else s.read_next = true;
    lex_step(br, &s);
    br_plotter_draw(br);
    br_dagens_handle(&br->groups, &br->dagens, &br->plots, GetTime() + 0.010);
  }
  s.c = 0;
  while (s.tokens_len > 0) {
    lex_step(br, &s);
    input_tokens_reduce(br, &s, true);
    br_plotter_draw(br);
    br_dagens_handle(&br->groups, &br->dagens, &br->plots, GetTime() + 0.010);
  }
  br_plotter_free(br);
  BR_FREE(br);
  return 0;  // Values other than 0 and -1 are reserved for future use.
}
#endif

#define TEST_COMMAND_SET_ZOOM(q, AXIS, VALUE) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_set_zoom_ ## AXIS); \
  TEST_EQUAL(c.value, VALUE); \
} while(false)

#define TEST_COMMAND_PUSH_POINT_Y(q, Y, GROUP) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_push_point_y); \
  TEST_EQUAL(c.push_point_y.y, Y); \
  TEST_EQUAL(c.push_point_y.group, GROUP); \
} while(false)

#define TEST_COMMAND_PUSH_POINT_XY(q, X, Y, GROUP) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_push_point_xy); \
  TEST_EQUAL(c.push_point_xy.x, X); \
  TEST_EQUAL(c.push_point_xy.y, Y); \
  TEST_EQUAL(c.push_point_xy.group, GROUP); \
} while(false)

#define TEST_COMMAND_END(q) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_none); \
  q_free(q); \
} while(false)

#define TEST_COMMAND_SET_NAME(q, GROUP, NAME) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_set_name); \
  TEST_EQUAL(c.set_quoted_str.group, GROUP); \
  char* cstr = br_str_to_c_str(c.set_quoted_str.str); \
  TEST_STREQUAL(cstr, NAME); \
  BR_FREE(cstr); \
  br_str_free(c.set_quoted_str.str); \
} while(false)

void test_input(br_plotter_t* br, const char* str) {
  lex_state_t s;
  lex_state_init(&s);
  br->commands = q_malloc();
  size_t str_len = strlen(str);
  for (size_t i = 0; i <= str_len;) {
    if (s.read_next) {
      s.c = str[i++];
      lex_step_extractor(br, &s);
    } else s.read_next = true;
    lex_step(br, &s);
  }
  s.c = 0;
  while (s.tokens_len > 0) {
    lex_step(br, &s);
    input_tokens_reduce(br, &s, true);
  }
  lex_state_deinit(&s);
}

TEST_CASE(InputTests) {
  br_plotter_t br;
  test_input(&br, "8.0,-16.0;1 -0.0078,16.0;1 \" \n \n 4.0;1\n\n\n\n\n\n 2.0 1;10;1;;;; 10e10 3e38 --test 1.2 --zoomx 10.0 1;12");

  TEST_COMMAND_PUSH_POINT_XY(br.commands, 8.f, -16.f, 1);
  TEST_COMMAND_PUSH_POINT_XY(br.commands, -0.0078f, 16.f, 1);
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 4.f, 1);
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 2.f, 0);
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 1.f, 10);
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 1.f, 0);
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 10e10f, 0);
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 3e38f, 0);
  TEST_COMMAND_SET_ZOOM(br.commands, x, 10.0f);
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 1.f, 12);
  TEST_COMMAND_END(br.commands);
}

TEST_CASE(InputTests2) {
  br_plotter_t br;
  test_input(&br, "--setname 1 \"hihi\" --setname 2 \"hihi hihi hihi hihi\" --setname 0 \"test");

  TEST_COMMAND_SET_NAME(br.commands, 1, "hihi");
  TEST_COMMAND_SET_NAME(br.commands, 2,  "hihi hihi hihi hihi");
  TEST_COMMAND_SET_NAME(br.commands, 0,  "test");
  TEST_COMMAND_END(br.commands);
}

#define VAL(is_valid, ex) do { \
  TEST_EQUAL(is_valid, extractor_is_valid(br_strv_from_c_str(ex))); \
} while(0)

#define VALX(ex, v, x) do { \
  float rx; \
  TEST_EQUAL(true, extractor_is_valid(br_strv_from_c_str(ex))); \
  extractor_res_state_t r = extractor_extract(br_strv_from_c_str(ex), br_strv_from_c_str(v), &rx, NULL); \
  TEST_EQUAL(r, extractor_res_state_x); \
  TEST_EQUAL(rx, x); \
} while(false)

#define VALXY(ex, v, x, y) do { \
  float rx, ry; \
  TEST_EQUAL(true, extractor_is_valid(br_strv_from_c_str(ex))); \
  extractor_res_state_t r = extractor_extract(br_strv_from_c_str(ex), br_strv_from_c_str(v), &rx, &ry); \
  TEST_EQUAL(r, extractor_res_state_xy); \
  TEST_EQUAL(rx, x); \
  TEST_EQUAL(ry, y); \
} while(false)


TEST_CASE(ExtractorsIsValid) {
  fprintf(stderr, "\n");
  VAL(true, ("abc%x"));
  VAL(true, ("a%xbc\\\\"));
  VAL(true, ("*%xabc"));
  VAL(true, ("*\\%a%xabc"));
  VAL(true, ("*\\\\%xabc"));
  VAL(true, ("%y*%x"));
  VAL(false, ("abc%a%x"));  // %a is not a valid capture
  VAL(false, ("abc%"));     // % is unfinised capture
  VAL(false, ("abc\\"));    // \ is unfinished escape
  VAL(false, ("a**bc"));    // wild can't follow wild
  VAL(false, ("a%xbc*"));   // wild can't be last character
  VAL(false, ("*\\%xabc")); // Nothing is captured. %x is escaped
  VAL(false, ("%y%x"));     // Capture can't follow capture Ex. : "1234" can be x=1,y=234, x=12,y=34, ...
  VAL(false, ("%y*%y"));    // Can't have multiple captures in the expression
  VAL(false, ("%x*%x"));    // Can't have multiple captures in the expression
}

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
}

#endif
