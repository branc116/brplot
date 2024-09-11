#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  FILE* file_out = stdout;
  FILE* in = NULL;
  if (argc == 2) {
  } else if (argc == 3) {
    file_out = fopen(argv[2], "wb");
    if (file_out == NULL) {
      fprintf(stderr, "Failed to open a file %s: %s\n", argv[2], strerror(errno));
      return 1;
    }
  } else {
    fprintf(stderr, "USAGE: %s <path-to-font> [<path-out>]\n", argv[0]);
    return 1;
  }
  FILE* file = fopen(argv[1], "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open a file %s: %s\n", argv[1], strerror(errno));
    return 1;
  }
  if (fseek(file, 0, SEEK_END) < 0) {
    fprintf(stderr, "Failed to seek end: %s\n", strerror(errno));
    return 1;
  }
  long long size = ftell(file);
  if (size <= 0) {
    fprintf(stderr, "Failed to tell file: %s\n", strerror(errno));
    return 1;
  }
  unsigned char* c = malloc(size);
  if (-1 == fseek(file, 0, SEEK_SET)) {
    fprintf(stderr, "Failed to seek 0: %s\n", strerror(errno));
    return 1;
  }
  if (1 != fread(c, size, 1, file)) {
    fprintf(stderr, "Failed to read a file %s: %s\n", argv[1], strerror(errno));
    return 1;
  }
  fprintf(file_out, "const unsigned char br_font_data[] = {");
  for (long long i = 0; i < size; ++i) {
    if (i % 30 == 0) fprintf(file_out, "\n  ");
    fprintf(file_out, "0x%x, ", (unsigned int)c[i]);
  }
  fprintf(file_out, "\n};\n");
  fprintf(file_out, "const long long br_font_data_size = %lld;\n", size);
  return 0;
}
