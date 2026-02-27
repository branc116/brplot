#include "src/br_data_generator.h"
#include "include/br_str_header.h"
#include "include/brplat.h"
#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_memory.h"
#include "src/br_resampling.h"
#include "src/br_plot.h"
#include "src/br_resampling.h"
#include "src/br_series.h"

#define BR_DAGEN_MAX_BATCH_LEN (16*1024)
#define BR_DAGEN_MAX_BATCHES 64

#define BR_DAGEN_EXPECT_TOKEN(TOKEN, EXPECTED) do { \
  if ((TOKEN).kind != (EXPECTED)) { \
    LOGE("[Parser] Unexpected token %s(%d,%.*s) at position %zu, expected token %s", \
        token_to_str((TOKEN).kind), (TOKEN).kind, (TOKEN).str.len, (TOKEN).str.str, (TOKEN).position, token_to_str(EXPECTED)); \
    return false; \
  } \
} while(0)

#define BR_DAGEN_GET_TOKEN(TOKEN_OUT, TOKENS, EXPECTED) do { \
  if ((TOKENS)->pos >= (TOKENS)->len) { \
    LOGE("[Parser] Expected '%s' but got end of stream", token_to_str((EXPECTED))); \
    return false; \
  } \
  (TOKEN_OUT) = (TOKENS)->arr[(TOKENS)->pos++]; \
  BR_DAGEN_EXPECT_TOKEN((TOKEN_OUT), (EXPECTED)); \
} while (0)

#define BR_DAGEN_PUSH_EXPR(OUT_ID, ARENA, EXPR) do { \
  br_dagen_expr_t tmp_expr = EXPR; \
  (OUT_ID) = (uint32_t)(ARENA).len; \
  br_da_push((ARENA), tmp_expr); \
} while(0)

#define BR_DAGEN_SINGLE(X) \
  X(token_kind_hash, '#') \
  X(token_kind_plus, '+') \
  X(token_kind_star, '*') \
  X(token_kind_dot, '.') \
  X(token_kind_open_paren, '(') \
  X(token_kind_close_paren, ')') \
  X(token_kind_comma, ',')

typedef struct br_dagen_expr_context_t {
  br_series_view_zero_t view;
} br_dagen_expr_context_t;

typedef struct {
  br_dagen_expr_context_t arr[BR_DAGEN_MAX_BATCHES];
  size_t len;
} br_dagen_batches_t;

static br_dagen_batches_t br_dagen_batches;

inline static size_t min_s(size_t a, size_t b) { return a < b ? a : b; }

// INTERPRETER
bool br_dagens_handle_once(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots);
static void br_dagen_handle(br_dagen_t* dagen, br_data_t* data, br_datas_t datas);
static size_t expr_len(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index);
static size_t br_dagen_expr_read_per_batch(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index);
static size_t expr_read_n(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index);
static size_t expr_add_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index);
static size_t expr_mul_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index);
static br_dagen_expr_context_t br_dagen_expr_push_batch(void);
static void br_dagen_expr_set_last_group(float* data, size_t len, double offset);
static void pop_batch(void);

// PARSER
bool br_dagen_tokens_get(tokens_t* tokens, br_strv_t str);
static const char* token_to_str(br_dagen_token_kind kind);
static bool expr_parse(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out);
static bool expr_parse_ref(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out);
void br_dagen_expr_to_str(br_str_t* out, br_dagen_exprs_t* arena, uint32_t index);

static br_dagen_expr_t br_dagen_iota(int start) {
  return (br_dagen_expr_t){ .kind = br_dagen_expr_iota, .iota_state = start};
}

static br_dagen_expr_t br_dagen_pair(uint32_t op1, uint32_t op2) {
  return (br_dagen_expr_t){ .kind = br_dagen_expr_pair, .operands = { .op1 = op1, .op2 = op2 }};
}

static br_dagen_expr_t br_dagen_operand(br_dagen_expr_kind_t kind, uint32_t op1, uint32_t op2) {
  return (br_dagen_expr_t){ .kind = kind, .operands = { .op1 = op1, .op2 = op2 }};
}

bool br_dagen_push_expr_xy(br_dagens_t* dagens, br_datas_t* datas, br_dagen_exprs_t arena, uint32_t x, uint32_t y, int group_id) {
  if (NULL != br_data_get1(*datas, group_id)) {
    LOGI("Data with id %d already exists, will not create expr with the same id", group_id);
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
  tokens_t tokens        = {0};
  br_dagen_exprs_t arena = {0};
  uint32_t entry         = 0;

  if (false == br_dagen_tokens_get(&tokens, str)) goto error;
  if (false == expr_parse(&arena, &tokens, &entry)) goto error;
  br_dagen_expr_t t = br_da_get(arena, entry);
  switch (t.kind)
  {
    case br_dagen_expr_pair: {
      br_dagen_push_expr_xy(dagens, datas, arena, t.operands.op1, t.operands.op2, group_id);
    } break;
    case br_dagen_expr_reference:
    case br_dagen_expr_add:
    case br_dagen_expr_mul:
    case br_dagen_expr_iota:
    case br_dagen_expr_constant:
    case br_dagen_expr_function_call: {
      uint32_t x_id;
      BR_DAGEN_PUSH_EXPR(x_id, arena, br_dagen_iota(0));
      br_dagen_push_expr_xy(dagens, datas, arena, x_id, entry, group_id);
    } break;
    default: BR_UNREACHABLE("Unreachable toke kind: %d", t.kind);
  }

  br_da_free(tokens);
  return true;

error:
  br_da_free(tokens);
  br_da_free(arena);
  return false;
}

void br_dagen_push_file(br_dagens_t* dagens, br_data_t* data, size_t data_len) {
  br_dagen_t new = {
    .data_kind = data->kind,
    .state = br_dagen_state_inprogress,
    .group_id = data->group_id,
    .file = {
      .num_points = data_len
    }
  };

  br_da_push(*dagens, new);
}

void br_dagens_handle(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots, double until) {
  for (size_t i = 0; i < dagens->len; ++i) {
    dagens->arr[i].state = br_dagen_state_inprogress;
  }
  while (brpl_time() < until && br_dagens_handle_once(datas, dagens, plots));
}

void br_dagens_free(br_dagens_t* dagens) {
  for (size_t i = 0; i < dagens->len; ++i) {
    if (dagens->arr[i].kind == br_dagen_kind_expr) {
      for (size_t j = 0; j < dagens->arr[i].expr_2d.arena.len; ++j) {
        if (dagens->arr[i].expr_2d.arena.arr[j].kind == br_dagen_expr_function_call) {
          br_str_free(dagens->arr[i].expr_2d.arena.arr[j].function.func_name);
        }
      }
      br_da_free(dagens->arr[i].expr_2d.arena);
    }
  }

  br_da_free(*dagens);
}

bool br_dagens_handle_once(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots) {
  bool any = false;
  for (size_t i = 0; i < dagens->len;) {
    br_dagen_t* cur = &dagens->arr[i];
    br_data_t* d = br_data_get1(*datas, cur->group_id);
    if (NULL == d) cur->state = br_dagen_state_failed;
    else br_dagen_handle(cur, d, *datas);
    switch (cur->state) {
      case br_dagen_state_failed: {
        if (d != NULL) {
          br_data_remove(datas, d->group_id);
          br_plots_remove_group(*plots, d->group_id);
        }
        br_da_remove_at(*dagens, i);
      } break;
      case br_dagen_state_finished: br_da_remove_at(*dagens, i); break;
      case br_dagen_state_inprogress: any = true; ++i; break;
      case br_dagen_state_paused: ++i; break;
      default: BR_UNREACHABLE("State: %d unknown", cur->state);
    }
  }
  return any;
}

static void br_dagen_handle(br_dagen_t* dagen, br_data_t* data, br_datas_t datas) {
  BR_ASSERT((data->kind == br_data_kind_2d) || (data->kind == br_data_kind_3d));
  switch (dagen->kind) {
    case br_dagen_kind_file: {
      size_t len = br_data_len(*data);
      size_t index = len - dagen->file.num_points;
      size_t left = len - index;
      size_t read_n = left < 1024 ? left : 1024;
      for (size_t j = index; j < read_n; ++j) {
        br_resampling_add_point(data->resampling, data, (uint32_t)j);
      }
      BR_ASSERT(dagen->file.num_points >= read_n);
      dagen->file.num_points -= read_n;
      if (0 == dagen->file.num_points) dagen->state = br_dagen_state_finished;
    } break;
    case br_dagen_kind_expr: {
      switch(data->kind) {
        case br_data_kind_2d: {
          BR_TODO("Implmenet this using stack machine");
#if 0 
          size_t read_index = data->len;
          br_dagen_exprs_t arena = dagen->expr_2d.arena;
          size_t read_per_batch_x = br_dagen_expr_read_per_batch(datas, arena, dagen->expr_2d.x_expr_index);
          size_t read_per_batch_y = br_dagen_expr_read_per_batch(datas, arena, dagen->expr_2d.y_expr_index);
          size_t read_per_batch = read_per_batch_x < read_per_batch_y ? read_per_batch_x : read_per_batch_y;
          size_t x_len = expr_len(datas, arena, dagen->expr_2d.x_expr_index);
          size_t y_len = expr_len(datas, arena, dagen->expr_2d.y_expr_index);
          size_t min_len = x_len < y_len ? x_len : y_len;
          if (min_len == 0xFFFFFFFF) min_len = 1024;
          if (min_len <= read_index) {
            dagen->state = br_dagen_state_paused;
            return;
          }
          min_len -= read_index;
          read_per_batch = min_s(read_per_batch, min_len);
          BR_ASSERT(br_dagen_batches.len == 0);
          expr_read_n(datas, arena, dagen->expr_2d.x_expr_index);
          expr_read_n(datas, arena, dagen->expr_2d.y_expr_index);
          BR_ASSERT(br_dagen_batches.len == 2);
          for (size_t i = 0; i < read_per_batch; ++i) {
            br_data_push_xy(datas, br_dagen_batches[0].arr[i], br_dagen_batches[1].arr[i], dagen->group_id);
          }
          br_dagen_batches.len = 0;
#endif
        } break;
        default: BR_UNREACHABLE("Unsupported data kind: %d", data->kind);
      }
    } break;
    default: BR_ASSERT(0);
  }
}

static size_t expr_len(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  switch (expr.kind) {
    case br_dagen_expr_reference: {
      br_data_t* data = br_data_get1(datas, expr.data_id);
      BR_ASSERT(data->serieses_len > expr.series_index);
      br_u64 len = br_serieses_len(data->series_handles[expr.series_index]);
      return len;
    } break;
    case br_dagen_expr_add:
    case br_dagen_expr_mul:
    case br_dagen_expr_pair: {
      return min_s(expr_len(datas, arena, expr.operands.op1), expr_len(datas, arena, expr.operands.op2));
    } break;
    case br_dagen_expr_iota: return 0xFFFFFFFF;
    case br_dagen_expr_constant: return 0xFFFFFFFF;
    case br_dagen_expr_function_call: {
       if (br_strv_eq(br_str_as_view(expr.function.func_name), BR_STRL("range"))) {
         br_dagen_expr_t arg = br_da_get(arena, expr.function.arg);
         if (arg.kind == br_dagen_expr_constant) {
           if (false == isfinite(arg.value)) return 0;
           if (arg.value <= 0) return 0;
           return (size_t)arg.value;
         } else if (arg.kind == br_dagen_expr_pair) {
           br_dagen_expr_t arg1 = br_da_get(arena, arg.operands.op1);
           br_dagen_expr_t arg2 = br_da_get(arena, arg.operands.op2);
           if (arg1.kind == br_dagen_expr_constant && arg2.kind == br_dagen_expr_constant) {
             return (size_t)(arg2.value - arg1.value);
           } else {
             LOGE("Bad arguments: %d & %d", arg1.kind, arg2.kind);
             return 0;
           }
         } else {
           LOGE("Bad argument: %d", arg.kind);
         }
       } else {
         return expr_len(datas, arena, expr.function.arg);
       }
    }
  }
  BR_UNREACHABLE("expr.kind: %d", expr.kind);
  BR_RETURN_IF_TINY_C(0);
}

static size_t br_dagen_expr_read_per_batch(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  switch (expr.kind) {
    case br_dagen_expr_reference: {
      return BR_SERIES_SUPPORT_CAP;
    } break;
    case br_dagen_expr_add:
    case br_dagen_expr_mul:
    case br_dagen_expr_pair: {
      return min_s(br_dagen_expr_read_per_batch(datas, arena, expr.operands.op1), br_dagen_expr_read_per_batch(datas, arena, expr.operands.op2));
    } break;
    case br_dagen_expr_iota: return BR_DAGEN_MAX_BATCH_LEN;
    case br_dagen_expr_constant: return BR_DAGEN_MAX_BATCH_LEN;
    case br_dagen_expr_function_call: {
       size_t n = br_dagen_expr_read_per_batch(datas, arena, expr.function.arg);
       if (n > 128 && br_strv_eq(br_str_as_view(expr.function.func_name), BR_STRL("fft"))) {
         return 128;
       }
       return n;
    } break;
    default: BR_UNREACHABLE("expr.kind: %d", expr.kind);
  }
  BR_RETURN_IF_TINY_C(0);
}

// TODO: This is all bullshit. Convert expressions to stack machine and execute it like that..
#if 0
static size_t expr_apply_function(float* data, br_dagen_exprs_t arena, br_datas_t datas, size_t offset, size_t n, br_strv_t func_name, size_t arg_index) {
  if (br_strv_eq(func_name, BR_STRL("sin"))) {
    n = expr_read_n(datas, arena, (uint32_t)arg_index, offset, n, data);
    for (size_t i = 0; i < n; ++i) data[i] = sinf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("cos"))) {
    n = expr_read_n(datas, arena, (uint32_t)arg_index, offset, n, data);
    for (size_t i = 0; i < n; ++i) data[i] = cosf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("tg"))) {
    n = expr_read_n(datas, arena, (uint32_t)arg_index, offset, n, data);
    for (size_t i = 0; i < n; ++i) data[i] = tanf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("log"))) {
    n = expr_read_n(datas, arena, (uint32_t)arg_index, offset, n, data);
    for (size_t i = 0; i < n; ++i) data[i] = logf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("abs"))) {
    n = expr_read_n(datas, arena, (uint32_t)arg_index, offset, n, data);
    for (size_t i = 0; i < n; ++i) data[i] = fabsf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("range"))) {
    br_dagen_expr_t arg = br_da_get(arena, arg_index);
    if (arg.kind == br_dagen_expr_kind_constant) {
      size_t i = 0;
      for (i = 0; i < n && arg.iota_state < (int)arg.value; ++i) {
        data[i] = (float)arg.iota_state++;
      }
      return i;
    } else if (arg.kind == br_dagen_expr_kind_pair) {
      br_dagen_expr_t arg1 = br_da_get(arena, arg.operands.op1);
      br_dagen_expr_t arg2 = br_da_get(arena, arg.operands.op2);
      if (arg1.kind == br_dagen_expr_kind_constant && arg1.kind == br_dagen_expr_kind_constant) {
        size_t i = 0;
        for (i = 0; i < n && arg1.iota_state + (int)arg1.value < (int)arg2.value; ++i) {
          data[i] = (arg1.value + (float)arg1.iota_state++);
        }
        return i;
      } else {
        LOGE("Bad kind: %d", arg.kind);
        return 0;
      }
    } else {
      LOGE("Bad kind: %d", arg.kind);
      return 0;
    }
  } else if (br_strv_eq(func_name, BR_STRL("fft"))) {
    n = expr_read_n(datas, arena, (uint32_t)arg_index, offset, n, data);
    br_dagen_expr_context_t im_part = br_dagen_expr_push_batch();
    br_dagen_expr_context_t re_part = br_dagen_expr_push_batch();
    if (br_dagen_batches.last_referenced_group_len > 0) {
      for (size_t k = 0; k < n; ++k) {
        im_part.data[k] = 0;
        re_part.data[k] = 0;
        float om = 2.f*BR_PI*(float)(k + offset)/(float)br_dagen_batches.last_referenced_group_len;
        for (size_t i = 0; i < br_dagen_batches.last_referenced_group_len; ++i) {
          im_part.data[k] -= (float)(br_dagen_batches.last_referenced_group_offset + (double)br_dagen_batches.last_referenced_group_data[i]) * sinf(om*(float)i);
          re_part.data[k] += (float)(br_dagen_batches.last_referenced_group_offset + (double)br_dagen_batches.last_referenced_group_data[i]) * cosf(om*(float)i);
        }
      }
      for (size_t k = 0; k < n; ++k) {
        data[k] = sqrtf(im_part.data[k] * im_part.data[k] + re_part.data[k] * re_part.data[k]);
      }
    }
    pop_batch();
    pop_batch();
  } else {
    LOGE("Unknown function name `%.*s`", func_name.len, func_name.str);
  }
  return n;
}

static size_t expr_read_n(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t index, size_t offset) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  switch (expr.kind) {
    case br_dagen_expr_kind_add: {
      size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, data);
      size_t added = expr_add_to(datas, arena, expr.operands.op2, offset, read, data);
      return added;
    } break;
    case br_dagen_expr_kind_mul: {
      size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, data);
      size_t added = expr_mul_to(datas, arena, expr.operands.op2, offset, read, data);
      return added;
    } break;
    case br_dagen_expr_reference: {
      br_serieses_view_zero(expr.series_handle);
      br_data_t* data_in = br_data_get1(datas, expr.group_id);
      if (NULL == data_in) return 0;
      size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
      double* fs = NULL;
      double rebase = br_dagen_rebase(data_in, expr.kind);
      switch (expr.kind) {
        case br_dagen_expr_kind_reference_x: fs = data_in->dd.xs; break;
        case br_dagen_expr_kind_reference_y: fs = data_in->dd.ys; break;
        case br_dagen_expr_kind_reference_z: fs = data_in->ddd.zs; break;
        default: BR_UNREACHABLE("Expr kind unknown: %d", expr.kind);
      }

      br_dagen_expr_set_last_group(fs, data_in->len, rebase);

      fs += offset;
      memcpy(data, fs, real_n * sizeof(data[0]));
      for (size_t i = 0; i < real_n; ++i) data[i] = (float)((double)data[i] + rebase);
      return real_n;
    } break;
    case br_dagen_expr_kind_iota: {
      for (size_t i = 0; i < n; ++i) data[i] = (float)(expr.iota_state++);
      arena.arr[expr_index] = expr;
      return n;
    } break;
    case br_dagen_expr_kind_constant: {
      for (size_t i = 0; i < n; ++i) data[i] = expr.value;
      return n;
    } break;
    case br_dagen_expr_kind_function_call: {
      n = expr_apply_function(data, arena, datas, offset, n, br_str_as_view(expr.function.func_name), expr.function.arg);
      return n;
    } break;
    default: BR_UNREACHABLE("Expr kind unknown: %d", expr.kind);
  }
  BR_RETURN_IF_TINY_C(0);
}

static size_t expr_add_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  switch (expr.kind) {
    case br_dagen_expr_kind_add: {
      size_t read = expr_add_to(datas, arena, expr.operands.op1, offset, n, data);
      read = expr_add_to(datas, arena, expr.operands.op2, offset, read, data);
      return read;
    } break;
    case br_dagen_expr_kind_mul: {
      br_dagen_expr_context_t batch = br_dagen_expr_push_batch();
      size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, batch.data);
      read = expr_mul_to(datas, arena, expr.operands.op2, offset, read, batch.data);
      for (size_t i = 0; i < read; ++i) data[i] += batch.data[i];
      pop_batch();
      return read;
    } break;
    case br_dagen_expr_kind_reference_x:
    case br_dagen_expr_kind_reference_y:
    case br_dagen_expr_kind_reference_z: {
      br_data_t* data_in = br_data_get1(datas, expr.group_id);
      if (data_in == NULL) return 0;
      float* fs = NULL;
      double rebase = br_dagen_rebase(data_in, expr.kind);
      size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
      switch (expr.kind) {
        case br_dagen_expr_kind_reference_x: fs = data_in->dd.xs; break;
        case br_dagen_expr_kind_reference_y: fs = data_in->dd.ys; break;
        case br_dagen_expr_kind_reference_z: fs = data_in->ddd.zs; break;
        default: BR_UNREACHABLE("Expr kind: %d", expr.kind);
      }

      br_dagen_expr_set_last_group(fs, data_in->len, rebase);

      fs += offset;
      for (size_t i = 0; i < real_n; ++i) {
        // TODO: Make this also be rebasable
        data[i] += (float)((double)fs[i] + rebase);
      }
      return real_n;
    } break;
    case br_dagen_expr_kind_constant: {
      for (size_t i = 0; i < n; ++i) data[i] += expr.value;
      return n;
    } break;
    case br_dagen_expr_kind_function_call: {
      br_dagen_expr_context_t batch = br_dagen_expr_push_batch();
      n = expr_apply_function(batch.data, arena, datas, offset, n, br_str_as_view(expr.function.func_name), expr.function.arg);
      for (size_t i = 0; i < n; ++i) data[i] += batch.data[i];
      pop_batch();
      return n;
    } break;
    default: BR_UNREACHABLE("Expr kind: %d", expr.kind);
  }
  BR_RETURN_IF_TINY_C(0);
}

static size_t expr_mul_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  switch (expr.kind) {
    case br_dagen_expr_kind_add: {
      br_dagen_expr_context_t batch = br_dagen_expr_push_batch();
      size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, batch.data);
      read = expr_add_to(datas, arena, expr.operands.op2, offset, read, batch.data);
      for (size_t i = 0; i < read; ++i) data[i] *= batch.data[i];
      pop_batch();
      return read;
    } break;
    case br_dagen_expr_kind_mul: {
      size_t read = expr_mul_to(datas, arena, expr.operands.op1, offset, n, data);
      read = expr_mul_to(datas, arena, expr.operands.op2, offset, read, data);
      return read;
    } break;
    case br_dagen_expr_kind_reference_x:
    case br_dagen_expr_kind_reference_y:
    case br_dagen_expr_kind_reference_z: {
      br_data_t* data_in = br_data_get1(datas, expr.group_id);
      if (data_in == NULL) return 0;
      double rebase = br_dagen_rebase(data_in, expr.kind);
      float* fs = NULL;
      size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
      switch (expr.kind) {
        case br_dagen_expr_kind_reference_x: fs = data_in->dd.xs; break;
        case br_dagen_expr_kind_reference_y: fs = data_in->dd.ys; break;
        case br_dagen_expr_kind_reference_z: fs = data_in->ddd.zs; break;
        default: BR_UNREACHABLE("Unknown expr kind %d", expr.kind);
      }

      br_dagen_expr_set_last_group(fs, data_in->len, rebase);
      fs += offset;

      for (size_t i = 0; i < real_n; ++i) data[i] *= (float)((double)fs[i] + rebase);
      return real_n;
    } break;
    case br_dagen_expr_kind_constant: {
      for (size_t i = 0; i < n; ++i) data[i] *= expr.value;
      return n;
    } break;
    case br_dagen_expr_kind_function_call: {
      br_dagen_expr_context_t batch = br_dagen_expr_push_batch();
      n = expr_apply_function(batch.data, arena, datas, offset, n, br_str_as_view(expr.function.func_name), expr.function.arg);
      for (size_t i = 0; i < n; ++i) data[i] *= batch.data[i];
      pop_batch();
      return n;
    } break;
    default: BR_UNREACHABLE("Unknown expr kind %d", expr.kind);
  }
  BR_RETURN_IF_TINY_C(0);
}

static br_dagen_expr_context_t br_dagen_expr_push_batch(void) {
  if (br_dagen_batches.len < br_dagen_batches.max_len) {
    return br_dagen_batches.arr[br_dagen_batches.len++];
  }

  float* batch = BR_MALLOC(sizeof(float) * MAX_BATCH_LEN);
  br_dagen_expr_context_t new = { batch };
  br_da_push(br_dagen_batches, new);
  br_dagen_batches.max_len = br_dagen_batches.len;
  return new;
}

static void br_dagen_expr_set_last_group(float* data, size_t len, double offset) {
  br_dagen_batches.last_referenced_group_data = data;
  br_dagen_batches.last_referenced_group_len = len;
  br_dagen_batches.last_referenced_group_offset = offset;
}

static void pop_batch(void) {
  --br_dagen_batches.len;
}
#endif

bool br_dagen_tokens_get(tokens_t* tokens, br_strv_t str) {
  br_dagen_token_t t;

  for (uint32_t i = 0; i < str.len; ++i) {
    uint32_t n = 0;
    while (str.str[i + n] >= 'a' && str.str[i + n] <= 'z') ++n;
    if (n > 0) {
      t = (br_dagen_token_t){ .kind = token_kind_ident, .str = br_str_sub(str, i, n), .position = i };
      i += n - 1;
      goto push_token;
    }
    if (str.str[i] >= '0' && str.str[i] <= '9') {
      while (i + n < str.len && ((str.str[i + n] >= '0' && str.str[i + n] <= '9') || str.str[i + n] == 'e' || str.str[i + n] == '.')) ++n;
      if (i + n < str.len && str.str[i + n - 1] == '.') --n;
      t = (br_dagen_token_t) { .kind = token_kind_number, .str = br_str_sub(str, i, n), .position = i };
      i += n - 1;
      goto push_token;
    }
    switch (str.str[i]) {
#define X(KIND, CHAR) \
      case CHAR: \
        t = (br_dagen_token_t){ .kind = KIND, .str = br_str_sub(str, i, 1), .position = i }; \
        goto push_token;
      BR_DAGEN_SINGLE(X)
#undef X
      case ' ': case '\t': case '\n': case '\r': continue;
      default:
        LOGE("[Tokenizer] Unknown character '%c' while tokenizing expression `%.*s`", str.str[i], str.len, str.str);
        return false;
    }
    continue;
push_token:
    br_da_push(*tokens, t);
  }
  return true;
}

static const char* token_to_str(br_dagen_token_kind kind) {
  switch (kind) {
#define X(name) case name: return #name;
    TOKEN_KINDS(X)
#undef X
    default: return "Unknown";
  }
}

static br_dagen_token_t expr_peek(tokens_t tokens) {
  return tokens.pos >= tokens.len ? (br_dagen_token_t){0} : tokens.arr[tokens.pos];
}

static bool expr_parse_primary(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out);
static bool expr_parse(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  br_dagen_token_t t;
  uint32_t ref1;
  if (false == expr_parse_primary(arena, tokens, &ref1)) return false;
  t = expr_peek(*tokens);
  if (t.kind == token_kind_comma) {
    uint32_t ref2;
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_comma);
    if (false == expr_parse(arena, tokens, &ref2)) return false;
    BR_DAGEN_PUSH_EXPR(*out, *arena, br_dagen_pair(ref1, ref2));
    return true;
  }
  *out = ref1;
  return true;
}

static bool expr_parse_ref(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  br_dagen_token_t t;
  t = expr_peek(*tokens);
  if (t.kind == token_kind_number) {
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_number);
    br_dagen_expr_t expr = { .kind = br_dagen_expr_constant, .value = br_strv_to_float(t.str) };
    *out = (uint32_t)arena->len;
    br_da_push(*arena, expr);
    return true;
  } else if (t.kind == token_kind_hash) {
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_hash);
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_number);
    int data_id = br_strv_to_int(t.str);
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_dot);
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_ident);
    br_dagen_expr_kind_t kind;
    int series_index = 0;
    switch (t.str.str[0]) {
      case 'x': kind = br_dagen_expr_reference;  break;
      case 'y': kind = br_dagen_expr_reference; series_index = 1; break;
      case 'z': kind = br_dagen_expr_reference; series_index = 2; break;
      default: LOGE("[Parser] Unexpected identifier '%.*s' at location %zu. Expected: 'x'|'y'|'z'", t.str.len, t.str.str, t.position); return false;
    }
    br_dagen_expr_t expr = { .kind = kind, .data_id = data_id, .series_index = series_index };
    *out = (uint32_t)arena->len;
    br_da_push(*arena, expr);
    return true;
  } else if (t.kind == token_kind_ident) {
    uint32_t arg_ref;
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_ident);
    br_str_t func_name = br_str_malloc(t.str.len + 1);
    br_str_push_strv(&func_name, t.str);
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_open_paren);
    if (false == expr_parse(arena, tokens, &arg_ref)) return false;
    BR_DAGEN_GET_TOKEN(t, tokens, token_kind_close_paren);
    br_dagen_expr_t expr = { .kind = br_dagen_expr_function_call, .function = { .func_name = func_name, .arg = arg_ref } };
    *out = (uint32_t)arena->len;
    br_da_push(*arena, expr);
    return true;
  }
  LOGE("[Parser] Unexpected identifier '%.*s' at location %zu. Expected ref or number", t.str.len, t.str.str, t.position);
  return false;
}


static bool expr_parse_add(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  br_dagen_token_t t;
  BR_DAGEN_GET_TOKEN(t, tokens, token_kind_plus);
  return expr_parse_primary(arena, tokens, out);
}

static bool expr_parse_mul(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  br_dagen_token_t t;
  uint32_t ref;
  BR_DAGEN_GET_TOKEN(t, tokens, token_kind_star);
  if (false == expr_parse_ref(arena, tokens, &ref)) return false;
  t = expr_peek(*tokens);
  if (t.kind == token_kind_star) {
    uint32_t ref2;
    if (false == expr_parse_mul(arena, tokens, &ref2)) return false;
    BR_DAGEN_PUSH_EXPR(*out, *arena, br_dagen_operand(br_dagen_expr_mul, ref, ref2));
  } else *out = ref;
  return true;
}

static bool expr_parse_primary(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  if (false == expr_parse_ref(arena, tokens, out)) return false;
  br_dagen_token_t t;
  uint32_t ref;
start:
  t = expr_peek(*tokens);
  ref = 0;
  switch (t.kind) {
    case token_kind_plus: {
      if (false == expr_parse_add(arena, tokens, &ref)) return false;
      BR_DAGEN_PUSH_EXPR(*out, *arena, br_dagen_operand(br_dagen_expr_add, *out, ref));
    } break;
    case token_kind_star: {
      if (false == expr_parse_mul(arena, tokens, &ref)) return false;
      BR_DAGEN_PUSH_EXPR(*out, *arena, br_dagen_operand(br_dagen_expr_mul, *out, ref));
      goto start;
    } break;
    default: break;
  }
  return true;
}

BR_TEST_ONLY void br_dagen_expr_to_str(br_str_t* out, br_dagen_exprs_t* arena, uint32_t index) {
  br_dagen_expr_t t = arena->arr[index];
  switch (t.kind) {
    case br_dagen_expr_reference: br_str_push_literal(out, "#"); br_str_push_int(out, t.data_id); br_str_push_literal(out, "."); br_str_push_int(out, t.series_index); return;
    case br_dagen_expr_add: br_str_push_literal(out, "(");
                                 br_dagen_expr_to_str(out, arena, t.operands.op1);
                                 br_str_push_literal(out, " + ");
                                 br_dagen_expr_to_str(out, arena, t.operands.op2);
                                 br_str_push_literal(out, ")");
                                 return;
    case br_dagen_expr_mul: br_str_push_literal(out, "(");
                            br_dagen_expr_to_str(out, arena, t.operands.op1);
                            br_str_push_literal(out, " * ");
                            br_dagen_expr_to_str(out, arena, t.operands.op2);
                            br_str_push_literal(out, ")");
                            return;
    case br_dagen_expr_pair:br_dagen_expr_to_str(out, arena, t.operands.op1);
                            br_str_push_literal(out, ", ");
                            br_dagen_expr_to_str(out, arena, t.operands.op2);
                            return;
    case br_dagen_expr_iota: br_str_push_literal(out, "0..4.8e9");
                             return;
    case br_dagen_expr_constant: br_str_push_float(out, t.value);
                                 return;
    case br_dagen_expr_function_call: br_str_push_strv(out, br_str_as_view(t.function.func_name));
                                      br_str_push_char(out, '(');
                                      br_dagen_expr_to_str(out, arena, t.function.arg);
                                      br_str_push_char(out, ')');
                                      return;
  }
  BR_UNREACHABLE("Unknown kind: %d", t.kind);
}

#undef BR_DAGEN_MAX_BATCHES
#undef BR_DAGEN_MAX_BATCH_LEN
#undef BR_DAGEN_EXPECT_TOKEN
#undef BR_DAGEN_GET_TOKEN
#undef BR_DAGEN_PUSH_EXPR
#undef BR_DAGEN_SINGLE
