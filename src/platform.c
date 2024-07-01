#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__) || defined (__APPLE__) ||  defined(_WIN32) || defined(__CYGWIN__)
#  include "desktop/platform.c"
#elif defined(__EMSCRIPTEN__)
#  include "web/platform.c"
#else
#  error "Unsupported Platform"
#endif

