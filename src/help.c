#include "src/br_plot.h"
#include "src/br_help.h"
#include "src/br_text_renderer.h"
#include "src/br_math.h"
#include "src/br_tl.h"
#include "src/br_theme.h"

#include ".generated/default_font.h"

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

void help_draw_text(br_text_renderer_t* r, char const* text, br_vec2_t pos, int font_size, br_color_t color) {
  int default_font_size = 10;
  if (font_size < default_font_size) font_size = default_font_size;
  br_text_renderer_push(r, pos.x, pos.y, 0.9f, font_size, color, text);
  //br_text_background(, br_theme.colors.btn_inactive, 0, 3);
}

br_vec2_t help_measure_text(const char* txt, int font_size) {
  //Todo
  return BR_VEC2((float)strlen(txt), (float)font_size);
}

// Draw current FPS
void help_draw_fps(br_text_renderer_t* r, int posX, int posY) {
    br_color_t color = BR_LIME;
    int round = 5;
    int fps = (int)((float)brtl_get_fps() + ((float)round / 2.f));
    fps = fps / round * round;

    if ((fps < 30) && (fps >= 15)) color = BR_ORANGE;
    else if (fps < 15) color = BR_RED;

    char* scrach = br_scrach_get(128);
    sprintf(scrach, "%2d FPS", fps);
    help_draw_text(r, scrach,  BR_VEC2((float)posX, (float)posY), 24, color);
    br_scrach_free();
}

min_distances_t min_distances_get(br_vec2_t const* points, size_t points_len, br_vec2_t to) {
  min_distances_t m = { points[0], points[0], points[0] };
  for (size_t i = 1; i < points_len; ++i) {
    if (absf(m.graph_point_x.x - to.x) > absf(points[i].x - to.x)) {
      m.graph_point_x = points[i];
    }
    if (absf(m.graph_point_y.y - to.y) > absf(points[i].y - to.y)) {
      m.graph_point_y = points[i];
    }
    if (br_vec2_dist2(m.graph_point, to) > br_vec2_dist2(points[i], to)) {
      m.graph_point = points[i];
    }
  }
  return m;
}

void min_distances_get1(min_distances_t* m, br_vec2_t const* points, size_t points_len, br_vec2_t to) {
  for (size_t i = 1; i < points_len; ++i) {
    if (absf(m->graph_point_x.x - to.x) > absf(points[i].x - to.x)) {
      m->graph_point_x = points[i];
    }
    if (absf(m->graph_point_y.y - to.y) > absf(points[i].y - to.y)) {
      m->graph_point_y = points[i];
    }
    if (br_vec2_dist2(m->graph_point, to) > br_vec2_dist2(points[i], to)) {
      m->graph_point = points[i];
    }
  }
}

br_vec2_t br_graph_to_screen(br_extent_t graph_rect, br_extenti_t screen_rect, br_vec2_t point) {
  graph_rect.y += graph_rect.height;
  if (point.y > (graph_rect.y + graph_rect.height)) return (br_vec2_t){0};
  if (point.y < graph_rect.y) return (br_vec2_t){0};
  if (point.x < graph_rect.x) return (br_vec2_t){0};
  if (point.x > graph_rect.x + graph_rect.width) return (br_vec2_t){0};
  return BR_VEC2((float)screen_rect.x + (float)screen_rect.width * (point.x - graph_rect.x) / graph_rect.width,
    (float)screen_rect.y + (float)screen_rect.height * (point.y - graph_rect.y) / graph_rect.height);
}

