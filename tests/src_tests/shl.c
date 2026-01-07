#include "tests/src_tests/shl.h"
#include "external/shl_impls.c"
#include "tests/src_tests/mock_gl.c"
#include "src/filesystem.c"
#include "src/ui.c"
#include "src/anim.c"
#include "src/text_renderer.c"
#include "src/platform2.c"
#include "src/shaders.c"
#include "src/theme.c"
#include "src/data_generator.c"
#include "src/data.c"
#include "src/resampling.c"
#include "src/mesh.c"
#include "src/plot.c"
#include "src/q.c"
#include "src/plotter.c"
#include "src/gui.c"
#include "src/permastate.c"
#include "src/read_input.c"
#include "src/threads.c"

static unsigned char buffer[4096];
br_test_file_t test_file;

size_t test_read(void* dest, size_t el_size, size_t n, br_test_file_t* d) {
  size_t size = n * el_size;
  if ((int)size + d->read_index > d->len) {
    errno = 1;
    return 0;
  }
  memcpy(dest, d->arr + d->read_index, size);
  d->read_index += (int)size;
  BR_ASSERT(d->read_index <= d->len);
  return n;
}

size_t test_write(const void* src, size_t el_size, size_t n, br_test_file_t* d) {
  size_t size = n * el_size;
  if ((size_t)d->cap < (size_t)d->len + size) {
    errno = 2;
    return 0;
  }
  memcpy(d->arr + d->len, src, size);
  d->len += (int)size;
  BR_ASSERT(d->len <= d->cap);
  return n;
}

BR_FILE* test_open(const char* path, const char* mode) {
  test_file.arr = buffer;
  test_file.cap = sizeof(buffer);
  test_file.read_index = 0;
  test_file.len = 0;
  return &test_file;
}

int test_close(BR_FILE* file) {
  file->len = 0;
  file->read_index = 0;
  return 0;
}

int test_feof(BR_FILE* file) {
  return file->read_index == file->len;
}
