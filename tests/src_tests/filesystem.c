#include "tests/src_tests/shl.h"

int main(void) {
  char c[128];
  br_str_t br = br_str_malloc(2);
  br_fs_cd(&br, br_strv_from_c_str("foo")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo");
  br_fs_cd(&br, br_strv_from_c_str("bar")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo/bar");
  br_fs_cd(&br, br_strv_from_c_str("../baz")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo/baz");
  br_fs_cd(&br, br_strv_from_c_str("../baz/bar")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo/baz/bar");
  br_fs_cd(&br, br_strv_from_c_str("../../../baz")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "baz");
  br_fs_cd(&br, br_strv_from_c_str("../")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "");
  br_fs_cd(&br, br_strv_from_c_str(".///")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "");
  br_fs_cd(&br, br_strv_from_c_str("./aa//")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "aa");
  br_fs_cd(&br, br_strv_from_c_str("./a//")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "aa/a");
  br_fs_cd(&br, br_strv_from_c_str("../../../")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "..");
  br_fs_cd(&br, br_strv_from_c_str("./..")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "../..");

  br.len = 0;
  br_str_push_c_str(&br, "./././././");
  br_fs_cd(&br, br_strv_from_c_str("./..")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "..");

  br.len = 0;
  br_str_push_c_str(&br, "../../././");
  br_fs_cd(&br, br_strv_from_c_str("./..")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "../../..");
  br_str_free(br);
}
