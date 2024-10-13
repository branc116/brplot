#include "br_plot.h"
#include "br_help.h"
#include "br_text_renderer.h"
#include ".generated/default_font.h"

#define RAYMATH_STATIC_INLINE
#include "raylib.h"
#include "raymath.h"

#include <string.h>
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

void help_draw_text(br_text_renderer_t* r, char const* text, Vector2 pos, int font_size, br_color_t color) {
  int default_font_size = 10;
  if (font_size < default_font_size) font_size = default_font_size;
  br_text_renderer_push(r, pos.x, pos.y, font_size, color, text);
}

Vector2 help_measure_text(const char* txt, int font_size) {
  //Todo
  return (Vector2){ (float)strlen(txt), (float)font_size };
}

// Draw current FPS
void help_draw_fps(br_text_renderer_t* r, int posX, int posY) {
    Color color = LIME;                         // Good FPS
    int fps = GetFPS();

    if ((fps < 30) && (fps >= 15)) color = ORANGE;  // Warning FPS
    else if (fps < 15) color = RED;             // Low FPS

    char* scrach = br_scrach_get(128);
    sprintf(scrach, "%2d FPS", fps);
    help_draw_text(r, scrach, (Vector2) { (float)posX, (float)posY }, 24, BR_COLOR_PUN(color));
    br_scrach_free();
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

