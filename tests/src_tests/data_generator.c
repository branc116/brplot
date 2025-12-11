#include "src/br_pp.h"
#include "src/br_test.h"
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"

#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"

#define BR_STR_IMPLEMENTATION
#include "src/br_str.h"

#define BRSP_IMPLEMENTATION
#include "src/br_string_pool.h"

#include "src/plot.c"
#include "src/anim.c"

#include "src/data_generator.c"
#include "src/data.c"
#include "src/resampling.c"
#include "tests/src_tests/mock_platform.c"
#include "tests/src_tests/mock_mesh.c"
#include "tests/src_tests/mock_gl.c"

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
  brsp_t sp = {0}; \
  br_anims_t anims = {0}; \
  br_data_construct(&sp, &anims);

#define FREE \
  br_datas_deinit(&datas); \
  br_dagens_free(&dagens); \
  br_da_free(plots); \
  brsp_free(&sp);

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

void dagen_simple_reference_x(void) {
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

void dagen_simple_reference_y(void) {
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

void dagen_simple_reference_z(void) {
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

void dagen_buffer_overflow(void) {
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

void dagen_add(void) {
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

void dagen_mul(void) {
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

void dagen_mul_add(void) {
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

void dagen_add_mul(void) {
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

void dagen_tokenizer(void) {
  TEST_TOKENIZER_PASS("#1.x", token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident);
  TEST_TOKENIZER_PASS("# 1 . x    ", token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident);
  TEST_TOKENIZER_PASS("# 1 . x  + #2.y * 2222 ", token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident, token_kind_plus,
      token_kind_hash, token_kind_number, token_kind_dot, token_kind_ident, token_kind_star, token_kind_number);
}

void dagen_parser_ref_x(void) {
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

void dagen_parser_ref_y(void) {
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

void dagen_parser_ref_z(void) {
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

void dagen_parser_add_zx(void) {
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

void dagen_parser_add_zxy(void) {
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

void dagen_parser_add_z__mul_xx__y(void) {
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

void dagen_parser_add_sqr_zyx(void) {
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

void dagen_parser_basic_xy(void) {
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

void dagen_parser_mul_const(void) {
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

void dagen_parser_add_mul_const(void) {
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

void dagen_parser_sin(void) {
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

void dagen_parser_fft(void) {
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

void dagen_parser_range(void) {
  INIT
  br_strv_t s = br_strv_from_literal("range(10)");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));

  br_expr_debug(dagens);

  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->kind, br_data_kind_2d);
  for (br_u32 i = 0; i < 10; ++i) {
    br_vec2d_t val = br_data_el_xy(datas, 2, i);
    TEST_EQUALF((float)val.y, (float)i);
  }
  FREE
}

void dagen_parser_range2(void) {
  INIT
  br_strv_t s = br_strv_from_literal("range(10, 20)");
  TEST_TRUE(br_dagens_add_expr_str(&dagens, &datas, s, 2));

  br_expr_debug(dagens);

  br_dagens_handle_once(&datas, &dagens, &plots);
  br_data_t* res = br_data_get1(datas, 2);

  TEST_EQUAL(res->kind, br_data_kind_2d);
  for (int i = 0; i < 10; ++i) {
    br_vec2d_t val = br_data_el_xy(datas, 2, i);
    TEST_EQUALF((float)val.y, (float)(i + 10));
  }
  FREE
}

int main(void) {
  dagen_simple_reference_x();
  dagen_simple_reference_y();
  dagen_simple_reference_z();
  dagen_buffer_overflow();
  dagen_add();
  dagen_mul();
  dagen_mul_add();
  dagen_add_mul();
  dagen_tokenizer();
  dagen_parser_ref_x();
  dagen_parser_ref_y();
  dagen_parser_ref_z();
  dagen_parser_add_zx();
  dagen_parser_add_zxy();
  dagen_parser_add_z__mul_xx__y();
  dagen_parser_add_sqr_zyx();
  dagen_parser_basic_xy();
  dagen_parser_mul_const();
  dagen_parser_add_mul_const();
  dagen_parser_sin();
  dagen_parser_fft();
  dagen_parser_range();
  dagen_parser_range2();
}

void br_on_fatal_error(void) {}
void brgui_push_log_line(const char* fmt, ...) {(void)fmt;}
