#if !defined(_MSC_VER)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wreturn-type"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#defien PGL_ASSERT BR_ASSERT
#include "external/portablegl.h"

#define WIDTH 800
#define HEIGHT 600
glContext br_c;
void br_gl_load(void) {
  u32* buff = NULL;
  init_glContext(&br_c, &buff, WIDTH, HEIGHT, 8, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  set_glContext(&br_c);
}

#if !defined(_MSC_VER)
#  pragma GCC diagnostic pop
#endif
