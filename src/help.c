#include "plotter.h"
#include "default_font.h"
#include "string.h"
#include "math.h"

Shader sdf_font_shader_s;
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
  BeginShaderMode(sdf_font_shader_s);
    DrawTextEx(default_font, text, (Vector2){roundf(pos.x), roundf(pos.y)}, floorf(fontSize), 1.0f, color);
  EndShaderMode();
}

Vector2 help_measure_text(const char* txt, float font_size) {
  Vector2 textSize = { 0.0f, 0.0f };

  float defaultFontSize = 10;   // Default Font chars height in pixel
  if (font_size < defaultFontSize) font_size = defaultFontSize;

  textSize = MeasureTextEx(default_font, txt, floorf(font_size), 1.0f);

  return textSize;
}

void help_load_default_sdf_font(void) {
  Font fontDefault = { 0 };
  fontDefault.baseSize = default_font_sz;
  fontDefault.glyphCount = default_font_gc;
  fontDefault.glyphs = LoadFontData(default_font_data, sizeof(default_font_data), default_font_sz, 0, default_font_gc, FONT_SDF);
  Image atlas = GenImageFontAtlas(fontDefault.glyphs, &fontDefault.recs, default_font_gc, default_font_sz, 0, 1);
  fontDefault.texture = LoadTextureFromImage(atlas);
  UnloadImage(atlas);
#ifdef RELEASE
  sdf_font_shader_s = LoadShaderFromMemory(NULL, SHADER_FONT_SDF),
#else
  sdf_font_shader_s  = LoadShader(NULL, "src/desktop/shaders/sdf_font.fs");
#endif
  SetTextureFilter(fontDefault.texture, TEXTURE_FILTER_BILINEAR);
  default_font = fontDefault;
}

