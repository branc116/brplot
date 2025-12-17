#include "src/br_pp.h"
#include "src/br_plotter.h"
#include "src/br_platform.h"
#include "src/br_plot.h"
#include "src/br_gui.h"
#include "src/br_da.h"
#include "src/br_resampling.h"
#include "src/br_q.h"
#include "src/br_permastate.h"
#include "src/br_text_renderer.h"
#include "src/br_theme.h"
#include "src/br_ui.h"
#include "src/br_icons.h"
#include "src/br_gl.h"
#include "include/br_free_list_header.h"
#include "src/br_memory.h"

#include <math.h>
#include <string.h>
#include <ctype.h>

#if BR_HAS_HOTRELOAD
#  include "src/desktop/hotreload.c"
#  include <pthread.h>
#endif

br_plotter_t* br_plotter_malloc(void) {
  br_plotter_t* br = BR_MALLOC(sizeof(br_plotter_t));
  *br = (br_plotter_t) {
    .uiw = {
      .pl = {
        .title = "brplot",
        .viewport = BR_EXTENTI(0, 0, 1280, 720),
        .opengl_version = { 3, 3 }
      }
    },
    .ui = {
      .fm_state = {
        .file_selected = -1,
        .path_id = -1,
      },
      .dark_theme = true,
    },
  };
#if BR_HAS_HOTRELOAD
  br->hot_state.plotter = br;
#endif
  br->commands = q_malloc();
  if (NULL == br->commands) {
    LOGE("Failed to malloc command queue. Exiting...\n");
    exit(1);
  }
  return br;
}

static br_plot_t br_plot_2d(float grid_line_thickness) {
  br_plot_t plot = {
    .kind = br_plot_kind_2d,
    .dd =  {
#if defined(__EMSCRIPTEN__)
      .zoom = BR_VEC2D(10.f, 10.f),
#else
      .zoom = BR_VEC2D(1.f, 1.f),
#endif
      .offset = BR_VEC2D(0.f, 0.f),
      .grid_line_thickness =  grid_line_thickness,
      .grid_major_line_thickness = 2.f,
      .line_thickness = 0.05f
    }
  };
  return plot;
}

void br_plotter_init(br_plotter_t* br) {
  br_resampling_construct(&br->uiw.shaders, &br->ui.min_sampling, &br->ui.cull_min, &br->uiw.anims);
  br_data_construct(&br->uiw.sp, &br->uiw.anims);
  br_mesh_construct(&br->uiw.shaders, &br->ui.debug, &br->uiw.theme, &br->uiw.anims);
  br_plot_construct(&br->uiw.anims);
  br->ui.default_grid_line_thickenss = 1.5f;

#if BR_HAS_HOTRELOAD
  br_hotreload_start(&br->hot_state);
#endif
  brui_window_init(&br->uiw);
  if (!br->uiw.pl.is_recording && !br->uiw.pl.is_replaying) br->loaded_status = br_permastate_load(br);
  if (br->ui.multisampling) brgl_enable_multisampling();
  else                      brgl_disable_multisampling();
  if (br->loaded_status != br_permastate_status_ok) {
    br_datas_deinit(&br->groups);
    br->plots.len = 0;
    if (br_permastate_status_ui_loaded == br->loaded_status) {
      float padding = 4.f;
      br_size_t window_size = BR_SIZEI_TOF(br->uiw.pl.viewport.size);
      br_extent_t plot_extent = BR_EXTENT(padding, padding, window_size.width - padding*2, window_size.height - padding*2);
      br_plot_t plot = br_plot_2d(br->ui.default_grid_line_thickenss);
      br_plot_create_texture(&plot, plot_extent);
      bool found_resizable = false, found_menu_ex = false, found_legend_ex = false;
      brfl_foreach(i, br->uiw.resizables) {
        if (br_da_get(br->uiw.resizables, i).tag != 100) continue;
        found_resizable = true;
        plot.extent_handle = i;
        break;
      }
      if (false == found_resizable) {
        plot.extent_handle = brui_resizable_new2(&br->uiw.resizables, plot_extent, 0, (brui_resizable_t) { .title_enabled = true, .tag = 100  });
      } else {
        brfl_foreach(i, br->uiw.resizables) {
          if (br_da_get(br->uiw.resizables, i).parent != plot.extent_handle) continue;
          int tag = br_da_get(br->uiw.resizables, i).tag;
          if (tag == 101) {
            found_menu_ex = true;
            plot.menu_extent_handle = i;
          }
          if (tag == 102) {
            found_legend_ex = true;
            plot.legend_extent_handle = i;
          }
        }
      }
      if (false == found_menu_ex) {
        br_extent_t menu_extent = BR_EXTENT(0, 0, 300, plot_extent.height);
        plot.menu_extent_handle = brui_resizable_new2(&br->uiw.resizables, menu_extent, plot.extent_handle, (brui_resizable_t) { .tag = 101, });
        brui_resizable_show(plot.menu_extent_handle, false);
      }
      if (false == found_legend_ex) {
        br_extent_t menu_extent = BR_EXTENT(plot_extent.width - 110, 10, 100, 60);
        plot.legend_extent_handle = brui_resizable_new2(&br->uiw.resizables, menu_extent, plot.extent_handle, (brui_resizable_t) { .tag = 102 });
      }
      br_da_push_t(int, (br->plots), plot);
    } else {
      br_plotter_add_plot_2d(br);
    }
  } else {
    for (int i = 0; i < br->plots.len; ++i) {
      br_plot_t* p = &br->plots.arr[i];
      br_extent_t ex = brui_resizable_cur_extent(p->extent_handle);
      br->plots.arr[i].texture_id = brgl_create_framebuffer((int)roundf(ex.width), (int)roundf(ex.height));
    }
  }
  if (br->loaded_status < br_permastate_status_ui_loaded) {
    br->uiw.resizables.menu_extent_handle = brui_resizable_new(&br->uiw.resizables, BR_EXTENT(10, 40, 160, (float)br->uiw.pl.viewport.height/2.f), 0);
    br_theme_dark(&br->uiw.theme);
    br_theme_reset_ui(&br->uiw.theme);
  } else {
    br_text_renderer_load_font(br->uiw.text, brsp_try_get(br->uiw.sp, br->ui.font_path_id));
  }
  memset(&br->ui.fm_state.cur_dir, 0, sizeof(br->ui.fm_state.cur_dir));
}

void br_plotter_deinit(br_plotter_t* br) {
  br_read_input_stop();
  br_permastate_save(br);
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_deinit(br_da_getp(br->plots, i));
  }
  br_datas_deinit(&br->groups);
  BR_FREE(br->plots.arr);
  br_dagens_free(&br->dagens);
  brui_window_deinit(&br->uiw);
  LOGI("Plotter deinited");
}

void br_plotter_free(br_plotter_t* br) {
  for (size_t i = 0; i < br->ui.fm_state.cur_dir.len; ++i) {
    br_str_free(br->ui.fm_state.cur_dir.arr[i].name);
  }
  br_str_free(br->ui.fm_state.cur_dir.cur_dir);
  br_str_free(br->ui.fm_state.cur_dir.last_good_dir);
  br_da_free(br->ui.fm_state.cur_dir);
  q_free(br->commands);
  br_scrach_finish();
  //BR_FREE(br);
  LOGI("Plotter freed");
}

int br_plotter_hovered_resizable(br_plotter_t* br) {
  if (br->hovered.active == br_plotter_entity_plot_2d ||
      br->hovered.active == br_plotter_entity_plot_3d) {
    return br->plots.arr[br->hovered.plot_id].extent_handle;
  } else {
    return br->hovered.resizable_id;
  }
  return -1;
}

void br_plotter_one_iter(br_plotter_t* br) {
  BR_PROFILE_FRAME_MARK();
  BR_MEMORY_FRAME();
  br_plotter_update(br);
}

void br_plotter_update(br_plotter_t* br) {
  brpl_event_t ev = brui_event_next(&br->uiw);
  while (ev.kind != brpl_event_none) {
    switch (ev.kind) {
      case brpl_event_key_press: {
        if (ev.key == BR_KEY_ESCAPE) {
          br->action.active = br_plotter_entity_none;
        } else {
          if (br->action.active == br_plotter_entity_none ||
              br->action.active == br_plotter_entity_plot_2d ||
              br->action.active == br_plotter_entity_plot_3d) {
            switch (ev.key) {
              case BR_KEY_2: {
                br_plotter_switch_2d(br);
              } break;
              case BR_KEY_3: {
                br_plotter_switch_3d(br);
              } break;
              case BR_KEY_C: {
                if (br->hovered.active == br_plotter_entity_plot_2d ||
                    br->hovered.active == br_plotter_entity_plot_3d) {
                  if (br->uiw.key.shift_down) br_plotter_datas_deinit_in_plot(br, br->hovered.plot_id);
                  else                        br_plotter_datas_empty_in_plot(br, br->hovered.plot_id);
                } else {
                  if (br->uiw.key.shift_down) br_plotter_datas_deinit(br);
                  else                        br_datas_empty(br->groups);
                }
              } break;
              case BR_KEY_F: {
                if (br->hovered.active == br_plotter_entity_plot_2d) {
                  br_plot_t* plot = br_da_getp(br->plots, br->action.plot_id);
                  br_extent_t ex = brui_resizable_cur_extent(plot->extent_handle);
                  if (br->uiw.key.ctrl_down) br_plot_focus_visible(plot, br->groups, ex);
                  else                       plot->follow = !plot->follow;
                } else {
                  BR_TODO("br->hovered.active: %d", br->hovered.active);
                }
              } break;
              case BR_KEY_H: {
                br->ui.help.show = !br->ui.help.show;
              } break;
              case BR_KEY_D: {
                if (br->hovered.active != br_plotter_entity_plot_3d) br->ui.debug = !br->ui.debug;
              } break;
              case BR_KEY_R: {
                if (br->hovered.active == br_plotter_entity_plot_2d) {
                  br_plot_t* plot = br_da_getp(br->plots, br->hovered.plot_id);
                  if (!br->uiw.key.ctrl_down) plot->dd.zoom.x = plot->dd.zoom.y = 1;
                  if (!br->uiw.key.ctrl_down) plot->dd.offset.x = plot->dd.offset.y = 0;
                } else if (br->hovered.active == br_plotter_entity_plot_3d) {
                  br_plot_t* plot = br_da_getp(br->plots, br->hovered.plot_id);
                  br_plot_3d_t* pi3 = &plot->ddd;
                  br_anim3_set(&br->uiw.anims, pi3->eye_ah, BR_VEC3(0, 0, 100));
                  pi3->target = BR_VEC3(0, 0, 0);
                  pi3->up = BR_VEC3(0, 1, 0);
                }
              } break;
              case BR_KEY_T: {
                br_datas_add_test_points(&br->groups);
              } break;
            }
          } else if (br->action.active == br_plotter_entity_text_input) {
            switch (ev.key) {
              case BR_KEY_TAB: {
                // TODO: Hosting out brui so this should be handled differently 
//                if (ta->id == br->ui.fm_state.path_id) {
//                  if (br->uiw.key.ctrl_down) {
//                    if (br->ui.fm_state.select_index > 0) --br->ui.fm_state.select_index;
//                  } else {
//                    if (br->ui.fm_state.select_index < (int)br->ui.fm_state.cur_dir.len) ++br->ui.fm_state.select_index;
//                  }
//                  br->ui.fm_state.has_tabed = true;
//                }
              } break;
              case BR_KEY_ENTER: {
//                if (ta->id == br->ui.fm_state.path_id) {
//                  br->ui.fm_state.has_entered = true;
//                }
              } break;
              default: LOGI("text input %d (%d)", ev.key, ev.keycode); break;
            }
          }
        }
      } break;
      case brpl_event_mouse_press: {
        if (ev.mouse_key == 3) br->action = br->hovered;
      } break;
      case brpl_event_mouse_release: {
        br->action.active = br_plotter_entity_none;
      } break;
      case brpl_event_mouse_move: {
        if (br->uiw.time.now - br->uiw.touch_points.last_touch_time < 1) break;
        if (false == br->uiw.pl.active) break;
        if (false == br->uiw.mouse.dragging_right) break;

        if (br->action.active == br_plotter_entity_plot_2d) {
          br_plot_t* plot = br_da_getp(br->plots, br->action.plot_id);
          //LOGI("br->uiw.mouse.delta: %f %f", BR_VEC2_(br->uiw.mouse.delta));
          br_plot2d_move_screen_space(plot, br->uiw.mouse.delta, br_animex(&br->uiw.anims, br_da_get(br->uiw.resizables, plot->extent_handle).cur_extent_ah).size);
        } else if (br->action.active == br_plotter_entity_plot_3d) {
          br_plot_t* plot = br_da_getp(br->plots, br->action.plot_id);
          float speed = (float)br->uiw.time.frame / 2.f;
          br_vec3_t eye = br_anim3_get_target(&br->uiw.anims, plot->ddd.eye_ah);
          br_vec3_t zeroed = br_vec3_sub(eye, plot->ddd.target);
          br_vec3_t zero_dir = br_vec3_normalize(zeroed);
          br_vec3_t right = br_vec3_normalize(br_vec3_cross(plot->ddd.up, zero_dir));
          br_vec2_t m = br->uiw.mouse.delta;
          br_vec2_t md = br_vec2_scale(BR_VEC2(m.x, m.y), -speed);
          br_vec3_t rotated_up = br_vec3_rot(zeroed, plot->ddd.up, md.x*0.5f);
          br_vec3_t rotated_right = br_vec3_rot(rotated_up, right, md.y*0.5f);
          float horizontal_factor = fabsf(br_vec3_dot(br_vec3_normalize(rotated_right), plot->ddd.up));
          if (horizontal_factor > 0.94f) br_anim3_set(&br->uiw.anims, plot->ddd.eye_ah, br_vec3_add(rotated_up,    plot->ddd.target));
          else                           br_anim3_set(&br->uiw.anims, plot->ddd.eye_ah, br_vec3_add(rotated_right, plot->ddd.target));
        }
      } break;
      case brpl_event_mouse_scroll: {
        if (br->action.active != br_plotter_entity_none) break;
        if (br->hovered.active == br_plotter_entity_plot_2d) {
          br_plot_t* plot = &br->plots.arr[br->hovered.plot_id];
          br_extent_t ex = brui_resizable_cur_extent(plot->extent_handle);
          br_vec2_t zoom = BR_VEC2(-ev.vec.y, -ev.vec.y);
          if (br->uiw.key.down[BR_KEY_X]) zoom.y = 0.f;
          if (br->uiw.key.down[BR_KEY_Y]) zoom.x = 0.f;
          br_plot2d_zoom(plot, zoom, ex, br->uiw.mouse.pos);
        } else if (br->hovered.active == br_plotter_entity_plot_3d) {
          br_plot_t* plot = &br->plots.arr[br->hovered.plot_id];
          float mw_scale = (1.0f + (float)br->uiw.time.frame*ev.vec.y*3.0f);
          br_vec3_t eye = br_anim3(&br->uiw.anims, plot->ddd.eye_ah);
          br_vec3_t zeroed = br_vec3_sub(eye, plot->ddd.target);
          br_vec3_t zero_dir = br_vec3_normalize(zeroed);
          float len = br_vec3_len(zeroed);
          len /= mw_scale;
          br_anim3_set(&br->uiw.anims, plot->ddd.eye_ah, br_vec3_add(plot->ddd.target, br_vec3_scale(zero_dir, len)));
        }
      } break;
      case brpl_event_frame_next: {
        int found_plot = -1;
        for (int j = 0; j < br->plots.len; ++j) {
          br_plot_t plot = br_da_get(br->plots, j);
          if (plot.extent_handle == br->uiw.resizables.active_resizable) {
            switch (plot.kind) {
              case br_plot_kind_2d: br->hovered.active = br_plotter_entity_plot_2d; break;
              case br_plot_kind_3d: br->hovered.active = br_plotter_entity_plot_3d; break;
              default: BR_TODO("Unknown plot kind %d", plot.kind); break;
            }
            found_plot = j;
          }
        }

        if (br->action.active == br_plotter_entity_none) {
          if (found_plot > -1) {
            br->hovered.plot_id = found_plot;
            br_plot_t* plot = &br->plots.arr[br->hovered.plot_id];
            if (br->hovered.active == br_plotter_entity_plot_2d) {
              br_vec2_t zoom = { 0 };
              if (br->uiw.key.down[BR_KEY_X]) {
                if (br->uiw.key.ctrl_down) zoom.x += -1.f;
                if (br->uiw.key.shift_down) zoom.x += 1.f;
              }
              if (br->uiw.key.down[BR_KEY_Y]) {
                if (br->uiw.key.ctrl_down) zoom.y += -1.f;
                if (br->uiw.key.shift_down) zoom.y += 1.f;
              }
              zoom.x *= 60.f*(float)br->uiw.time.frame;
              zoom.y *= 60.f*(float)br->uiw.time.frame;
              br_extent_t ex = brui_resizable_cur_extent(plot->extent_handle);
              br_plot2d_zoom(plot, zoom, ex, br->uiw.mouse.pos);
            } else if (br->hovered.active == br_plotter_entity_plot_3d) {
              br_vec3_t eye = br_anim3_get_target(&br->uiw.anims, plot->ddd.eye_ah);
              br_vec3_t zeroed = br_vec3_sub(eye, plot->ddd.target);
              br_vec3_t zero_dir = br_vec3_normalize(zeroed);
              br_vec3_t right = br_vec3_normalize(br_vec3_cross(plot->ddd.up, zero_dir));
              br_vec3_t delta = { 0 };
              float speed = 10.f;
              float dt = (float)br->uiw.time.frame;
              if (br->uiw.key.ctrl_down) speed *= 0.1f;
              if (br->uiw.key.shift_down) speed *= 10.f;
              if (br->uiw.key.down[BR_KEY_W]) delta = br_vec3_scale(zero_dir, -dt*speed);
              if (br->uiw.key.down[BR_KEY_S]) delta = br_vec3_add(delta, br_vec3_scale(zero_dir, dt*speed));
              if (br->uiw.key.down[BR_KEY_A]) delta = br_vec3_add(delta, br_vec3_scale(right, -dt*speed));
              if (br->uiw.key.down[BR_KEY_D]) delta = br_vec3_add(delta, br_vec3_scale(right, dt*speed));
              if (br->uiw.key.down[BR_KEY_Q]) delta = br_vec3_add(delta, br_vec3_scale(plot->ddd.up, -dt*speed));
              if (br->uiw.key.down[BR_KEY_E]) delta = br_vec3_add(delta, br_vec3_scale(plot->ddd.up, dt*speed));
              plot->ddd.target = br_vec3_add(plot->ddd.target, delta);
              br_anim3_set(&br->uiw.anims, plot->ddd.eye_ah, br_vec3_add(eye, delta));
            }
          } else {
            br->hovered.resizable_id = br->uiw.resizables.active_resizable;
            br->hovered.active = br_plotter_entity_ui;
          }
        }
        brui_frame_start(&br->uiw);
          br_plotter_draw(br);
          br_dagens_handle(&br->groups, &br->dagens, &br->plots, brpl_time() + 0.010);
          handle_all_commands(br, br->commands);
          br_plotter_frame_end(br);
          if (brui_action()->kind == brui_action_sliding) {
            br->action.active = br_plotter_entity_ui;
          } else if (brui_action()->kind == brui_action_typing) {
            br->action.active = br_plotter_entity_text_input;
          }
        brui_frame_end(&br->uiw);
        return;
      } break;
      case brpl_event_touch_update: {
        br->uiw.touch_points.last_touch_time = br->uiw.time.now;
        brpl_touch_point_t* tpp = NULL;
        brfl_foreach(i, br->uiw.touch_points) {
          brpl_touch_point_t tp = br->uiw.touch_points.arr[i];
          if (tp.id != ev.touch.id) continue;
          tpp = &br->uiw.touch_points.arr[i];
        }
        if (tpp == NULL) break;
        if (br->hovered.active == br_plotter_entity_plot_2d) {
          br_plot_t* plot = br_da_getp(br->plots, br->hovered.plot_id);
          br_extent_t ex = brui_resizable_cur_extent(plot->extent_handle);
          if (br->uiw.touch_points.free_len == 1) {
              br_vec2_t delta = br_vec2_sub(ev.touch.pos, tpp->pos);
              br_plot2d_move_screen_space(plot, delta, ex.size);
              tpp->pos = ev.touch.pos;
          } else if (br->uiw.touch_points.free_len == 2) {
            brpl_touch_point_t* other = NULL;
            brfl_foreach(i, br->uiw.touch_points) {
              brpl_touch_point_t tp = br->uiw.touch_points.arr[i];
              if (tp.id == ev.touch.id) continue;
              other = &br->uiw.touch_points.arr[i];
            }
            if (NULL == other) break;
            br_vec2d_t middle_old = { 0 };
            // Handle Zoom
            {
              br_vec2d_t pa = br_plot2d_to_plot(plot, tpp->pos, ex);
              br_vec2d_t pb = br_plot2d_to_plot(plot, other->pos, ex);
              br_vec2d_t pc = br_plot2d_to_plot(plot, ev.touch.pos, ex);
              middle_old = br_vec2d_add(br_vec2d_scale(pa, 0.5f), br_vec2d_scale(pb, 0.5f));
              float old_len = br_vec2_dist(BR_VEC2D_TOF(pb), BR_VEC2D_TOF(pa));
              float new_len = br_vec2_dist(BR_VEC2D_TOF(pb), BR_VEC2D_TOF(pc));
              float zoom = old_len/new_len;
              plot->dd.zoom.x *= zoom;
              plot->dd.zoom.y *= zoom;
            }
            // Handle offset
            {
              br_vec2d_t pb = br_plot2d_to_plot(plot, other->pos, ex);
              br_vec2d_t pc = br_plot2d_to_plot(plot, ev.touch.pos, ex);
              br_vec2d_t middle_new = br_vec2d_add(br_vec2d_scale(pb, 0.5f), br_vec2d_scale(pc, 0.5f));
              br_vec2d_t delta = br_vec2d_sub(middle_new, middle_old);
              plot->dd.offset = br_vec2d_sub(plot->dd.offset, delta);
            }
            tpp->pos = ev.touch.pos;
          }
        }
      } break;
      case brpl_event_touch_end: {
        br->uiw.touch_points.last_touch_time = br->uiw.time.now;
        int to_remove = -1;
        brfl_foreach(i, br->uiw.touch_points) {
          brpl_touch_point_t tp = br->uiw.touch_points.arr[i];
          if (tp.id != ev.touch.id) continue;
          to_remove = i;
          break;
        }
        if (to_remove >= 0) {
          brpl_touch_point_t point = br_da_get(br->uiw.touch_points, to_remove);
          brfl_remove(br->uiw.touch_points, to_remove);
          double diff = br->uiw.time.now - br->uiw.touch_points.last_free_time;
          if (br->uiw.touch_points.free_len == 0) {
            if (diff < 0.5) {
              br->uiw.mouse.pos = ev.pos;
              brui_resizable_mouse_move(&br->uiw.resizables, point.pos);
              brui_resizable_mouse_pressl(&br->uiw.resizables, point.pos, br->uiw.key.ctrl_down);
              brui_resizable_mouse_releasel(&br->uiw.resizables, point.pos);
            }
            br->uiw.touch_points.last_free_time = br->uiw.time.now;
          }
        }
      } break;
      default: break;
    }
    ev = brui_event_next(&br->uiw);
  }
}

void br_plotter_resize(br_plotter_t* br, float width, float height) {
  (void)br; (void)width; (void)height;
  BR_TODO("br_plotter_resize");
  //brtl_window_size_set((int)width, (int)height);
}

br_datas_t* br_plotter_get_br_datas(br_plotter_t* br) {
  return &br->groups;
}

void br_plotter_switch_2d(br_plotter_t* br) {
  for (int i = 0; i < br->plots.len; ++i) {
    if (br->plots.arr[i].kind != br_plot_kind_2d) continue;
    brui_resizable_show(br->plots.arr[i].extent_handle, true);
    brui_resizable_move_on_top(&br->uiw.resizables, br->plots.arr[i].extent_handle);
    return;
  }
  br_plotter_add_plot_2d(br);
}

void br_plotter_switch_3d(br_plotter_t* br) {
  for (int i = 0; i < br->plots.len; ++i) {
    if (br->plots.arr[i].kind != br_plot_kind_3d) continue;
    brui_resizable_show(br->plots.arr[i].extent_handle, true);
    brui_resizable_move_on_top(&br->uiw.resizables, br->plots.arr[i].extent_handle);
    return;
  }
  br_plotter_add_plot_3d(br);
}

int br_plotter_add_plot_2d(br_plotter_t* br) {
  float padding = 4.f;
  br_size_t window_size = BR_SIZEI_TOF(br->uiw.pl.viewport.size);
  br_extent_t ex = BR_EXTENT(padding, padding, window_size.width - padding*2, window_size.height - padding*2);
  br_plot_t plot = br_plot_2d(br->ui.default_grid_line_thickenss);
  br_plot_create_texture(&plot, ex);
  plot.extent_handle = brui_resizable_new2(&br->uiw.resizables, ex, 0, (brui_resizable_t) { .tag = 100, .title_enabled = true });
#if defined(__EMSCRIPTEN__)
  brui_resizable_maximize(plot.extent_handle, true);
#endif
  plot.menu_extent_handle = brui_resizable_new2(&br->uiw.resizables, BR_EXTENT(0, 0, 300, ex.height), plot.extent_handle, (brui_resizable_t) { .tag = 101 });
  brui_resizable_show(plot.menu_extent_handle, false);
  plot.legend_extent_handle = brui_resizable_new2(&br->uiw.resizables, BR_EXTENT(ex.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { .tag = 102 });
  br_da_push_t(int, (br->plots), plot);
  return br->plots.len - 1;
}

int br_plotter_add_plot_3d(br_plotter_t* br) {
  float padding = 4;
  br_extent_t ex = BR_EXTENT(padding, padding, (float)(br->uiw.pl.viewport.width) - 2.f * padding, (float)(br->uiw.pl.viewport.height) - 2.f * padding);
  br_plot_t plot = {
    .data_info = { 0 },
    .follow = false,
    .jump_around = false,
    .kind = br_plot_kind_3d,
    .ddd =  {
      .target = BR_VEC3(0, 0, 0),
      .up = BR_VEC3(0, 1, 0),
      .fov_y = 1,
      .near_plane = 0.1f,
      .far_plane = 1e3f,
    }
  };
  plot.ddd.eye_ah = br_anim3_new(&br->uiw.anims, BR_VEC3(0, 0, 0), BR_VEC3(0, 0, 100));
  br_anim_slerp(&br->uiw.anims, plot.ddd.eye_ah, true);
  br_plot_create_texture(&plot, ex);
  plot.extent_handle = brui_resizable_new(&br->uiw.resizables, ex, 0);
  plot.menu_extent_handle = brui_resizable_new(&br->uiw.resizables, BR_EXTENT(0, 0, 300, ex.height), plot.extent_handle);
  brui_resizable_show(plot.menu_extent_handle, false);
  plot.legend_extent_handle = brui_resizable_new2(&br->uiw.resizables, BR_EXTENT(ex.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { 0 });
  br_da_push_t(int, (br->plots), plot);
  return br->plots.len - 1;
}

void br_plotter_remove_plot(br_plotter_t* br, int plot_index) {
  // TODO: Implement free list with plots...
  br_plot_deinit(br_da_getp(br->plots, plot_index));
  // 0 1 2 3 | 4
  //   |
  // 0 2 3   | 3
  int count_to_move = (br->plots.len - plot_index - 1);
  if (count_to_move > 0) {
    size_t bytes_to_move = sizeof(br->plots.arr[0]) * (size_t)count_to_move;
    memmove(br_da_getp(br->plots, plot_index), br_da_getp(br->plots, plot_index + 1), bytes_to_move);
  }
  --br->plots.len;
}

void br_plotter_data_remove(br_plotter_t* br, int group_id) {
  br_datas_t* pg = &br->groups;
  br_plots_t plots = br->plots;
  br_data_remove(pg, group_id);
  br_plots_remove_group(plots, group_id);
  br_da_remove_feeld(br->dagens, group_id, group_id);
}

void br_plotter_frame_end(br_plotter_t* br) {
  brfl_foreach(i, br->groups) {
    if (br->groups.arr[i].is_new == false) continue;
    bool any_added = false;
    for (int j = 0; j < br->plots.len; ++j) {
      if (br->plots.arr[j].kind == br_plot_kind_2d && br->groups.arr[i].kind == br_data_kind_2d) {
        br_da_push_t(int, br->plots.arr[j].data_info, br_plot_data(br->groups.arr[i].group_id));
        any_added = true;
      } else if (br->plots.arr[j].kind == br_plot_kind_3d) {
        br_da_push_t(int, br->plots.arr[j].data_info, br_plot_data(br->groups.arr[i].group_id));
        any_added = true;
      }
    }
    if (false == any_added) {
      switch (br->groups.arr[i].kind) {
        case br_data_kind_2d: {
          int id = br_plotter_add_plot_2d(br);
          br_da_push_t(int, br->plots.arr[id].data_info, br_plot_data(br->groups.arr[i].group_id));
        } break;
        case br_data_kind_3d: {
          int id = br_plotter_add_plot_3d(br);
          br_da_push_t(int, br->plots.arr[id].data_info, br_plot_data(br->groups.arr[i].group_id));
        } break;
        default: BR_UNREACHABLE("Kind: %d", br->groups.arr[i].kind);
      }
    }
    br->groups.arr[i].is_new = false;
  }
}

void br_plotter_export(br_plotter_t const* br, char const * path) {
  FILE* file = fopen(path, "w");
  if (file == NULL) {
    fprintf(stderr, "Failed to open path: `%s`", path);
    return;
  }
// TODO
#if 0
  fprintf(file, "--zoomx %f\n", br->uvZoom.x);
  fprintf(file, "--zoomy %f\n", br->uvZoom.y);
  fprintf(file, "--offsetx %f\n", br->uvOffset.x);
  fprintf(file, "--offsety %f\n", br->uvOffset.y);
#endif
  br_datas_export(br->groups, file);
  fclose(file);
}

void br_plotter_export_csv(br_plotter_t const* br, char const* path) {
  FILE* file = fopen(path, "w");
  // TODO: Show user an error message
  if (file == NULL) return;
  br_datas_export_csv(br->groups, file);
  fclose(file);
}

void br_plotter_datas_deinit(br_plotter_t* br) {
  br_datas_deinit(&br->groups);
  for (int i = 0; i < br->plots.len; ++i) {
    br->plots.arr[i].data_info.len = 0;
  }
  br_dagens_free(&br->dagens);
}

void br_plotter_datas_deinit_in_plot(br_plotter_t* br, int plot_id) {
  br_plot_t* plot = br_da_getp(br->plots, plot_id);
  for (int i = 0; i < plot->data_info.len; ++i) {
    br_plot_data_t pd = br_da_get(plot->data_info, i);
    br_anim_delete(&br->uiw.anims, pd.thickness_multiplyer_ah);
    br_data_remove(&br->groups, pd.group_id);
    for (int j = 0; j < br->plots.len; ++j) {
      if (plot_id == j) continue;
      br_plot_t* plot2 = br_da_getp(br->plots, j);
      br_da_remove_feeld(plot2->data_info, group_id, pd.group_id);
    }
  }
  plot->data_info.len = 0;
}
void br_plotter_datas_empty_in_plot(br_plotter_t* br, int plot_id) {
  br_plot_t* plot = br_da_getp(br->plots, plot_id);
  for (int i = 0; i < plot->data_info.len; ++i) {
    br_plot_data_t pd = br_da_get(plot->data_info, i);
    br_data_empty(br_data_get1(br->groups, pd.group_id));
  }
}

void br_on_fatal_error(void) {
  br_str_t config_file = { 0 };
  br_str_t config_file_old = { 0 };
  br_fs_get_config_dir(&config_file);
  br_fs_cd(&config_file, BR_STRL("plotter.br"));
  br_str_copy2(&config_file_old, config_file);
  br_str_push_strv(&config_file_old, BR_STRL(".old."));
  br_str_push_float(&config_file_old, (float)brpl_time());
  br_str_push_zero(&config_file);
  br_str_push_zero(&config_file_old);

  br_fs_move(config_file.str, config_file_old.str);

  br_str_free(config_file);
  br_str_free(config_file_old);
}
