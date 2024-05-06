
#if defined (__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__)
#  include "desktop/linux/filesystem.c"
#elif defined(_WIN32) || defined(__CYGWIN__)
#elif defined(__EMSCRIPTEN__)
#  include "web/filesystem.c"
#else
#  error "Unsupported Platform"
#endif
