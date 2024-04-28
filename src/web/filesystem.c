#include "src/br_filesystem.h"

bool br_fs_up_dir(br_str_t* cwd) {
  (void)cwd;
  return false;
}

bool br_fs_cd(br_str_t* cwd, br_strv_t path) {
  (void)cwd, (void)path;
  return false;
}
