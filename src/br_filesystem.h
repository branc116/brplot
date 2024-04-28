#pragma once
#include "br_str.h"

#ifdef __cplusplus
extern "C" {
#endif

bool br_fs_mkdir(br_strv_t path);
bool br_fs_exists(br_strv_t path);
uint32_t br_fs_crc(const void* data, size_t len);
bool br_fs_up_dir(br_str_t* cwd);
bool br_fs_cd(br_str_t* cwd, br_strv_t path);

#ifdef __cplusplus
}
#endif
