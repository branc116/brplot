#include "br_plot.h"
#include "br_help.h"
#include "br_gui_internal.h"
#include "raymath.h"
#include "assert.h"
#include "src/br_da.h"
#include "tracy/TracyC.h"

bool br_plot_update_variables_2d(br_plot_t* plot, points_groups_t const groups, Vector2 mouse_pos) {
  assert(plot->kind == br_plot_kind_2d);
  if (plot->follow) {
    Rectangle sr = plot->dd.graph_rect;
    Vector2 middle = { sr.x + sr.width/2, sr.y - sr.height/2 };
    for (size_t i = 0; i < groups.len; ++i) {
      points_group_t* pg = &groups.arr[i];
      size_t gl = pg->len;
      if (!pg->is_selected || gl == 0) continue;
      plot->dd.delta.x += ((middle.x - pg->points[gl - 1].x))/1000.f;
      plot->dd.delta.y += ((middle.y - pg->points[gl - 1].y))/1000.f;
    }
    plot->dd.offset.x -= plot->dd.delta.x;
    plot->dd.offset.y -= plot->dd.delta.y;
    plot->dd.delta.x *= plot->dd.recoil;
    plot->dd.delta.y *= plot->dd.recoil;
  } else {
    plot->dd.delta = (Vector2){ 0.f, 0.f };
  }
  if (plot->mouse_inside_graph) {
    // TODO: Move this to br_keybindings.c
    // Stuff related to zoom
    {
      float mw = -GetMouseWheelMove();
      Vector2 old = plot->dd.mouse_pos;
      bool any = false;
      if (false == help_near_zero(mw)) {
        float mw_scale = (1 + mw/10);
        if (IsKeyDown(KEY_X)) {
          plot->dd.zoom.x *= mw_scale;
        } else if (IsKeyDown(KEY_Y)) {
          plot->dd.zoom.y *= mw_scale;
        } else {
          plot->dd.zoom.x *= mw_scale;
          plot->dd.zoom.y *= mw_scale;
        }
        any = true;
      }
      if (IsKeyDown(KEY_X) && IsKeyDown(KEY_LEFT_SHIFT)) any = true,   plot->dd.zoom.x *= 1.1f;
      if (IsKeyDown(KEY_Y) && IsKeyDown(KEY_LEFT_SHIFT)) any = true,   plot->dd.zoom.y *= 1.1f;
      if (IsKeyDown(KEY_X) && IsKeyDown(KEY_LEFT_CONTROL)) any = true, plot->dd.zoom.x *= .9f;
      if (IsKeyDown(KEY_Y) && IsKeyDown(KEY_LEFT_CONTROL)) any = true, plot->dd.zoom.y *= .9f;
      if (any) {
        br_plot_update_context(plot, mouse_pos);
        Vector2 now = plot->dd.mouse_pos;
        plot->dd.offset.x -= now.x - old.x;
        plot->dd.offset.y -= now.y - old.y;
      }
    }
    if (plot->jump_around) {
      plot->graph_screen_rect.x += 100.f * (float)sin(GetTime());
      plot->graph_screen_rect.y += 77.f * (float)cos(GetTime());
      plot->graph_screen_rect.width += 130.f * (float)sin(GetTime());
      plot->graph_screen_rect.height += 177.f * (float)cos(GetTime());
    }
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
      Vector2 delt = GetMouseDelta();
      //float speed = 1.f;
      if (IsKeyDown(KEY_W)) {
        //Vector3 diff = Vector3Subtract(br->eye, br->target);
      }
      plot->dd.offset.x -= plot->dd.zoom.x*delt.x/plot->graph_screen_rect.height;
      plot->dd.offset.y += plot->dd.zoom.y*delt.y/plot->graph_screen_rect.height;
      return false;
    } else return true;
  }
  return false;
}

bool br_plot_update_variables_3d(br_plot_t* plot, points_groups_t const groups, Vector2 mouse_pos) {
  assert(plot->kind == br_plot_kind_3d);
  if (!plot->mouse_inside_graph) return false;
  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    Vector2 md = Vector2Scale(GetMouseDelta(), -0.003f);
    Vector3 zeroed = Vector3Subtract(plot->ddd.eye, plot->ddd.target);
    Vector3 rotated_up = Vector3RotateByAxisAngle(zeroed, plot->ddd.up, md.x);
    Vector3 right = Vector3CrossProduct(plot->ddd.up, Vector3Normalize(zeroed));
    Vector3 rotated_right = Vector3RotateByAxisAngle(rotated_up, right, md.y);
    if (fabsf(Vector3DotProduct(rotated_right, plot->ddd.up)) > 0.94f) plot->ddd.eye = Vector3Add(rotated_up,    plot->ddd.target);
    else                                                               plot->ddd.eye = Vector3Add(rotated_right, plot->ddd.target);
    plot->ddd.eye = Vector3Add(rotated_right, plot->ddd.target);
    return false;
  }
  {
    float mw = GetMouseWheelMove();
    float mw_scale = (1 + mw/10);
    Vector3 zeroed = Vector3Subtract(plot->ddd.eye, plot->ddd.target);
    float len = Vector3Length(zeroed);
    len *= mw_scale;
    plot->ddd.eye = Vector3Add(Vector3Scale(Vector3Normalize(zeroed), len), plot->ddd.target);
  }
  return true;
}


void br_plot_update_shader_values(br_plot_t* plot) {
  switch (plot->kind) {
    case br_plot_kind_2d: {
      TracyCFrameMarkStart("update_shader_values_2d");
      Vector2 zoom = plot->dd.zoom;
      Vector2 zoom_log = { .x = powf(10.f, -floorf(log10f(zoom.x))), .y = powf(10.f, -floorf(log10f(zoom.y))) };
      Vector2 zoom_final = { .x = zoom.x * zoom_log.x, .y = zoom.y * zoom_log.y };
      plot->dd.grid_shader->uvs.zoom_uv = zoom_final;
      Vector2 off_zoom = Vector2Multiply(plot->dd.offset, zoom_log);
      Vector2 off = Vector2Divide(off_zoom, (Vector2) { 10, 10 });
      plot->dd.grid_shader->uvs.offset_uv = Vector2Subtract(off_zoom, (Vector2) { floorf(off.x) * 10.f, floorf(off.y) * 10.f });

      plot->dd.grid_shader->uvs.screen_uv = (Vector2) { .x = plot->graph_screen_rect.width, .y = plot->graph_screen_rect.height };

      plot->dd.line_shader->uvs.zoom_uv = plot->dd.zoom;
      plot->dd.line_shader->uvs.offset_uv = plot->dd.offset;
      plot->dd.line_shader->uvs.screen_uv = plot->resolution;
      plot->dd.line_shader->uvs.resolution_uv = *(Vector4*)&plot->graph_screen_rect;
      TracyCFrameMarkEnd("update_shader_values_2d");
    } break;
    case br_plot_kind_3d: {
      TracyCFrameMarkStart("update_shader_values_3d");
      Vector2 re = plot->ddd.grid_shader->uvs.resolution_uv = (Vector2) { .x = plot->graph_screen_rect.width, .y = plot->graph_screen_rect.height };
      Matrix per = MatrixPerspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
      Matrix look = MatrixLookAt(plot->ddd.eye, plot->ddd.target, plot->ddd.up);
      plot->ddd.grid_shader->uvs.m_mvp_uv = MatrixMultiply(look, per);
      plot->ddd.grid_shader->uvs.eye_uv = plot->ddd.eye;

      plot->ddd.line_shader->uvs.m_mvp_uv = MatrixMultiply(look, per);
      plot->ddd.line_shader->uvs.eye_uv = plot->ddd.eye;
      TracyCFrameMarkEnd("update_shader_values_3d");
    } break;
    default: assert(0);
  }
}

void br_plot_update_context(br_plot_t* plot, Vector2 mouse_pos) {
  Vector2 mp_in_graph = { mouse_pos.x - plot->graph_screen_rect.x, mouse_pos.y - plot->graph_screen_rect.y };
  plot->mouse_inside_graph = CheckCollisionPointRec(mouse_pos, plot->graph_screen_rect);
  if (plot->kind == br_plot_kind_2d) {
    plot->dd.mouse_pos = (Vector2) {
    -(plot->graph_screen_rect.width  - 2.f*mp_in_graph.x)/plot->graph_screen_rect.height*plot->dd.zoom.x/2.f + plot->dd.offset.x,
     (plot->graph_screen_rect.height - 2.f*mp_in_graph.y)/plot->graph_screen_rect.height*plot->dd.zoom.y/2.f + plot->dd.offset.y};
    plot->dd.graph_rect = (Rectangle){-plot->graph_screen_rect.width/plot->graph_screen_rect.height*plot->dd.zoom.x/2.f + plot->dd.offset.x,
      plot->dd.zoom.y/2.f + plot->dd.offset.y,
      plot->graph_screen_rect.width/plot->graph_screen_rect.height*plot->dd.zoom.x,
      plot->dd.zoom.y};
  } else {
    // TODO 2D/3D
    //assert(false);
  }
}

void br_plot_remove_group(br_plots_t plots, int group) {
  for (int i = 0; i < plots.len; ++i) {
    br_plot_t plot = plots.arr[i];
    br_da_remove(plot.groups_to_show, group);
  }
}
