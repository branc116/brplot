#include "src/br_str.h"
#include "src/br_filesystem.h"
#include "src/br_da.h"

#include "external/tests.h"

#include <errno.h>
#include <dirent.h>

bool br_fs_up_dir(br_str_t* cwd) {
start:
  if (cwd->len == 0) return br_str_push_c_str(cwd, "..");
  if (cwd->len == 1) {
    if (cwd->str[0] == '.') return br_str_push_c_str(cwd, ".");
    else {
      cwd->len = 0;
      return true;
    }
  }
  while (cwd->len > 1 && cwd->str[cwd->len - 1] == '/') --cwd->len;
  if (cwd->len >= 2 &&
      cwd->str[cwd->len - 1] == '.' &&
      cwd->str[cwd->len - 2] == '.') return br_str_push_c_str(cwd, "/..");
  else if (cwd->len >= 2 &&
      cwd->str[cwd->len - 1] == '.' &&
      cwd->str[cwd->len - 2] == '/') {
    cwd->len -= 2;
    goto start;
  }
  while (cwd->len != 0 && cwd->str[cwd->len - 1] != '/') --cwd->len;
  if (cwd->len == 0) {
    return true;
  }
  if (cwd->len == 1) {
    if (cwd->str[0] == '.') return br_str_push_c_str(cwd, ".");
    else {
      cwd->len = 0;
      return true;
    }
  }
  --cwd->len;
  return true;
}

bool br_fs_cd(br_str_t* cwd, br_strv_t path) {
  if (path.str[0] == '/') {
    cwd->len = 0;
    br_str_push_strv(cwd, path);
    return true;
  }

  while (path.len != 0) {
    br_strv_t next_name = { path.str, 0 };
    for (; next_name.len < path.len && path.str[next_name.len] != '/'; ++next_name.len);
    if (next_name.len == 0) /* DO NOTHING */;
    else if (br_strv_eq(next_name, br_strv_from_c_str(".."))) br_fs_up_dir(cwd);
    else if (br_strv_eq(next_name, br_strv_from_c_str("."))) /* DO NOTHING */;
    else if (cwd->len == 0) br_str_push_strv(cwd, next_name);
    else if (cwd->str[cwd->len - 1] == '/') br_str_push_strv(cwd, next_name);
    else {
      br_str_push_char(cwd, '/');
      br_str_push_strv(cwd, next_name);
    }
    if (next_name.len + 1 <= path.len) {
      path.str += next_name.len + 1;
      path.len -= next_name.len + 1;
    } else return true;
  }
  return true;
}

static int br_fs_files_sort(void const* a, void const* b) {
  br_fs_file_t const * af = a, *bf = b;
  if (af->kind > bf->kind) return -1;
  if (af->kind < bf->kind) return 1;
  return strncmp(af->name.str, bf->name.str, af->name.len < bf->name.len ? af->name.len : bf->name.len);
}


#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
bool br_fs_list_dir(br_strv_t path, br_fs_files_t* out_files) {
  DIR* dir = NULL;
  bool success = true;
  struct dirent *de = NULL;
  size_t i = 0;
  br_fs_file_t* s = NULL;

  out_files->cur_dir.len = 0;
  br_str_push_strv(&out_files->cur_dir, path);
  br_str_push_zero(&out_files->cur_dir);
  if (NULL == (dir = opendir(out_files->cur_dir.str))) goto error;
  br_str_copy2(&out_files->last_good_dir, out_files->cur_dir);
  while (NULL != (de = readdir(dir))) {
    if (strcmp(".", de->d_name) == 0) continue;
    if (strcmp("..", de->d_name) == 0) continue;
    if (out_files->len <= i) br_da_push(*out_files, ((br_fs_file_t) {0}));
    s = br_da_getp(*out_files, i);
    s->name.len = 0;
    br_str_push_c_str(&s->name, de->d_name);
    switch (de->d_type) {
      case DT_DIR: s->kind = br_fs_file_kind_dir; break;
      case DT_REG: s->kind = br_fs_file_kind_file; break;
      default: s->kind = br_fs_file_kind_unknown; break;
    };
    ++i;
  }
  out_files->real_len = i;
  qsort(out_files->arr, out_files->real_len, sizeof(out_files->arr[0]), br_fs_files_sort);
  goto done;

error:
  success = false;

done:
  if (NULL != dir) closedir(dir);
  return success;
}
#else
bool br_fs_list_dir(br_strv_t path, br_fs_files_t* out_files) {
  (void)path; (void)out_files;
  (void)br_fs_files_sort;
  LOGI("List dirs not implemented on windows");
  return false;
}
#endif

#if defined(BR_UNIT_TEST)
TEST_CASE(paths) {
  char c[128];
  br_str_t br = br_str_malloc(2);
  br_fs_cd(&br, br_strv_from_c_str("foo")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo");
  br_fs_cd(&br, br_strv_from_c_str("bar")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo/bar");
  br_fs_cd(&br, br_strv_from_c_str("../baz")); br_str_to_c_str1(br, c);
  TEST_STREQUAL(c, "foo/baz");
  br_fs_cd(&br, br_strv_from_c_str("../../baz")); br_str_to_c_str1(br, c);
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
#endif
