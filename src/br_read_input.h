#include "include/br_str_header.h"

#define BR_MAX_REDUCE 6
#define BR_MAX_NAME 64
#define BR_REDUCTORS 10
#define BR_INPUT_TOKENS_COUNT BR_MAX_REDUCE
typedef enum {
  br_input_token_any,
  br_input_token_number,
  br_input_token_name,
  br_input_token_quoted_string,
  br_input_token_path,
  br_input_token_comma,
  br_input_token_semicollon,
  br_input_token_dashdash
} br_input_token_kind_t;

typedef enum {
  br_input_br_lex_state_init,
  br_input_br_lex_state_number,
  br_input_br_lex_state_number_decimal,
  br_input_br_lex_state_number_exp,
  br_input_br_lex_state_number_reset,
  br_input_br_lex_state_name,
  br_input_br_lex_state_path,
  br_input_br_lex_state_dash,
  br_input_br_lex_state_quoted
} br_input_br_lex_state_t;

typedef struct br_input_token_t {
  union {
    struct {
      double value_d;
      long long value_l;
    };
    char name[BR_MAX_NAME];
    br_str_t br_str;
  };
  br_input_token_kind_t kind;
} br_input_token_t;

struct br_lex_state_s;

typedef struct {
  br_input_token_kind_t kinds[BR_MAX_REDUCE];
  void (*reduce_func)(br_plotter_t* commands, struct br_lex_state_s* s);
} br_input_reduce_t;

typedef struct {
  br_str_t ex;
  int group;
} br_extractor_t;

typedef struct {
  br_extractor_t* arr;
  size_t len;
  size_t cap;
} br_extractors_t;

typedef struct br_lex_state_s {
  double value_d;
  long long decimall;
  long long value_l;
  char name[BR_MAX_NAME];
  size_t name_len;
  bool is_neg_whole;
  bool is_neg;
  bool read_next;
  br_str_t cur_line;
  br_input_br_lex_state_t state;
  int c;

  br_input_token_t tokens[BR_INPUT_TOKENS_COUNT];
  size_t tokens_len;
  br_extractors_t extractors;
  bool should_push_eagre;
  br_input_reduce_t reductors[BR_REDUCTORS];
} br_lex_state_t;

typedef struct br_plotter_t br_plotter_t;
void br_input_reduce_command(br_plotter_t* br, br_lex_state_t* s);
void br_input_reduce_ygroup(br_plotter_t* br, br_lex_state_t* s);
void br_input_reduce_xygroup(br_plotter_t* br, br_lex_state_t* s);
void br_input_reduce_xyz(br_plotter_t* br, br_lex_state_t* s);
void br_input_reduce_xy(br_plotter_t* br, br_lex_state_t* s);
void br_input_reduce_y(br_plotter_t* br, br_lex_state_t* s);

void br_input_tokens_reduce(br_plotter_t* br, br_lex_state_t* s, bool force_reduce);

void br_lex_state_init(br_lex_state_t* lex_state);
void br_lex_step(br_plotter_t* br, br_lex_state_t* s);
void br_lex_step_extractor(br_plotter_t* br, br_lex_state_t* s);
void br_lex_state_deinit(br_lex_state_t* s);

typedef enum {
  br_extractor_state_init,
  br_extractor_state_capture,
  br_extractor_state_escape
} br_extractor_state_t;

typedef enum {
  br_extractor_res_state_none = 0,
  br_extractor_res_state_unfinished = 1,
  br_extractor_res_state_x = 2,
  br_extractor_res_state_y = 4,
  br_extractor_res_state_xy = br_extractor_res_state_x | br_extractor_res_state_y
} br_extractor_res_state_t;
int br_extractor_extract_number(br_strv_t view, float* outF);
br_extractor_res_state_t br_extractor_extract(br_strv_t ex, br_strv_t view, float* x, float* y);
bool br_extractor_is_valid(br_strv_t ex);
