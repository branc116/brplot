#include <stdio.h>
#include <string.h>
#include <errno.h>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image_write.h"

int main(int argc, char** argv) {
  char* in_name = NULL;
  char* out_name = NULL;

  if (argc == 3) {
    in_name = argv[1];
    out_name = argv[2];
    fprintf(stderr, "ERROR: expected %s <input-png> <output-png>\n", argv[0]);
  } else if (argc == 2) {
    in_name = argv[1];
    out_name = argv[1];
  } else {
    fprintf(stderr, "ERROR: expected %s <input-png> <output-png>\n"
                    "       or       %s <in-out-png>\n", argv[0], argv[0]);
    return 1;
  }

  FILE* in = fopen(in_name, "rb");
  if (in == 0) {
    fprintf(stderr, "ERROR Opening file %s: %s\n", in_name, strerror(errno));
    return 1;
  }
  int w, h, c, dc;
  stbi_uc* img = stbi_load_from_file(in, &w, &h, &c, 0);
  printf("w=%d, h=%d, c=%d\n", w, h, c);
  if (img == NULL) {
    fprintf(stderr, "ERROR while parsing file: %s\n", in_name);
    return 1;
  }

  unsigned char* out_data = calloc(w*h, 1);
  for (int i = 0; i < h; ++i) {
    for (int j = 0; j < w; ++j) {
      unsigned char d = out_data[j + i*w] = img[(c - 1) + j * c + i * w * c];
      printf("%s", d > 0 ? "##" : "..");
    }
    printf("\n");
  }
  printf("\n");
  stbi_write_png(out_name, w, h, 1, out_data, w);
  return 0;
}
// gcc -I. -o bin/extract_alpha tools/extract_alpha.c -lm
