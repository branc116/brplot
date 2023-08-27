#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"

void trim_zeros(void * data, int* data_size) {
  for (int i = *data_size - 1; i >= 0; --i) {
    if (((unsigned char*)data)[i] == 0) --(*data_size);
    else return;
  }
}

void export_image_as_code(Image image)
{
    int dataSize = GetPixelDataSize(image.width, image.height, image.format);

    // Add image information
    printf("// Image data information\n");
    printf("#define FONT_IMAGE_WIDTH    %i\n", image.width);
    printf("#define FONT_IMAGE_HEIGHT   %i\n", image.height);
    printf("#define FONT_IMAGE_FORMAT   %i\n\n", image.format);

    printf("static unsigned char FONT_IMAGE_DATA[%i] = { ", dataSize);
    for (int i = 0; i < dataSize - 1; ++i) printf(((i%20 == 0) ? "0x%x,\n" : "0x%x, "), ((unsigned char *)image.data)[i]);
    printf("0x%x };\n", ((unsigned char *)image.data)[dataSize - 1]);
}

void export_font_data_as_code(unsigned char* data, unsigned int len) {
  printf("//Exported font data\n");
  printf("static unsigned char const default_font_data[%u] = {\n", len);
  for (unsigned int i = 0; i < len - 1; ++i) printf(((i%20 == 0) ? "0x%x,\n" : "0x%x, "), data[i]);
  printf("0x%x };\n", data[len - 1]);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "ERROR: bad number of arguments.\nUSAGE: %s <name of font you want to export>\n", argv[0]); 
    exit(-1);
  }
  SetTraceLogLevel(40);
  unsigned int bytesRead = 0;
  unsigned char* data = LoadFileData(argv[1], &bytesRead);
  int sz = 64, gc = 95;
  printf("static int const default_font_sz = %d;\n", sz);
  printf("static int const default_font_gc = %d;\n", gc);
  export_font_data_as_code(data, bytesRead);
}
