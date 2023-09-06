#include "plotter.h"
#include "default_font.h"
#include "string.h"
#include "math.h"
#include <raylib.h>

Font default_font;

void help_trim_zeros(char * buff) {
  size_t sl = strlen(buff);
  for (int i = (int)sl; i >= 0; --i) {
    if (buff[i] == '0' || buff[i] == (char)0) buff[i] = (char)0;
    else if (buff[i] == '.') {
      buff[i] = 0;
      return;
    }
    else return;
  }
}

void help_draw_text(const char *text, Vector2 pos, float fontSize, Color color) {
  float defaultFontSize = 10.f;
  if (fontSize < defaultFontSize) fontSize = defaultFontSize;
  DrawTextEx(default_font, text, (Vector2){roundf(pos.x), roundf(pos.y)}, floorf(fontSize), 1.0f, color);
}

Vector2 help_measure_text(const char* txt, float font_size) {
  Vector2 textSize = { 0.0f, 0.0f };

  float defaultFontSize = 10;   // Default Font chars height in pixel
  if (font_size < defaultFontSize) font_size = defaultFontSize;

  textSize = MeasureTextEx(default_font, txt, floorf(font_size), 1.0f);

  return textSize;
}
#define STB_TRUETYPE_IMPLEMENTATION
#include "../raylib/src/external/stb_truetype.h"
#include "stdlib.h"
#undef STB_TRUETYPE_IMPLEMENTATION

GlyphInfo *LoadFontData2(const unsigned char *fileData, int dataSize, int fontSize, int glyphCount)
{
    GlyphInfo *chars = NULL;

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
            glyphCount = (glyphCount > 0)? glyphCount : 95;

            chars = (GlyphInfo *)RL_MALLOC(glyphCount*sizeof(GlyphInfo));

            // NOTE: Using simple packaging, one char after another
            for (int i = 0; i < glyphCount; i++)
            {
                int chw = 0, chh = 0;   // Character width and height (on generation)
                int ch = 32+i;  // Character value to get info for
                chars[i].value = ch;

                chars[i].image.data = stbtt_GetCodepointBitmap(&fontInfo, scaleFactor, scaleFactor, ch, &chw, &chh, &chars[i].offsetX, &chars[i].offsetY);

                stbtt_GetCodepointHMetrics(&fontInfo, ch, &chars[i].advanceX, NULL);
                chars[i].advanceX = (int)((float)chars[i].advanceX*scaleFactor);

                // Load characters images
                chars[i].image.width = chw;
                chars[i].image.height = chh;
                chars[i].image.mipmaps = 1;
                chars[i].image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;

                chars[i].offsetY += (int)((float)ascent*scaleFactor);

                // NOTE: We create an empty image for space character, it could be further required for atlas packing
                if (ch == 32)
                {
                    Image imSpace = {
                        .data = RL_CALLOC(chars[i].advanceX*fontSize, 2),
                        .width = chars[i].advanceX,
                        .height = fontSize,
                        .mipmaps = 1,
                        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
                    };

                    chars[i].image = imSpace;
                }
            }
        }
    }

    return chars;
}

Font LoadFontFromMemory2(const unsigned char *fileData, int dataSize, int fontSize)
{
    Font font = { 0 };

    font.baseSize = fontSize;
    font.glyphCount = 95;
    font.glyphPadding = 0;
    font.glyphs = LoadFontData2(fileData, dataSize, font.baseSize, font.glyphCount);
    font.glyphPadding = 4;

    Image atlas = GenImageFontAtlas(font.glyphs, &font.recs, font.glyphCount, font.baseSize, font.glyphPadding, 0);
    font.texture = LoadTextureFromImage(atlas);

    // Update glyphs[i].image to use alpha, required to be used on ImageDrawText()
    for (int i = 0; i < font.glyphCount; i++)
    {
        UnloadImage(font.glyphs[i].image);
        font.glyphs[i].image = ImageFromImage(atlas, font.recs[i]);
    }

    UnloadImage(atlas);

    return font;
}

int cur_font_size = 46;

void help_load_default_sdf_font(void) {
  default_font = (Font){0};
  default_font = LoadFontFromMemory2(default_font_data, sizeof(default_font_data), cur_font_size);
  SetTextureFilter(default_font.texture, TEXTURE_FILTER_BILINEAR);
}

