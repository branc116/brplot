#include "src/br_data_generator.h"
#include "src/br_da.h"
#include "src/br_data.h"
#include "src/br_permastate.h"
#include "src/br_plot.h"
#include "src/br_pp.h"
#include "src/br_resampling2.h"
#include "src/br_str.h"
#include "src/br_tl.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MAX_BATCH_LEN (16*1024)

#define EXPECT_TOKEN(TOKEN, EXPECTED) do { \
  if ((TOKEN).kind != (EXPECTED)) { \
    LOGE("[Parser] Unexpected token %s(%d,%.*s) at position %zu, expected token %s", \
        token_to_str((TOKEN).kind), (TOKEN).kind, (TOKEN).str.len, (TOKEN).str.str, (TOKEN).position, token_to_str(EXPECTED)); \
    return false; \
  } \
} while(0)

#define GET_TOKEN(TOKEN_OUT, TOKENS, EXPECTED) do { \
  if ((TOKENS)->pos >= (TOKENS)->len) { \
    LOGE("[Parser] Expected '%s' but got end of stream", token_to_str((EXPECTED))); \
    return false; \
  } \
  (TOKEN_OUT) = (TOKENS)->arr[(TOKENS)->pos++]; \
  EXPECT_TOKEN((TOKEN_OUT), (EXPECTED)); \
} while (0)

#define PUSH_EXPR(OUT_ID, ARENA, EXPR) do { \
  br_dagen_expr_t tmp_expr = EXPR; \
  (OUT_ID) = (uint32_t)(ARENA).len; \
  br_da_push((ARENA), tmp_expr); \
} while(0)

#define SINGLE(X) \
  X(token_kind_hash, '#') \
  X(token_kind_plus, '+') \
  X(token_kind_star, '*') \
  X(token_kind_dot, '.') \
  X(token_kind_open_paren, '(') \
  X(token_kind_close_paren, ')') \
  X(token_kind_comma, ',')

#define TOKEN_KINDS(X) \
  X(token_kind_invalid) \
  X(token_kind_number) \
  X(token_kind_ident) \
  X(token_kind_hash) \
  X(token_kind_plus) \
  X(token_kind_star) \
  X(token_kind_dot) \
  X(token_kind_open_paren) \
  X(token_kind_close_paren) \
  X(token_kind_contant) \
  X(token_kind_comma)

typedef enum {
#define X(name) name,
  TOKEN_KINDS(X)
#undef X
} br_dagen_token_kind;

typedef struct {
  br_dagen_token_kind kind;
  br_strv_t str;
  size_t position;
} br_dagen_token_t;

typedef struct {
  br_dagen_token_t* arr;
  size_t len, cap;
  size_t pos;
} tokens_t;

typedef struct br_dagen_expr_context_t {
	float* data;
	int last_referenced_group;
} br_dagen_expr_context_t;

typedef struct {
  br_dagen_expr_context_t* arr;
  size_t len, cap;
  size_t max_len;
} batches_t;

static batches_t batches;

inline static size_t min_s(size_t a, size_t b) { return a < b ? a : b; }

// INTERPRETER
static bool br_dagens_handle_once(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots);
static void br_dagen_handle(br_dagen_t* dagen, br_data_t* data, br_datas_t datas);
static size_t expr_len(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index);
static size_t expr_read_n(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data);
static size_t expr_add_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data);
static size_t expr_mul_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data);
static br_dagen_expr_context_t push_batch(void);
static void pop_batch(void);

// PARSER
static bool tokens_get(tokens_t* tokens, br_strv_t str);
static const char* token_to_str(br_dagen_token_kind kind);
static bool expr_parse(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out);
static bool expr_parse_ref(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out);
static void expr_to_str(br_str_t* out, br_dagen_exprs_t* arena, uint32_t index);

static br_dagen_expr_t br_dagen_expr_iota(int start) {
  return (br_dagen_expr_t){ .kind = br_dagen_expr_kind_iota, .iota_state = start};
}

static br_dagen_expr_t br_dagen_expr_pair(uint32_t op1, uint32_t op2) {
  return (br_dagen_expr_t){ .kind = br_dagen_expr_kind_pair, .operands = { .op1 = op1, .op2 = op2 }};
}

static br_dagen_expr_t br_dagen_expr_operand(br_dagen_expr_kind_t kind, uint32_t op1, uint32_t op2) {
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

  if (false == tokens_get(&tokens, str)) goto error;
  if (false == expr_parse(&arena, &tokens, &entry)) goto error;
  br_dagen_expr_t t = br_da_get(arena, entry);
  switch (t.kind)
  {
    case br_dagen_expr_kind_pair: {
      br_dagen_push_expr_xy(dagens, datas, arena, t.operands.op1, t.operands.op2, group_id);
    } break;
    case br_dagen_expr_kind_reference_x:
    case br_dagen_expr_kind_reference_y:
    case br_dagen_expr_kind_reference_z:
    case br_dagen_expr_kind_add:
    case br_dagen_expr_kind_mul:
    case br_dagen_expr_kind_iota:
    case br_dagen_expr_kind_constant:
    case br_dagen_expr_kind_function_call: {
      uint32_t x_id;
      PUSH_EXPR(x_id, arena, br_dagen_expr_iota(0));
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

bool br_dagen_push_file_2d(br_dagens_t* dagens, br_datas_t* datas, br_data_desc_t* desc, FILE* file) {
  size_t data_left = 0;
  bool success = true;
  br_color_t color;
  size_t cap;

  if (file == NULL)                               BR_ERRORE("File is null");
  if (1 != fread(&color, sizeof(color), 1, file)) BR_ERRORE("Failed to read color");
  if (1 != fread(&cap, sizeof(cap), 1, file))     BR_ERROR("Failed to read capacity");

  br_data_t* data = br_datas_create2(datas, desc->group_id, br_data_kind_2d, color, cap, desc->name);
  if (NULL == data)                               BR_ERROR("Failed to create data");

  if (0 != cap) {
    data_left = cap;
    if (1 != fread(&data->dd.bounding_box, sizeof(data->dd.bounding_box), 1, file)) BR_ERRORE("Failed to read bounding box");
    if (1 != fread(&data->dd.rebase_x, sizeof(data->dd.rebase_x), 1, file)) BR_ERRORE("Failed to read rebase x");
    if (1 != fread(&data->dd.rebase_y, sizeof(data->dd.rebase_y), 1, file)) BR_ERRORE("Failed to read rebase y");
    br_dagen_t new = {
      .data_kind = br_data_kind_2d,
      .state = br_dagen_state_inprogress,
      .group_id = desc->group_id,
      .file = {
        .file = file,
        .x_left = data_left,
        .y_left = data_left,
        .num_points = data_left
      }
    };
    br_da_push(*dagens, new);
  }
  goto done;

error:
  success = false;

done:
  return success;
}

bool br_dagen_push_file(br_dagens_t* dagens, br_datas_t* datas, br_data_desc_t* desc, FILE* file) {
  br_save_state_command_t command = br_save_state_command_save_plots;
  size_t data_left = 0;
  br_data_kind_t kind;
  br_color_t color;
  size_t cap;
  bool success = true;

  if (file == NULL)                                   BR_ERRORE("File is null");
  if (1 != fread(&command, sizeof(command), 1, file)) BR_ERRORE("Failed to read command");
  switch (command) {
    case br_save_state_command_save_data_2d: kind = br_data_kind_2d; break;
    case br_save_state_command_save_data_3d: kind = br_data_kind_3d; break;
    default:                                          BR_ERROR("Unknown save command: %d", command);
  }
  if (1 != fread(&color, sizeof(color), 1, file))     BR_ERRORE("Failed to read color");
  if (1 != fread(&cap, sizeof(cap), 1, file))         BR_ERROR("Failed to read capacity");

  br_data_t* data = br_datas_create2(datas, desc->group_id, kind, color, cap, desc->name);
  if (NULL == data)                                   BR_ERROR("Failed to create data");

  if (0 != cap) {
    data_left = cap;
    switch (kind) {
      case br_data_kind_2d: {
        if (1 != fread(&data->dd.bounding_box, sizeof(data->dd.bounding_box), 1, file)) BR_ERRORE("Failed to read bounding box");
        if (1 != fread(&data->dd.rebase_x, sizeof(data->dd.rebase_x), 1, file))         BR_ERRORE("Failed to read rebase x");
        if (1 != fread(&data->dd.rebase_y, sizeof(data->dd.rebase_y), 1, file))         BR_ERRORE("Failed to read rebase y");
      } break;
      case br_data_kind_3d: {
        if (1 != fread(&data->ddd.bounding_box, sizeof(data->ddd.bounding_box), 1, file)) BR_ERRORE("Failed to read bounding box");
        if (1 != fread(&data->ddd.rebase_x, sizeof(data->ddd.rebase_x), 1, file))         BR_ERRORE("Failed to read rebase x");
        if (1 != fread(&data->ddd.rebase_y, sizeof(data->ddd.rebase_y), 1, file))         BR_ERRORE("Failed to read rebase y");
        if (1 != fread(&data->ddd.rebase_z, sizeof(data->ddd.rebase_z), 1, file))         BR_ERRORE("Failed to read rebase z");
      } break;
      default:                                                                            BR_ERROR("data kind unknown %d", kind);
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
      .num_points = data_left
    }
  };
  br_da_push(*dagens, new);
  return success;

error:
  if (file != NULL) fclose(file);
  return success;
}

void br_dagens_handle(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots, double until) {
  for (size_t i = 0; i < dagens->len; ++i) {
    dagens->arr[i].state = br_dagen_state_inprogress;
  }
  while (brtl_time() < until && br_dagens_handle_once(datas, dagens, plots));
}

void br_dagens_free(br_dagens_t* dagens) {
  for (size_t i = 0; i < dagens->len; ++i)
    if (dagens->arr[i].kind == br_dagen_kind_file) fclose(dagens->arr[i].file.file);
    else if (dagens->arr[i].kind == br_dagen_kind_expr) br_da_free(dagens->arr[i].expr_2d.arena);
  br_da_free(*dagens);
  for (size_t i = 0; i < batches.max_len; ++i) {
    BR_FREE(batches.arr[i].data);
  }
  br_da_free(batches);
  batches.max_len = 0;
}

static bool br_dagens_handle_once(br_datas_t* datas, br_dagens_t* dagens, br_plots_t* plots) {
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
  br_strv_t name;
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
      size_t index = dagen->file.num_points - *left;
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
      name = brsp_get(*brtl_brsp(), data->name);
      LOGE("Failed to read data for a plot %.*s: %d(%s)", name.len, name.str, errno, strerror(errno));
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
          if (min_len == 0xFFFFFFFF) min_len = 1024;
          if (data->cap < min_len) if (false == br_data_realloc(data, min_len)) {
            LOGE("Failed to alloc %zu kB of memory for generated plot", min_len >> 10);
            dagen->state = br_dagen_state_failed;
          }
          min_len -= read_index;
          if (min_len == 0) {
            dagen->state = br_dagen_state_paused;
            return;
          }
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
        default: LOGE("Unsupported data kind: %d", data->kind); BR_ASSERT(0);
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
    case br_dagen_expr_kind_reference_z: {
      br_data_t* data_in = br_data_get1(datas, expr.group_id);
      if (NULL == data_in) return 0;
      return data_in->len;
    } break;
    case br_dagen_expr_kind_add:
    case br_dagen_expr_kind_mul:
    case br_dagen_expr_kind_pair: {
      return min_s(expr_len(datas, arena, expr.operands.op1), expr_len(datas, arena, expr.operands.op2));
    } break;
    case br_dagen_expr_kind_iota: return 0xFFFFFFFF;
    case br_dagen_expr_kind_constant: return 0xFFFFFFFF;
    case br_dagen_expr_kind_function_call: return expr_len(datas, arena, expr.function.arg);
  }
  BR_UNREACHABLE("expr.kind: %d", expr.kind);
  return 0;
}

static double br_dagen_rebase(br_data_t const* data, br_dagen_expr_kind_t kind) {
  switch (data->kind) {
    case br_data_kind_2d: {
      switch (kind) {
        case br_dagen_expr_kind_reference_x: return data->dd.rebase_x; break;
        case br_dagen_expr_kind_reference_y: return data->dd.rebase_y; break;
        default: BR_UNREACHABLE();
      }
    } break;
    case br_data_kind_3d: {
      switch (kind) {
        case br_dagen_expr_kind_reference_x: return data->ddd.rebase_x; break;
        case br_dagen_expr_kind_reference_y: return data->ddd.rebase_y; break;
        case br_dagen_expr_kind_reference_z: return data->ddd.rebase_z; break;
        default: BR_UNREACHABLE();
      }
    } break;
    default: BR_UNREACHABLE();
  }
  return 0.0;
}

static void expr_apply_function(float* data, size_t n, br_strv_t func_name) {
  if (br_strv_eq(func_name, BR_STRL("sin"))) {
    for (size_t i = 0; i < n; ++i) data[i] = sinf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("cos"))) {
    for (size_t i = 0; i < n; ++i) data[i] = cosf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("tg"))) {
    for (size_t i = 0; i < n; ++i) data[i] = tanf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("log"))) {
    for (size_t i = 0; i < n; ++i) data[i] = logf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("abs"))) {
    for (size_t i = 0; i < n; ++i) data[i] = fabsf(data[i]);
  } else if (br_strv_eq(func_name, BR_STRL("fft"))) {
    br_dagen_expr_context_t im_part = push_batch();
    br_dagen_expr_context_t re_part = push_batch();
    for (size_t k = 0; k < n; ++k) im_part[k] = 0.f;
    for (size_t k = 0; k < n; ++k) re_part[k] = 0.f;
    for (size_t k = 0; k < n; ++k) {
      im_part[k] = 0;
      re_part[k] = 0;
      float om = 2.f*BR_PI*(float)k/(float)n;
      for (size_t i = 0; i < n; ++i) {
        im_part[k] -= data[i] * sinf(om*(float)i);
        re_part[k] += data[i] * cosf(om*(float)i);
      }
    }
    for (size_t k = 0; k < n; ++k) {
      data[k] = sqrtf(im_part[k] * im_part[k] + re_part[k] * re_part[k]);
    }
    pop_batch();
    pop_batch();
  } else {
    LOGE("Unknown function name `%.*s`", func_name.len, func_name.str);
  }
}

static size_t expr_read_n(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data) {
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
    case br_dagen_expr_kind_reference_x: 
    case br_dagen_expr_kind_reference_y: 
    case br_dagen_expr_kind_reference_z: {
      br_data_t* data_in = br_data_get1(datas, expr.group_id);
      if (NULL == data_in) return 0;
      size_t real_n = (offset + n > data_in->len) ? data_in->len - offset : n;
      switch (expr.kind) {
        case br_dagen_expr_kind_reference_x: memcpy(data, &data_in->dd.xs[offset], real_n * sizeof(data[0])); break;
        case br_dagen_expr_kind_reference_y: memcpy(data, &data_in->dd.ys[offset], real_n * sizeof(data[0])); break;
        case br_dagen_expr_kind_reference_z: memcpy(data, &data_in->ddd.zs[offset], real_n * sizeof(data[0])); break;
        default: BR_UNREACHABLE("Expr kind unknown: %d", expr.kind);
      }
      double rebase = br_dagen_rebase(data_in, expr.kind);
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
      size_t real_n = expr_read_n(datas, arena, expr.function.arg, offset, n, data);
      expr_apply_function(data, real_n, expr.function.func_name);
      return n;
    } break;
    default: BR_UNREACHABLE("Expr kind unknown: %d", expr.kind);
  }
  return 0;
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
      float* batch = push_batch();
      size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, batch);
      read = expr_mul_to(datas, arena, expr.operands.op2, offset, read, batch);
      for (size_t i = 0; i < read; ++i) data[i] += batch[i];
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
        case br_dagen_expr_kind_reference_x: fs = &data_in->dd.xs[offset]; break;
        case br_dagen_expr_kind_reference_y: fs = &data_in->dd.ys[offset]; break;
        case br_dagen_expr_kind_reference_z: fs = &data_in->ddd.zs[offset]; break;
        default: BR_UNREACHABLE("Expr kind: %d", expr.kind);
      }
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
      float* batch = push_batch();
      size_t read = expr_read_n(datas, arena, expr.function.arg, offset, n, batch);
      expr_apply_function(batch, read, expr.function.func_name);
      for (size_t i = 0; i < n; ++i) data[i] += batch[i];
      pop_batch();
      return n;
    } break;
    default: BR_UNREACHABLE("Expr kind: %d", expr.kind);
  }
  return 0;
}

static size_t expr_mul_to(br_datas_t datas, br_dagen_exprs_t arena, uint32_t expr_index, size_t offset, size_t n, float* data) {
  br_dagen_expr_t expr = arena.arr[expr_index];
  switch (expr.kind) {
    case br_dagen_expr_kind_add: {
      float* new_batch = push_batch();
      size_t read = expr_read_n(datas, arena, expr.operands.op1, offset, n, new_batch);
      read = expr_add_to(datas, arena, expr.operands.op2, offset, read, new_batch);
      for (size_t i = 0; i < read; ++i) data[i] *= new_batch[i];
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
        case br_dagen_expr_kind_reference_x: fs = &data_in->dd.xs[offset]; break;
        case br_dagen_expr_kind_reference_y: fs = &data_in->dd.ys[offset]; break;
        case br_dagen_expr_kind_reference_z: fs = &data_in->ddd.zs[offset]; break;
        default: BR_UNREACHABLE("Unknown expr kind %d", expr.kind);
      }
      for (size_t i = 0; i < real_n; ++i) data[i] *= (float)((double)fs[i] + rebase);
      return real_n;
    } break;
    case br_dagen_expr_kind_constant: {
      for (size_t i = 0; i < n; ++i) data[i] *= expr.value;
      return n;
    } break;
    case br_dagen_expr_kind_function_call: {
      float* batch = push_batch();
      size_t read = expr_read_n(datas, arena, expr.function.arg, offset, n, batch);
      expr_apply_function(batch, read, expr.function.func_name);
      for (size_t i = 0; i < n; ++i) data[i] *= batch[i];
      pop_batch();
      return n;
    } break;
    default: BR_UNREACHABLE("Unknown expr kind %d", expr.kind);
  }
  return 0;
}


static br_dagen_expr_context_t br_dagen_expr_push_batch(void) {
  int last_group_id = -1;
  if (batches.len > 0) {
    br_dagen_expr_context_t c = br_da_get(batches, batches.len - 1);
    last_group_id = c.last_referenced_group;
  }
  if (batches.len < batches.max_len) {
    br_dagen_expr_context_t* ret = &batches.arr[batches.len++];
    ret->last_referenced_group = last_group_id;
    return *ret;
  }
  float* batch = BR_MALLOC(sizeof(float) * MAX_BATCH_LEN);
  br_dagen_expr_context_t new = { batch, last_group_id };
  br_da_push(batches, new);
  batches.max_len = batches.len;
  return new;
}

static br_dagen_expr_context_t br_dagen_expr_set_last_group(int group) {
  if (batches.len > 0) {
    br_da_getp(batches, batches.len - 1)->last_referenced_group = group;
  }
}

static void pop_batch(void) {
  --batches.len;
}

static bool tokens_get(tokens_t* tokens, br_strv_t str) {
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
      while ((str.str[i + n] >= '0' && str.str[i + n] <= '9') || str.str[i + n] == 'e' || str.str[i + n] == '.') ++n; 
      if (str.str[i + n - 1] == '.') --n;
      t = (br_dagen_token_t) { .kind = token_kind_number, .str = br_str_sub(str, i, n), .position = i };
      i += n - 1;
      goto push_token;
    }
    switch (str.str[i]) {
#define X(KIND, CHAR) \
      case CHAR: \
        t = (br_dagen_token_t){ .kind = KIND, .str = br_str_sub(str, i, 1), .position = i }; \
        goto push_token;
      SINGLE(X)
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
  if (tokens->pos < tokens->len) {
    uint32_t ref2;
    GET_TOKEN(t, tokens, token_kind_comma);
    if (false == expr_parse(arena, tokens, &ref2)) return false;
    PUSH_EXPR(*out, *arena, br_dagen_expr_pair(ref1, ref2));
    return true;
  }
  *out = ref1;
  return true;
}

static bool expr_parse_ref(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  br_dagen_token_t t;
  t = expr_peek(*tokens);
  if (t.kind == token_kind_number) {
    GET_TOKEN(t, tokens, token_kind_number);
    br_dagen_expr_t expr = { .kind = br_dagen_expr_kind_constant, .value = br_strv_to_float(t.str) };
    *out = (uint32_t)arena->len;
    br_da_push(*arena, expr);
    return true;
  } else if (t.kind == token_kind_hash) {
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
      default: LOGE("[Parser] Unexpected identifier '%.*s' at location %zu. Expected: 'x'|'y'|'z'", t.str.len, t.str.str, t.position); return false;
    }
    br_dagen_expr_t expr = { .kind = kind, .group_id = group_id };
    *out = (uint32_t)arena->len;
    br_da_push(*arena, expr);
    return true;
  } else if (t.kind == token_kind_ident) {
    uint32_t arg_ref;
    GET_TOKEN(t, tokens, token_kind_ident);
    br_strv_t func_name = t.str;
    GET_TOKEN(t, tokens, token_kind_open_paren);
    expr_parse_primary(arena, tokens, &arg_ref);
    GET_TOKEN(t, tokens, token_kind_close_paren);
    br_dagen_expr_t expr = { .kind = br_dagen_expr_kind_function_call, .function = { .func_name = func_name, .arg = arg_ref } };
    *out = (uint32_t)arena->len;
    br_da_push(*arena, expr);
    return true;
  }
  LOGE("[Parser] Unexpected identifier '%.*s' at location %zu. Expected ref or number", t.str.len, t.str.str, t.position); return false;
  return false;
}


static bool expr_parse_add(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  br_dagen_token_t t;
  GET_TOKEN(t, tokens, token_kind_plus);
  return expr_parse_primary(arena, tokens, out);
}

static bool expr_parse_mul(br_dagen_exprs_t* arena, tokens_t* tokens, uint32_t* out) {
  br_dagen_token_t t;
  uint32_t ref;
  GET_TOKEN(t, tokens, token_kind_star);
  if (false == expr_parse_ref(arena, tokens, &ref)) return false;
  t = expr_peek(*tokens);
  if (t.kind == token_kind_star) {
    uint32_t ref2;
    if (false == expr_parse_mul(arena, tokens, &ref2)) return false;
    PUSH_EXPR(*out, *arena, br_dagen_expr_operand(br_dagen_expr_kind_mul, ref, ref2));
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
      PUSH_EXPR(*out, *arena, br_dagen_expr_operand(br_dagen_expr_kind_add, *out, ref));
    } break;
    case token_kind_star: {
      if (false == expr_parse_mul(arena, tokens, &ref)) return false;
      PUSH_EXPR(*out, *arena, br_dagen_expr_operand(br_dagen_expr_kind_mul, *out, ref));
      goto start;
    } break;
    default: break;
  }
  return true;
}

TEST_ONLY static void expr_to_str(br_str_t* out, br_dagen_exprs_t* arena, uint32_t index) {
  br_dagen_expr_t t = arena->arr[index];
  switch (t.kind) {
    case br_dagen_expr_kind_reference_x: br_str_push_literal(out, "#"); br_str_push_int(out, t.group_id); br_str_push_literal(out, ".x"); return;
    case br_dagen_expr_kind_reference_y: br_str_push_literal(out, "#"); br_str_push_int(out, t.group_id); br_str_push_literal(out, ".y"); return;
    case br_dagen_expr_kind_reference_z: br_str_push_literal(out, "#"); br_str_push_int(out, t.group_id); br_str_push_literal(out, ".z"); return;
    case br_dagen_expr_kind_add: br_str_push_literal(out, "(");
                                 expr_to_str(out, arena, t.operands.op1);
                                 br_str_push_literal(out, " + ");
                                 expr_to_str(out, arena, t.operands.op2);
                                 br_str_push_literal(out, ")");
                                 return;
    case br_dagen_expr_kind_mul: br_str_push_literal(out, "(");
                                 expr_to_str(out, arena, t.operands.op1);
                                 br_str_push_literal(out, " * ");
                                 expr_to_str(out, arena, t.operands.op2);
                                 br_str_push_literal(out, ")");
                                 return;
    case br_dagen_expr_kind_pair:expr_to_str(out, arena, t.operands.op1);
                                 br_str_push_literal(out, ", ");
                                 expr_to_str(out, arena, t.operands.op2);
                                 return;
    case br_dagen_expr_kind_iota: br_str_push_literal(out, "0..4.8e9");
                                  return;
    case br_dagen_expr_kind_constant: br_str_push_float(out, t.value);
                                      return;
    case br_dagen_expr_kind_function_call: br_str_push_strv(out, t.function.func_name);
                                           br_str_push_char(out, '(');
                                           expr_to_str(out, arena, t.function.arg);
                                           br_str_push_char(out, ')');
                                           return;
  }
  BR_UNREACHABLE("Unknown kind: %d", t.kind);
}


#if defined(BR_UNIT_TEST)

#include "external/tests.h"

void br_expr_debug(br_dagens_t dagens) {
  br_str_t dbg = { 0 };
  expr_to_str(&dbg, &dagens.arr[0].expr_2d.arena, dagens.arr[0].expr_2d.x_expr_index);
  LOGI("xid = %d, yid = %d", dagens.arr[0].expr_2d.x_expr_index, dagens.arr[0].expr_2d.y_expr_index);
  br_str_push_char(&dbg, '|');
  expr_to_str(&dbg, &dagens.arr[0].expr_2d.arena, dagens.arr[0].expr_2d.y_expr_index);
  br_str_free(dbg);
}

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
  br_dagen_token_kind tokens[] = {__VA_ARGS__}; \
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
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF(val.y, 10.f);
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
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF(val.y, 20.f);
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
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF(val.y, 30.f);
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
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF(val.y, 40.f);
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
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF(val.y, 60.f);
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
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF(val.y, 150.f);
  FREE
}

TEST_CASE(dagen_parser_add_sqr_zyx) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.z * #1.z + #1.y * #1.y + #1.x * #1.x");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));
  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  br_expr_debug(dagens);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF((float)val.y, 10.f * 10.f + 20.f * 20.f + 30.f * 30.f);
  FREE
}

TEST_CASE(dagen_parser_basic_xy) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.x, #1.y");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));

  br_expr_debug(dagens);

  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF((float)val.x, 10.f);
  TEST_EQUALF((float)val.y, 20.f);
  FREE
}

TEST_CASE(dagen_parser_mul_const) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("#1.x * 10.4");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));

  br_expr_debug(dagens);

  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF((float)val.y, 10.f * 10.4f);
  FREE
}

TEST_CASE(dagen_parser_add_mul_const) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("1.01 + 1.00 * 2 * 3 + #1.x * 2 * 2");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));

  br_expr_debug(dagens);

  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF((float)val.y, 1.01f + 46.f);
  FREE
}

TEST_CASE(dagen_parser_sin) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("cos(#1.x + sin(#1.y)) + sin(#1.y) * sin(#1.z)");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));

  br_expr_debug(dagens);

  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 1);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF((float)val.y, cosf(10.f + sinf(20.f)) + sinf(20.f) * sinf(30.f));
  FREE
}

TEST_CASE(dagen_parser_fft) {
  INIT
  br_data_push_xyz(&datas, 10.f, 20.f, 30.f, 1);
  br_data_push_xyz(&datas, 20.f, 20.f, 30.f, 1);
  br_data_push_xyz(&datas, 30.f, 20.f, 30.f, 1);
  br_strv_t s = br_strv_from_literal("fft(#1.x)");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));

  br_expr_debug(dagens);

  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->len, 3);
  TEST_EQUAL(res->kind, br_data_kind_2d);
  br_vec2d_t val = br_data_el_xy(datas, 2, 0);
  TEST_EQUALF((float)val.y, 60.f);
  FREE
}

#endif
