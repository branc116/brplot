#include "br_filesystem.h"
#include "br_str.h"
#include "br_pp.h"

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#elif defined(_WIN32) || defined(__CYGWIN__)
#  include "desktop/win/filesystem.cpp"
#  include <filesystem>

bool br_fs_mkdir(br_strv_t path) {
  BR_ASSERT(path.len < 512);
  char buff[512];
  br_strv_to_c_str1(path, buff);
  if (false == std::filesystem::exists(buff)) return std::filesystem::create_directory(buff);
  return std::filesystem::is_directory(buff);
}

bool br_fs_exists(br_strv_t path) {
  BR_ASSERT(path.len < 512);
  char buff[512];
  br_strv_to_c_str1(path, buff);
  return std::filesystem::exists(buff);
}

#elif defined(__EMSCRIPTEN__)
#else
#  error "Unsupported Platform"
#endif

#ifdef IMGUI
#include "imgui_internal.h"
uint32_t br_fs_crc(const void* data, size_t len, uint32_t seed) {
  return ImHashData(data, len, seed);
}
#endif
