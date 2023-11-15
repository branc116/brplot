#include "br_plot.h"
#include "default_font.h"
#include "string.h"
#include "math.h"
#include <raylib.h>
#include <stdio.h>

void help_trim_zeros(char * buff) {
  size_t sl = strlen(buff);
  for (int i = (int)sl; i >= 0; --i) {
    if (buff[i] == '0' || buff[i] == (char)0) buff[i] = (char)0;
    else if (buff[i] == '.') {
      buff[i] = 0;
      break;
    }
    else break;
  }
  if (buff[0] == '-' && buff[1] == '0' && buff[2] == (char)0) {
    buff[0] = '0'; buff[1] = (char)0;
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

void help_load_default_font(void) {
  Image atlas = GenImageFontAtlas(default_font.glyphs, &default_font.recs, default_font.glyphCount, default_font.baseSize, default_font.glyphPadding, 0);
  default_font.texture = LoadTextureFromImage(atlas);
  SetTextureFilter(default_font.texture, TEXTURE_FILTER_BILINEAR);
  UnloadImage(atlas);
}

// Draw current FPS
void help_draw_fps(int posX, int posY)
{
    Color color = LIME;                         // Good FPS
    int fps = GetFPS();

    if ((fps < 30) && (fps >= 15)) color = ORANGE;  // Warning FPS
    else if (fps < 15) color = RED;             // Low FPS

    sprintf(context.buff, "%2i FPS", fps);
    help_draw_text(context.buff, (Vector2) { (float)posX, (float)posY }, 24, color);
}

void help_resampling_dir_to_str(char* buff, resampling_dir r) {
  size_t i = 0;
  if (r == resampling_dir_null) memcpy(buff, "NODIR", sizeof("NODIR"));
  else {
    if (r & resampling_dir_left)  buff[i++] = 'L';
    if (r & resampling_dir_up)    buff[i++] = 'U';
    if (r & resampling_dir_right) buff[i++] = 'R';
    if (r & resampling_dir_down)  buff[i++] = 'D';
  }
  buff[i] = (char)0;
}

