#pragma once
#include "src/br_str.h"

#include <stdint.h>

typedef enum br_fs_file_kind_t {
  br_fs_file_kind_unknown,
  br_fs_file_kind_file,
  br_fs_file_kind_dir,
} br_fs_file_kind_t;

typedef struct br_fs_file_t {
  br_str_t name;
  br_fs_file_kind_t kind;
} br_fs_file_t;

typedef struct br_fs_files_t {
  br_fs_file_t* arr;
  size_t len, cap;

  size_t real_len;
  br_str_t cur_dir;
  br_str_t last_good_dir;
  br_str_t tmp_path;
} br_fs_files_t;

#define br_fs_read(PATH, OUT_CONTENT) br_fs_read_internal(PATH, OUT_CONTENT, __FILE__, __LINE__)

bool br_fs_mkdir(br_strv_t path);
bool br_fs_exists(br_strv_t path);
bool br_fs_get_config_dir(br_str_t* path);
uint32_t br_fs_crc(const void* data, size_t len, uint32_t seed);
bool br_fs_up_dir(br_str_t* cwd);
bool br_fs_cd(br_str_t* cwd, br_strv_t path);

bool br_fs_move(const char* from, const char* to);
bool br_fs_read_internal(const char* path, br_str_t* out_content, const char* file_name, int line);
br_str_t br_fs_read1(const char* path);
bool br_fs_list_dir(br_strv_t path, br_fs_files_t* out_files);

