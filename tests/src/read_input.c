#include "src/br_pp.h"
#include "src/br_test.h"
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"
#define BR_STR_IMPLMENTATION
#include "src/br_str.h"
#include "src/read_input.c"
#include "src/q.c"

#if defined(FUZZ)
#  include "src/br_data_generator.h"
#  include "src/br_plotter.h"
#  include "src/br_gui.h"
#  include "src/br_icons.h"
#  include "src/br_tl.h"

int LLVMFuzzerTestOneInput(const char *str, size_t str_len) {
  lex_state_t s = { 0 };
  lex_state_init(&s);
  br_plotter_t* br = br_plotter_malloc();
  br_plotter_init(br);
  for (size_t i = 0; i < str_len;) {
    if (s.read_next) {
      s.c = str[i++];
      lex_step_extractor(br, &s);
    } else s.read_next = true;
    lex_step(br, &s);
    br_plotter_draw(br);
    br_dagens_handle(&br->groups, &br->dagens, &br->plots, brtl_time() + 0.010);
  }
  s.c = 0;
  while (s.tokens_len > 0) {
    lex_step(br, &s);
    input_tokens_reduce(br, &s, true);
    br_plotter_draw(br);
    br_dagens_handle(&br->groups, &br->dagens, &br->plots, brtl_time() + 0.010);
  }
  br_plotter_deinit(br);
  br_plotter_free(br);
  BR_FREE(br);
  printf("DONE\n");
  return 0;  // Values other than 0 and -1 are reserved for future use.
}
#endif

#define TEST_COMMAND_SET_ZOOM(q, AXIS, VALUE) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_set_zoom_ ## AXIS); \
  TEST_EQUALF(c.value, VALUE); \
} while(false)

#define TEST_COMMAND_PUSH_POINT_Y(q, Y, GROUP) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_push_point_y); \
  TEST_EQUALF(c.push_point_y.y, Y); \
  TEST_EQUAL(c.push_point_y.group, GROUP); \
} while(false)

#define TEST_COMMAND_PUSH_POINT_XY(q, X, Y, GROUP) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_push_point_xy); \
  TEST_EQUALF(c.push_point_xy.x, X); \
  TEST_EQUALF(c.push_point_xy.y, Y); \
  TEST_EQUAL(c.push_point_xy.group, GROUP); \
} while(false)

#define TEST_COMMAND_PUSH_POINT_XYZ(q, X, Y, Z, GROUP) do { \
  q_command c = q_pop(q); \
  TEST_EQUAL(c.type, q_command_push_point_xyz); \
  TEST_EQUALF(c.push_point_xyz.x, X); \
  TEST_EQUALF(c.push_point_xyz.y, Y); \
  TEST_EQUALF(c.push_point_xyz.z, Z); \
  TEST_EQUAL(c.push_point_xyz.group, GROUP); \
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

static void test_input(br_plotter_t* br, const char* str) {
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

void InputTestsExp(void) {
  br_plotter_t br;
  test_input(&br, "10e10");
  TEST_COMMAND_PUSH_POINT_Y(br.commands, 10e10f, 0);
  TEST_COMMAND_END(br.commands);
}

void InputTestsNeg() {
  br_plotter_t br;
  test_input(&br, "-2,-2.0,-2.");
  TEST_COMMAND_PUSH_POINT_XYZ(br.commands, -2.f, -2.f, -2.f, 0);
  TEST_COMMAND_END(br.commands);
}

void InputTests() {
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

void InputTests2() {
  br_plotter_t br;
  test_input(&br, "--setname 1 \"hihi\" --setname 2 \"hihi hihi hihi hihi\" --setname 0 \"test");

  TEST_COMMAND_SET_NAME(br.commands, 1, "hihi");
  TEST_COMMAND_SET_NAME(br.commands, 2,  "hihi hihi hihi hihi");
  TEST_COMMAND_SET_NAME(br.commands, 0,  "test");
  TEST_COMMAND_END(br.commands);
}

void ExtractorsIsValid() {
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

void Extractors(void) {
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

int main(void) {
  InputTestsExp();
  InputTestsNeg();
  InputTests();
  InputTests2();
  Extractors();
}

void br_on_fatal_error(void) {}

void br_data_push_x(br_datas_t* pg, double x, int group) { (void)pg; (void)x; (void)group; BR_TODO("br_data_push_x"); }
void br_data_push_y(br_datas_t* pg, double x, int group) { (void)pg; (void)x; (void)group; BR_TODO("br_data_push_y"); }
void br_data_push_xy(br_datas_t* pg, double x, double y, int group) { (void)pg; (void)x; (void)y; (void)group; BR_TODO("br_data_push_xy"); }
void br_data_push_xyz(br_datas_t* pg, double x, double y, double z, int group) { (void)pg; (void)x; (void)y; (void)z; (void)group; BR_TODO("br_data_push_xyz"); }
br_data_t* br_data_get1(br_datas_t pg, int group) { (void)pg; (void)group; BR_TODO("br_data_get1"); }
void br_data_empty(br_data_t* pg) { (void)pg; BR_TODO("br_data_empty"); }
void br_plotter_data_remove(br_plotter_t* br, int group_id) { (void)br; (void)group_id; BR_TODO("br_plotter_data_remove"); }
void br_plotter_datas_deinit(br_plotter_t* br) { (void)br; BR_TODO("br_plotter_datas_deinit"); }
void br_plot_screenshot(br_text_renderer_t* tr, br_plot_t* br, br_shaders_t* shaders, br_datas_t groups, char const* path) {
  (void)tr; (void)br; (void)shaders; (void)groups; (void)path;
  BR_TODO("br_plot_screenshot"); }
void br_plotter_export(br_plotter_t const* br, char const* path) { (void)br; (void)path; BR_TODO("br_plotter_export"); }
void br_plotter_export_csv(br_plotter_t const* br, char const* path) { (void)br; (void)path; BR_TODO("br_plotter_export_csv"); }
void br_data_set_name(br_datas_t* pg_array, int group, br_str_t name) { (void)pg_array; (void)group; (void)name; BR_TODO("br_data_set_name"); }
bool br_dagens_add_expr_str(br_dagens_t* dagens, br_datas_t* datas, br_strv_t str, int group_id) { (void)dagens; (void)datas; (void)str; (void)group_id; BR_TODO("br_dagens_add_expr_str"); return true; }
void br_plots_focus_visible(br_plots_t plot, br_datas_t groups) { (void)plot; (void)groups; BR_TODO("br_plots_focus_visible"); }
br_data_t* br_datas_create(br_datas_t* datas, int group_id, br_data_kind_t kind) { (void)datas; (void)group_id; (void)kind; BR_TODO("br_datas_create"); return NULL; }


