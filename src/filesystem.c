#include "br_filesystem.h"

#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#  include "desktop/linux/filesystem.c"
#elif defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__EMSCRIPTEN__)
#  include "web/filesystem.c"
#else
#  error "Unsupported Platform"
#endif

static bool br_permastate_get_home_dir(br_str_t* home) {
#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
  return br_str_push_c_str(home, getenv("HOME"));
#elif defined(_WIN32) || defined(__CYGWIN__)
  return br_str_push_c_str(home, getenv("LOCALAPPDATA"));
#else
  return false;
#endif
}

bool br_fs_get_config_dir(br_str_t* path) {
  if (false == br_permastate_get_home_dir(path)) return false;;
#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
  return br_fs_cd(path, br_strv_from_literal(".config/brplot"));
#elif defined(_WIN32) || defined(__CYGWIN__)
  return br_fs_cd(config, br_strv_from_literal("brplot"));
#else
  return false;
#endif
}

