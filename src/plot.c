#include "br_plot.h"
#include "br_help.h"
#include "br_gui_internal.h"
#include "br_resampling2.h"
#include "src/br_da.h"

#define RAYMATH_STATIC_INLINE
#include "raymath.h"
#include "tracy/TracyC.h"

#include "assert.h"

#define GL_SRC_ALPHA 0x0302
#define GL_DST_ALPHA 0x0304
#define GL_MAX 0x8008
static void br_plot_2d_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders) {
  TracyCFrameMarkStart("br_datas_draw_2d");
  rlSetBlendFactors(GL_SRC_ALPHA, GL_DST_ALPHA, GL_MAX);
  rlSetBlendMode(BLEND_CUSTOM);
  for (int j = 0; j < plot->groups_to_show.len; ++j) {
    int group = plot->groups_to_show.arr[j];
    br_data_t const* g = br_data_get1(datas, group);
    if (g->len == 0) continue;
    resampling2_draw(g->resampling, g, plot, shaders);
  }
  if (shaders->line->len > 0) {
    br_shader_line_draw(shaders->line);
    shaders->line->len = 0;
  }
}

static void br_plot_3d_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders) {
  TracyCFrameMarkStart("br_datas_draw_3d");
  int h = (int)plot->graph_screen_rect.height;
  rlViewport((int)plot->graph_screen_rect.x, (int)plot->resolution.y - h - (int)plot->graph_screen_rect.y, (int)plot->graph_screen_rect.width, h);
  rlDisableBackfaceCulling();
  rlEnableDepthTest();
  for (int j = 0; j < plot->groups_to_show.len; ++j) {
    int group = plot->groups_to_show.arr[j];
    br_data_t const* g = br_data_get1(datas, group);
    if (g->len == 0) continue;
    resampling2_draw(g->resampling, g, plot, shaders);
  }
  if (shaders->line_3d->len > 0) {
    br_shader_line_3d_draw(shaders->line_3d);
    shaders->line_3d->len = 0;
  }
  rlDisableDepthTest();
  rlEnableBackfaceCulling();
  rlViewport(0, 0, (int)plot->resolution.x, (int)plot->resolution.y);
  TracyCFrameMarkEnd("br_datas_draw_3d");
}

void br_plot_draw(br_plot_t* plot, br_datas_t datas, br_shaders_t* shaders) {
  switch (plot->kind) {
    case br_plot_kind_2d: br_plot_2d_draw(plot, datas, shaders); break;
    case br_plot_kind_3d: br_plot_3d_draw(plot, datas, shaders); break;
    default: BR_ASSERT(0);
  }
}

void br_plot_update_variables(br_plotter_t* br, br_plot_t* plot, br_datas_t groups, Vector2 mouse_pos) {
    switch (plot->kind) {
      case br_plot_kind_2d: {
        if (br_plot_update_variables_2d(plot, groups, mouse_pos))
          br_keybinding_handle_keys(br, plot);
      } break;
      case br_plot_kind_3d: {
        if (br_plot_update_variables_3d(plot, groups, mouse_pos))
          br_keybinding_handle_keys(br, plot);
      } break;
      default: assert(0);
    }
}

bool br_plot_update_variables_2d(br_plot_t* plot, br_datas_t const groups, Vector2 mouse_pos) {
  assert(plot->kind == br_plot_kind_2d);
  if (plot->follow) {
    Rectangle sr = plot->dd.graph_rect;
    Vector2 middle = { sr.x + sr.width/2, sr.y - sr.height/2 };
    for (size_t i = 0; i < groups.len; ++i) {
      br_data_t* pg = &groups.arr[i];
      size_t gl = pg->len;
      if (pg->kind != br_data_kind_2d || gl == 0) continue;
      plot->dd.delta.x += ((middle.x - pg->dd.xs[gl - 1]))/1000.f;
      plot->dd.delta.y += ((middle.y - pg->dd.ys[gl - 1]))/1000.f;
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

bool br_plot_update_variables_3d(br_plot_t* plot, br_datas_t const groups, Vector2 mouse_pos) {
  (void)groups; (void)mouse_pos;
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


void br_plot_update_shader_values(br_plot_t* plot, br_shaders_t* shaders) {
  switch (plot->kind) {
    case br_plot_kind_2d: {
      TracyCFrameMarkStart("update_shader_values_2d");
      Vector2 zoom = plot->dd.zoom;
      Vector2 zoom_log = { .x = powf(10.f, -floorf(log10f(zoom.x))), .y = powf(10.f, -floorf(log10f(zoom.y))) };
      Vector2 zoom_final = { .x = zoom.x * zoom_log.x, .y = zoom.y * zoom_log.y };
      shaders->grid->uvs.zoom_uv = zoom_final;
      Vector2 off_zoom = Vector2Multiply(plot->dd.offset, zoom_log);
      Vector2 off = Vector2Divide(off_zoom, (Vector2) { 10, 10 });
      shaders->grid->uvs.offset_uv = Vector2Subtract(off_zoom, (Vector2) { floorf(off.x) * 10.f, floorf(off.y) * 10.f });

      shaders->grid->uvs.screen_uv = (Vector2) { .x = plot->graph_screen_rect.width, .y = plot->graph_screen_rect.height };

      shaders->line->uvs.zoom_uv = plot->dd.zoom;
      shaders->line->uvs.offset_uv = plot->dd.offset;
      shaders->line->uvs.screen_uv = plot->resolution;
      shaders->line->uvs.resolution_uv = *(Vector4*)&plot->graph_screen_rect.x;
      TracyCFrameMarkEnd("update_shader_values_2d");
    } break;
    case br_plot_kind_3d: {
      TracyCFrameMarkStart("update_shader_values_3d");
      Vector2 re = shaders->grid_3d->uvs.resolution_uv = (Vector2) { .x = plot->graph_screen_rect.width, .y = plot->graph_screen_rect.height };
      Vector3 eye_zero = Vector3Subtract(plot->ddd.eye, plot->ddd.target);
      float eye_scale = 10.f * powf(10.f, -floorf(log10f(fmaxf(fmaxf(fabsf(eye_zero.x), fabsf(eye_zero.y)), fabsf(eye_zero.z)))));
      Vector3 eye_final = Vector3Add(Vector3Scale(eye_zero, eye_scale), plot->ddd.target);
      Matrix per = MatrixPerspective(plot->ddd.fov_y, re.x / re.y, plot->ddd.near_plane, plot->ddd.far_plane);
      Matrix look_grid = MatrixLookAt(eye_final, plot->ddd.target, plot->ddd.up);
      Matrix look = MatrixLookAt(plot->ddd.eye, plot->ddd.target, plot->ddd.up);
      shaders->grid_3d->uvs.m_mvp_uv = MatrixMultiply(look_grid, per);
      shaders->grid_3d->uvs.eye_uv = eye_final;
      shaders->grid_3d->uvs.look_dir_uv = Vector3Subtract(plot->ddd.target, plot->ddd.eye);

      shaders->line_3d->uvs.m_mvp_uv = MatrixMultiply(look, per);
      shaders->line_3d->uvs.eye_uv = plot->ddd.eye;
      TracyCFrameMarkEnd("update_shader_values_3d");
    } break;
    default: assert(0);
  }
}

Vector2 br_plot_2d_get_mouse_position(br_plot_t* plot) {
  Vector2 mouse_pos = GetMousePosition();
  Vector2 mp_in_graph = { mouse_pos.x - plot->graph_screen_rect.x, mouse_pos.y - plot->graph_screen_rect.y };
  return (Vector2) {
  -(plot->graph_screen_rect.width  - 2.f*mp_in_graph.x)/plot->graph_screen_rect.height*plot->dd.zoom.x/2.f + plot->dd.offset.x,
   (plot->graph_screen_rect.height - 2.f*mp_in_graph.y)/plot->graph_screen_rect.height*plot->dd.zoom.y/2.f + plot->dd.offset.y};
}

void br_plot_update_context(br_plot_t* plot, Vector2 mouse_pos) {
  Vector2 mp_in_graph = { mouse_pos.x - plot->graph_screen_rect.x, mouse_pos.y - plot->graph_screen_rect.y };
  plot->mouse_inside_graph = CheckCollisionPointRec(mouse_pos, plot->graph_screen_rect);
  if (plot->kind == br_plot_kind_2d) {
    float aspect = plot->graph_screen_rect.width/plot->graph_screen_rect.height;
    plot->dd.mouse_pos = (Vector2) {
    -(plot->graph_screen_rect.width  - 2.f*mp_in_graph.x)/plot->graph_screen_rect.height*plot->dd.zoom.x/2.f + plot->dd.offset.x,
     (plot->graph_screen_rect.height - 2.f*mp_in_graph.y)/plot->graph_screen_rect.height*plot->dd.zoom.y/2.f + plot->dd.offset.y};
    plot->dd.graph_rect = (Rectangle){-aspect*plot->dd.zoom.x/2.f + plot->dd.offset.x,
      plot->dd.zoom.y/2.f + plot->dd.offset.y,
      aspect*plot->dd.zoom.x,
      plot->dd.zoom.y};
  } else {
    // TODO 2D/3D
    //assert(false);
  }
}

void br_plot_remove_group(br_plots_t plots, int group) {
  for (int i = 0; i < plots.len; ++i) {
    br_da_remove(plots.arr[i].groups_to_show, group);
  }
}
