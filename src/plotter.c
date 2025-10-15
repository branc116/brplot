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
#include "src/br_free_list.h"
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
    .win = {
      .title = "brplot",
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__)
      .kind = brpl_window_x11,
#else
      .kind = brpl_window_glfw,
#endif
      .viewport = BR_EXTENTI(0, 0, 1280, 720)
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
  br_resampling_construct(&br->shaders, &br->ui.theme.ui.min_sampling, &br->ui.theme.ui.cull_min);
  br_data_construct(&br->sp);
  br_mesh_construct(&br->shaders, &br->ui.theme.ui.debug);
  brgl_construct(&br->shaders);
  return br;
}

static br_plot_t br_plot_2d(br_sizei_t window_size, float grid_line_thickness) {
  int padding = 4;
  br_plot_t plot = {
    .cur_extent = BR_EXTENTI( padding, padding, window_size.width - padding*2, window_size.height - padding*2 ),
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
  if (false == brpl_window_open(&br->win)) {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__)
    br->win.kind = brpl_window_glfw;
    if (false == brpl_window_open(&br->win)) {
      LOGF("Failed to open window either with x11 or glfw. Please install one of those two.");
    }
#else
    LOGF("Failed to open window either with GLFW. Please install GLFW on your device.");
#endif
  }
  brgl_disable_back_face_cull();
  brgl_enable_depth_test();
  brgl_enable(GL_BLEND);
  brgl_enable_clip_distance();
  brgl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  brgl_blend_equation(GL_FUNC_ADD);
  brgl_viewport(0, 0, br->win.viewport.width, br->win.viewport.height);

  br->shaders = br_shaders_malloc();
  br->text = br_text_renderer_malloc(1024, 1024, br_font_data, &br->shaders.font);
  brui_construct(&br->ui.theme, &br->resizables, &br->sp, br->text, &br->shaders);
  br->time.now = brpl_time();

#if BR_HAS_HOTRELOAD
  br_hotreload_start(&br->hot_state);
#endif
  br_theme_reset_ui(&br->ui.theme);
  br->loaded_status = br_permastate_load(br);
  if (br->loaded_status != br_permastate_status_ok) {
    br_datas_deinit(&br->groups);
    br->plots.len = 0;
    if (br_permastate_status_ui_loaded == br->loaded_status) {
      br_plot_t plot = br_plot_2d(br->win.viewport.size, br->ui.theme.ui.default_grid_line_thickenss);
      br_plot_create_texture(&plot);
      bool found_resizable = false, found_menu_ex = false, found_legend_ex = false;
      brfl_foreach(i, br->resizables) {
        if (br_da_get(br->resizables, i).tag != 100) continue;
        found_resizable = true;
        plot.extent_handle = i;
        break;
      }
      if (false == found_resizable) {
        plot.extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENTI_TOF(plot.cur_extent), 0, (brui_resizable_t) { .current = { .title_enabled = true, .tag = 100 } });
      } else {
        brfl_foreach(i, br->resizables) {
          if (br_da_get(br->resizables, i).parent != plot.extent_handle) continue;
          int tag = br_da_get(br->resizables, i).tag;
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
        plot.menu_extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENT(0, 0, 300, (float)plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .current.tag = 101, .target = { .hidden_factor = 1.f } });
      }
      if (false == found_legend_ex) {
        plot.legend_extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENT((float)plot.cur_extent.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { .current.tag = 102 });
      }
      br_da_push_t(int, (br->plots), plot);
    } else {
      brui_resizable_init(&br->resizables, BR_EXTENTI_TOF(br->win.viewport));
      br_plotter_add_plot_2d(br);
    }
  } else {
    for (int i = 0; i < br->plots.len; ++i) {
      br_plot_t* p = &br->plots.arr[i];
      br->plots.arr[i].texture_id = brgl_create_framebuffer(p->cur_extent.width, p->cur_extent.height);
    }
  }
  br_icons_init(br->shaders.icon);
  if (br->loaded_status < br_permastate_status_ui_loaded) {
    br->resizables.menu_extent_handle = brui_resizable_new(&br->resizables, BR_EXTENT(10, 40, 160, (float)br->win.viewport.height/2.f), 0);
    br_theme_dark(&br->ui.theme);
    br_theme_reset_ui(&br->ui.theme);
  } else {
    br_text_renderer_load_font(br->text, brsp_try_get(br->sp, br->ui.font_path_id));
  }
  memset(&br->ui.fm_state.cur_dir, 0, sizeof(br->ui.fm_state.cur_dir));
  bruir_resizable_refresh(&br->resizables, 0);
}

void br_plotter_deinit(br_plotter_t* br) {
  br_read_input_stop();
  br_permastate_save(br);
  br_icons_deinit();
  for (int i = 0; i < br->plots.len; ++i) {
    br_plot_deinit(br_da_getp(br->plots, i));
  }
  br_datas_deinit(&br->groups);
  brui_resizable_deinit();
  br_text_renderer_free(br->text);
  br_shaders_free(br->shaders);
  brpl_window_close(&br->win);
  BR_FREE(br->plots.arr);
  br_dagens_free(&br->dagens);
  brsp_free(&br->sp);
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
  brui_finish();
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
  TracyCFrameMark;
  br_memory_frame();
  br_plotter_update(br);
}

void br_plotter_update(br_plotter_t* br) {
  static int n = 0;
  brpl_event_t ev = brpl_event_next(&br->win);
  ++n;
  br->mouse.click = false;
  while (ev.kind != brpl_event_none) {
    //LOGI("[%d] ev.kind = %d", n, ev.kind);
    switch (ev.kind) {
      case brpl_event_nop: break;
      case brpl_event_key_press: {
        if (ev.key == BR_KEY_LEFT_CONTROL)    br->key.ctrl_down = true;
        else if (ev.key == BR_KEY_LEFT_SHIFT) br->key.shift_down = true;
        else if (ev.key == BR_KEY_LEFT_ALT)   br->key.alt_down = true;
        else if (ev.key < 255)                br->key.down[ev.key] = true;

        if (ev.key == BR_KEY_ESCAPE) {
          br->action.active = br_plotter_entity_none;
          brui_resizable_mouse_releasel(&br->resizables, br->mouse.pos);
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
                  if (br->key.shift_down) br_plotter_datas_deinit_in_plot(br, br->hovered.plot_id);
                  else                    br_plotter_datas_empty_in_plot(br, br->hovered.plot_id);
                } else {
                  if (br->key.shift_down) br_plotter_datas_deinit(br);
                  else                    br_datas_empty(br->groups);
                }
              } break;
              case BR_KEY_F: {
                if (br->hovered.active == br_plotter_entity_plot_2d) {
                  br_plot_t* plot = br_da_getp(br->plots, br->action.plot_id);
                  if (br->key.ctrl_down) br_plot_focus_visible(plot, br->groups);
                  else                   plot->follow = !plot->follow;
                } else {
                  BR_TODO("br->hovered.active: %d", br->hovered.active);
                }
              } break;
              case BR_KEY_H: {
                br->ui.help.show = !br->ui.help.show;
              } break;
              case BR_KEY_D: {
                if (br->hovered.active != br_plotter_entity_plot_3d) br->ui.theme.ui.debug = !br->ui.theme.ui.debug;
              } break;
              case BR_KEY_R: {
                if (br->hovered.active == br_plotter_entity_plot_2d) {
                  br_plot_t* plot = br_da_getp(br->plots, br->hovered.plot_id);
                  if (!br->key.ctrl_down) plot->dd.zoom.x = plot->dd.zoom.y = 1;
                  if (!br->key.ctrl_down) plot->dd.offset.x = plot->dd.offset.y = 0;
                } else if (br->hovered.active == br_plotter_entity_plot_3d) {
                  br_plot_t* plot = br_da_getp(br->plots, br->hovered.plot_id);
                  br_plot_3d_t* pi3 = &plot->ddd;
                  pi3->eye = BR_VEC3(0, 0, 100);
                  pi3->target = BR_VEC3(0, 0, 0);
                  pi3->up = BR_VEC3(0, 1, 0);
                }
              } break;
              case BR_KEY_T: {
                br_datas_add_test_points(&br->groups);
              } break;
              case BR_KEY_U: brui_log(true); break;
              case BR_KEY_PAGE_UP: brui_resizable_page(&br->resizables, BR_VEC2(0, -1.f)); break;
              case BR_KEY_PAGE_DOWN: brui_resizable_page(&br->resizables, BR_VEC2(0, 1.f)); break;
              case BR_KEY_END: brui_resizable_scroll_percent_set(&br->resizables, 1.f); break;
              case BR_KEY_HOME: brui_resizable_scroll_percent_set(&br->resizables, 0.f); break;
              default: {
                LOGI("pressed %d (%d)", ev.key, ev.keycode);
              } break;
            }
          } else if (br->action.active == br_plotter_entity_text_input) {
            brui_action_text_t* ta = &brui_action()->args.text;
            br_strv_t strv = brsp_get(br->sp, ta->id);

            switch (ev.key) {
              case BR_KEY_LEFT: {
                do {
                  ta->cursor_pos = br_strv_utf8_add(strv, ta->cursor_pos, -1);
                } while (ta->cursor_pos > 0 && br->key.ctrl_down && isalnum(strv.str[ta->cursor_pos]));
              } break;
              case BR_KEY_RIGHT: {
                do {
                  ta->cursor_pos = br_strv_utf8_add(strv, ta->cursor_pos, 1);
                } while (ta->cursor_pos < (int)strv.len && br->key.ctrl_down && isalnum(strv.str[ta->cursor_pos]));
              } break;
              case BR_KEY_ESCAPE: {
                brui_action()->kind = brui_action_none;
                br->action.active = br_plotter_entity_none;
              } break;
              case BR_KEY_DELETE: {
                ta->changed = brsp_remove_utf8_after(&br->sp, ta->id, ta->cursor_pos) > 0;
                br_strv_t s = brsp_get(br->sp, ta->id);
                char c;
                bool is_alnum;
                do {
                  if (ta->cursor_pos == (int)s.len) break;
                  c = s.str[ta->cursor_pos];
                  is_alnum = isalnum(c) || isspace(c);
                  if (brsp_remove_utf8_after(&br->sp, ta->id, ta->cursor_pos) > 0) {
                    ta->changed = true;
                  }
                  if (false == is_alnum) break;
                  s = brsp_get(br->sp, ta->id);
                  c = s.str[ta->cursor_pos];
                  is_alnum = isalnum(c);
                } while (ta->cursor_pos < (int)s.len && br->key.ctrl_down && is_alnum);
              } break;
              case BR_KEY_BACKSPACE: {
                br_strv_t s = brsp_get(br->sp, ta->id);
                char c;
                bool is_alnum;
                do {
                  if (ta->cursor_pos == 0) break;
                  ta->cursor_pos = br_strv_utf8_add(strv, ta->cursor_pos, -1);
                  c = s.str[ta->cursor_pos];
                  is_alnum = isalnum(c) || isspace(c);
                  if (brsp_remove_utf8_after(&br->sp, ta->id, ta->cursor_pos) > 0) {
                    ta->changed = true;
                  }
                  if (false == is_alnum) break;
                  s = brsp_get(br->sp, ta->id);
                  c = s.str[ta->cursor_pos - 1];
                  is_alnum = isalnum(c);
                } while (ta->cursor_pos > 0 && br->key.ctrl_down && is_alnum);
              } break;
              case BR_KEY_HOME: {
                ta->cursor_pos = 0;
                ta->changed = true;
              } break;
              case BR_KEY_END: {
                ta->cursor_pos = (int)strv.len;
                ta->changed = true;
              } break;
              case BR_KEY_TAB: {
                if (ta->id == br->ui.fm_state.path_id) {
                  if (br->key.ctrl_down) {
                    if (br->ui.fm_state.select_index > 0) --br->ui.fm_state.select_index;
                  } else {
                    if (br->ui.fm_state.select_index < (int)br->ui.fm_state.cur_dir.len) ++br->ui.fm_state.select_index;
                  }
                  br->ui.fm_state.has_tabed = true;
                }
              } break;
              case BR_KEY_ENTER: {
                if (ta->id == br->ui.fm_state.path_id) {
                  br->ui.fm_state.has_entered = true;
                }
              } break;
              default: LOGI("text input %d (%d)", ev.key, ev.keycode); break;
            }
          } else {
            BR_TODO("active: %d", br->action.active);
          }
        }
      } break;
      case brpl_event_key_release: {
        if (ev.key == BR_KEY_LEFT_CONTROL) br->key.ctrl_down = false;
        else if (ev.key == BR_KEY_LEFT_SHIFT) br->key.shift_down = false;
        else if (ev.key == BR_KEY_LEFT_ALT) br->key.alt_down = false;
        else if (ev.key < 255) br->key.down[ev.key] = false;
        //LOGI("release %d (%d)", ev.key, ev.keycode);
      } break;
      case brpl_event_input: {
        if (br->action.active == br_plotter_entity_text_input) {
          if (ev.utf8_char < 32 || ev.utf8_char == 127) break;
          brui_action_t* action = brui_action();
          brsp_t* sp = &br->sp;
          brsp_id_t str_id = action->args.text.id;
          action->args.text.cursor_pos += brsp_insert_unicode(sp, str_id, action->args.text.cursor_pos, ev.utf8_char);
          action->args.text.changed = true;
        }
      } break;
      case brpl_event_mouse_press: {
        if (br->action.active == br_plotter_entity_text_input) {
          br->action.active = br_plotter_entity_none;
          brui_resizable_mouse_releasel(&br->resizables, br->mouse.pos);
        } else {
          if (ev.mouse_key == 0) {
            br->mouse.dragging_left = true;
            brui_resizable_mouse_pressl(&br->resizables, br->mouse.pos, br->key.ctrl_down);
            if (false == br->key.ctrl_down) {
              br->mouse.click = true;
            }
          } else if (ev.mouse_key == 3) {
            br->mouse.dragging_right = true;
            br->action = br->hovered;
          } else {
            LOGI("Mouse Press: %d", ev.mouse_key);
          }
        }
      } break;
      case brpl_event_mouse_release: {
        if (br->action.active != br_plotter_entity_text_input) {
          brui_resizable_mouse_releasel(&br->resizables, br->mouse.pos);
          br->action.active = br_plotter_entity_none;
        }
        br->mouse.dragging_left = false;
        br->mouse.dragging_right = false;
      } break;
      case brpl_event_mouse_move: {
        if (br->win.active) {
          if (br->mouse.active) br->mouse.delta = br_vec2_sub(ev.pos, br->mouse.pos);
          else                  br->mouse.delta = BR_VEC2(0, 0);
          br->mouse.active = true;
          br->mouse.pos = ev.pos;
          if (br->mouse.dragging_right) {
            if (br->action.active == br_plotter_entity_plot_2d) {
              br_plot_t* plot = br_da_getp(br->plots, br->action.plot_id);
              br_plot2d_move_screen_space(plot, br->mouse.delta, br->resizables.arr[plot->extent_handle].cur_extent.size);
            } else if (br->action.active == br_plotter_entity_plot_3d) {
              br_plot_t* plot = br_da_getp(br->plots, br->action.plot_id);
              float speed = (float)br->time.frame / 2.f;
              br_vec3_t zeroed = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
              br_vec3_t zero_dir = br_vec3_normalize(zeroed);
              br_vec3_t right = br_vec3_normalize(br_vec3_cross(plot->ddd.up, zero_dir));
              br_vec2_t m = br->mouse.delta;
              br_vec2_t md = br_vec2_scale(BR_VEC2(m.x, m.y), -speed);
              br_vec3_t rotated_up = br_vec3_rot(zeroed, plot->ddd.up, md.x);
              br_vec3_t rotated_right = br_vec3_rot(rotated_up, right, md.y);
              float horizontal_factor = fabsf(br_vec3_dot(br_vec3_normalize(rotated_right), plot->ddd.up));
              if (horizontal_factor > 0.94f) plot->ddd.eye = br_vec3_add(rotated_up,    plot->ddd.target);
              else                           plot->ddd.eye = br_vec3_add(rotated_right, plot->ddd.target);
            }
          } else {
            brui_resizable_mouse_move(&br->resizables, br->mouse.pos);
          }
          br->mouse.delta = BR_VEC2(0, 0);
        } else br->mouse.active = false;
      } break;
      case brpl_event_mouse_scroll:
        if (br->action.active == br_plotter_entity_none) {
          if (br->hovered.active == br_plotter_entity_plot_2d) {
            br_plot_t* plot = &br->plots.arr[br->hovered.plot_id];
            br_extent_t ex = br->resizables.arr[plot->extent_handle].cur_extent;
            br_vec2_t zoom = BR_VEC2(-ev.vec.y, -ev.vec.y);
            if (br->key.down[BR_KEY_X]) zoom.y = 0.f;
            if (br->key.down[BR_KEY_Y]) zoom.x = 0.f;
            br_plot2d_zoom(plot, zoom, ex, br->mouse.pos);
          } else if (br->hovered.active == br_plotter_entity_plot_3d) {
            br_plot_t* plot = &br->plots.arr[br->hovered.plot_id];
            float mw_scale = (1.0f + (float)br->time.frame*ev.vec.y*3.f);
            br_vec3_t zeroed = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
            br_vec3_t zero_dir = br_vec3_normalize(zeroed);
            float len = br_vec3_len(zeroed);
            len /= mw_scale;
            plot->ddd.eye = br_vec3_add(plot->ddd.target, br_vec3_scale(zero_dir, len));
            LOGI("scale: %f  len: %f, eye: %f %f %f", mw_scale, len, BR_VEC3_(plot->ddd.eye));
          } else {
            brui_resizable_mouse_scroll(&br->resizables, ev.vec);
          }
        } else {
          LOGW("Scrolling %d", br->action.active);
        }
        break;
      case brpl_event_window_focused:
        br->win.active = true;
        break;
      case brpl_event_window_unfocused:
        br->win.active = false;
        break;
      case brpl_event_window_shown:
        LOGI("SHOW");
        break;
      case brpl_event_window_hidden:
        LOGI("HIDE");
        br->win.active = false;
        break;
      case brpl_event_window_resize:
        br->win.viewport.size = BR_SIZE_TOI(ev.size);
        br->mouse.active = false;
        break;
      case brpl_event_close:
        br->should_close = true;
        break;
      case brpl_event_next_frame: {
        br->time.old = br->time.now;
        br->time.now = ev.time;
        br->time.frame = (br->time.now - br->time.old);

        int found_plot = -1;
        for (int j = 0; j < br->plots.len; ++j) {
          br_plot_t* plot = br_da_getp(br->plots, j);
          if (br->plots.arr[j].extent_handle == br->resizables.active_resizable) {
            switch (br->plots.arr[j].kind) {
              case br_plot_kind_2d: br->hovered.active = br_plotter_entity_plot_2d; break;
              case br_plot_kind_3d: br->hovered.active = br_plotter_entity_plot_3d; break;
              default: BR_TODO("Unknown plot kind %d", br->plots.arr[j].kind); break;
            }
            found_plot = j;
          }
          for (int i = 0; i < plot->data_info.len; ++i) {
            br_plot_data_t* pd = br_da_getp(plot->data_info, i);
            pd->thickness_multiplyer = br_float_lerp(pd->thickness_multiplyer, pd->thickness_multiplyer_target, (float)br->time.frame*5.0f);
          }
        }

        if (br->action.active == br_plotter_entity_none) {
          if (found_plot > -1) {
            br->hovered.plot_id = found_plot;
            br_plot_t* plot = &br->plots.arr[br->hovered.plot_id];
            if (br->hovered.active == br_plotter_entity_plot_2d) {
              br_vec2_t zoom = { 0 };
              if (br->key.down[BR_KEY_X]) {
                if (br->key.ctrl_down) zoom.x += -1.f;
                if (br->key.shift_down) zoom.x += 1.f;
              }
              if (br->key.down[BR_KEY_Y]) {
                if (br->key.ctrl_down) zoom.y += -1.f;
                if (br->key.shift_down) zoom.y += 1.f;
              }
              zoom.x *= 60.f*(float)br->time.frame;
              zoom.y *= 60.f*(float)br->time.frame;
              br_extent_t ex = br->resizables.arr[plot->extent_handle].cur_extent;
              br_plot2d_zoom(plot, zoom, ex, br->mouse.pos);
            } else if (br->hovered.active == br_plotter_entity_plot_3d) {
              br_vec3_t zeroed = br_vec3_sub(plot->ddd.eye, plot->ddd.target);
              br_vec3_t zero_dir = br_vec3_normalize(zeroed);
              br_vec3_t right = br_vec3_normalize(br_vec3_cross(plot->ddd.up, zero_dir));
              br_vec3_t delta = { 0 };
              float speed = 10.f;
              float dt = (float)br->time.frame;
              if (br->key.ctrl_down) speed *= 0.1f;
              if (br->key.shift_down) speed *= 10.f;
              if (br->key.down[BR_KEY_W]) delta = br_vec3_scale(zero_dir, -dt*speed);
              if (br->key.down[BR_KEY_S]) delta = br_vec3_add(delta, br_vec3_scale(zero_dir, dt*speed));
              if (br->key.down[BR_KEY_A]) delta = br_vec3_add(delta, br_vec3_scale(right, -dt*speed));
              if (br->key.down[BR_KEY_D]) delta = br_vec3_add(delta, br_vec3_scale(right, dt*speed));
              if (br->key.down[BR_KEY_Q]) delta = br_vec3_add(delta, br_vec3_scale(plot->ddd.up, -dt*speed));
              if (br->key.down[BR_KEY_E]) delta = br_vec3_add(delta, br_vec3_scale(plot->ddd.up, dt*speed));
              plot->ddd.target = br_vec3_add(plot->ddd.target, delta);
              plot->ddd.eye = br_vec3_add(plot->ddd.eye, delta);
            }
          } else {
            br->hovered.resizable_id = br->resizables.active_resizable;
            br->hovered.active = br_plotter_entity_ui;
          }
        }

        if (false == br->mouse.dragging_right) brui_resizable_update(&br->resizables, BR_EXTENTI_TOF(br->win.viewport));

        brpl_frame_start(&br->win);
#if BR_HAS_HOTRELOAD
        br_hotreload_tick(&br->hot_state);
#endif
        br_plotter_draw(br);
        br_dagens_handle(&br->groups, &br->dagens, &br->plots, brpl_time() + 0.010);
        br_shaders_draw_all(br->shaders);
        br_text_renderer_dump(br->text);
        handle_all_commands(br, br->commands);
#if BR_HAS_SHADER_RELOAD
        if (br->shaders_dirty) {
          br_shaders_refresh(br->shaders);
          br->shaders_dirty = false;
        }
#endif
        br_plotter_frame_end(br);
        if (brui_action()->kind == brui_action_sliding) {
          br->action.active = br_plotter_entity_ui;
        } else if (brui_action()->kind == brui_action_typing) {
          br->action.active = br_plotter_entity_text_input;
        }
        brgl_finish();
        brpl_frame_end(&br->win);
        return;
      } break;
      case brpl_event_scale: {
        LOGI("Window scale: %f %f", ev.size.width, ev.size.height);
      } break;
      default: BR_TODO("br_plotter_update active: %d, event: %d", br->action.active, ev.kind);
    }
    ev = brpl_event_next(&br->win);
    ++n;
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
    br->resizables.arr[br->plots.arr[i].extent_handle].target.hidden_factor = 0.f;
    brui_resizable_move_on_top(&br->resizables, br->plots.arr[i].extent_handle);
    return;
  }
  br_plotter_add_plot_2d(br);
}

void br_plotter_switch_3d(br_plotter_t* br) {
  for (int i = 0; i < br->plots.len; ++i) {
    if (br->plots.arr[i].kind != br_plot_kind_3d) continue;
    br->resizables.arr[br->plots.arr[i].extent_handle].target.hidden_factor = 0.f;
    brui_resizable_move_on_top(&br->resizables, br->plots.arr[i].extent_handle);
    return;
  }
  br_plotter_add_plot_3d(br);
}

int br_plotter_add_plot_2d(br_plotter_t* br) {
  br_plot_t plot = br_plot_2d(br->win.viewport.size, br->ui.theme.ui.default_grid_line_thickenss);
  br_plot_create_texture(&plot);
  plot.extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENTI_TOF(plot.cur_extent), 0, (brui_resizable_t) { .tag = 100, .title_enabled = true });
#if defined(__EMSCRIPTEN__)
  brui_resizable_maximize(plot.extent_handle, true);
#endif
  plot.menu_extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENT(0, 0, 300, (float)plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .current.tag = 101, .target.hidden_factor = 1.f });
  plot.legend_extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENT((float)plot.cur_extent.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { .current.tag = 102 });
  br_da_push_t(int, (br->plots), plot);
  return br->plots.len - 1;
}

int br_plotter_add_plot_3d(br_plotter_t* br) {
  int padding = 4;
  br_plot_t plot = {
    .data_info = { 0 },
    .cur_extent = BR_EXTENTI(padding, padding, br->win.viewport.width - 2 * padding, br->win.viewport.height - 2 * padding),
    .follow = false,
    .jump_around = false,
    .mouse_inside_graph = false,
    .kind = br_plot_kind_3d,
    .ddd =  {
      .eye = BR_VEC3(0, 0, 100),
      .target = BR_VEC3(0, 0, 0),
      .up = BR_VEC3(0, 1, 0),
      .fov_y = 1,
      .near_plane = 0.001f,
      .far_plane = 10e7f,
    }
  };
  br_plot_create_texture(&plot);
  plot.extent_handle = brui_resizable_new(&br->resizables, BR_EXTENT(500, 50, (float)br->win.viewport.width - 500 - 60, (float)br->win.viewport.height - 110), 0);
  plot.menu_extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENT(0, 0, 300, (float)plot.cur_extent.height), plot.extent_handle, (brui_resizable_t) { .target.hidden_factor = 1.f });
  plot.legend_extent_handle = brui_resizable_new2(&br->resizables, BR_EXTENT((float)plot.cur_extent.width - 110, 10, 100, 60), plot.extent_handle, (brui_resizable_t) { 0 });
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
        br_da_push_t(int, br->plots.arr[j].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
        any_added = true;
      } else if (br->plots.arr[j].kind == br_plot_kind_3d) {
        br_da_push_t(int, br->plots.arr[j].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
        any_added = true;
      }
    }
    if (false == any_added) {
      switch (br->groups.arr[i].kind) {
        case br_data_kind_2d: {
          int id = br_plotter_add_plot_2d(br);
          br_da_push_t(int, br->plots.arr[id].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
        } break;
        case br_data_kind_3d: {
          int id = br_plotter_add_plot_3d(br);
          br_da_push_t(int, br->plots.arr[id].data_info, BR_PLOT_DATA(br->groups.arr[i].group_id));
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
