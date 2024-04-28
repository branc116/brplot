#include "src/br_filesystem.h"
#include "src/br_str.h"
#include "src/br_pp.h"

#include <filesystem>

bool br_fs_up_dir(br_str_t* cwd) {
  const char* parent = NULL;
  BR_ASSERT(cwd->len < 512);
  char buff[512];
  br_str_to_c_str1(*cwd, buff);
  std::filesystem::path path(buff);
  cwd->len = 0;
  parent = path.parent_path().string().c_str(), br_str_push_c_str(cwd, parent);
  return true;
}

bool br_fs_cd(br_str_t* cwd, br_strv_t path) {
  BR_ASSERT(cwd->len < 512);
  const char* parent = NULL;
  char buff[512];
  br_str_to_c_str1(*cwd, buff);
  std::filesystem::path p(buff);
  char buff2[512];
  br_strv_to_c_str1(path, buff2);
  std::filesystem::path p2(buff2);
  cwd->len = 0;
  parent = (p / p2).string().c_str(), br_str_push_c_str(cwd, parent);
  return true;
}
