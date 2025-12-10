#include "include/brui.h"
#if !defined(BR_INCLUDE_UNITY_BRPLAT_C)
#  define BR_INCLUDE_UNITY_BRPLAT_C
#  include "tools/unity/brplat.c"
#endif
#if !defined(BR_INCLUDE_SHL_IMPLS_C)
#  define BR_INCLUDE_SHL_IMPLS_C
#  include "external/shl_impls.c"
#endif
#if !defined(BR_INCLUDE_THREADS_C)
#  define BR_INCLUDE_THREADS_C
#  include "src/threads.c"
#endif
#if !defined(BR_INCLUDE_FILESYSTEM_C)
#  define BR_INCLUDE_FILESYSTEM_C
#  include "src/filesystem.c"
#endif
#if !defined(BR_INCLUDE_SHADERS_C)
#  define BR_INCLUDE_SHADERS_C
#  include "src/shaders.c"
#endif
#if !defined(BR_INCLUDE_ICONS_C)
#  define BR_INCLUDE_ICONS_C
#  include "src/icons.c"
#endif
#if !defined(BR_INCLUDE_GL_C)
#  define BR_INCLUDE_GL_C
#  include "src/gl.c"
#endif
#if !defined(BR_INCLUDE_TEXT_RENDERER_C)
#  define BR_INCLUDE_TEXT_RENDERER_C
#  include "src/text_renderer.c"
#endif
#if !defined(BR_INCLUDE_THEME_C)
#  define BR_INCLUDE_THEME_C
#  include "src/theme.c"
#endif
#if !defined(BR_INCLUDE_UI_C)
#  define BR_INCLUDE_UI_C
#  include "src/ui.c"
#endif
#if !defined(BR_INCLUDE_ANIM_C)
#  define BR_INCLUDE_ANIM_C
#  include "src/anim.c"
#endif
