#include "src/br_filesystem.h"
#include "src/br_str.h"
#include "src/br_pp.h"

#include <windows.h>

#define IS_SEP(X) (((X) == '/') || ((X) == '\\'))

// TODO: This will brake for some cases when path contains C: os something like that.... But fuck it for now...
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
  while (cwd->len > 1 && IS_SEP(cwd->str[cwd->len - 1])) --cwd->len;
  if (cwd->len >= 2 &&
      cwd->str[cwd->len - 1] == '.' &&
      cwd->str[cwd->len - 2] == '.') return br_str_push_c_str(cwd, "/..");
  else if (cwd->len >= 2 &&
      cwd->str[cwd->len - 1] == '.' &&
      cwd->str[cwd->len - 2] == '/') {
    cwd->len -= 2;
    goto start;
  }
  while (cwd->len != 0 && !IS_SEP(cwd->str[cwd->len - 1])) --cwd->len;
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
  if (IS_SEP(path.str[0])) {
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

bool br_fs_mkdir(br_strv_t path) {
  if (br_fs_exists(path)) return true;
  char* buff = br_strv_to_scrach(path);
  bool ok = CreateDirectory(buff, NULL);
  br_scrach_free();
  return ok;
}

bool br_fs_exists(br_strv_t path) {
  char* buff = br_strv_to_scrach(path);
  DWORD dwAttrib = GetFileAttributesA(buff);
  bool exists = dwAttrib != INVALID_FILE_ATTRIBUTES;
  br_scrach_free();
  return exists;
}

bool br_fs_list_dir(br_strv_t path, br_fs_files_t* out_files) {
  (void)path; (void)out_files;
  LOGI("List dirs not implemented on windows");
  return false;
}
