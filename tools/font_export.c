#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include "stdbool.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../imgui/imstb_truetype.h"

// Texture pixel formats
// NOTE: Support depends on OpenGL version
typedef enum {
    RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE = 1,     // 8 bit per pixel (no alpha)
    RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA,        // 8*2 bpp (2 channels)
    RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5,            // 16 bpp
    RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8,            // 24 bpp
    RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,          // 16 bpp (1 bit alpha)
    RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4,          // 16 bpp (4 bit alpha)
    RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,          // 32 bpp
    RL_PIXELFORMAT_UNCOMPRESSED_R32,               // 32 bpp (1 channel - float)
    RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32,         // 32*3 bpp (3 channels - float)
    RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32,      // 32*4 bpp (4 channels - float)
    RL_PIXELFORMAT_UNCOMPRESSED_R16,               // 16 bpp (1 channel - half float)
    RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16,         // 16*3 bpp (3 channels - half float)
    RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16,      // 16*4 bpp (4 channels - half float)
    RL_PIXELFORMAT_COMPRESSED_DXT1_RGB,            // 4 bpp (no alpha)
    RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA,           // 4 bpp (1 bit alpha)
    RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA,           // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA,           // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_ETC1_RGB,            // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_ETC2_RGB,            // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA,       // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_PVRT_RGB,            // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA,           // 4 bpp
    RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA,       // 8 bpp
    RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA        // 2 bpp
} rlPixelFormat;

// Image, pixel data stored in CPU memory (RAM)
typedef struct Image {
    void *data;             // Image raw data
    int width;              // Image base width
    int height;             // Image base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} Image;

// Get pixel data size in bytes for certain format
// NOTE: Size can be requested for Image or Texture data
int GetPixelDataSize(int width, int height, int format)
{
    int dataSize = 0;       // Size in bytes
    int bpp = 0;            // Bits per pixel

    switch (format)
    {
        case RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: bpp = 8; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA:
        case RL_PIXELFORMAT_UNCOMPRESSED_R5G6B5:
        case RL_PIXELFORMAT_UNCOMPRESSED_R5G5B5A1:
        case RL_PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: bpp = 16; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: bpp = 32; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8: bpp = 24; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R32: bpp = 32; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32: bpp = 32*3; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: bpp = 32*4; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R16: bpp = 16; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16: bpp = 16*3; break;
        case RL_PIXELFORMAT_UNCOMPRESSED_R16G16B16A16: bpp = 16*4; break;
        case RL_PIXELFORMAT_COMPRESSED_DXT1_RGB:
        case RL_PIXELFORMAT_COMPRESSED_DXT1_RGBA:
        case RL_PIXELFORMAT_COMPRESSED_ETC1_RGB:
        case RL_PIXELFORMAT_COMPRESSED_ETC2_RGB:
        case RL_PIXELFORMAT_COMPRESSED_PVRT_RGB:
        case RL_PIXELFORMAT_COMPRESSED_PVRT_RGBA: bpp = 4; break;
        case RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA:
        case RL_PIXELFORMAT_COMPRESSED_DXT5_RGBA:
        case RL_PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA:
        case RL_PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA: bpp = 8; break;
        case RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA: bpp = 2; break;
        default: break;
    }

    dataSize = width*height*bpp/8;  // Total data size in bytes

    // Most compressed formats works on 4x4 blocks,
    // if texture is smaller, minimum dataSize is 8 or 16
    if ((width < 4) && (height < 4))
    {
        if ((format >= RL_PIXELFORMAT_COMPRESSED_DXT1_RGB) && (format < RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA)) dataSize = 8;
        else if ((format >= RL_PIXELFORMAT_COMPRESSED_DXT3_RGBA) && (format < RL_PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA)) dataSize = 16;
    }

    return dataSize;
}

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

// GlyphInfo, font characters glyphs info
typedef struct GlyphInfo {
    int value;              // Character value (Unicode)
    int offsetX;            // Character offset X when drawing
    int offsetY;            // Character offset Y when drawing
    int advanceX;           // Character advance position X
    Image image;            // Character image data
} GlyphInfo;

// Texture, tex data stored in GPU memory (VRAM)
typedef struct Texture {
    unsigned int id;        // OpenGL texture id
    int width;              // Texture base width
    int height;             // Texture base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} Texture;

// Texture2D, same as Texture
typedef Texture Texture2D;

// Rectangle, 4 components
typedef struct Rectangle {
    float x;                // Rectangle top-left corner position x
    float y;                // Rectangle top-left corner position y
    float width;            // Rectangle width
    float height;           // Rectangle height
} Rectangle;

// Font, font texture and GlyphInfo array data
typedef struct Font {
    int baseSize;           // Base size (default chars height)
    int glyphCount;         // Number of glyph characters
    int glyphPadding;       // Padding around the glyph characters
    Texture2D texture;      // Texture atlas containing the glyphs
    Rectangle *recs;        // Rectangles in texture for the glyphs
    GlyphInfo *glyphs;      // Glyphs info data
} Font;

#define SUPPORT_FILEFORMAT_TTF

// Load font data for further use
// NOTE: Requires TTF font memory data and can generate SDF data
GlyphInfo *LoadFontData(const unsigned char *fileData, int fontSize, int *codepoints, int codepointCount)
{
    // NOTE: Using some SDF generation default values,
    // trades off precision with ability to handle *smaller* sizes
#ifndef FONT_SDF_CHAR_PADDING
    #define FONT_SDF_CHAR_PADDING            4      // SDF font generation char padding
#endif
#ifndef FONT_SDF_ON_EDGE_VALUE
    #define FONT_SDF_ON_EDGE_VALUE         128      // SDF font generation on edge value
#endif
#ifndef FONT_SDF_PIXEL_DIST_SCALE
    #define FONT_SDF_PIXEL_DIST_SCALE     64.0f     // SDF font generation pixel distance scale
#endif
#ifndef FONT_BITMAP_ALPHA_THRESHOLD
    #define FONT_BITMAP_ALPHA_THRESHOLD     80      // Bitmap (B&W) font generation alpha threshold
#endif

    GlyphInfo *chars = NULL;

#if defined(SUPPORT_FILEFORMAT_TTF)
    // Load font data (including pixel data) from TTF memory file
    // NOTE: Loaded information should be enough to generate font image atlas, using any packaging method
    if (fileData != NULL)
    {
        bool genFontChars = false;
        stbtt_fontinfo fontInfo = { 0 };

        if (stbtt_InitFont(&fontInfo, (unsigned char *)fileData, 0))     // Initialize font for data reading
        {
            // Calculate font scale factor
            float scaleFactor = stbtt_ScaleForPixelHeight(&fontInfo, (float)fontSize);

            // Calculate font basic metrics
            // NOTE: ascent is equivalent to font baseline
            int ascent, descent, lineGap;
            stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

            // In case no chars count provided, default to 95
            codepointCount = (codepointCount > 0)? codepointCount : 95;

            // Fill fontChars in case not provided externally
            // NOTE: By default we fill glyphCount consecutively, starting at 32 (Space)

            if (codepoints == NULL)
            {
                codepoints = (int *)malloc(codepointCount*sizeof(int));
                if (codepoints == NULL) {
                  fprintf(stderr, "malloc failed");
                  exit(EXIT_FAILURE);
                }
                for (int i = 0; i < codepointCount; i++) codepoints[i] = i + 32;
                genFontChars = true;
            }

            chars = (GlyphInfo *)malloc(codepointCount*sizeof(GlyphInfo));
              if (chars == NULL) {
                fprintf(stderr, "chars null");
                exit(EXIT_FAILURE);
              }

            // NOTE: Using simple packaging, one char after another
            for (int i = 0; i < codepointCount; i++)
            {
                int chw = 0, chh = 0;   // Character width and height (on generation)
                int ch = codepoints[i];  // Character value to get info for
                chars[i].value = ch;

                //  Render a unicode codepoint to a bitmap
                //      stbtt_GetCodepointBitmap()           -- allocates and returns a bitmap
                //      stbtt_GetCodepointBitmapBox()        -- how big the bitmap must be
                //      stbtt_MakeCodepointBitmap()          -- renders into bitmap you provide

                chars[i].image.data = stbtt_GetCodepointBitmap(&fontInfo, scaleFactor, scaleFactor, ch, &chw, &chh, &chars[i].offsetX, &chars[i].offsetY);

                stbtt_GetCodepointHMetrics(&fontInfo, ch, &chars[i].advanceX, NULL);
                chars[i].advanceX = (int)((float)chars[i].advanceX*scaleFactor);

                // Load characters images
                chars[i].image.width = chw;
                chars[i].image.height = chh;
                chars[i].image.mipmaps = 1;
                chars[i].image.format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;

                chars[i].offsetY += (int)((float)ascent*scaleFactor);

                // NOTE: We create an empty image for space character, it could be further required for atlas packing
                if (ch == 32)
                {
                    Image imSpace = {
                        .data = calloc(chars[i].advanceX*fontSize, 2),
                        .width = chars[i].advanceX,
                        .height = fontSize,
                        .mipmaps = 1,
                        .format = RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
                    };

                    chars[i].image = imSpace;
                }

                for (int p = 0; p < chw*chh; p++)
                {
                    if (((unsigned char *)chars[i].image.data)[p] < FONT_BITMAP_ALPHA_THRESHOLD) ((unsigned char *)chars[i].image.data)[p] = 0;
                    else ((unsigned char *)chars[i].image.data)[p] = 255;
                }

                // Get bounding box for character (maybe offset to account for chars that dip above or below the line)
                /*
                int chX1, chY1, chX2, chY2;
                stbtt_GetCodepointBitmapBox(&fontInfo, ch, scaleFactor, scaleFactor, &chX1, &chY1, &chX2, &chY2);

                TRACELOGD("FONT: Character box measures: %i, %i, %i, %i", chX1, chY1, chX2 - chX1, chY2 - chY1);
                TRACELOGD("FONT: Character offsetY: %i", (int)((float)ascent*scaleFactor) + chY1);
                */
            }
        }
        if (genFontChars) free(codepoints);
    }
#endif

    return chars;
}

void export_font_as_code(unsigned char* data) {
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
  GlyphInfo* glyphs = font.glyphs = LoadFontData(data, font.baseSize, chars, sizeof(chars)/sizeof(int));
  printf("#include \"raylib.h\"\n");
  printf("#include \"stdlib.h\"\n");
  unsigned int count = sizeof(chars)/sizeof(int);
  for (unsigned i = 0; i < count; ++i) {
    int dataSize = glyphs[i].image.width * glyphs[i].image.height;
    printf("static unsigned char data_%d[] = {", glyphs[i].value);
    for (int j = 0; j < dataSize - 1; ++j) printf(((j%20 == 0) ? "0x%x,\n" : "0x%x, "), ((unsigned char *)glyphs[i].image.data)[j]);
    printf("0x%x };\n", ((unsigned char *)glyphs[i].image.data)[dataSize - 1]);
  }
  printf("static GlyphInfo glyphs[] = {\n");
  for (unsigned int i = 0; i < count; ++i) {
    if (glyphs[i].image.format != RL_PIXELFORMAT_UNCOMPRESSED_GRAYSCALE) {
      fprintf(stderr, "Format of a font must be PIXELFORMAT_UNCOMPRESSED_GRAYSCALE\n");
      assert(false);
    }
    printf("{ .value = %d, .offsetX = %d, .offsetY = %d, .advanceX = %d, .image = {\n", glyphs[i].value, glyphs[i].offsetX, glyphs[i].offsetY, glyphs[i].advanceX);
    printf("  .data = (void *)data_%d, .width = %d, .height = %d, .mipmaps = %d, .format = %d }\n}", glyphs[i].value, glyphs[i].image.width, glyphs[i].image.height, glyphs[i].image.mipmaps, glyphs[i].image.format);
    printf((int)i < (int)count - 1 ? ",\n" : "\n};\n");
  }
  printf("static Font default_font = { .baseSize = %d, .glyphCount = %d, .glyphPadding = %d, .recs = NULL, .glyphs = glyphs };",
      font.baseSize, font.glyphCount, font.glyphPadding);
}

// Load data from file into a buffer
unsigned char *LoadFileData(const char *fileName, unsigned int *dataSize)
{
    unsigned char *data = NULL;
    *dataSize = 0;

    if (fileName != NULL)
    {
        FILE *file = fopen(fileName, "rb");

        if (file != NULL)
        {
            // WARNING: On binary streams SEEK_END could not be found,
            // using fseek() and ftell() could not work in some (rare) cases
            fseek(file, 0, SEEK_END);
            int size = ftell(file);     // WARNING: ftell() returns 'long int', maximum size returned is INT_MAX (2147483647 bytes)
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                data = (unsigned char *)malloc(size*sizeof(unsigned char));

                if (data != NULL)
                {
                    // NOTE: fread() returns number of read elements instead of bytes, so we read [1 byte, size elements]
                    size_t count = fread(data, sizeof(unsigned char), size, file);
                    
                    // WARNING: fread() returns a size_t value, usually 'unsigned int' (32bit compilation) and 'unsigned long long' (64bit compilation)
                    // dataSize is unified along raylib as a 'int' type, so, for file-sizes > INT_MAX (2147483647 bytes) we have a limitation
                    if (count > 2147483647)
                    {
                        free(data);
                        data = NULL;
                    }
                    else
                    {
                        *dataSize = (int)count;
                    }
                }
            }

            fclose(file);
        }
    }

    return data;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "ERROR: bad number of arguments.\nUSAGE: %s <name of font you want to export>\n", argv[0]); 
    exit(-1);
  }
  unsigned int bytesRead = 0;
  unsigned char* data = LoadFileData(argv[1], &bytesRead);
  export_font_as_code(data);
}
