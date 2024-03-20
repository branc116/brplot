#include "br_resampling2.h"
#include "br_plot.h"
#include "br_help.h"
#include "misc/default_font.h"

#include "raylib.h"
#include "raymath.h"

#include <string.h>
#include <math.h>
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

min_distances_t min_distances_get(Vector2 const* points, size_t points_len, Vector2 to) {
  min_distances_t m = { points[0], points[0], points[0] };
  for (size_t i = 1; i < points_len; ++i) {
    if (absf(m.graph_point_x.x - to.x) > absf(points[i].x - to.x)) {
      m.graph_point_x = points[i];
    }
    if (absf(m.graph_point_y.y - to.y) > absf(points[i].y - to.y)) {
      m.graph_point_y = points[i];
    }
    if (Vector2DistanceSqr(m.graph_point, to) > Vector2DistanceSqr(points[i], to)) {
      m.graph_point = points[i];
    }
  }
  return m;
}

void min_distances_get1(min_distances_t* m, Vector2 const* points, size_t points_len, Vector2 to) {
  for (size_t i = 1; i < points_len; ++i) {
    if (absf(m->graph_point_x.x - to.x) > absf(points[i].x - to.x)) {
      m->graph_point_x = points[i];
    }
    if (absf(m->graph_point_y.y - to.y) > absf(points[i].y - to.y)) {
      m->graph_point_y = points[i];
    }
    if (Vector2DistanceSqr(m->graph_point, to) > Vector2DistanceSqr(points[i], to)) {
      m->graph_point = points[i];
    }
  }
}

Vector2 br_graph_to_screen(Rectangle graph_rect, Rectangle screen_rect, Vector2 point) {
  graph_rect.y += graph_rect.height;
  if (point.y > (graph_rect.y + graph_rect.height)) return (Vector2){0};
  if (point.y < graph_rect.y) return (Vector2){0};
  if (point.x < graph_rect.x) return (Vector2){0};
  if (point.x > graph_rect.x + graph_rect.width) return (Vector2){0};
  return (Vector2) { screen_rect.x + screen_rect.width * (point.x - graph_rect.x) / graph_rect.width,
    screen_rect.y + screen_rect.height * (point.y - graph_rect.y) / graph_rect.height };
}

