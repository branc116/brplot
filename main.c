#include "raylib.h"
#include "stddef.h"
#include "stdio.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "udp.h"

#include "pthread.h"

#define WIDTH 800
#define HEIGHT 600

#define POINTS_CAP 1024
Vector2 points[POINTS_CAP];

typedef struct {
  int start, cap, len;
  int group_id;
} point_group_t;

#define GROUP_CAP 8
point_group_t groups[GROUP_CAP];
int groups_len;

struct pp_ret {Vector2* v; point_group_t* group;} push_point(Vector2 v, int group) {
  struct pp_ret ret;
  if (groups_len == 0) {
    point_group_t g = { 0, POINTS_CAP, 1, group };
    groups[0] = g;
    points[0] = v;
    groups_len++;
    ret.v = &points[0]; 
    ret.group = &groups[0];
    return ret;
  } 
  int max_size = 0;
  point_group_t* max_group = &groups[0];
  for (int i = 0; i < groups_len; ++i) {
    if (groups[i].group_id == group) {
      if (groups[i].len + 1 > groups[i].cap) {
        fprintf(stderr, "Trying to add point to a group thats full");
        exit(-1);
      }
      int index = groups[i].start + groups[i].len++;
      points[index] = v;
      ret.v = &points[index]; 
      ret.group = &groups[i];
      return ret;
    }
    int size = groups[i].cap;
    if (size > max_size) {
      max_group = &groups[i];
      max_size = size;
    }
  }
  int l = max_group->cap;
  // s1 = 0
  // l1 = 3
  // l1 = 3
  // l1/2 = 1
  // ns2 = s1 + (l1 - (l1 / 2)) = 2
  // nl2 = l1/2 = 1 
  // ns1 = s1 = 0
  // nl1 = l1 - (l1 / 2)
  int ns2 = max_group->start + (l - (l / 2));
  point_group_t ng = { ns2, l / 2, 1, group };
  groups[groups_len++] = ng;
  max_group->cap = l - (l / 2);
  points[ns2] = v;
  ret.v = &points[ns2];
  ret.group = &groups[groups_len - 1];
  return ret;
}

Color colors[8] = {
  RED, GREEN, BLUE, LIGHTGRAY, PINK, GOLD, VIOLET, DARKPURPLE
};

static void test_points(void) {
  for(int i = 0; i < 1025; ++i) {
    int group = i % 8;
    int x = i / 8;
    float y = (float)i / 10;
    Vector2 p = {x, y * y * (1 + group) };
    push_point(p, group);
  }
}

int main_gui(void) {
  //test_points();
  InitWindow(WIDTH, HEIGHT, "Ray plot");
  SetTargetFPS(165);
  SetWindowState(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  Shader gridShader = LoadShader(NULL, "shaders/grid.fs");
  Shader lineShader = LoadShader("shaders/line.vs", "shaders/line.fs");

  Vector2 uvResolution = {WIDTH, HEIGHT};  
  int uResolutionG = GetShaderLocation(gridShader, "resolution");
  int uResolutionL = GetShaderLocation(lineShader, "resolution");

  Vector2 uvZoom = {1., 1.};
  int uZoomG = GetShaderLocation(gridShader, "zoom");
  int uZoomL = GetShaderLocation(lineShader, "zoom");

  Vector2 uvOffset = {0., 0.};
  int uOffsetG = GetShaderLocation(gridShader, "offset");
  int uOffsetL = GetShaderLocation(lineShader, "offset");

  while(!WindowShouldClose()) {
    uvResolution.x = GetScreenWidth();
    uvResolution.y = GetScreenHeight();
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      Vector2 delt = GetMouseDelta();
      uvOffset.x -= uvZoom.x*delt.x/uvResolution.y;
      uvOffset.y += uvZoom.y*delt.y/uvResolution.y;
    }

    float mw = GetMouseWheelMove();
    if (IsKeyDown(KEY_X)) {
      uvZoom.x *= (1 + mw/10);
    } else if (IsKeyDown(KEY_Y)) {
      uvZoom.y *= (1 + mw/10);
    } else {
      uvZoom.x *= (1 + mw/10);
      uvZoom.y *= (1 + mw/10);
    }

    if (IsKeyPressed(KEY_R)) {
      uvZoom.x = uvZoom.y = 1;
      uvOffset.x = uvOffset.y = 0;
    }

    SetShaderValue(gridShader, uResolutionG, &uvResolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(gridShader, uZoomG, &uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(gridShader, uOffsetG, &uvOffset, SHADER_UNIFORM_VEC2);

    SetShaderValue(lineShader, uResolutionL, &uvResolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(lineShader, uZoomL, &uvZoom, SHADER_UNIFORM_VEC2);
    SetShaderValue(lineShader, uOffsetL, &uvOffset, SHADER_UNIFORM_VEC2);

    BeginDrawing();
      ClearBackground(BLACK);
      BeginShaderMode(gridShader);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), RED);
      EndShaderMode();
        for (int i = 0; i < groups_len; ++i) {
          BeginShaderMode(lineShader);
            DrawLineStrip(&points[groups[i].start], groups[i].len, colors[i]);
          EndShaderMode();
        }
      
    EndDrawing();
  }
  CloseWindow();
  return 0;
}

void add_point_callback(char* buffer, size_t s) {
  Vector2 p = {0, 0};
  int group = 0;
  sscanf(buffer, "%f;%d", &p.y, &group);
  struct pp_ret r = push_point(p, group);
  r.v->y = r.group->len - 1;
}


int main(void) {
  add_point_callback("1.1", 3);
  pthread_t thread;
  pthread_attr_t attrs;

  pthread_attr_init(&attrs);
  if (pthread_create(&thread, &attrs, udp_main, NULL)) {
    fprintf(stderr, "ERROR while creating thread %d:`%s`\n", errno, strerror(errno));
  }
  main_gui();
} 
