#define BR_FREAD test_read
#define BR_FWRITE test_write
#define BR_FOPEN test_open
#define BR_FCLOSE test_close
#define BR_FEOF test_feof
#define BR_FILE br_test_file_t

#include <stddef.h>
#include <string.h>
typedef struct {
  unsigned char* arr;
  int len, cap;
  int read_index;
} br_test_file_t;
size_t test_read(void* dest, size_t el_size, size_t n, br_test_file_t* d);
size_t test_write(const void* src, size_t el_size, size_t n, br_test_file_t* d);
BR_FILE* test_open(const char* path, const char* mode);
int test_close(BR_FILE* file);
int test_feof(BR_FILE* file);


#include "src/br_pp.h"
#include "src/br_da.h"
#include "src/br_ui.h"
#include "src/br_anim.h"
#include "src/br_text_renderer.h"
#include "src/br_platform.h"
#include "src/br_shaders.h"
#include "src/br_test.h"
#include "src/br_data_generator.h"
#include "src/br_plot.h"
#include "src/br_resampling.h"
#include "src/br_data.h"
#include "src/br_q.h"
#include "src/br_filesystem.h"
#include "src/br_permastate.h"
#include "src/br_plotter.h"
