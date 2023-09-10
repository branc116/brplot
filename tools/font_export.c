#include "raylib.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"

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
void export_font_as_code(unsigned char* data, unsigned int len) {
  int chars[] =  {
        (int)'#', (int)'.', (int)',', (int)'(', (int)')', (int)'1', (int)'2', (int)'3', (int)'4', (int)'5',
        (int)'6', (int)'7', (int)'8', (int)'9', (int)'0', (int)'A', (int)'B', (int)'C', (int)'D', (int)'E',
        (int)'F', (int)'G', (int)'H', (int)'I', (int)'J', (int)'K', (int)'L', (int)'M', (int)'N', (int)'O',
        (int)'P', (int)'Q', (int)'R', (int)'S', (int)'T', (int)'U', (int)'V', (int)'W', (int)'X', (int)'Y',
        (int)'Z', (int)'a', (int)'b', (int)'c', (int)'d', (int)'e', (int)'f', (int)'g', (int)'h', (int)'i',
        (int)'j', (int)'k', (int)'l', (int)'m', (int)'n', (int)'o', (int)'p', (int)'q', (int)'r', (int)'s',
        (int)'t', (int)'u', (int)'v', (int)'w', (int)'x', (int)'y', (int)'z', (int)'"', (int)'\'', (int)' ',
        (int)'-', (int)'/', (int)':', (int)';', (int)'+' };
  Font font = {0};
  font.baseSize = 46;
  font.glyphCount = sizeof(chars)/sizeof(int);
  font.glyphPadding = 4;
  GlyphInfo* glyphs = font.glyphs = LoadFontData(data, len, font.baseSize, chars, sizeof(chars)/sizeof(int) , FONT_DEFAULT);
  printf("#include \"raylib.h\"\n");
  printf("#include \"stdlib.h\"\n");
  unsigned int count = sizeof(chars)/sizeof(int);
  for (int i = 0; i < count; ++i) {
    int dataSize = glyphs[i].image.width * glyphs[i].image.height;
    printf("static unsigned char data_%d[] = {", glyphs[i].value);
    for (int j = 0; j < dataSize - 1; ++j) printf(((j%20 == 0) ? "0x%x,\n" : "0x%x, "), ((unsigned char *)glyphs[i].image.data)[j]);
    printf("0x%x };\n", ((unsigned char *)glyphs[i].image.data)[dataSize - 1]);
  }
  printf("static GlyphInfo glyphs[] = {\n");
  for (int i = 0; i < count; ++i) {
    if (glyphs[i].image.format != PIXELFORMAT_UNCOMPRESSED_GRAYSCALE) {
      fprintf(stderr, "Format of a font must be PIXELFORMAT_UNCOMPRESSED_GRAYSCALE\n");
      assert(false);
    }
    printf("{ .value = %d, .offsetX = %d, .offsetY = %d, .advanceX = %d, .image = {\n", glyphs[i].value, glyphs[i].offsetX, glyphs[i].offsetY, glyphs[i].advanceX);
    printf("  .data = (void *)data_%d, .width = %d, .height = %d, .mipmaps = %d, .format = %d }\n}", glyphs[i].value, glyphs[i].image.width, glyphs[i].image.height, glyphs[i].image.mipmaps, glyphs[i].image.format);
    printf(i < count - 1 ? ",\n" : "\n};\n");
  }
  printf("static Font default_font = { .baseSize = %d, .glyphCount = %d, .glyphPadding = %d, .recs = NULL, .glyphs = glyphs };",
      font.baseSize, font.glyphCount, font.glyphPadding);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "ERROR: bad number of arguments.\nUSAGE: %s <name of font you want to export>\n", argv[0]); 
    exit(-1);
  }
  SetTraceLogLevel(40);
  unsigned int bytesRead = 0;
  unsigned char* data = LoadFileData(argv[1], &bytesRead);
  export_font_as_code(data, bytesRead);
}
