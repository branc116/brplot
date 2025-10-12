#include "src/br_gl.h"


br_extent_t brui_resizable_cur_extent(int handle) { (void)handle; return BR_EXTENT(0,0,0,0); };
void brui_resizable_delete(int handle) { (void)handle; }
void brgl_destroy_framebuffer(GLuint br_id) { (void)br_id; }
GLuint brgl_create_framebuffer(int width, int height) { (void)width; (void)height; return 0; }
