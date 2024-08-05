#include "br_data_generator.h"
#include "br_da.h"
#include "br_data.h"
#include "br_permastate.h"
#include "br_plot.h"
#include "br_pp.h"
#include "br_resampling2.h"
#include "src/br_str.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MAX_BATCH_LEN (16*1024)

#define SINGLE(X) \
  X(token_kind_hash, '#') \
  X(token_kind_plus, '+') \
  X(token_kind_star, '*') \
  X(token_kind_dot, '.') \
  X(token_kind_comma, ',') \
  X(token_kind_ident, 'x') \
  X(token_kind_ident, 'y') \
  X(token_kind_ident, 'z')

#define TOKEN_KINDS(X) \
  X(token_kind_number) \
  X(token_kind_ident) \
  X(token_kind_hash) \
  X(token_kind_plus) \
  X(token_kind_star) \
  X(token_kind_dot) \
  X(token_kind_comma)

typedef enum {
#define X(name) name,
  TOKEN_KINDS(X)
#undef X
} token_kind_t;

typedef struct {
  token_kind_t kind;
  br_strv_t str;
  size_t position;
} token_t;

typedef struct {
  token_t* arr;
  size_t len, cap;
  size_t pos;
} tokens_t;

typedef struct {
  float** arr;
  size_t len, cap;
  size_t max_len;
} batches_t;

static batches_t batches;

inline static size_t min_s(size_t a, size_t b) { return a < b ? a : b; }

// INTERPRETER
static void br_dagens_handle_once(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots);
static void br_dagen_handle(br_dagen_t* dagen, br_data_t* data, br_datas_t datas);
static size_t expr_len(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index);
static size_t expr_read_n(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data);
static size_t expr_add_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data);
static size_t expr_mul_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data);
static float* push_batch(void);
static void pop_batch(void);

// PARSER
static bool tokens_get(tokens_t* tokens, br_strv_t str);
static const char* token_to_str(token_kind_t kind);
static bool expr_parse(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out);
static bool expr_parse_ref(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out);
static void expr_to_str(br_str_t* out, br_dagen_exprs_t* arena, uint32_t index);

bool br_dagen_push_expr_xy(br_dagens_t* dagens, br_datas_t* datas, br_dagen_exprs_t arena, uint32_t x, uint32_t y, int group_id) {
  if (NULL != br_data_get1(*datas, group_id)) {
    LOGI("Data with id %d already exists, will not create expr with the same id\n", group_id);
    return false;
  }
  br_dagen_t dagen = {
    .kind = br_dagen_kind_expr,
    .data_kind = br_data_kind_2d,
    .state = br_dagen_state_inprogress,
    .group_id = group_id,
    .expr_2d = {
      .x_expr_index = x,
      .y_expr_index = y,
      .arena = arena
    }
  };

  if (NULL == br_datas_create(datas, group_id, br_data_kind_2d)) return false;
  br_da_push(*dagens, dagen);
  return true;
}

bool br_dagens_add_expr_str(br_dagens_t* dagens, br_datas_t* datas, br_strv_t str, int group_id) {
  tokens_t tokens = {0};
  br_dagen_exprs_t arena = {0};
  uint32_t entry = 0;

  if (false == tokens_get(&tokens, str)) goto error;
  if (false == expr_parse(&arena, &tokens, &entry)) goto error;
  br_dagen_push_expr_xy(dagens, datas, arena, entry, entry, group_id);
  br_da_free(tokens);
  return true;

error:
  br_da_free(tokens);
  br_da_free(arena);
  return false;
}

bool br_dagen_push_file(br_dagens_t* dagens, br_datas_t* datas, br_data_desc_t* desc, FILE* file) {
  if (file == NULL) goto error;
  br_save_state_command_t command = br_save_state_command_save_plots;
  size_t data_left = 0;
  br_data_kind_t kind;
  Color color;
  size_t cap;

  if (1 != fread(&command, sizeof(command), 1, file)) goto error;
  switch (command) {
    case br_save_state_command_save_data_2d: kind = br_data_kind_2d; break;
    case br_save_state_command_save_data_3d: kind = br_data_kind_3d; break;
    default: goto error;
  }
  if (1 != fread(&color, sizeof(color), 1, file)) goto error;
  if (1 != fread(&cap, sizeof(cap), 1, file)) goto error;

  br_data_t* data = br_datas_create2(datas, desc->group_id, kind, color, cap, desc->name);
  br_str_invalidata(desc->name);
  if (NULL == data) goto error;

  if (0 != cap) {
    data_left = cap;
    switch (kind) {
      case br_data_kind_2d: {
        if (1 != fread(&data->dd.bounding_box, sizeof(data->dd.bounding_box), 1, file)) goto error;
      } break;
      case br_data_kind_3d: {
        if (1 != fread(&data->ddd.bounding_box, sizeof(data->ddd.bounding_box), 1, file)) goto error;
      } break;
      default: BR_ASSERT(0);
    }
  }
  br_dagen_t new = {
    .data_kind = kind,
    .state = br_dagen_state_inprogress,
    .group_id = desc->group_id,
    .file = {
      .file = file,
      .x_left = data_left,
      .y_left = data_left,
      .z_left = kind == br_data_kind_2d ? 0 : data_left,
    }
  };
  br_da_push(*dagens, new);
  return true;

error:
  LOGEF("Failed to read plot file: %d(%s)\n", errno, strerror(errno));
  if (file != NULL) fclose(file);
  return false;
}

void br_dagens_handle(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots, double until) {
  while (GetTime() < until) br_dagens_handle_once(datas, dagens, plots);
}

void br_dagens_free(br_dagens_t* dagens) {
  for (size_t i = 0; i < dagens->len; ++i)
    if (dagens->arr[i].kind == br_dagen_kind_file) fclose(dagens->arr[i].file.file);
    else if (dagens->arr[i].kind == br_dagen_kind_expr) br_da_free(dagens->arr[i].expr_2d.arena);
  br_da_free(*dagens);
  for (size_t i = 0; i < batches.max_len; ++i) {
    BR_FREE(batches.arr[i]);
  }
  br_da_free(batches);
  batches.max_len = 0;
}

static void br_dagens_handle_once(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots) {
  for (size_t i = 0; i < dagens->len;) {
    br_dagen_t* cur = &dagens->arr[i];
    br_data_t* d = br_data_get1(*datas, cur->group_id);
    if (NULL == cur || NULL == d) cur->state = br_dagen_state_failed;
    else br_dagen_handle(cur, d, *datas);
    switch (cur->state) {
      case br_dagen_state_failed: {
        br_data_clear(datas, plots, d->group_id);
        br_da_remove_at(*dagens, i);
      } break;
      case br_dagen_state_finished: br_da_remove_at(*dagens, i); break;
      case br_dagen_state_inprogress: ++i; break;
      default: BR_ASSERT(0);
    }
  }
}

static void br_dagen_handle(br_dagen_t* dagen, br_data_t* data, br_datas_t datas) {
  switch (dagen->kind) {
    case br_dagen_kind_file:
    {
      BR_ASSERT((data->kind == br_data_kind_2d) || (data->kind == br_data_kind_3d));
      size_t* left = &dagen->file.x_left;
      float* d = data->dd.xs;
      if (*left == 0) {
        left = &dagen->file.y_left;
        d = data->dd.ys;
      }
      if (*left == 0) {
        if (data->kind == br_data_kind_2d) {
          dagen->state = br_dagen_state_finished;
          fclose(dagen->file.file);
          return;
        }
        left = &dagen->file.z_left;
        d = data->ddd.zs;
      }
      size_t index = data->cap - *left;
      size_t read_n = 1024 < *left ? 1024 : *left;
      if (read_n != fread(&d[index], sizeof(d[index]), read_n, dagen->file.file)) goto error;
      *left -= read_n;
      if ((data->kind == br_data_kind_2d && d == data->dd.ys) || (data->kind == br_data_kind_3d && d == data->ddd.zs)) {
        data->len += read_n;
        for (size_t i = data->len - read_n; i < data->len; ++i) {
          resampling2_add_point(data->resampling, data, (uint32_t)i);
        }
        if (*left == 0) {
          dagen->state = br_dagen_state_finished;
          fclose(dagen->file.file);
        }
      }
      return;
error:
      LOGEF("Failed to read data for a plot %s: %d(%s)\n", br_str_to_c_str(data->name), errno, strerror(errno));
      fclose(dagen->file.file);
      dagen->state = br_dagen_state_failed;
    } break;
    case br_dagen_kind_expr:
    {
      switch(data->kind) {
        case br_data_kind_2d:
        {
          size_t read_index = data->len;
          size_t read_per_batch = MAX_BATCH_LEN;
          br_dagen_exprs_t arena = dagen->expr_2d.arena;
          size_t x_len = expr_len(datas, arena, dagen->expr_2d.x_expr_index);
          size_t y_len = expr_len(datas, arena, dagen->expr_2d.y_expr_index);
          size_t min_len = x_len < y_len ? x_len : y_len;
          if (data->cap < min_len) if (false == br_data_realloc(data, min_len)) {
            LOGE("Failed to alloc memory for generated plot\n");
            dagen->state = br_dagen_state_failed;
          }
          min_len -= read_index;
          if (min_len == 0) return;
          read_per_batch = min_s(read_per_batch, min_len);
          float* out_xs = &data->dd.xs[read_index];
          float* out_ys = &data->dd.ys[read_index];
          expr_read_n(datas, arena, dagen->expr_2d.x_expr_index, read_index, read_per_batch, out_xs);
          expr_read_n(datas, arena, dagen->expr_2d.y_expr_index, read_index, read_per_batch, out_ys);
          for (size_t i = 0; i < read_per_batch; ++i) {
            ++data->len;
            resampling2_add_point(data->resampling, data, (uint32_t)(read_index + i));
          }
        } break;
        default: LOGEF("Unsupported data kind: %d\n", data->kind); BR_ASSERT(0);
      }
    } break;
    default: BR_ASSERT(0);
  }
}

static size_t expr_len(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  switch (expr.kind) {
    case br_dagen_expr_kind_reference_x:
    case br_dagen_expr_kind_reference_y:
    case br_dagen_expr_kind_reference_z:
    {
      br_data_t* data_in = br_data_get1(datas, expr.group_id);
      if (NULL == data_in) return 0;
      return data_in->len;
    }
    case br_dagen_expr_kind_add:
    case br_dagen_expr_kind_mul:
    {
      return min_s(expr_len(datas, arena, expr.operands.op1), expr_len(datas, arena, expr.operands.op2));
    }
  }
  BR_ASSERT(0);
}

static size_t expr_read_n(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  if (expr.kind == br_dagen_expr_kind_add) {
    size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, data);
    size_t added = expr_add_to(datas, arena, expr.operands.op2, offset, read, data);
    return added;
  }

  if (expr.kind == br_dagen_expr_kind_mul) {
    size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, data);
    size_t added = expr_mul_to(datas, arena, expr.operands.op2, offset, read, data);
    return added;
  }

  br_data_t* data_in = br_data_get1(datas, expr.group_id);
  if (NULL == data_in) return 0;
  size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
  switch (expr.kind) {
    case br_dagen_expr_kind_reference_x: memcpy(data, &data_in->dd.xs[offset], real_n * sizeof(data[0])); break;
    case br_dagen_expr_kind_reference_y: memcpy(data, &data_in->dd.ys[offset], real_n * sizeof(data[0])); break;
    case br_dagen_expr_kind_reference_z: memcpy(data, &data_in->ddd.zs[offset], real_n * sizeof(data[0])); break;
    default: BR_ASSERT(0);
  }
  return real_n;
}

static size_t expr_add_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  if (expr.kind == br_dagen_expr_kind_add) {
    size_t read = expr_add_to(datas, arena, expr.operands.op1, offset, n, data);
    read = expr_add_to(datas, arena, expr.operands.op2, offset, read, data);
    return read;
  }

  if (expr.kind == br_dagen_expr_kind_mul) {
    float* batch = push_batch();
    size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, batch);
    read = expr_mul_to(datas, arena, expr.operands.op2, offset, read, batch);
    for (size_t i = 0; i < read; ++i) data[i] += batch[i];
    return read;
  }

  br_data_t* data_in = br_data_get1(datas, expr.group_id);
  if (data_in == NULL) return 0;
  float* fs = NULL;
  size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
  switch (expr.kind) {
    case br_dagen_expr_kind_reference_x: fs = &data_in->dd.xs[offset]; break;
    case br_dagen_expr_kind_reference_y: fs = &data_in->dd.ys[offset]; break;
    case br_dagen_expr_kind_reference_z: fs = &data_in->ddd.zs[offset]; break;
    default: BR_ASSERT(0);
  }
  for (size_t i = 0; i < real_n; ++i) {
    data[i] += fs[i];
  }
  return real_n;
}

static size_t expr_mul_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  if (expr.kind == br_dagen_expr_kind_add) {
    float* new_batch = push_batch();
    size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, new_batch);
    read = expr_add_to(datas, arena, expr.operands.op2, offset, read, new_batch);
    for (size_t i = 0; i < read; ++i) data[i] *= new_batch[i];
    pop_batch();
    return read;
  }

  if (expr.kind == br_dagen_expr_kind_mul) {
    size_t read = expr_mul_to(datas, arena, expr.operands.op1, offset, n, data);
    read = expr_mul_to(datas, arena, expr.operands.op2, offset, read, data);
    return read;
  }

  br_data_t* data_in = br_data_get1(datas, expr.group_id);
  if (data_in == NULL) return 0;
  float* fs = NULL;
  size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
  switch (expr.kind) {
    case br_dagen_expr_kind_reference_x: fs = &data_in->dd.xs[offset]; break;
    case br_dagen_expr_kind_reference_y: fs = &data_in->dd.ys[offset]; break;
    case br_dagen_expr_kind_reference_z: fs = &data_in->ddd.zs[offset]; break;
    default: BR_ASSERT(0);
  }
  for (size_t i = 0; i < real_n; ++i) data[i] *= fs[i];
  return real_n;
}

static float* push_batch(void) {
  if (batches.len < batches.max_len) return batches.arr[batches.len++];
  float* batch = BR_MALLOC(sizeof(float) * MAX_BATCH_LEN);
  br_da_push(batches, batch);
  batches.max_len = batches.len;
  return batch;
}

static void pop_batch(void) {
  --batches.len;
}

static bool tokens_get(tokens_t* tokens, br_strv_t str) {
  token_t t;
  uint32_t j = 0;

  for (uint32_t i = 0; i < str.len; ++i) {
    switch (str.str[i]) {
#define X(KIND, CHAR) \
      case CHAR: \
        t = (token_t){ .kind = KIND, .str = br_strv_sub(str, i, 1), .position = i }; \
        goto push_token;
      SINGLE(X)
#undef X
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        j = i + 1;
        while (j < str.len && str.str[j] >= '0' && str.str[j] <= '9') ++j;
        t = (token_t) { .kind = token_kind_number, .str = br_strv_sub(str, i, j - i), .position = i };
        i += j - i - 1;
        goto push_token;
      case ' ': case '\t': case '\n': case '\r': continue;
      default:
        LOGEF("[Tokenizer] Unknown character '%c' while tokenizing expression `%.*s`\n", str.str[i], str.len, str.str);
        return false;
    }
    continue;
push_token:
    br_da_push(*tokens, t);
  }
  return true;
}

static const char* token_to_str(token_kind_t kind) {
  switch (kind) {
#define X(name) case name: return #name;
    TOKEN_KINDS(X)
#undef X
    default: return "Unknown";
  }
}

static bool expr_parse(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  token_t t;
  uint32_t ref1;
  if (false == expr_parse_ref(arena, tokens, &ref1)) return false;
  if (tokens->pos < tokens->len) {
    t = tokens->arr[tokens->pos++];
    while (t.kind == token_kind_star) {
      uint32_t ref2;
      if (false == expr_parse_ref(arena, tokens, &ref2)) return false;
      br_dagen_expr_t expr = { .kind = br_dagen_expr_kind_mul, .operands = { .op1 = ref1, .op2 = ref2 } };
      ref1 = (uint32_t)arena->len;
      br_da_push(*arena, expr);
      if (tokens->pos >= tokens->len) {
        *out = ref1;
        return true;
      }
      t = tokens->arr[tokens->pos++];
    }
    if (t.kind == token_kind_plus) {
      uint32_t ref2;
      if (false == expr_parse(arena, tokens, &ref2)) return false;
      br_dagen_expr_t expr = { .kind = br_dagen_expr_kind_add, .operands = { .op1 = ref1, .op2 = ref2 } };
      *out = (uint32_t)arena->len;
      br_da_push(*arena, expr);
      return true;
    }
    LOGEF("[Parser] Unexpected token %s(%d,%.*s) at position %zu, expected tokens `+`|`*`\n", token_to_str(t.kind), t.kind, t.str.len, t.str.str, t.position);
    return false;
  } else {
    *out = ref1;
    return true;
  }
}

#define EXPECT_TOKEN(TOKEN, EXPECTED) do { \
  if ((TOKEN).kind != (EXPECTED)) { \
    LOGEF("[Parser] Unexpected token %s(%d,%.*s) at position %zu, expected token %s\n", \
        token_to_str((TOKEN).kind), (TOKEN).kind, (TOKEN).str.len, (TOKEN).str.str, (TOKEN).position, token_to_str(EXPECTED)); \
    return false; \
  } \
} while(0)

#define GET_TOKEN(TOKEN_OUT, TOKENS, EXPECTED) do { \
  if ((TOKENS)->pos >= (TOKENS)->len) { \
    LOGEF("[Parser] Expected '%s' but got end of stream\n", token_to_str((EXPECTED))); \
    return false; \
  } \
  (TOKEN_OUT) = (TOKENS)->arr[(TOKENS)->pos++]; \
  EXPECT_TOKEN((TOKEN_OUT), (EXPECTED)); \
} while (0)

static bool expr_parse_ref(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  token_t t;
  GET_TOKEN(t, tokens, token_kind_hash);
  GET_TOKEN(t, tokens, token_kind_number);
  int group_id = br_strv_to_int(t.str);
  GET_TOKEN(t, tokens, token_kind_dot);
  GET_TOKEN(t, tokens, token_kind_ident);
  br_dagen_expr_kind_t kind;
  switch (t.str.str[0]) {
    case 'x': kind = br_dagen_expr_kind_reference_x; break;
    case 'y': kind = br_dagen_expr_kind_reference_y; break;
    case 'z': kind = br_dagen_expr_kind_reference_z; break;
    default: LOGEF("[Parser] Unexpected identifier '%.*s' at location %zu. Expected: 'x'|'y'|'z'\n", t.str.len, t.str.str, t.position); return false;
  }
  br_dagen_expr_t expr = { .kind = kind, .group_id = group_id };
  *out = (uint32_t)arena->len;
  br_da_push(*arena, expr);
  return true;
}

TEST_ONLY static void expr_to_str(br_str_t* out, br_dagen_exprs_t* arena, uint32_t index) {
  br_dagen_expr_t t = arena->arr[index];
  switch (t.kind) {
    case br_dagen_expr_kind_reference_x: br_str_push_literal(out, "#"); br_str_push_int(out, t.group_id); br_str_push_literal(out, ".x"); break;
    case br_dagen_expr_kind_reference_y: br_str_push_literal(out, "#"); br_str_push_int(out, t.group_id); br_str_push_literal(out, ".y"); break;
    case br_dagen_expr_kind_reference_z: br_str_push_literal(out, "#"); br_str_push_int(out, t.group_id); br_str_push_literal(out, ".z"); break;
    case br_dagen_expr_kind_add: br_str_push_literal(out, "(");
                                 expr_to_str(out, arena, t.operands.op1);
                                 br_str_push_literal(out, " + ");
                                 expr_to_str(out, arena, t.operands.op2);
                                 br_str_push_literal(out, ")");
                                 break;
    case br_dagen_expr_kind_mul: br_str_push_literal(out, "(");
                                 expr_to_str(out, arena, t.operands.op1);
                                 br_str_push_literal(out, " * ");
                                 expr_to_str(out, arena, t.operands.op2);
                                 br_str_push_literal(out, ")");
                                 break;
  }
}


#ifndef _MSC_VER
#include "misc/tests.h"

#define INIT \
  br_datas_t datas = {0}; \
  br_dagens_t dagens = {0}; \
  br_plots_t plots = {0}; \
  br_dagen_exprs_t arena = {0}; \
  (void)arena;

#define FREE \
  br_datas_deinit(&datas); \
  br_dagens_free(&dagens); \
  br_da_free(plots);

TEST_CASE(dagen_simple_reference_x) {
  INIT
  br_data_push_xy(&datas, 10.f, 20.f, 0);
  br_dagen_expr_t expr = { .kind = br_dagen_expr_kind_reference_x, .group_id = 0 };
  br_da_push(arena, expr);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 0, 0, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 10.f);
  TEST_EQUALF(res->dd.ys[0], 10.f);

  FREE
}

TEST_CASE(dagen_simple_reference_y) {
  INIT
  br_data_push_xy(&datas, 10.f, 20.f, 0);
  br_dagen_expr_t expr = { .kind = br_dagen_expr_kind_reference_y, .group_id = 0 };
  br_da_push(arena, expr);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 0, 0, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 20.f);
  TEST_EQUALF(res->dd.ys[0], 20.f);

  FREE
}

TEST_CASE(dagen_simple_reference_z) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 0);
  br_dagen_expr_t expr = { .kind = br_dagen_expr_kind_reference_z, .group_id = 0 };
  br_da_push(arena, expr);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 0, 0, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 30.f);
  TEST_EQUALF(res->dd.ys[0], 30.f);

  FREE
}

TEST_CASE(dagen_buffer_overflow) {
  INIT
  for (int i = 0; i < MAX_BATCH_LEN + 1; ++i) br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 0);
  br_dagen_expr_t exprx = { .kind = br_dagen_expr_kind_reference_x, .group_id = 0 };
  br_dagen_expr_t expry = { .kind = br_dagen_expr_kind_reference_z, .group_id = 0 };
  br_da_push(arena, exprx);
  br_da_push(arena, expry);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 0, 1, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, MAX_BATCH_LEN);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  for (size_t i = 0; i < res->len; ++i) TEST_EQUALF(res->dd.xs[i], 10.f);
  for (size_t i = 0; i < res->len; ++i) TEST_EQUALF(res->dd.ys[i], 30.f);

  br_dagens_handle_once(&datas, &dagens, &plots);
  TEST_EQUAL(res->len, MAX_BATCH_LEN + 1);
  FREE
}

TEST_CASE(dagen_add) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 0);
  br_dagen_expr_t expr_rx = { .kind = br_dagen_expr_kind_reference_x, .group_id = 0 };
  br_dagen_expr_t expr_rz = { .kind = br_dagen_expr_kind_reference_z, .group_id = 0 };
  br_dagen_expr_t expr_add = { .kind = br_dagen_expr_kind_add, .operands = { .op1 = 0, .op2 = 1 } };
  br_da_push(arena, expr_rx);
  br_da_push(arena, expr_rz);
  br_da_push(arena, expr_add);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 0, 2, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 10.f);
  TEST_EQUALF(res->dd.ys[0], 10.f + 30.f);
  FREE
}

TEST_CASE(dagen_mul) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 0);
  br_dagen_expr_t expr_rx = { .kind = br_dagen_expr_kind_reference_x, .group_id = 0 };
  br_dagen_expr_t expr_rz = { .kind = br_dagen_expr_kind_reference_z, .group_id = 0 };
  br_dagen_expr_t expr_mul = { .kind = br_dagen_expr_kind_mul, .operands = { .op1 = 0, .op2 = 1 } };
  br_da_push(arena, expr_rx);
  br_da_push(arena, expr_rz);
  br_da_push(arena, expr_mul);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 0, 2, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 10.f);
  TEST_EQUALF(res->dd.ys[0], 10.f * 30.f);
  FREE
}

TEST_CASE(dagen_mul_add) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 0);
  br_dagen_expr_t expr_rx = { .kind = br_dagen_expr_kind_reference_x, .group_id = 0 };
  br_dagen_expr_t expr_rz = { .kind = br_dagen_expr_kind_reference_z, .group_id = 0 };
  br_dagen_expr_t expr_add = { .kind = br_dagen_expr_kind_add, .operands = { .op1 = 0, .op2 = 1 } };
  br_dagen_expr_t expr_mul = { .kind = br_dagen_expr_kind_mul, .operands = { .op1 = 2, .op2 = 2 } };
  br_da_push(arena, expr_rx);
  br_da_push(arena, expr_rz);
  br_da_push(arena, expr_add);
  br_da_push(arena, expr_mul);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 0, 3, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 10.f);
  TEST_EQUALF(res->dd.ys[0], (10.f + 30.f) * (10.f + 30.f));
  FREE
}

TEST_CASE(dagen_add_mul) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 0);
  br_dagen_expr_t expr_rx = { .kind = br_dagen_expr_kind_reference_x, .group_id = 0 };
  br_dagen_expr_t expr_rz = { .kind = br_dagen_expr_kind_reference_z, .group_id = 0 };
  br_dagen_expr_t expr_mul = { .kind = br_dagen_expr_kind_mul, .operands = { .op1 = 0, .op2 = 1 } };
  br_dagen_expr_t expr_add = { .kind = br_dagen_expr_kind_add, .operands = { .op1 = 2, .op2 = 2 } };
  br_da_push(arena, expr_rx);
  br_da_push(arena, expr_rz);
  br_da_push(arena, expr_mul);
  br_da_push(arena, expr_add);
  br_dagen_push_expr_xy(&dagens, &datas, arena, 2, 3, 1);
  br_dagens_handle_once(&datas, &dagens, &plots);

  br_data_t* res = br_data_get1(datas, 1);
  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 10.f * 30.f);
  TEST_EQUALF(res->dd.ys[0], (10.f * 30.f) + (10.f * 30.f));
  FREE
}

#define TEST_TOKENIZER_PASS(STR, ...) do { \
  tokens_t ts = {0}; \
  br_strv_t s = br_strv_from_literal(STR); \
  token_kind_t tokens[] = {__VA_ARGS__}; \
  size_t len = sizeof(tokens)/sizeof(tokens[0]); \
  TEST_TRUE(tokens_get(&ts, s)); \
  TEST_EQUAL(ts.len, len); \
  for (size_t i = 0; i < len; ++i) { \
    TEST_EQUAL(ts.arr[i].kind, tokens[i]); \
  } \
  br_da_free(ts); \
} while(0)

TEST_CASE(dagen_tokenizer) {
  TEST_TOKENIZER_PASS("#1.x", token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident);
  TEST_TOKENIZER_PASS("# 1 . x    ", token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident);
  TEST_TOKENIZER_PASS("# 1 . x  + #2.y * 2222 ", token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident, token_kind_plus,
      token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident, token_kind_star, token_kind_number);
}

TEST_CASE(dagen_parser_ref_x) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.x");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 10.f);
  FREE
}

TEST_CASE(dagen_parser_ref_y) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("# 1 .    y");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 20.f);
  FREE
}

TEST_CASE(dagen_parser_ref_z) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("# 1 .    z");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 30.f);
  FREE
}

TEST_CASE(dagen_parser_add_zx) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.z + #1.x");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 40.f);
  FREE
}

TEST_CASE(dagen_parser_add_zxy) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.z + #1.x + #1.y");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 60.f);
  FREE
}

TEST_CASE(dagen_parser_add_z__mul_xx__y) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.z + #1.x * #1.x + #1.y");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 150.f);
  FREE
}

TEST_CASE(dagen_parser_add_sqr_zyx) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.z * #1.z + #1.y * #1.y + #1.x * #1.x");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  TEST_EQUALF(res->dd.xs[0], 10.f * 10.f + 20.f * 20.f + 30.f * 30.f);
  FREE
}

#endif
