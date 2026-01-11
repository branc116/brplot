#include "src/br_ui.h"
#include "include/br_free_list_header.h"
#include "include/br_str_header.h"
#include "include/br_string_pool_header.h"
#include "src/br_anim.h"
#include "src/br_da.h"
#include "src/br_math.h"
#include "src/br_memory.h"
#include "src/br_shaders.h"
#include "src/br_text_renderer.h"
#include "src/br_theme.h"

#include "external/stb_ds.h"

#define BR_THEME (brui_state.uiw->theme)
#define BRUI_DEF (brui_state.uiw->def)
#define TOP (brui_state.arr[brui_state.len ? brui_state.len - 1 : 0])
#define ACTION (brui_state.action.kind)
#define ACPARM (brui_state.action)
#define TOP2 (brui_state.arr[brui_state.len > 1 ? brui_state.len - 2 : 0])
#define Z (brui_state.max_z = br_i_max(brui_state.max_z, TOP.z), ++TOP.z)
#define ZGL BR_Z_TO_GL(Z)
#define BRUI_RS(RESIZABLE_HANDLE) (brui_state.uiw->resizables.arr[RESIZABLE_HANDLE])
#define BRUI_ANIMF(ANIM_HANDLE) br_animf(&brui_state.uiw->anims, (ANIM_HANDLE))
#define BRUI_ANIMFS(ANIM_HANDLE, VALUE) br_animf_set(&brui_state.uiw->anims, (ANIM_HANDLE), (VALUE))
#define BRUI_ANIMEX(ANIM_HANDLE) br_animex(&brui_state.uiw->anims, (ANIM_HANDLE))
#define BRUI_ANIMEXS(ANIM_HANDLE, VALUE) br_animex_set(&brui_state.uiw->anims, (ANIM_HANDLE), (VALUE))

#if 1
#define BRUI_LOG(fmt, ...) do { \
  if (brui_state.log == false) break; \
  for (size_t i = 0; i < brui_state.len; ++i) { \
    printf("  "); \
  } \
  if (brui_state.len > 0) { \
    printf("[CUR:%.3f,%.3f][LIMIT:%.2f,%.2f,%.2f,%.2f][PSUM:%.2f %.2f][TOPZ:%d,STARTZ:%d] " fmt "\n", TOP.cur_pos.x, TOP.cur_pos.y, BR_BB_(TOP.limit), TOP.psum.x, TOP.psum.y, TOP.z, TOP.start_z, ##__VA_ARGS__); \
  } else { \
    printf(fmt "\n", ##__VA_ARGS__); \
  } \
} while(0)
#else
#define BRUI_LOG(...)
#endif
#if 0
#  define BRUI_LOGI(...) do { \
  BR_STACKTRACE(); \
  LOGI(__VA_ARGS__); \
} while(0)
#  define BRUI_LOGV LOGI
#else
#  define BRUI_LOGI(...)
#  define BRUI_LOGV(...)
#endif

typedef struct {
  br_vec2_t cur_pos;
  float cur_content_height;
  br_bb_t limit;
  int start_z, z;
  // TODO: Write down why I need psum or remove it!
  br_vec2_t psum;

  br_vec2_t padding;

  int cur_resizable;
  int cur_vsplit;

  float vsplit_max_height;

  bool is_active;
  bool hide_border;
  bool hide_bg;
} brui_stack_el_t;

typedef struct brui_resizable_temp_state_t {
  int resizable_handle;
  bool was_drawn;
  bool is_deleted;
} brui_resizable_temp_state_t;

typedef struct brui_resizable_temp_t {
  size_t key;
  brui_resizable_temp_state_t value;
} brui_resizable_temp_t;

typedef struct {
  brui_stack_el_t* arr;
  size_t len, cap;

  brui_window_t* uiw;

  brui_resizable_temp_t* temp_res;
  bruir_children_t temp_children;

  br_str_t scrach;
  int rs_temp_last;
  br_strv_t rs_temp_last_str;

  int max_z;

  float snap_cooldown;

  brui_action_t action;

  bool select_next;
  bool log;
} brui_state_t;

static BR_THREAD_LOCAL brui_state_t brui_state;

static void brui_push_simple(brui_stack_el_t el);
static brtr_stack_el_t* brui_push_text(brui_stack_el_t el);
static brtr_stack_el_t* brui_pop_text(void);
static void brui_cur_set(float cur_x, float cur_y, float cur_content_height);
static void brui_cur_push(float height);

bool brui_window_init(brui_window_t* uiw) {
  brui_state.rs_temp_last = -1;
  brui_state.uiw = uiw;
  br_anims_construct(&uiw->def.animation_speed);

  if (false == brpl_window_open(&uiw->pl)) {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined( __NetBSD__) || defined(__DragonFly__)
    uiw->pl.kind = brpl_window_glfw;
    if (false == brpl_window_open(&uiw->pl)) {
      LOGF("Failed to open window either with x11 or glfw. Please install one of those two.");
    }
#elif defined(_WIN32)
    uiw->pl.kind = brpl_window_glfw;
    if (false == brpl_window_open(&uiw->pl)) {
      LOGF("Failed to open window either with native win32 windowing stuff or glfw. Please fix your PC.\n"
           "Or download glfw:\n"
           "  https://www.glfw.org/download\n"
           "And place the glfw3.dll next to the brplot.exe file in the file explorer");
    }
#else
    LOGF("Failed to open window with GLFW. Please install GLFW on your device.\nhttps://www.glfw.org/download");
#endif
  }

  brgl_disable_back_face_cull();
  brgl_enable_depth_test();
  brgl_enable(GL_BLEND);
  brgl_enable_clip_distance();
  brgl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  brgl_blend_equation(GL_FUNC_ADD);
  brgl_viewport(0, 0, uiw->pl.viewport.width, uiw->pl.viewport.height);

  uiw->shaders = br_shaders_malloc();
  brtr_construct(2*1024, 2*1024, &uiw->shaders);
  uiw->time.now = brpl_time();
#if BR_HAS_SHADER_RELOAD
  br_start_refreshing_shaders(&uiw->shaders_dirty, &uiw->pl.should_close);
#endif

  br_theme_reset_ui(&uiw->theme);
  br_theme_dark(&uiw->theme);
  brui_resizable_init(&uiw->resizables, BR_EXTENTI_TOF(uiw->pl.viewport));
  uiw->def.padding = BR_VEC2(4, 2);
  uiw->def.font_size = 20;
  uiw->def.border_thick = 1;
  uiw->def.animation_speed = 10.f;
  uiw->inited = true;
  ACPARM.text.offset_ahandle = br_animf_new(&uiw->anims, 0, 0);
  return true;
}

bool brui_window_deinit(brui_window_t* uiw) {
  BR_ASSERT(brui_state.len == 0);
  br_da_free(brui_state);
  br_da_free(brui_state.temp_children);
  brfl_free(uiw->resizables);

  brsp_free(&uiw->sp);
  br_anims_delete(&uiw->anims);
  brtr_free();
  br_shaders_free(uiw->shaders);
  brpl_window_close(&uiw->pl);
  brfl_free(uiw->touch_points);
  memset(uiw, 0, sizeof(*uiw));
  uiw->pl.should_close = true;
  return true;
}

static void brui_resizable_set_ancor(int res_id, int sibling_id, brui_ancor_t ancor);
static int bruir_find_at(bruirs_t* rs, int index, br_vec2_t loc, br_vec2_t* out_local_pos);
brpl_event_t brui_event_next(brui_window_t* uiw) {
  brpl_event_t ev = brpl_event_next(&uiw->pl);
  brui_action_t* ta = brui_action();
  if (false == uiw->pl.active) {
    ta->kind = brui_action_none;
  }
  switch (ev.kind) {
    case brpl_event_key_press: {
      if (ev.key == BR_KEY_LEFT_CONTROL)    uiw->key.ctrl_down = true;
      else if (ev.key == BR_KEY_LEFT_SHIFT) uiw->key.shift_down = true;
      else if (ev.key == BR_KEY_LEFT_ALT)   uiw->key.alt_down = true;
      else {
        if (ev.key < 255)                uiw->key.down[ev.key] = true;
        if (ev.key == BR_KEY_ESCAPE) {
          ta->kind = brui_action_none;
          brui_resizable_mouse_releasel(&uiw->resizables, uiw->mouse.pos);
        } else {
          if (ta->kind == brui_action_typing) {
            br_strv_t strv = brsp_get(uiw->sp, ta->text.id);
            switch (ev.key) {
              case BR_KEY_LEFT: {
                do {
                  ta->text.cursor_pos = br_strv_utf8_add(strv, ta->text.cursor_pos, -1);
                } while (ta->text.cursor_pos > 0 && uiw->key.ctrl_down && isalnum(strv.str[ta->text.cursor_pos]));
              } break;
              case BR_KEY_RIGHT: {
                do {
                  ta->text.cursor_pos = br_strv_utf8_add(strv, ta->text.cursor_pos, 1);
                } while (ta->text.cursor_pos < (int)strv.len && uiw->key.ctrl_down && isalnum(strv.str[ta->text.cursor_pos]));
              } break;
              case BR_KEY_ESCAPE: {
                ta->kind = brui_action_none;
              } break;
              case BR_KEY_DELETE: {
                ta->text.changed = brsp_remove_utf8_after(&uiw->sp, ta->text.id, ta->text.cursor_pos) > 0;
                br_strv_t s = brsp_get(uiw->sp, ta->text.id);
                char c;
                bool is_alnum;
                do {
                  if (ta->text.cursor_pos == (int)s.len) break;
                  c = s.str[ta->text.cursor_pos];
                  is_alnum = isalnum(c) || isspace(c);
                  if (brsp_remove_utf8_after(&uiw->sp, ta->text.id, ta->text.cursor_pos) > 0) {
                    ta->text.changed = true;
                  }
                  if (false == is_alnum) break;
                  s = brsp_get(uiw->sp, ta->text.id);
                  c = s.str[ta->text.cursor_pos];
                  is_alnum = isalnum(c);
                } while (ta->text.cursor_pos < (int)s.len && uiw->key.ctrl_down && is_alnum);
              } break;
              case BR_KEY_BACKSPACE: {
                br_strv_t s = brsp_get(uiw->sp, ta->text.id);
                char c;
                bool is_alnum;
                do {
                  if (ta->text.cursor_pos == 0) break;
                  ta->text.cursor_pos = br_strv_utf8_add(strv, ta->text.cursor_pos, -1);
                  c = s.str[ta->text.cursor_pos];
                  is_alnum = isalnum(c) || isspace(c);
                  if (brsp_remove_utf8_after(&uiw->sp, ta->text.id, ta->text.cursor_pos) > 0) {
                    ta->text.changed = true;
                  }
                  if (false == is_alnum) break;
                  s = brsp_get(uiw->sp, ta->text.id);
                  c = s.str[ta->text.cursor_pos - 1];
                  is_alnum = isalnum(c);
                } while (ta->text.cursor_pos > 0 && uiw->key.ctrl_down && is_alnum);
              } break;
              case BR_KEY_HOME: {
                ta->text.cursor_pos = 0;
                ta->text.changed = true;
              } break;
              case BR_KEY_END: {
                ta->text.cursor_pos = (int)strv.len;
                ta->text.changed = true;
              } break;
              default: LOGI("text input %d (%d)", ev.key, ev.keycode); break;
            }
          } else {
            switch (ev.key) {
              case BR_KEY_U:         brui_log(true);                                           break;
              case BR_KEY_PAGE_UP:   brui_resizable_page(&uiw->resizables, BR_VEC2(0, -1.f));  break;
              case BR_KEY_PAGE_DOWN: brui_resizable_page(&uiw->resizables, BR_VEC2(0, 1.f));   break;
              case BR_KEY_END:       brui_resizable_scroll_percent_set(&uiw->resizables, 1.f); break;
              case BR_KEY_HOME:      brui_resizable_scroll_percent_set(&uiw->resizables, 0.f); break;
            }
          }
        }
      }
    } break;
    case brpl_event_key_release: {
      if (ev.key == BR_KEY_LEFT_CONTROL) uiw->key.ctrl_down = false;
      else if (ev.key == BR_KEY_LEFT_SHIFT) uiw->key.shift_down = false;
      else if (ev.key == BR_KEY_LEFT_ALT) uiw->key.alt_down = false;
      else if (ev.key < 255) uiw->key.down[ev.key] = false;
    } break;
    case brpl_event_input: {
      if (ta->kind == brui_action_typing) {
        if (ev.utf8_char < 32 || ev.utf8_char == 127) break;
        brui_action_t* action = brui_action();
        brsp_t* sp = &uiw->sp;
        brsp_id_t str_id = action->text.id;
        action->text.cursor_pos += brsp_insert_unicode(sp, str_id, action->text.cursor_pos, ev.utf8_char);
        action->text.changed = true;
      }
    } break;
    case brpl_event_mouse_press: {
      if (uiw->time.now - uiw->touch_points.last_touch_time < 1) break;
      if (ta->kind == brui_action_typing) {
        ta->kind = brui_action_none;
        brui_resizable_mouse_releasel(&uiw->resizables, uiw->mouse.pos);
      } else {
        if (ev.mouse_key == 0) {
          uiw->mouse.dragging_left = true;
          brui_resizable_mouse_pressl(&uiw->resizables, uiw->mouse.pos, uiw->key.ctrl_down);
          if (false == uiw->key.ctrl_down) {
            uiw->mouse.click = true;
          }
        } else if (ev.mouse_key == 3) {
          uiw->mouse.dragging_right = true;
        } else {
          LOGI("Mouse Press: %d", ev.mouse_key);
        }
      }
    } break;
    case brpl_event_mouse_release: {
      if (uiw->time.now - uiw->touch_points.last_touch_time < 1) break;
      if (ta->kind != brui_action_typing) {
        brui_resizable_mouse_releasel(&uiw->resizables, uiw->mouse.pos);
      }
      uiw->mouse.dragging_left = false;
      uiw->mouse.dragging_right = false;
    } break;
    case brpl_event_mouse_move: {
      if (uiw->time.now - uiw->touch_points.last_touch_time < 1) break;
      if (uiw->pl.active) {
        if (uiw->mouse.active) uiw->mouse.delta = br_vec2_sub(ev.pos, uiw->mouse.pos);
        else                   uiw->mouse.delta = BR_VEC2(0, 0);
        uiw->mouse.active = true;
        uiw->mouse.pos    = ev.pos;
        brui_resizable_mouse_move(&uiw->resizables, uiw->mouse.pos);
      } else uiw->mouse.active = false;
    } break;
    case brpl_event_mouse_scroll: {
      brui_resizable_mouse_scroll(&uiw->resizables, ev.vec);
    } break;
    case brpl_event_window_resize: {
      uiw->mouse.active = false;
    } break;
    case brpl_event_frame_next: {
      uiw->time.old = uiw->time.now;
      uiw->time.now = ev.time;
      uiw->time.frame = (float)(uiw->time.now - uiw->time.old);

      if (false == uiw->mouse.dragging_right) brui_resizable_update(&uiw->resizables, BR_EXTENTI_TOF(uiw->pl.viewport));
    } break;
    case brpl_event_touch_begin: {
      uiw->touch_points.last_touch_time = uiw->time.now;
      brpl_touch_point_t* tpp = NULL;
      brfl_foreach(i, uiw->touch_points) {
        brpl_touch_point_t tp = uiw->touch_points.arr[i];
        if (tp.id != ev.touch.id) continue;
        tpp = &uiw->touch_points.arr[i];
      }
      if (uiw->touch_points.free_len == 0) {
        uiw->touch_points.last_free_time = uiw->time.now;
      }
      if (tpp == NULL) (void)brfl_push(uiw->touch_points, ev.touch);
    } break;
    case brpl_event_touch_update: {
      uiw->touch_points.last_touch_time = uiw->time.now;

      brpl_touch_point_t* tpp = NULL;
      brfl_foreach(i, uiw->touch_points) {
        brpl_touch_point_t tp = uiw->touch_points.arr[i];
        if (tp.id != ev.touch.id) continue;
        tpp = &uiw->touch_points.arr[i];
      }
      if (tpp == NULL) break;
      if (uiw->touch_points.free_len == 3) {
        br_vec2_t points_old[3] = { tpp->pos };
        br_vec2_t points_new[3] = { ev.touch.pos };
        int index = 1;
        brfl_foreach(i, uiw->touch_points) {
          brpl_touch_point_t tp = uiw->touch_points.arr[i];
          if (tp.id == ev.touch.id) continue;
          points_new[index] = points_old[index] = uiw->touch_points.arr[i].pos;
          ++index;
        }
        float old_max_x = br_float_max3(points_old[0].x, points_old[1].x, points_old[2].x);
        float old_min_x = br_float_min3(points_old[0].x, points_old[1].x, points_old[2].x);
        float new_max_x = br_float_max3(points_new[0].x, points_new[1].x, points_new[2].x);
        float new_min_x = br_float_min3(points_new[0].x, points_new[1].x, points_new[2].x);

        float old_max_y = br_float_max3(points_old[0].y, points_old[1].y, points_old[2].y);
        float old_min_y = br_float_min3(points_old[0].y, points_old[1].y, points_old[2].y);
        float new_max_y = br_float_max3(points_new[0].y, points_new[1].y, points_new[2].y);
        float new_min_y = br_float_min3(points_new[0].y, points_new[1].y, points_new[2].y);

        float new_x_mid = new_max_x*0.5f + new_min_x*0.5f;
        float new_y_mid = new_max_y*0.5f + new_min_y*0.5f;
        int drag_index = uiw->resizables.drag_index;
        if (uiw->resizables.drag_mode == brui_drag_mode_none) {
          uiw->mouse.pos = ev.pos;
          uiw->resizables.drag_mode = brui_drag_mode_touch;
          uiw->resizables.active_resizable = bruir_find_at(&uiw->resizables, 0, BR_VEC2(new_x_mid, new_y_mid), &(br_vec2_t) {0});
          drag_index = uiw->resizables.drag_index = uiw->resizables.active_resizable;
          brui_resizable_set_ancor(drag_index, 0, brui_ancor_none);
        }
        brui_resizable_t* r = br_da_getp(uiw->resizables, drag_index);
        br_extent_t ex = br_animex_get_target(&brui_state.uiw->anims, r->cur_extent_ah);
        // Handle Zoom
        {
          float old_w = old_max_x - old_min_x;
          float new_w = new_max_x - new_min_x;
          float old_h = old_max_y - old_min_y;
          float new_h = new_max_y - new_min_y;
          float dw = new_w - old_w;
          float dh = new_h - old_h;
          LOGI("dw: %f, dh: %f", dw, dh);
          ex.width += dw;
          ex.height += dh;
        }
        // Handle offset
        {
          float old_x_mid = old_max_x*0.5f + old_min_x*0.5f;
          float old_y_mid = old_max_y*0.5f + old_min_y*0.5f;
          ex.x += new_x_mid - old_x_mid;
          ex.y += new_y_mid - old_y_mid;
        }
        br_animex_set(&uiw->anims, r->cur_extent_ah, ex);
        tpp->pos = ev.touch.pos;
      } else if (uiw->touch_points.free_len == 1) {
        //uiw->resizables.active_resizable = bruir_find_at(&uiw->resizables, 0, ev.touch.pos, &(br_vec2_t) {0});
        br_vec2_t diff = br_vec2_sub(ev.touch.pos, tpp->pos);
        LOGI("diff %f %f", BR_VEC2_(diff));
        brui_resizable_mouse_scroll_px(&uiw->resizables, diff);
        tpp->pos = ev.touch.pos;
        uiw->mouse.pos = ev.touch.pos;
      } else {
        uiw->resizables.drag_mode = brui_drag_mode_none;
      }
    } break;
    case brpl_event_touch_end: {
      uiw->touch_points.last_touch_time = uiw->time.now;
      int to_remove = -1;
      brfl_foreach(i, uiw->touch_points) {
        brpl_touch_point_t tp = uiw->touch_points.arr[i];
        if (tp.id != ev.touch.id) continue;
        to_remove = i;
        break;
      }
      if (to_remove >= 0) {
        brpl_touch_point_t point = br_da_get(uiw->touch_points, to_remove);
        brfl_remove(uiw->touch_points, to_remove);
        if (uiw->touch_points.free_len == 0) {
          double diff = uiw->time.now - uiw->touch_points.last_free_time;
          if (diff < 0.5) {
            uiw->mouse.pos = ev.pos;
            uiw->mouse.click = true;
            brui_resizable_mouse_move(&uiw->resizables, point.pos);
            brui_resizable_mouse_pressl(&uiw->resizables, point.pos, uiw->key.ctrl_down);
            brui_resizable_mouse_releasel(&uiw->resizables, point.pos);
          }
        }
      }
    } break;
    default: break;
  }
  return ev;
}

void brui_frame_start(brui_window_t* uiw) {
  br_anims_tick(&uiw->anims, uiw->time.frame);
  brpl_frame_start(&uiw->pl);
  brgl_clear(BR_COLOR_COMPF(uiw->theme.colors.bg));
  brui_begin();
}

void brui_frame_end(brui_window_t* uiw) {
  brui_end();
  br_shaders_draw_all(uiw->shaders);
#if BR_HAS_SHADER_RELOAD
  if (uiw->shaders_dirty) {
    br_shaders_refresh(uiw->shaders);
    uiw->shaders_dirty = false;
  }
#endif
  brpl_frame_end(&uiw->pl);
  uiw->mouse.delta = BR_VEC2(0, 0);
  uiw->mouse.click = false;
}

bool brui_buttonf(const char* fmt, ...) {
  brui_state.scrach.len = 0;
  va_list args;
  va_start(args, fmt);
  br_str_printfvalloc(&brui_state.scrach, fmt, args);
  va_end(args);

  va_start(args, fmt);
  br_strv_t strv = br_str_printfv(&brui_state.scrach, fmt, args);
  va_end(args);

  return brui_button(strv);
}

brui_stack_el_t brui_stack_el(void) {
  if (brui_state.len > 0) {
    brui_stack_el_t new_el = TOP;
    new_el.psum.x += TOP.padding.x;
    new_el.cur_pos.x = new_el.limit.min_x + new_el.padding.x;
    new_el.cur_pos.y += TOP.padding.y;
    new_el.limit.min.y += TOP.padding.y;
    new_el.limit.max.y -= TOP.padding.y;
    new_el.limit.max.x -= TOP.padding.y;
    new_el.start_z = new_el.z;
    new_el.hide_border = false;
    new_el.hide_bg = false;
    new_el.cur_content_height = 2 * new_el.padding.y;
    return new_el;
  } else {
    br_extent_t viewport = BRUI_ANIMEX(BRUI_RS(0).cur_extent_ah);
    brui_stack_el_t root = {
      .limit = BR_EXTENT_TOBB(viewport),
      .padding = BRUI_DEF.padding,
      .psum = BRUI_DEF.padding,
      .is_active = true
    };
    root.limit.max = br_vec2_sub(root.limit.max, root.padding);
    root.limit.min = br_vec2_add(root.limit.min, root.padding);
    root.cur_pos = root.padding;
    root.cur_content_height = root.padding.y;
    return root;
  }
}

#if TRACY_ENABLE
static BR_THREAD_LOCAL struct ___tracy_source_location_data brui_begin_end_bench = { "UI Begin End", "Static",  __FILE__, (uint32_t)__LINE__, 0 };
TracyCZoneCtx brui_begin_end_ctx;
#endif

void brui_begin(void) {
#if TRACY_ENABLE
  brui_begin_end_ctx = ___tracy_emit_zone_begin_callstack(&brui_begin_end_bench, 0, true);
#endif
  BRUI_LOG("BEGIN");
  brtr_stack_el_t* el = brui_push_text(brui_stack_el());
  el->viewport = BRUI_ANIMEX(BRUI_RS(0).cur_extent_ah);
  el->limits = TOP.limit;
  el->ancor = br_dir_left_up;
  el->pos = TOP.cur_pos;
  el->font_size = BRUI_DEF.font_size;
  el->z = 1;
  el->forground = BR_THEME.colors.btn_txt_inactive;
  el->background = BR_THEME.colors.plot_menu_color;
  brui_state.uiw->shaders.rect->uvs.spred_uv = brui_state.uiw->theme.shadow_spred;
  brui_state.uiw->shaders.rect->uvs.px_round_uv = brui_state.uiw->theme.px_round;
  brui_state.uiw->shaders.rect->uvs.shadow_intesity_uv = brui_state.uiw->theme.shadow_intesity * (brui_state.uiw->theme.colors.highlite_factor > 0 ? 1.f : -1.f);

  TOP.is_active = 0 == brui_state.uiw->resizables.active_resizable;
  // TODO: This should be pushed but rn, something breaks and I get the balck screen after a few seconds.
  //       Most likely z buffer shit..
  //brui_resizable_push(0);
  BRUI_LOG("beginafter");
}

void brui_end(void) {
  BRUI_LOG("end");
  //brui_resizable_pop();
  BR_ASSERT(brui_state.len == 1);
  brui_pop_text();
#if TRACY_ENABLE
  ___tracy_emit_zone_end(brui_begin_end_ctx);
#endif
  brui_state.log = false;
}

void brui_push(void) {
  BRUI_LOG("PUSH");
  Z;
  brtr_stack_el_t* el = brui_push_text(brui_stack_el());
  Z;
  /*
  if (TOP.is_active) el->background = br_color_lighter(el->background, BR_THEME.colors.highlite_factor*0.2f);
  else el->background = br_color_lighter(el->background, BR_THEME.colors.highlite_factor*0.1f);
  */
  el->background = br_color_lighter(el->background, BR_THEME.colors.highlite_factor*0.1f);
  el->limits = TOP.limit;
  el->pos = TOP.cur_pos;
}

brui_pop_t brui_pop(void) {
  br_color_t bg = brtr_state()->background;

  float width = BR_BBW(TOP.limit) - 2 * fminf(TOP2.psum.x, TOP.psum.x);
  float height = TOP.cur_content_height;
  br_size_t size = BR_SIZE(width, height);
  br_vec2_t c2 = TOP2.cur_pos;
  br_bb_t bb = BR_BB(c2.x, c2.y, c2.x + size.width, c2.y + size.height);
  float content_height = TOP.cur_content_height;
  bool clicked = false;
  bool hovered = br_col_vec2_bb(bb, brui_state.uiw->mouse.pos);

  float factor = 1 + 0.3f*(float)brui_state.len;
  if (TOP.is_active) {
    if (hovered) {
      factor *= 1.5f;
      clicked = brui_state.uiw->mouse.click;
    } else factor *= 1.1f;
  }
  bg = br_color_lighter(bg, BR_THEME.colors.highlite_factor*0.01f*(factor - 1));
  if (TOP.hide_bg == false) brui_background(bb, bg);
  if (TOP.hide_border == false) brui_border1(bb);

  BRUI_LOG("Pre pop ex: [%.2f,%.2f,%.2f,%.2f]", BR_BB_(bb));
  brui_pop_text();
  brui_cur_push(content_height + TOP.padding.y);
  BRUI_LOG("pop");
  return (brui_pop_t) { .clicked = clicked, .hovered = hovered, .bb = bb };
}

void brui_select_next(void) {
  brui_state.select_next = true;
}

BR_TEST_ONLY static inline bool brui_extent_is_good(br_extent_t e, br_extent_t parent) {
  return e.x >= 0 &&
    e.y >= 0 &&
    e.y + e.height <= parent.height &&
    e.x + e.width <= parent.width &&
    e.height > 100 &&
    e.width > 100;
}

br_size_t brui_text(br_strv_t strv) {
  BRUI_LOG("Text: %.*s", strv.len, strv.str);
  brtr_stack_el_t* el = brtr_state_push();
    el->z = Z;
    el->limits.max.x -= TOP.psum.x;
    br_size_t size = brtr_push(strv).size;
  brtr_state_pop();
  brui_cur_push(size.height + TOP.padding.y);
  return size;
}

br_size_t brui_textf(const char* fmt, ...) {
  brui_state.scrach.len = 0;
  va_list args;
  va_start(args, fmt);
  br_str_printfvalloc(&brui_state.scrach, fmt, args);
  va_end(args);

  va_start(args, fmt);
  br_strv_t strv = br_str_printfv(&brui_state.scrach, fmt, args);
  va_end(args);

  return brui_text(strv);
}

void brui_text_at(br_strv_t strv, br_vec2_t at) {
  float font_size = brui_text_size();
  br_size_t size = brtr_measure(strv);
  size.height += font_size;
  br_bb_t text_limit = TOP.limit;
  float padd = TOP.padding.x * 2;
  br_vec2_t at_og = at;
  br_bb_t rect = { 0 };
  br_vec2_t b = { 0 }, c = { 0 };

  if (at.x - size.width - padd * 3 > text_limit.min_x) {
    at.x -= size.width + padd * 2;
    at.y -= size.height * 0.5f;
    rect = BR_BB(at.x - padd, at.y, at.x + size.width, at.y + font_size);
    b = BR_VEC2(rect.max_x, rect.min_y);
    c = BR_VEC2(rect.max_x, rect.max_y);
  } else if (at.y - size.height > text_limit.min_y) {
    at.x = text_limit.min_x + padd;
    at.y -= size.height * 1.5f;
    rect = BR_BB(at.x - padd, at.y, at.x + size.width + padd, at.y + font_size);
    b.x = at_og.x - font_size*.5f;
    c.x = at_og.x + font_size*.5f;
    float diffb = rect.min_x - b.x;
    float diffc = c.x - rect.max_x;
    float diff = br_float_max(diffb, diffc);
    if (diff > 0) {
      rect.max_y += diff;
      rect.min_y += diff;
    }
    b.y = rect.max_y;
    c.y = rect.max_y;
    if (diffb > 0) {
      b.x = rect.min_x;
      b.y -= diffb;
    }
    if (diffc > 0) {
      c.x = rect.max_x;
      c.y -= diffc;
    }
  }
  brtr_rectangle_draw(rect);
  brtr_triangle_draw(at_og, b, c);
  brtr_state()->pos = BR_VEC2(rect.min_x + padd, rect.min_y);
  brtr_push(strv);
}

bool brui_text_input(brsp_id_t str_id) {
  brsp_t* sp = &brui_state.uiw->sp;
  br_strv_t strv = brsp_get(*sp, str_id);
  bool is_active = brui_action_typing == ACTION && str_id == ACPARM.text.id;

  br_vec2_t loc = TOP.cur_pos;
  loc.x -= BRUI_ANIMF(ACPARM.text.offset_ahandle);

  if (is_active)       brui_cur_set(loc.x, loc.y, TOP.cur_content_height);
  float font_size      = brui_text_size();
  float opt_height     = font_size + TOP.padding.y;
  br_extent_t ex       = BR_EXTENT(TOP.cur_pos.x, TOP.cur_pos.y, TOP.limit.max_x - TOP.cur_pos.x, font_size);
  float half_thick     = 1.0f;
  bool changed         = false;

  brtr_push(strv);

  float text_height = font_size;
  brui_cur_push(opt_height);
  if (is_active) {
    br_size_t size = brtr_measure(br_str_sub(strv, 0, (uint32_t)ACPARM.text.cursor_pos));
    loc.x += size.width;
    brtr_stack_el_t* el = brtr_state_push();
      el->background = el->forground;
      el->limits.min_x -= TOP.padding.x;
      brtr_rectangle_draw(BR_BB(loc.x, loc.y, loc.x + 2*half_thick, loc.y + text_height));
    brtr_state_pop();
    int off_ahandle = ACPARM.text.offset_ahandle;
    float offt = br_animf_get_target(&brui_state.uiw->anims, off_ahandle);
    float off = br_animf(&brui_state.uiw->anims, off_ahandle);
    if (loc.x - (offt - off) > TOP.limit.max_x) BRUI_ANIMFS(off_ahandle, offt + (TOP.limit.max_x - TOP.limit.min_x)/2);
    if (loc.x - (offt - off) < TOP.limit.min_x - TOP.padding.x) BRUI_ANIMFS(off_ahandle, offt - (TOP.limit.max_x - TOP.limit.min_x)/2);
    changed = ACPARM.text.changed;
    ACPARM.text.changed = false;
  } else {
    if (TOP.is_active) {
      if (brui_state.uiw->mouse.click) {
        if (br_col_vec2_bb(BR_EXTENT_TOBB(ex), brui_state.uiw->mouse.pos)) {
          ACTION = brui_action_typing;
          ACPARM.text.cursor_pos = 0;
          ACPARM.text.id = str_id;
          BRUI_ANIMFS(ACPARM.text.offset_ahandle, 0.f);
        }
      }
    }
  }
  return changed;
}

void brui_new_lines(int n) {
  brui_cur_push((brui_text_size() + TOP.padding.y) * (float)n);
}

void brui_background(br_bb_t bb, br_color_t color) {
  Z;
  brtr_stack_el_t* el = brtr_state_push();
    el->background = color;
    el->z = TOP.start_z;
    brtr_rectangle_draw(bb);
  brtr_state_pop();
}

void brui_border1(br_bb_t bb) {
  brui_border2(bb, TOP.is_active && br_col_vec2_bb(bb, brui_state.uiw->mouse.pos));
}

void brui_border2(br_bb_t bb, bool active) {
	(void)bb; (void)active;
	/*
  br_color_t ec = BR_THEME.colors.ui_edge_inactive;
  br_color_t bc = BR_THEME.colors.ui_edge_bg_inactive;

  float edge = 0;
  float thick = BRUI_DEF.border_thick;

  if (active) {
    ec = BR_THEME.colors.ui_edge_active;
    bc = BR_THEME.colors.ui_edge_bg_active;
  }

  int z = Z;
  br_sizei_t viewport = BR_SIZE_TOI(BRUI_ANIMEX(BRUI_RS(0).cur_extent_ah));

  //br_icons_draw(BR_BB(bb.min_x + edge, bb.min_y, bb.max_x - edge, bb.min_y + thick), BR_EXTENT_TOBB(br_extra_icons.edge_t.size_8), ec, bc, TOP.limit, z, viewport);
  //br_icons_draw(BR_BB(bb.min_x + edge, bb.max_y - thick, bb.max_x - edge, bb.max_y), BR_EXTENT_TOBB(br_extra_icons.edge_b.size_8), ec, bc, TOP.limit, z, viewport);
  //br_icons_draw(BR_BB(bb.min_x, bb.min_y + edge, bb.min_x + thick, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_l.size_8), ec, bc, TOP.limit, z, viewport);
  //br_icons_draw(BR_BB(bb.max_x - thick, bb.min_y + edge, bb.max_x, bb.max_y - edge), BR_EXTENT_TOBB(br_extra_icons.edge_r.size_8), ec, bc, TOP.limit, z, viewport);
  */
}

bool brui_collapsable(br_strv_t name, bool* expanded) {
  float font_size = brui_text_size();
  //float opt_header_height /* text + 2*1/2*padding */ = font_size + TOP.padding.y;
  brui_push();
    brui_vsplitvp(2, BRUI_SPLITR(1), BRUI_SPLITA(1.3f*font_size));
      if (brui_button(name)) *expanded = !*expanded;
    brui_vsplit_pop();
      if (*expanded) {
        if (brui_button(BR_STRL("V"))) *expanded = false;
      } else {
        if (brui_button(BR_STRL("<"))) *expanded = true;
      }
    brui_vsplit_pop();
  if (!*expanded) {
    TOP.hide_bg = true;
    brui_pop();
  }
  return *expanded;
}

void brui_collapsable_end(void) {
  brui_pop();
}

bool brui_button(br_strv_t text) {
  // BUTTON
  float font_size = brui_text_size();
  float opt_height /* text + 2*1/2*padding */ = font_size + TOP.padding.y;
  float button_max_x = TOP.limit.max_x - TOP.psum.x;
  float button_max_y = TOP.cur_pos.y + opt_height;
  br_bb_t button_limit = BR_BB(TOP.cur_pos.x, TOP.cur_pos.y, button_max_x, button_max_y);
  bool hovers = (br_col_vec2_bb(button_limit, brui_state.uiw->mouse.pos) && TOP.is_active);
  bool is_selected = ACTION != brui_action_sliding && (hovers || brui_state.select_next);

  brui_push();
    brtr_stack_el_t* el = brtr_state();
    el->background = is_selected ? BR_THEME.colors.btn_hovered : BR_THEME.colors.btn_inactive;
    el->forground = is_selected ? BR_THEME.colors.btn_txt_hovered : BR_THEME.colors.btn_txt_inactive;
    el->justify = br_dir_mid_up;
    el->ancor = br_dir_mid_up;
    brui_text(text);
    el = brtr_state();
    brui_cur_push((float)el->font_size*0.2f);
  bool is_click = brui_pop().clicked;
  return TOP.is_active && hovers && false == brui_state.uiw->key.ctrl_down && is_click;
}

bool brui_checkbox(br_strv_t text, bool* checked) {
  brtr_stack_el_t* el = brtr_state();
  float font_size = (float)el->font_size;
  float sz = font_size * 0.6f;
  br_bb_t cb_extent = BR_BB(TOP.cur_pos.x, TOP.cur_pos.y + font_size * 0.2f, TOP.cur_pos.x + sz, TOP.cur_pos.y + sz + font_size * 0.2f);
  float top_out /* neg or 0 */ = fminf(0.f, TOP.cur_pos.y - TOP.limit.min_y);
  float opt_height = font_size + TOP.padding.y;
  bool hover = false;

  brui_push_simple(TOP);
    br_extent_t icon = *checked ?  br_icon_cb_1(font_size) : br_icon_cb_0(font_size);
    hover = br_col_vec2_bb(cb_extent, brui_state.uiw->mouse.pos);
    el = brtr_state_push();
      el->background = hover ? BR_THEME.colors.btn_hovered : BR_THEME.colors.btn_inactive;
      el->forground  = hover ? BR_THEME.colors.btn_txt_hovered : BR_THEME.colors.btn_txt_inactive;
      el->z = Z;
      brtr_glyph_draw(cb_extent, icon);
    brtr_state_pop();
    el = brtr_state_push();
      el->pos.x += TOP.padding.x + sz;
      el->limits.max_y = fminf(TOP.limit.max_y, TOP.cur_pos.y + opt_height + top_out);
      el->ancor = br_dir_left_up;
      el->z = Z;
      brui_text(text);
    brtr_state_pop();
  brui_pop_simple();

  brui_cur_push(opt_height);
  if (hover && brui_state.uiw->mouse.click) {
    *checked = !*checked;
    return true;
  }
  return false;
}

void brui_texture(unsigned int texture_id) {
  brui_push_simple(TOP);
    TOP.limit.min.x += TOP.psum.x;
    TOP.limit.max.x -= TOP.psum.x;
    br_shader_img_t* img = brui_state.uiw->shaders.img;
    img->uvs.image_uv = texture_id;
    
    float gl_z = BR_Z_TO_GL(Z);
    br_size_t viewport = BRUI_ANIMEX(BRUI_RS(0).cur_extent_ah).size;
    br_vec2_t a = TOP.cur_pos;
    br_vec2_t b = BR_VEC2(TOP.limit.max.x, a.y);
    br_vec2_t c = BR_VEC2(TOP.limit.max.x, TOP.limit.max.y);
    br_vec2_t d = BR_VEC2(a.x, TOP.limit.max.y);
    br_shader_img_push_quad(img, (br_shader_img_el_t[4]) {
        { .pos = BR_VEC42(br_vec2_stog(a, viewport), BR_VEC2(0, 1)), .z = gl_z },
        { .pos = BR_VEC42(br_vec2_stog(b, viewport), BR_VEC2(1, 1)), .z = gl_z },
        { .pos = BR_VEC42(br_vec2_stog(c, viewport), BR_VEC2(1, 0)), .z = gl_z },
        { .pos = BR_VEC42(br_vec2_stog(d, viewport), BR_VEC2(0, 0)), .z = gl_z },
    });
    br_shader_img_draw(img);
  brui_pop_simple();
}

void brui_framebuffer(unsigned int framebuffer_id) {
  brui_texture(brgl_framebuffer_to_texture(framebuffer_id));
}

void brui_icon(float size, br_extent_t icon, br_color_t forground, br_color_t background) {
  brtr_stack_el_t* el = brtr_state_push();
    el->background = background;
    el->forground = forground;
    brtr_glyph_draw(BR_BB(TOP.cur_pos.x, TOP.cur_pos.y, TOP.cur_pos.x + size, TOP.cur_pos.y + size), icon);
  brtr_state_pop();
}

bool brui_button_icon(br_size_t size, br_extent_t icon) {
  bool is_pressed = false;
  float out_top /* neg or 0 */ = fminf(0.f, TOP.cur_pos.y - TOP.limit.min_y);
  float opt_height = size.height + TOP.padding.y;
  float height = size.height + out_top;
  br_bb_t bb = BR_BB(TOP.cur_pos.x, TOP.cur_pos.y, TOP.cur_pos.x + size.width, TOP.cur_pos.y + height);

  brtr_stack_el_t* el = brtr_state_push();
    el->z = Z;
    if (bb.max_y > bb.min_y && bb.max_x > bb.min_x) {
      bool hovered = (TOP.is_active && br_col_vec2_bb(bb, brui_state.uiw->mouse.pos));
      if (brui_state.select_next || hovered) {
        el->forground  = BR_THEME.colors.btn_txt_hovered;
        el->background = BR_THEME.colors.btn_hovered;
      } else {
        el->forground  = BR_THEME.colors.btn_txt_inactive;
        el->background = BR_THEME.colors.btn_inactive;
      }
      is_pressed = brui_state.uiw->mouse.click && hovered;
    }
    brtr_glyph_draw(bb, icon);
  brtr_state_pop();
  brui_cur_push(opt_height);
  brui_state.select_next = false;
  return is_pressed;
}

bool brui_triangle(br_vec2_t a, br_vec2_t b, br_vec2_t c, br_color_t color, int z) {
  brtr_stack_el_t* el = brtr_state_push();
    el->background = color;
    el->z = z;
    brtr_triangle_draw(a, b, c);
  brtr_state_pop();
  return true;
}

bool brui_rectangle(br_bb_t bb, br_color_t color, int z) {
  brtr_stack_el_t* el = brtr_state_push();
    el->background = color;
    el->z = z;
    brtr_rectangle_draw(bb);
  brtr_state_pop();
  return true;
}

bool brui_sliderf(br_strv_t text, float* val) {
  // SLIDERF
  BRUI_LOG("Slider");
  brtr_stack_el_t* el = brui_push_text(TOP);
    TOP.padding.y *= 0.1f;
    el->font_size = (int)((float)el->font_size * 0.8f);
    float font_size = (float)el->font_size;
    const float lt = font_size * 0.3f;
    const float ss = lt + 3.f;
    const float opt_height /* 2*text + 4*padding + ss */ = 2 * font_size + 4 * TOP.padding.y + ss;
    const float opt_max_y = TOP.cur_pos.y + opt_height;
    const float opt_next_content_height = TOP.cur_content_height + opt_height + TOP.padding.y;

    brui_push();
      TOP.limit.min_y = fmaxf(TOP.limit.min_y, TOP.cur_pos.y);
      TOP.limit.max_y = fminf(TOP.limit.max_y, opt_max_y);
      brtr_state()->limits = TOP.limit;
      BRUI_LOG("Slider font: %d, pady: %.2f, lt: %.2f", el->font_size, TOP.padding.y, lt);
      el = brtr_state_push();
        brui_text(text);
        el->justify = br_dir_mid_y;
        el->pos.y -= font_size;
        brui_textf("%f", text.len, text.str, *val);
      brtr_state_pop();

      br_vec2_t mouse = brui_state.uiw->mouse.pos;
      bool is_down = brui_state.uiw->mouse.click;

      TOP.cur_pos.y = TOP.limit.min_y + 2.0f*font_size + TOP.padding.y;
      const float lw = BR_BBW(TOP.limit) - 2 * TOP.psum.x;
      br_bb_t line_bb = BR_BB(TOP.cur_pos.x, TOP.cur_pos.y + (ss - lt)*0.5f, TOP.cur_pos.x + lw, TOP.cur_pos.y + (ss + lt)*0.5f);
      br_color_t lc = BR_THEME.colors.btn_inactive;
      br_bb_t slider_bb = BR_BB(TOP.cur_pos.x + (lw - ss)*.5f, TOP.cur_pos.y, TOP.cur_pos.x + (lw + ss)*.5f, TOP.cur_pos.y + ss);
      br_color_t sc = BR_THEME.colors.btn_txt_inactive;

      if (ACTION == brui_action_sliding && ACPARM.slider.value == val) {
        lc = BR_THEME.colors.btn_active;
        sc = BR_THEME.colors.btn_txt_active;
        float speed = brui_state.uiw->time.frame;
        float factor = br_float_clamp((mouse.x - line_bb.min_x) / lw, 0.f, 1.f) * speed + (1 - speed * 0.5f);
        *val *= factor;
        slider_bb.min_x = br_float_clamp(mouse.x, line_bb.min_x, line_bb.max_x) - ss * 0.5f;
        slider_bb.max_x = slider_bb.min_x + ss;
      } else if (ACTION == brui_action_none && br_col_vec2_bb(line_bb, mouse)) {
        lc = BR_THEME.colors.btn_hovered;
        sc = BR_THEME.colors.btn_txt_hovered;
        if (is_down) {
          ACPARM.slider.value = val;
          ACTION = brui_action_sliding;
        }
      }

      BRUI_LOG("Slider inner %f %f %f %f", BR_BB_(line_bb));

      el = brtr_state_push();
        el->z = Z;
        el->background = lc;
        brtr_rectangle_draw(line_bb);
        el->background = sc;
        brtr_rectangle_draw(slider_bb);
      brtr_state_pop();
    //brui_cur_push(opt_height + TOP.padding.y);
    brui_pop();
  brui_pop_text();
  brui_cur_set(TOP.limit.min_x, opt_max_y + TOP.padding.y, opt_next_content_height);
  brui_cur_push(TOP.padding.y);
  return false;
}

bool brui_sliderf2(br_strv_t text, float* value) {
  return brui_sliderf3(text, value, 2);
}

bool brui_sliderf3(br_strv_t text, float* value, int percision) {
  float font_size = brui_text_size();
  float opt_height /* text + 2*1/2*padding */ = font_size + TOP.padding.y;
  brui_state.scrach.len = 0;
  br_strv_t value_str = br_str_printf(&brui_state.scrach, "%.*f", percision, *value);
  br_size_t size = brtr_measure(value_str);
  size.width += TOP.padding.x * 2.f;
  br_bb_t bb = BR_BB(TOP.cur_pos.x, TOP.cur_pos.y, TOP.limit.max_x - TOP.psum.x, TOP.cur_pos.y + opt_height);
  br_vec2_t mouse_pos = brui_state.uiw->mouse.pos;
  br_color_t background = BR_THEME.colors.btn_inactive;
  bool hovers = br_col_vec2_bb(bb, mouse_pos);
  if (hovers) background = BR_THEME.colors.btn_hovered;
  if (ACTION == brui_action_sliding && ACPARM.slider.value == value) {
    *value *= 1.f - brui_state.uiw->time.frame * (ACPARM.slider.drag_ancor_point.x - mouse_pos.x) / 1000.f;
    background = BR_THEME.colors.btn_active;
  } else if (ACTION == brui_action_none && brui_state.uiw->mouse.click && hovers) {
    ACTION = brui_action_sliding;
    ACPARM.slider.value = value;
    ACPARM.slider.drag_ancor_point = mouse_pos;
  }
  brui_push();
    brtr_state()->background = background;
    brui_vsplitvp(2, BRUI_SPLITR(1), BRUI_SPLITA(size.width));
      brui_text(text);
    brui_vsplit_pop();
      brui_text(value_str);
    brui_vsplit_pop();
    brui_cur_push(font_size*0.2f);
  brui_pop();
  return false;
}

bool brui_slideri(br_strv_t text, int* value) {
  float text_size = brui_text_size();
  float opt_height /* text + 2*1/2*padding */ = text_size + TOP.padding.y;
  brui_state.scrach.len = 0;
  br_strv_t value_str = br_str_printf(&brui_state.scrach, "%d", *value);
  br_size_t size = brtr_measure(value_str);
  size.width += TOP.padding.x * 2.f;
  float avaliable_width = BR_BBW(TOP.limit) - TOP.psum.x * 2.f;
  br_bb_t bb = BR_BB(TOP.cur_pos.x, TOP.cur_pos.y, TOP.limit.max_x - TOP.psum.x, TOP.cur_pos.y + opt_height);
  brtr_stack_el_t* el = brui_push_text(TOP);
    TOP.limit.min_y = fmaxf(TOP.cur_pos.y, TOP.limit.min_y);
    TOP.limit.max_y = fminf(TOP.limit.min_y + opt_height, TOP.limit.max_y);
    el->limits = TOP.limit;
    el->ancor = br_dir_mid_mid;
    el->justify = br_dir_mid_mid;
    if (size.width < avaliable_width) {
      brui_vsplitvp(4, BRUI_SPLITR(1), BRUI_SPLITA(size.width), BRUI_SPLITA(text_size), BRUI_SPLITA(text_size));
        brui_text(text);
      brui_vsplit_pop();
        brui_text(value_str);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("-"))) --(*value);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("+"))) ++(*value);
      brui_vsplit_pop();
    }
  brui_pop_text();
  brui_border1(bb);
  brui_cur_push(opt_height + TOP.padding.y);
  return false;
}

void brui_vsplit(int n) {
  BRUI_LOG("vsplit %d", n);
  brui_stack_el_t top = TOP;
  float width = (BR_BBW(TOP.limit) - 2*TOP.psum.x) / (float)n;
  for (int i = 0; i < n; ++i) {
    brui_stack_el_t new_el = top;
    new_el.z += 5;
    new_el.psum.x = 0;
    new_el.limit.min_x = (float)(n - i - 1) * width + top.cur_pos.x;
    new_el.limit.max_x = new_el.limit.min_x + width;
    new_el.cur_pos.x = new_el.limit.min.x;
    new_el.hide_border = true;
    new_el.vsplit_max_height = 0;
    new_el.cur_content_height = 0;
    new_el.cur_vsplit = top.cur_vsplit + 1;
    brtr_stack_el_t* el = brui_push_text(new_el);
    el->pos = TOP.cur_pos;
    el->limits = TOP.limit;
    el->z = TOP.z + 1;
  }
}

void brui_vsplitarr(int n, brui_split_t* splits) {
  float absolute = 0;
  float relative = 0;

  for (int i = 0; i < n; ++i) {
    brui_split_t t = splits[i];
    switch (t.kind) {
      case brui_split_absolute: absolute += t.absolute; break;
      case brui_split_relative: relative += t.relative; break;
      default: BR_UNREACHABLE("kind: %d", t.kind);
    }
  }

  float space_left = BR_BBW(TOP.limit) - TOP.psum.x * 2;
  float space_after_abs = space_left - absolute;
  if (space_after_abs < 0) {
    brui_vsplit(n);
    return;
  }

  brui_stack_el_t top = TOP;
  top.z += 5;
  top.psum.x = 0;
  top.hide_border = true;
  top.vsplit_max_height = 0;
  top.cur_content_height = 0;
  float cur_x = top.limit.max_x - TOP.psum.x;
  for (int i = n - 1; i >= 0; --i) {
    brui_stack_el_t new_el = top;
    brui_split_t t = splits[i];
    float width = 0;
    switch (t.kind) {
      case brui_split_relative: width = space_after_abs * (t.relative / relative); break;
      case brui_split_absolute: width = t.absolute; break;
      default: BR_UNREACHABLE("kind: %d", t.kind);
    }
    new_el.limit.max_x = cur_x;
    new_el.limit.min_x = cur_x - width;
    new_el.cur_pos.x = new_el.limit.min.x;
    new_el.cur_vsplit = top.cur_vsplit + 1;
    cur_x -= width;
    brtr_stack_el_t* el = brui_push_text(new_el);
    el->pos = TOP.cur_pos;
    el->limits = TOP.limit;
    el->z = TOP.z + 1;
  }
}

void brui_vsplitvp(int n, ...) {
  static BR_THREAD_LOCAL struct { brui_split_t* arr; size_t len, cap; } splits = { 0 };
  splits.len = 0;
  {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; ++i) {
      brui_split_t t = va_arg(ap, brui_split_t);
      br_da_push(splits, t);
    }
    va_end(ap);
  }

  brui_vsplitarr((int)splits.len, splits.arr);
}

bool brui_vsplit_pop(void) {
  BRUI_LOG("vsplit pre pop");
  int cur_vsplit = TOP.cur_vsplit;
  float cur_max = TOP.vsplit_max_height;
  float cur_ch = TOP.cur_content_height;
  brui_pop_text();
  bool more = cur_vsplit == TOP.cur_vsplit;
  float max_ch = fmaxf(cur_max, cur_ch);
  if  (more) TOP.vsplit_max_height = max_ch;
  else              brui_cur_push(max_ch + TOP.padding.y);
  BRUI_LOG("vsplit post pop");
  return more;
}

void brui_scroll_bar(int bar_offset_fract_ah) {
  float thick = TOP.padding.x * 0.5f;
  float slider_thick = TOP.padding.x * 0.8f;
  br_vec2_t mouse = brui_state.uiw->mouse.pos;
  bool is_down = brui_state.uiw->mouse.click;

  float cur = br_animf(&brui_state.uiw->anims, bar_offset_fract_ah);
  float limit_height = BR_BBH(TOP.limit);
  float hidden_height = TOP.cur_content_height - limit_height;
  float slider_height = limit_height * (limit_height / (limit_height + hidden_height));
  float slider_upper = cur * (limit_height - slider_height);
  float slider_downer = (1.f - cur) * (limit_height - slider_height);

  br_bb_t line_bb = BR_BB(TOP.limit.max_x - thick, TOP.limit.min_y, TOP.limit.max_x, TOP.limit.max_y);
  br_bb_t slider_bb = BR_BB(TOP.limit.max_x - slider_thick, TOP.limit.min_y + slider_upper, TOP.limit.max_x, TOP.limit.max_y - slider_downer);
  br_color_t lc = BR_THEME.colors.btn_inactive;
  br_color_t bc = BR_THEME.colors.btn_txt_inactive;
  if (ACTION == brui_action_sliding && ACPARM.slider.value == (void*)(size_t)bar_offset_fract_ah) {
    lc = BR_THEME.colors.btn_active;
    bc = BR_THEME.colors.btn_txt_active;
    float new = br_float_clamp((mouse.y - TOP.limit.min_y) / limit_height, 0, 1);
    BRUI_ANIMFS(bar_offset_fract_ah, new);
  } else if (ACTION == brui_action_none && br_col_vec2_bb(line_bb, mouse)) {
    lc = BR_THEME.colors.btn_hovered;
    bc = BR_THEME.colors.btn_txt_hovered;
    ACTION = brui_action_none;
    if (is_down) {
      ACTION = brui_action_sliding;
      ACPARM.slider.value = (void*)(size_t)bar_offset_fract_ah;
    }
  }
  brtr_stack_el_t* el = brtr_state_push();
    el->limits.max_x += TOP.padding.x;
    el->background = lc;
    el->z = Z;
    brtr_rectangle_draw(line_bb);
    el->background = bc;
    brtr_rectangle_draw(slider_bb);
  brtr_state_pop();
}

void  brui_text_justify_set(br_dir_t ancor) {
  brtr_state()->justify = ancor;
}

void brui_text_ancor_set(br_dir_t ancor) {
  brtr_state()->ancor = ancor;
}

void brui_text_color_set(br_color_t color) {
  brtr_state()->forground = color;
}

void brui_ancor_set(brui_ancor_t ancor) {
  brui_resizable_set_ancor(TOP.cur_resizable, 0, ancor);
}

float brui_text_size(void) {
  return (float)brtr_state()->font_size;
}

void brui_text_size_set(float size) {
  brtr_state()->font_size = (int)size;
}

void brui_z_set(int z) {
  TOP.z = z;
  brtr_state()->z = z;
}

void brui_maxy_set(float value) {
  TOP.limit.max_y = value;
}

void brui_scroll_move(float value) {
  brui_resizable_t* res = br_da_getp(brui_state.uiw->resizables, TOP.cur_resizable);
  float hidden_height = res->full_height - BRUI_ANIMEX(res->cur_extent_ah).height;
  if (hidden_height <= 0) return;

  float cur_scroll_px = hidden_height * br_animf(&brui_state.uiw->anims, res->scroll_offset_percent_ah);
  float next_scroll_px = cur_scroll_px + value;
  if (next_scroll_px < 0) next_scroll_px = 0.f;
  if (next_scroll_px > hidden_height) next_scroll_px = hidden_height;
  BRUI_ANIMFS(res->scroll_offset_percent_ah, next_scroll_px / hidden_height);
}

float brui_min_y(void) {
  return TOP.limit.min_y;
}

float brui_max_y(void) {
  return TOP.limit.max_y;
}

void brui_height_set(float value) {
  TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.cur_pos.y + value);
  TOP.limit.min_y = fmaxf(TOP.limit.min_y, TOP.cur_pos.y);
  brtr_state()->limits = TOP.limit;
}

void brui_padding_set(br_vec2_t value) {
  TOP.padding = value;
}

float brui_padding_x(void) {
  return TOP.padding.x;
}

float brui_padding_y(void) {
  return TOP.padding.y;
}

void brui_padding_y_set(float value) {
  TOP.padding.y = value;
}

int brui_z(void) {
  return Z;
}

br_vec2_t brui_pos(void) {
  return TOP.cur_pos;
}

float brui_y(void) {
  return TOP.cur_pos.y;
}

float brui_width(void) {
  return TOP.limit.max_x - TOP.limit.min_x - 2*TOP.psum.x;
}

float brui_height(void) {
  return TOP.limit.max_y - TOP.limit.min_y;
}

float brui_height_left(void) {
  return TOP.limit.max_y - TOP.cur_pos.y - TOP.padding.y;
}

float brui_local_y(void) {
  return TOP.cur_pos.y - TOP.limit.min_y;
}

bool brui_active(void) {
  return TOP.is_active;
}

void brui_debug(void) {
  brui_state.log = true;
}

void brui_log(bool should_log) {
  LOGI("LOG. Action: %d", ACTION);

  brui_state.log = ACTION == brui_action_none && should_log;
}

brui_action_t* brui_action(void) {
  return &brui_state.action;
}

void brui_action_stop(void) {
  ACTION = brui_action_none;
}

// ---------------------------Resizables--------------------------
static int bruir_find_at(bruirs_t* rs, int index, br_vec2_t loc, br_vec2_t* out_local_pos);
static void bruir_update_extent(bruirs_t* rs, int index, br_extent_t new_ex);

void brui_resizable_init(bruirs_t* rs, br_extent_t extent) {
  brui_resizable_t screen = { 0 };
  screen.cur_extent_ah = br_animex_new(&brui_state.uiw->anims, extent, extent);
  screen.scroll_offset_percent_ah = br_animf_new(&brui_state.uiw->anims, 0, 0);
  br_anim_instant(&brui_state.uiw->anims, screen.cur_extent_ah);

  screen.ancor = brui_ancor_all;
  screen.z = 0;
  (void)brfl_push(*rs, screen);
}

static int brui_resizable_new0(bruirs_t* rs, brui_resizable_t* new, br_extent_t init_extent, int parent) {
  BR_ASSERTF(rs->len > parent, "Len: %d, parent: %d", rs->len > parent, parent);
  BR_ASSERT(rs->free_arr[parent] == -1);
  new->z = brui_resizable_children_top_z(rs, parent) + 1;
  new->parent = parent;
  new->cur_extent_ah = br_animex_new(&brui_state.uiw->anims, init_extent, init_extent);
  new->title_height_ah = br_animf_new(&brui_state.uiw->anims, 0, 0);
  new->scroll_offset_percent_ah = br_animf_new(&brui_state.uiw->anims, 0, 0);
  int new_id = brfl_push(*rs, *new);

  if (parent > 0) bruir_update_extent(rs, new_id, init_extent);
  return new_id;
}

int brui_resizable_new(bruirs_t* rs, br_extent_t init_extent, int parent) {
  brui_resizable_t new = { 0 };
  new.title_enabled = true;

  return brui_resizable_new0(rs, &new, init_extent, parent);
}

int brui_resizable_new2(bruirs_t* rs, br_extent_t init_extent, int parent, brui_resizable_t template) {
  brui_resizable_t new = template;
  return brui_resizable_new0(rs, &new, init_extent, parent);
}

static void brui_resizable_check_parents(bruirs_t* rs);
void brui_resizable_delete(int handle) {
  brui_resizable_t resizable = br_da_get(brui_state.uiw->resizables, handle);
  if (resizable.parent != handle) {
    bruir_children_t children = brui_resizable_children_temp(handle);
    for (int i = 0; i < children.len; ++i) brui_resizable_delete(children.arr[i]);

    if (resizable.ancor != brui_ancor_none) brui_resizable_set_ancor(handle, 0, brui_ancor_none);
  }

  brsp_remove(&brui_state.uiw->sp, BRUI_RS(handle).title_id);
  br_anim_delete(&brui_state.uiw->anims, resizable.cur_extent_ah);
  br_anim_delete(&brui_state.uiw->anims, resizable.title_height_ah);
  br_anim_delete(&brui_state.uiw->anims, resizable.scroll_offset_percent_ah);
  brfl_remove(brui_state.uiw->resizables, handle);
  brui_resizable_check_parents(&brui_state.uiw->resizables);
}

static bool brui_snap_area(brui_ancor_t ancor, br_bb_t bb, br_vec2_t mouse_pos, int r_id, int sibling_id, bruirs_t* rs, float light_f) {
  bool is_in = false;
  if (br_col_vec2_bb(bb, mouse_pos)) {
    rs->drag_point = mouse_pos;
    if (brui_state.len == 0) {
      brui_resizable_set_ancor(r_id, sibling_id, ancor);
    }
    is_in = true;
  } else light_f = 0;
  if (brui_state.len > 0) {
    brtr_stack_el_t* el = brtr_state_push();
      el->background = br_color_lighter(el->background, light_f);
      brtr_rectangle_draw(bb);
    brtr_state_pop();
  }
  return is_in;
}

static bool brui_snap_areas(br_vec2_t mouse_pos, int r_id, bruirs_t* rs) {
  if (brui_state.snap_cooldown > 0) return false;
  brui_resizable_t* r = br_da_getp(*rs, r_id);
  brui_resizable_t p = br_da_get(*rs, r->parent);
  br_extent_t pex = brui_resizable_cur_extent(r->parent);
  br_bb_t pbb = BR_EXTENT_TOBB(pex);
  br_color_t base_color = BR_COLOR(25, 200, 25, 64);
  float light_f = 1.4f;

  float snap_sib_height = 7;
  float snap_sib_width = 30;
  if (p.tag == brui_resizable_tag_ancor_helper) return false;
  brtr_stack_el_t* el = brtr_state_push();
    el->background = base_color;
    el->limits = pbb;
    el->z = r->max_z + 2;
    brfl_foreach(i, *rs) {
      if (i == 0) continue;
      if (i == r_id) continue;
      brui_resizable_t* sib = br_da_getp(*rs, i);
      if (sib->parent != r->parent) continue;
      if (brui_resizable_is_hidden(i)) continue;
      br_extent_t sex = brui_resizable_cur_extent(i);
      br_bb_t snap_bb = { 0 };
      {
        snap_bb.min.y = sex.y - snap_sib_height;
        snap_bb.max.y = sex.y + snap_sib_height;
        snap_bb.min.x = sex.x + sex.width / 2 - snap_sib_width;
        snap_bb.max.x = sex.x + sex.width / 2 + snap_sib_width;
        if (brui_snap_area(brui_ancor_top, snap_bb, mouse_pos, r_id, i, rs, light_f)) return true;
        snap_bb.min.y += sex.height;
        snap_bb.max.y += sex.height;
        if (brui_snap_area(brui_ancor_bottom, snap_bb, mouse_pos, r_id, i, rs, light_f)) return true;
      }
      {
        snap_bb.min.x = sex.x - snap_sib_height;
        snap_bb.max.x = sex.x + snap_sib_height;
        snap_bb.min.y = sex.y + sex.height / 2 - snap_sib_width;
        snap_bb.max.y = sex.y + sex.height / 2 + snap_sib_width;
        if (brui_snap_area(brui_ancor_left, snap_bb, mouse_pos, r_id, i, rs, light_f)) return true;
        snap_bb.min.x += sex.width;
        snap_bb.max.x += sex.width;
        if (brui_snap_area(brui_ancor_right, snap_bb, mouse_pos, r_id, i, rs, light_f)) return true;
      }
    }

    br_vec2_t center = BR_VEC2(pex.x + pex.width * .5f, pex.y + pex.height * .5f);
    float halfs = 10.f;
    float pad = 2.f;

    brui_snap_area(brui_ancor_all,          BR_BB(center.x - halfs, center.y - halfs, center.x + halfs, center.y + halfs), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_right,        BR_BB(center.x + halfs + pad, center.y - halfs, center.x + halfs + pad + halfs, center.y + halfs), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_left,         BR_BB(center.x - halfs - pad - halfs, center.y - halfs, center.x - pad - halfs, center.y + halfs), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_top,          BR_BB(center.x - halfs, center.y - halfs - pad - halfs, center.x + halfs, center.y - halfs - pad), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_bottom,       BR_BB(center.x - halfs, center.y + halfs + pad, center.x + halfs, center.y + halfs + pad + halfs), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_right_top,    BR_BB(center.x + halfs + pad, center.y - halfs - pad - halfs, center.x + halfs + pad + halfs, center.y - halfs - pad), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_left_top,     BR_BB(center.x - halfs - pad - halfs, center.y - halfs - pad - halfs, center.x - halfs - pad, center.y - halfs - pad), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_right_bottom, BR_BB(center.x + halfs + pad, center.y + halfs + pad, center.x + halfs + pad + halfs, center.y + halfs + pad + halfs), mouse_pos, r_id, 0, rs, light_f);
    brui_snap_area(brui_ancor_left_bottom,  BR_BB(center.x - halfs - pad - halfs, center.y + halfs + pad, center.x - halfs - pad, center.y + halfs + pad + halfs), mouse_pos, r_id, 0, rs, light_f);
  brtr_state_pop();
  return br_col_vec2_bb(BR_BB(center.x - 3*halfs - pad, center.y - 3*halfs - pad, center.x + 3*halfs + pad, center.y + 3*halfs + pad), mouse_pos);
}


static void bruir_update_extent(bruirs_t* rs, int index, br_extent_t new_ex) {
  brui_resizable_check_parents(rs);
  brui_resizable_t* res = br_da_getp(*rs, index);
  BRUI_ANIMEXS(res->cur_extent_ah, new_ex);
}

void brui_resizable_update(bruirs_t* rs, br_extent_t viewport) {
  bruir_update_extent(rs, 0, viewport);
  brui_state.snap_cooldown -= brui_state.uiw->time.frame;

  if (rs->drag_mode == brui_drag_mode_none) {
    rs->active_resizable = bruir_find_at(rs, 0, brui_state.uiw->mouse.pos, &(br_vec2_t) {0});
  }

  brfl_foreach(i, *rs) {
    brui_resizable_t* res = br_da_getp(*rs, i);
    br_extent_t target_ex = br_animex_get_target(&brui_state.uiw->anims, res->cur_extent_ah);
    if (i == 0) continue;
    if (res->ancor == brui_ancor_none) {
      brui_resizable_t parent = BRUI_RS(res->parent);
      if (parent.ancor != brui_ancor_hidden) {
        br_extent_t pex = BRUI_ANIMEX(parent.cur_extent_ah);
        br_extent_t target_pex = br_animex_get_target(&brui_state.uiw->anims, parent.cur_extent_ah);
        float max_x = br_float_max(pex.width, target_pex.width);
        float max_y = br_float_max(pex.height, target_pex.height);
        if (target_ex.x + target_ex.width > max_x) target_ex.x -= target_ex.x + target_ex.width - max_x;
        if (target_ex.x < 0) target_ex.x = 0;
        if (target_ex.width > max_x) target_ex.width = max_x;
        if (target_ex.y + target_ex.height > max_y) target_ex.y -= target_ex.y + target_ex.height - max_y;
        if (target_ex.y < 0) target_ex.y = 0;
        if (target_ex.height > max_y) target_ex.height = max_y;
        BRUI_ANIMEXS(res->cur_extent_ah, target_ex);
      }
    } else {
      br_extent_t ex = BRUI_ANIMEX(res->cur_extent_ah);
      br_extent_t pex = BRUI_ANIMEX(BRUI_RS(res->parent).cur_extent_ah);
      pex.x = 0;
      pex.y = 0;
      if (res->ancor == brui_ancor_hidden) {
        target_ex.size  = BR_SIZE(0, 0);
        target_ex.pos   = BR_VEC2(0, 0);
      } else if (res->ancor == brui_ancor_right) {
        target_ex = pex;
        target_ex.width *= 0.5f;
        target_ex.x += target_ex.width;
      } else if (res->ancor == brui_ancor_left) {
        target_ex = pex;
        target_ex.width *= 0.5f;
      } else if (res->ancor == brui_ancor_bottom) {
        target_ex = pex;
        target_ex.height *= 0.5f;
        target_ex.y += target_ex.height;
      } else if (res->ancor == brui_ancor_top) {
        target_ex = pex;
        target_ex.height *= 0.5f;
      } else if (res->ancor == brui_ancor_right_top) {
        target_ex.x = pex.width - ex.width;
        target_ex.y = 0;
      } else if (res->ancor == brui_ancor_left_top) {
        target_ex.x = 0;
        target_ex.y = 0;
      } else if (res->ancor == brui_ancor_right_bottom) {
        target_ex.x = pex.width - ex.width;
        target_ex.y = pex.height - ex.height;
      } else if (res->ancor == brui_ancor_left_bottom) {
        target_ex.x = 0;
        target_ex.y = pex.height - ex.height;
      } else if (res->ancor == brui_ancor_all) {
        target_ex = pex;
      }
    }
    BRUI_ANIMEXS(res->cur_extent_ah, target_ex);
  }

  brfl_foreach(i, *rs) {
    brui_resizable_t* r = br_da_getp(*rs, i);
    r->was_draw_last_frame = true;
    if (r->tag == brui_resizable_tag_ancor_helper) r->max_z = 0;
  }

  brfl_foreach(i, *rs) {
    brui_resizable_t* r = br_da_getp(*rs, i);
    brui_resizable_t* parent = br_da_getp(*rs, r->parent);
    if (parent->tag == brui_resizable_tag_ancor_helper) parent->max_z = br_i_max(parent->max_z, r->max_z);
  }

  for (ptrdiff_t i = 0; i < stbds_hmlen(brui_state.temp_res); ++i) {
    brui_resizable_temp_state_t* state = &brui_state.temp_res[i].value;
    if (state->is_deleted == true) continue;
    if (false == state->was_drawn) {
      brui_resizable_delete(state->resizable_handle);
      state->is_deleted = true;
    } else {
      state->was_drawn = false;
    }
  }
}

static void brui_resizable_check_parents(bruirs_t* rs) {
#if BR_DEBUG
  brfl_foreach(i, *rs) {
    int parent = i;
    for (int j = 0; j < 16; ++j) {
      if (parent == 0) goto next;
      BR_ASSERT(false == brfl_is_free(*rs, parent));
      parent = rs->arr[parent].parent;
      if (parent == j) goto next;
    }
    brui_resizable_t r = br_da_get(*rs, i);

    br_strv_t title = { 0 };
    if (r.title_id > 0) brsp_get(brui_state.uiw->sp, r.title_id);
    if (r.title_id <= 0 || (title.str == 0 && title.len == 0) || title.len > 0xFFFFFF) title = BR_STRL("Unnnamed");
    BR_ASSERTF(0, "Resizable has no parent: id=%d, name=%.*s, parent_id=%d", i, title.len, title.str, parent);
next:;
  }
  brfl_foreach(i, *rs) {
    brui_resizable_t r = br_da_get(*rs, i);
    if (r.tag == brui_resizable_tag_ancor_helper) {
      int count = 0;
      static BR_THREAD_LOCAL br_str_t s = { 0 };
      s.len = 0;
      brfl_foreach(j, *rs) {
        brui_resizable_t r2 = br_da_get(*rs, j);
        if (r2.parent == j) {
          count = 2;
          break;
        }
        if (r2.parent == i) {
          count += 1;
          br_str_push_int(&s, j);
          br_str_push_char(&s, ' ');
        }
      }
      br_strv_t title = brsp_get(brui_state.uiw->sp, r.title_id);
      if (r.title_id <= 0 || (title.str == 0 && title.len == 0) || title.len > 0xFFFFFF) title = BR_STRL("Unnnamed");
      BR_ASSERTF(count == 2, "Thing with id of %d `%.*s` had %d children ( %.*s )", i, title.len, title.str, count, s.len, s.str);
    }
  }
  BRUI_LOGV("Check ok");
#else
  (void)rs;
#endif
}

void brui_resizable_mouse_move(bruirs_t* rs, br_vec2_t mouse_pos) {
  if (rs->drag_mode == brui_drag_mode_none) return;

  int drag_index = rs->drag_index;
  brui_resizable_t* r = br_da_getp(*rs, drag_index);
  br_extent_t new_extent = rs->drag_old_ex;
  if (rs->drag_mode == brui_drag_mode_touch) {
    return;
  } else if (rs->drag_mode == brui_drag_mode_move) {
    new_extent.pos = br_vec2_sub(rs->drag_old_ex.pos, br_vec2_sub(rs->drag_point, mouse_pos));
    if (r->ancor != brui_ancor_none) {
      if (br_vec2_len2(br_vec2_sub(new_extent.pos, rs->drag_old_ex.pos)) > 10*10) {
        rs->drag_old_ex.pos = rs->drag_point = mouse_pos;
        brui_resizable_set_ancor(drag_index, 0, brui_ancor_none);
      }
    }

    if (r->tag != brui_resizable_tag_ancor_helper) {
      brui_snap_areas(mouse_pos, drag_index, rs);
    }
  } else {
    if (rs->drag_mode & brui_drag_mode_left) {
      float dif = rs->drag_point.x - mouse_pos.x;
      new_extent.width  = rs->drag_old_ex.width  + dif;
      new_extent.x      = rs->drag_old_ex.x      - dif;
    } else if (rs->drag_mode & brui_drag_mode_right) {
      float dif = rs->drag_point.x - mouse_pos.x;
      new_extent.width  = rs->drag_old_ex.width  - dif;
    }
    if (rs->drag_mode & brui_drag_mode_top) {
      float dif = rs->drag_point.y - mouse_pos.y;
      new_extent.y      = rs->drag_old_ex.y      - dif;
      new_extent.height = rs->drag_old_ex.height + dif;
    } else if (rs->drag_mode & brui_drag_mode_bottom) {
      float dif = rs->drag_point.y - mouse_pos.y;
      new_extent.height = rs->drag_old_ex.height - dif;
    }
  }
  bruir_update_extent(rs, drag_index, new_extent);
}

static br_strv_t brui_ancor_to_str(brui_ancor_t ancor);
void brui_resizable_mouse_pressl(bruirs_t* rs, br_vec2_t mouse_pos, bool ctrl_down) {
  if (rs->active_resizable < 0) return;
  brui_resizable_t* hovered = br_da_getp(*rs, rs->active_resizable);
  bool title_shown = rs->active_resizable == 0 ? false : BRUI_ANIMF(hovered->title_height_ah) > 0.1f;
  brui_drag_mode_t new_mode = brui_drag_mode_none;
  if ((ctrl_down || title_shown)) {
    float slack = 20;
    if (rs->active_resizable != 0) {
      br_extent_t ex = BRUI_ANIMEX(hovered->cur_extent_ah);
      rs->drag_index = rs->active_resizable;
      rs->drag_point = mouse_pos;
      if (title_shown) new_mode = brui_drag_mode_move;
      else {
        br_vec2_t local_pos = brui_resizable_local(rs, rs->active_resizable, mouse_pos);
        if      (local_pos.x < slack)                    new_mode |= brui_drag_mode_left;
        else if (local_pos.x > (float)ex.width - slack)  new_mode |= brui_drag_mode_right;
        if      (local_pos.y < slack)                    new_mode |= brui_drag_mode_top;
        else if (local_pos.y > (float)ex.height - slack) new_mode |= brui_drag_mode_bottom;
        if      (new_mode == brui_drag_mode_none)        new_mode  = brui_drag_mode_move;
      }
      if (!(new_mode & brui_drag_mode_move)) {
        brui_resizable_t* parent = br_da_getp(brui_state.uiw->resizables, hovered->parent);
        while (!(new_mode & brui_drag_mode_move) && parent->tag == brui_resizable_tag_ancor_helper) {
          BRUI_LOGI("Hover ancor: %s, parent->tag = %d", brui_ancor_to_str(hovered->ancor).str, parent->tag);
          if (hovered->ancor & brui_ancor_top) {
            if (new_mode & brui_drag_mode_bottom) new_mode = brui_drag_mode_move;
          } else if (hovered->ancor & brui_ancor_bottom) {
            if (new_mode & brui_drag_mode_top) new_mode = brui_drag_mode_move;
          } else if (hovered->ancor & brui_ancor_left) {
            if (new_mode & brui_drag_mode_right) new_mode = brui_drag_mode_move;
          } else if (hovered->ancor & brui_ancor_right) {
            if (new_mode & brui_drag_mode_left) new_mode = brui_drag_mode_move;
          }
          rs->drag_index = hovered->parent;
          rs->active_resizable = hovered->parent;
          hovered = parent;
          parent = br_da_getp(brui_state.uiw->resizables, hovered->parent);
          BRUI_LOGI("Hover ancor: %s, parent->tag = %d", brui_ancor_to_str(hovered->ancor).str, parent->tag);
        }
      }
      rs->drag_mode = new_mode;
      rs->drag_old_ex = BRUI_ANIMEX(hovered->cur_extent_ah);

      ACTION = brui_action_sliding;
      ACPARM.slider.value = hovered;
    }
  }
}

void brui_resizable_mouse_releasel(bruirs_t* rs, br_vec2_t mouse_pos) {
  (void)mouse_pos;
  rs->drag_index = 0;
  rs->drag_mode = brui_drag_mode_none;
  rs->drag_point = BR_VEC2(0, 0);
  ACTION = brui_action_none;
}

bool brui_resizable_mouse_scroll_px(bruirs_t* rs, br_vec2_t delta) {
  brui_resizable_t hovered = rs->arr[rs->active_resizable];
  LOGI("ar: %d, delta: %f %f", rs->active_resizable, BR_VEC2_(delta));
  float new = 0.f;
  float hidden_height = hovered.full_height - (float)BRUI_ANIMEX(hovered.cur_extent_ah).height;
  if (hidden_height > 0.f) {
    float cur = br_animf_get_target(&brui_state.uiw->anims, hovered.scroll_offset_percent_ah);
    cur -= delta.y / hidden_height;
    new = br_float_clamp(cur, 0.f, 1.f);
  }
  BRUI_ANIMFS(hovered.scroll_offset_percent_ah, new);
  return br_anim_alive(&brui_state.uiw->anims, hovered.scroll_offset_percent_ah);
}

bool brui_resizable_mouse_scroll(bruirs_t* rs, br_vec2_t delta) {
  brui_resizable_t hovered = rs->arr[rs->active_resizable];
  float new = 0.f;
  float hidden_height = hovered.full_height - (float)BRUI_ANIMEX(hovered.cur_extent_ah).height;
  if (hidden_height > 0.f) {
    float cur = br_animf_get_target(&brui_state.uiw->anims, hovered.scroll_offset_percent_ah) * hidden_height;
    cur -= delta.y * 20.f;
    new = br_float_clamp(cur / hidden_height, 0.f, 1.f);
  }
  BRUI_ANIMFS(hovered.scroll_offset_percent_ah, new);
  return br_anim_alive(&brui_state.uiw->anims, hovered.scroll_offset_percent_ah);
}

void brui_resizable_page(bruirs_t* rs, br_vec2_t delta) {
  brui_resizable_t hovered = rs->arr[rs->active_resizable];
  br_extent_t ex = BRUI_ANIMEX(hovered.cur_extent_ah);
  float hidden_height = hovered.full_height - ex.height;
  float page_percent = ex.height / hidden_height;
  page_percent *= 0.9f;
  float cur = BRUI_ANIMF(hovered.scroll_offset_percent_ah);
  float new = br_float_clamp(cur + delta.y * page_percent, 0.f, 1.f);
  BRUI_ANIMFS(hovered.scroll_offset_percent_ah, new);
}

void brui_resizable_scroll_percent_set(bruirs_t* rs, float percent) {
  brui_resizable_t hovered = rs->arr[rs->active_resizable];
  BRUI_ANIMFS(hovered.scroll_offset_percent_ah, percent);
}

br_vec2_t brui_resizable_local(bruirs_t* rs, int id, br_vec2_t global_pos) {
  while (id > 0) {
    brui_resizable_t r = rs->arr[id];
    global_pos = br_vec2_sub(global_pos, BRUI_ANIMEX(r.cur_extent_ah).pos);
    id = r.parent;
  }
  return global_pos;
}

static br_vec2_t bruir_pos_global(brui_resizable_t r) {
  br_extent_t ex = BRUI_ANIMEX(r.cur_extent_ah);
  if (r.parent == 0) return BR_VEC2I_TOF(ex.pos);
  brui_resizable_t par = br_da_get(brui_state.uiw->resizables, r.parent);
  br_extent_t pex = BRUI_ANIMEX(par.cur_extent_ah);
  br_vec2_t p = br_vec2_add(bruir_pos_global(par), ex.pos);
  float hidden_heigth = par.full_height - pex.height;
  if (hidden_heigth > 0) p.y -= BRUI_ANIMF(par.scroll_offset_percent_ah) * hidden_heigth;
  return p;
}

static void brui_resizable_decrement_z(bruirs_t* rs, brui_resizable_t* res) {
  int cur_z = res->z;
  if (cur_z <= 1) return;

  bruir_children_t c = brui_resizable_children_temp(res->parent);
  for (int i = 0; i < c.len; ++i) {
    int sibling_index = c.arr[i];
    brui_resizable_t* sibling = br_da_getp(*rs, sibling_index);
    if (sibling->z + 1 != res->z) continue;
    res->max_z = 0;
    sibling->max_z = 0;
    --res->z;
    ++sibling->z;
    return;
  }
  // We didn't find any sibling that has z value == cur->z - 1 so we can just decrement cur z
  --res->z;
  res->max_z = 0;
}

static bool brui_resizable_increment_z(bruirs_t* rs, int resizable_handle) {
  brui_resizable_t* res = br_da_getp(*rs, resizable_handle);
  bool any_bigger = false;
  bool any_equal = false;
  brfl_foreach(sibling_index, *rs) {
    if (sibling_index == resizable_handle) continue;
    brui_resizable_t* sibling = br_da_getp(*rs, sibling_index);
    if (sibling->parent != res->parent) continue;
    if (sibling->z > res->z) any_bigger = true;
    if (sibling->z == res->z) any_equal = true;
    if (sibling->z - 1 != res->z) continue;
    res->max_z = 0;
    sibling->max_z = 0;
    ++res->z;
    --sibling->z;
    return true;
  }
  if (true  == any_equal);
  else if (false == any_bigger) return false;
  // We didn't find any sibling that has z value == cur->z + 1 so we can just increment cur z
  ++res->z;
  res->max_z = 0;
  return true;
}

void brui_resizable_move_on_top(bruirs_t* rs, int r_handle) {
  brui_resizable_t* res = br_da_getp(*rs, r_handle);
  while (res->parent != 0) {
    r_handle = res->parent;
    res = br_da_getp(*rs, r_handle);
  }
  while (brui_resizable_increment_z(rs, r_handle));
}

int brui_resizable_sibling_max_z(int id) {
  if (id == 0) return 0;
  brui_resizable_t* res = br_da_getp(brui_state.uiw->resizables, id);
  bruir_children_t siblings = brui_resizable_children_temp(res->parent);
  int max_z = 0;
  for (int i = 0; i < siblings.len; ++i) {
    int sibling_id = br_da_get(siblings, i);
    if (sibling_id == id) continue;
    brui_resizable_t* sibling = br_da_getp(brui_state.uiw->resizables, sibling_id);
    if (sibling->z >= res->z) continue;
    int sib_z = sibling->max_z;
    max_z = sib_z > max_z ? sib_z : max_z;
  }
  return max_z;
}

bruir_children_t brui_resizable_children_temp(int resizable_handle) {
  brui_state.temp_children.len = 0;
  brfl_foreach(i, brui_state.uiw->resizables) {
    if (resizable_handle == br_da_get(brui_state.uiw->resizables, i).parent) {
      br_da_push_t(int, brui_state.temp_children, i);
    }
  }
  return brui_state.temp_children;
}

int brui_resizable_children_count(int resizable_handle) {
  int sum = 0;
  brfl_foreach(i, brui_state.uiw->resizables) if (resizable_handle == br_da_get(brui_state.uiw->resizables, i).parent) ++sum;
  return sum;
}

int brui_resizable_children_top_z(bruirs_t* rs, int resizable_handle) {
  int top_z = 0;
  brfl_foreach(i, *rs) if (resizable_handle == br_da_get(*rs, i).parent) top_z = br_i_max(top_z, br_da_get(*rs, i).z);
  return top_z;
}

br_extent_t brui_resizable_cur_extent(int resizable_handle) {
  brui_resizable_t r = BRUI_RS(resizable_handle);
  br_extent_t ce = BRUI_ANIMEX(r.cur_extent_ah);
  if (resizable_handle > 0) {
    int parent = r.parent;
    br_extent_t pe = brui_resizable_cur_extent(parent);
    ce.x += pe.x;
    ce.y += pe.y;
  }
  return ce;
}

static br_strv_t brui_ancor_to_str(brui_ancor_t ancor) {
  brui_state.scrach.len = 0;
  if (brui_ancor_none  == ancor) br_str_push_c_str(&brui_state.scrach, "none");
  if (brui_ancor_left   & ancor) br_str_push_c_str(&brui_state.scrach, "left ");
  if (brui_ancor_right  & ancor) br_str_push_c_str(&brui_state.scrach, "right ");
  if (brui_ancor_top    & ancor) br_str_push_c_str(&brui_state.scrach, "top ");
  if (brui_ancor_bottom & ancor) br_str_push_c_str(&brui_state.scrach, "bottom ");
  if (brui_state.scrach.len == 0) br_str_push_int(&brui_state.scrach, (int)ancor);
  br_str_push_zero(&brui_state.scrach);
  return br_str_as_view(brui_state.scrach);
}

static void brui_resizable_set_ancor(int res_id, int sibling_id, brui_ancor_t ancor) {
  (void)brui_ancor_to_str;
  bruirs_t* rs = &brui_state.uiw->resizables;
  brui_resizable_t* res = br_da_getp(*rs, res_id);
  int parent_id = res->parent;
  brui_resizable_t* parent = br_da_getp(*rs, parent_id);
  brui_resizable_t* sibling = br_da_getp(*rs, sibling_id);
  br_extent_t ex = BRUI_ANIMEX(res->cur_extent_ah);

  BRUI_LOGI("Ancor %d and %d: %s", res_id, sibling_id, brui_ancor_to_str(ancor).str);

  if (res->ancor == brui_ancor_none) res->ancor_none_extent = ex;
  if (sibling_id != 0) brui_state.snap_cooldown = 1.f;

  res->ancor = ancor;

  if (0 != sibling_id) {
    brui_state.scrach.len = 0;
    brsp_id_t title_id = brsp_push(&brui_state.uiw->sp, br_str_printf(&brui_state.scrach, "Ancor %d and %d", res_id, sibling_id));
    br_extent_t sex = BRUI_ANIMEX(sibling->cur_extent_ah);
    br_extent_t new_extent = sex;
    if (ancor & (brui_ancor_top  | brui_ancor_bottom)) new_extent.height += ex.height;
    else                                               new_extent.height = br_float_max(ex.height, new_extent.height);
    if (ancor & (brui_ancor_left | brui_ancor_right))  new_extent.width  += ex.width;
    else                                               new_extent.width  = br_float_max(ex.width,  new_extent.width);
    br_extent_t pex = BRUI_ANIMEX(parent->cur_extent_ah);
    new_extent.height = br_float_min(pex.height, new_extent.height);
    new_extent.width = br_float_min(pex.width, new_extent.width);

    brui_resizable_t new = {
      .tag = brui_resizable_tag_ancor_helper,
      .title_id = title_id,
      .parent = sibling->parent,
      .was_draw_last_frame = true,
      .ancor_none_extent = sex,
      .title_enabled = false,
      .z = sibling->z > res->z ? sibling->z : res->z,
    };
    new.title_height_ah = br_animf_new(&brui_state.uiw->anims, 0, 0);
    new.cur_extent_ah = br_animex_new(&brui_state.uiw->anims, new_extent, new_extent);
    new.scroll_offset_percent_ah = br_animf_new(&brui_state.uiw->anims, 0, 0);

    int new_id = brfl_push(brui_state.uiw->resizables, new);
    res = br_da_getp(brui_state.uiw->resizables, res_id);
    sibling = br_da_getp(brui_state.uiw->resizables, sibling_id);
    res->parent = new_id;
    br_anim_rebase(&brui_state.uiw->anims, res->cur_extent_ah, sex.pos);
    sibling->ancor_none_extent = br_animex_get_target(&brui_state.uiw->anims, sibling->cur_extent_ah);
    sibling->parent = new_id;
    br_anim_rebase(&brui_state.uiw->anims, sibling->cur_extent_ah, sex.pos);
    rs->drag_point = brui_state.uiw->mouse.pos;
    rs->drag_old_ex = BRUI_ANIMEX(res->cur_extent_ah);
    brui_resizable_check_parents(&brui_state.uiw->resizables);
    if      (ancor == brui_ancor_top)    sibling->ancor = brui_ancor_bottom;
    else if (ancor == brui_ancor_bottom) sibling->ancor = brui_ancor_top;
    else if (ancor == brui_ancor_left)   sibling->ancor = brui_ancor_right;
    else if (ancor == brui_ancor_right)  sibling->ancor = brui_ancor_left;
  } else {
    if (parent->tag == brui_resizable_tag_ancor_helper) {
      int gp = parent->parent;
      if (parent->parent == -1) return;
      parent->parent = parent_id;
      br_extent_t pex = BRUI_ANIMEX(parent->cur_extent_ah);
      br_vec2_t rebase = br_vec2_scale(pex.pos, -1.f);
      brui_resizable_t* gparent = br_da_getp(*rs, gp);
      res->parent = gp;
      br_anim_rebase(&brui_state.uiw->anims, res->cur_extent_ah, rebase);
      rs->drag_old_ex = ex;

      brfl_foreach(i, *rs) {
        brui_resizable_t* sib = &rs->arr[i];
        if (sib->parent != parent_id) continue;
        if (i == parent_id) continue;
        br_extent_t sex = br_anim_rebase(&brui_state.uiw->anims, sib->cur_extent_ah, rebase);
        br_extent_t gpex = BRUI_ANIMEX(gparent->cur_extent_ah);
        sex.size = sib->ancor_none_extent.size;
        float r = sex.x + sex.width;
        float b = sex.y + sex.height;
        if (r > gpex.width)  sex.pos.x -= r - sex.width;
        if (b > gpex.height) sex.pos.y -= b - sex.height;
        BRUI_ANIMEXS(sib->cur_extent_ah, sex);

        sib->ancor = parent->ancor;
        sib->parent = gp;
        if (sib->z > res->z) {
          int tmp = sib->z;
          sib->z = res->z;
          res->z = tmp;
        }
        break;
      }

      while (gparent->tag == brui_resizable_tag_ancor_helper) {
        if (parent->z > res->z) {
          int tmp = parent->z;
          parent->z = res->z;
          res->z = tmp;
        }
        res->parent = gparent->parent;
        parent->max_z = 0;
        parent = gparent;
        pex = BRUI_ANIMEX(parent->cur_extent_ah);
        rebase = br_vec2_scale(pex.pos, -1.f);
        br_anim_rebase(&brui_state.uiw->anims, res->cur_extent_ah, rebase);
        gp = gparent->parent;
        gparent = br_da_getp(*rs, gp);
      }
#if BR_DEBUG
      brfl_foreach(i, *rs) {
        brui_resizable_t r = br_da_get(*rs, i);
        if (i == r.parent) continue;
        BR_ASSERTF(r.parent != parent_id, "i == %d, parent = %d", i, parent_id);
      }
#endif
      brui_resizable_delete(parent_id);
      rs->drag_old_ex = br_animex_get_target(&brui_state.uiw->anims, res->cur_extent_ah);
      brui_resizable_increment_z(rs, res_id);
      brui_resizable_check_parents(rs);
    } else {
      if (ancor == brui_ancor_none) {
        BRUI_ANIMEXS(res->cur_extent_ah, res->ancor_none_extent);
      }
    }
  }
}

br_bb_t brui_resizable_limit(int id) {
  brui_resizable_t res = BRUI_RS(id);
  br_extent_t ex = BRUI_ANIMEX(res.cur_extent_ah);
  if (id == 0) return BR_BB(ex.x, ex.y, ex.x + ex.width, ex.y + ex.height);

  br_bb_t pbb = brui_resizable_limit(res.parent);
  br_vec2_t cur_p = bruir_pos_global(res);
  br_bb_t bb = BR_BB(cur_p.x, cur_p.y, cur_p.x + ex.width, cur_p.y + ex.height);
  return br_bb_and(pbb, bb);
}

brui_resizable_t* brui_resizable_push(int id) {
  bruirs_t* rs = &brui_state.uiw->resizables;
  brui_resizable_t* res = br_da_getp(*rs, id);

  res->was_draw_last_frame = true;
  br_extent_t rex = BRUI_ANIMEX(res->cur_extent_ah);
  int cur_z = TOP.z;
  float title_height = id == 0 ? 0 : BRUI_ANIMF(res->title_height_ah);
  bool is_menu_shown = title_height > .01f;
  brui_push();
  TOP.psum = BR_VEC2(0, 0);
  br_vec2_t cur_p = bruir_pos_global(*res);
  TOP.limit = brui_resizable_limit(id);

  brui_cur_set(cur_p.x, cur_p.y, TOP.cur_content_height);
  brtr_state()->limits = TOP.limit;

  float scroll_y = (res->full_height - rex.height) * BRUI_ANIMF(res->scroll_offset_percent_ah);

  int max_z_id = id;
  int ancor_parent_id = res->parent;
  while (ancor_parent_id > 0) {
    brui_resizable_t* parent = br_da_getp(*rs, ancor_parent_id);
    if (parent->tag == brui_resizable_tag_ancor_helper) {
      max_z_id = res->parent;
      ancor_parent_id = parent->parent;
    }
    else break;
  }
  brui_z_set(cur_z + brui_resizable_sibling_max_z(max_z_id) + (is_menu_shown ? 5 : 15));

  TOP.cur_resizable = id;

  TOP.start_z = TOP.z;
  TOP.is_active = id == rs->active_resizable;
  TOP.hide_border = true;
  TOP.hide_bg = true;
  brui_push();
  TOP.cur_resizable = id;

  brui_cur_set(TOP.cur_pos.x, TOP.cur_pos.y, TOP.cur_content_height);
  brtr_stack_el_t* el = brtr_state();
  el->limits = TOP.limit;

  br_vec2_t mp = brui_state.uiw->mouse.pos;
  float title_full_height = brui_text_size() + 2.f * TOP.padding.y;
  float new_title_height = is_menu_shown ? title_full_height : 0.f;
  if (res->title_enabled && false == brui_state.uiw->key.ctrl_down && TOP.is_active) {
    if (br_col_vec2_extent(BR_EXTENT2(cur_p, BR_SIZE(rex.width, title_full_height * 0.2f)), mp)) {
      new_title_height = title_full_height;
    } else if (ACTION != brui_action_sliding && false == br_col_vec2_extent(BR_EXTENT2(cur_p, BR_SIZE(rex.width, title_full_height * 1.2f)), mp)) {
      new_title_height = 0;
    }
  } else if (brui_state.uiw->key.ctrl_down || !TOP.is_active) {
    new_title_height = 0.f;
  }
  if (id != 0) BRUI_ANIMFS(res->title_height_ah, new_title_height);
  if (is_menu_shown) {
    brui_push_simple(TOP);
      TOP.start_z = res->max_z + 2;
      TOP.z = TOP.start_z;
      TOP.limit.max_y = fminf(TOP.limit.max_y, TOP.limit.min_y + title_height);
      float button_width = brui_text_size() * 1.2f;
      brui_vsplitvp(5, BRUI_SPLITR(1), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width), BRUI_SPLITA(button_width));
        if (res->title_id) brui_text_input(res->title_id);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Z-"))) brui_resizable_decrement_z(rs, res);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Z+"))) brui_resizable_increment_z(rs, id);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("[]"))) brui_resizable_set_ancor(id, 0, res->ancor == brui_ancor_all ? brui_ancor_none : brui_ancor_all);
      brui_vsplit_pop();
        if (brui_button(BR_STRL("X"))) brui_resizable_show(id, false);
      brui_vsplit_pop();
      brui_background(TOP.limit, BR_THEME.colors.plot_bg);
    brui_pop_simple();
  }
  brui_state.max_z = 0;
  TOP.limit.min_y += title_height;
  brui_cur_set(TOP.cur_pos.x, TOP.cur_pos.y - scroll_y + title_height, TOP.cur_content_height);
  brtr_state()->limits = TOP.limit; // NOTE: el-> is invalidated...
  return res;
}

void brui_resizable_pop(void) {
  bruirs_t* rs = &brui_state.uiw->resizables;
  int cur_res = TOP.cur_resizable;
  brui_resizable_t* res = br_da_getp(*rs, cur_res);
  br_extent_t ex = BRUI_ANIMEX(res->cur_extent_ah);

  float full_height = res->full_height = TOP.cur_content_height;
  float hidden_height = full_height - ex.height;
  if (hidden_height > 0.f) {
    if (false == brui_resizable_is_hidden(cur_res) && false == brui_state.uiw->key.ctrl_down) brui_scroll_bar(res->scroll_offset_percent_ah);
  } else BRUI_ANIMFS(res->scroll_offset_percent_ah, 0.f);
  brui_cur_set(ex.x, ex.y + ex.height, ex.height);
  brui_pop();
  brui_pop();
  res->max_z = brui_state.max_z;
  TOP.cur_content_height = ex.height;
  if (cur_res == rs->active_resizable && rs->drag_mode == brui_drag_mode_move) {
    brui_snap_areas(brui_state.uiw->mouse.pos, cur_res, rs);
  }
}

void brui_resizable_show(int resizable_handle, bool show) {
  if (show) brui_resizable_set_ancor(resizable_handle, 0, brui_ancor_none);
  else      brui_resizable_set_ancor(resizable_handle, 0, brui_ancor_hidden);
}

void brui_resizable_maximize(int resizable_handle, bool maximize) {
  if (maximize) brui_resizable_set_ancor(resizable_handle, 0, brui_ancor_all);
  else          brui_resizable_set_ancor(resizable_handle, 0, brui_ancor_none);
}

bool brui_resizable_is_hidden(int resizable_handle) {
  if (0 == resizable_handle) return false;
  brui_resizable_t res = BRUI_RS(resizable_handle);
  br_extent_t ex = BRUI_ANIMEX(res.cur_extent_ah);
  if (res.ancor == brui_ancor_hidden && ex.width < 20.f) return true;
  return brui_resizable_is_hidden(res.parent);
}

br_vec2_t brui_resizable_to_global(int resizable_handle, br_vec2_t pos) {
  if (resizable_handle == 0) return pos;
  brui_resizable_t* r = br_da_getp(brui_state.uiw->resizables, resizable_handle);
  br_extent_t ex = BRUI_ANIMEX(r->cur_extent_ah);
  pos = br_vec2_sub(pos, ex.pos);
  float hidden_heigth = r->full_height - ex.height;
  if (hidden_heigth > 0) pos.y += BRUI_ANIMF(r->scroll_offset_percent_ah) * hidden_heigth;
  return brui_resizable_to_global(r->parent, pos);
}


brui_resizable_temp_push_t brui_resizable_temp_push(br_strv_t id) {
  // TODO: Handle hash collisions

  if (-1 != brui_state.rs_temp_last) LOGF("Can't have temp resizables in temp resizables, yet");
  brui_state.rs_temp_last_str = id;
#if defined(BR_IS_SIZE_T_32_BIT)
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeef);
#else
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeefdeadbeef);
#endif

  ptrdiff_t index = stbds_hmgeti(brui_state.temp_res, hash);
  bool just_created = false;
  brui_resizable_temp_state_t state = { 0 };
  brsp_id_t title_id = -1;
  if (index < 0) {
    state.was_drawn = true;
    state.resizable_handle = brui_resizable_new(&brui_state.uiw->resizables, BR_EXTENT(0, 0, 100, 100), 0);
    state.is_deleted = false;
    stbds_hmput(brui_state.temp_res, hash, state);
    title_id = brsp_push(&brui_state.uiw->sp, id);
    just_created = true;
  } else {
    state = brui_state.temp_res[index].value;
    if (state.is_deleted) {
      just_created = true;
      brui_state.temp_res[index].value.is_deleted = false;
      brui_state.temp_res[index].value.resizable_handle = brui_resizable_new(&brui_state.uiw->resizables, BR_EXTENT(0, 0, 100, 100), 0);
      state = brui_state.temp_res[index].value;
      title_id = brsp_push(&brui_state.uiw->sp, id);
    }
    brui_state.temp_res[index].value.was_drawn = true;
  }

  brui_state.rs_temp_last = state.resizable_handle;
  brui_resizable_t* res = brui_resizable_push(state.resizable_handle);
  if (title_id > -1) res->title_id = title_id;
  if (just_created) {
    br_extent_t ex = BRUI_ANIMEX(BRUI_RS(0).cur_extent_ah);
    ex.width -= 100;
    ex.height -= 100;
    ex.x += 50;
    ex.y += 50;
    BRUI_ANIMEXS(res->cur_extent_ah, ex);
  }
  return (brui_resizable_temp_push_t) { .res = res, .resizable_handle = state.resizable_handle, .just_created = just_created };
}

bool brui_resizable_temp_pop(void) {
  int id = brui_state.rs_temp_last;
  br_strv_t handle = brui_state.rs_temp_last_str;
  bool is_hidden = brui_resizable_is_hidden(id);
  brui_resizable_pop();
  brui_state.rs_temp_last = -1;
  if (is_hidden) {
    brui_resizable_temp_delete(handle);
    return true;
  }
  return false;
}

void brui_resizable_temp_delete(br_strv_t id) {
#if defined(BR_IS_SIZE_T_32_BIT)
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeef);
#else
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeefdeadbeef);
#endif
  int handle = -1;
  ptrdiff_t index = stbds_hmgeti(brui_state.temp_res, hash);
  if (index >= 0) {
    handle = brui_state.temp_res[index].value.resizable_handle;
    brui_resizable_delete(handle);
    (void)stbds_hmdel(brui_state.temp_res, hash);
  }
}

void brui_resizable_temp_delete_all(void) {
  bruirs_t* rs = &brui_state.uiw->resizables;
  ptrdiff_t len = stbds_hmlen(brui_state.temp_res);
  for (int i = 0; i < len; ++i) {
    if (brui_state.temp_res[i].value.is_deleted) continue;
    brfl_remove(*rs, brui_state.temp_res[i].value.resizable_handle);
  }
  br_anim_delete(&brui_state.uiw->anims, ACPARM.text.offset_ahandle);
}

static int bruir_find_at(bruirs_t* rs, int index, br_vec2_t loc, br_vec2_t* out_local_pos) {
  brui_resizable_t res = br_da_get(*rs, index);
  if (brui_resizable_is_hidden(index)) return -1;
  if (index != 0 && false == res.was_draw_last_frame) return -1;
  br_extent_t ex = BRUI_ANIMEX(res.cur_extent_ah);
  br_vec2_t local = BR_VEC2(loc.x - ex.x, loc.y - ex.y);
  if (local.x < 0)         return -1;
  if (local.y < 0)         return -1;
  if (local.x > ex.width)  return -1;
  if (local.y > ex.height) return -1;

  int found = -1;
  int best_z = 0;
  brfl_foreach(i, *rs) {
    brui_resizable_t* c = br_da_getp(*rs, i);
    if (c->parent != index) continue;
    int cur_z = c->z;
    if (cur_z > best_z) {
      int cur = bruir_find_at(rs, i, local, out_local_pos);
      if (cur > 0) {
        best_z = cur_z;
        found = cur;
      }
    }
  }
  if (found == -1) {
    *out_local_pos = local;
    return index;
  }
  return found;
}

bool brui_save(BR_FILE* file, brui_window_t* uiw) {
  int fl_write_error = 0;
  bool success = true;

  brui_resizable_temp_delete_all();
  brsp_compress(&uiw->sp, 1.0f, 0);

  if (false == br_anim_save(file, &uiw->anims))                               BR_ERRORE("Failed to write anim.");
  brfl_write(file, uiw->resizables, fl_write_error); if (fl_write_error != 0) BR_ERRORE("Failed to write resizables.");
  if (false == brsp_write(file, &uiw->sp))                                    BR_ERRORE("Failed to write string pool.");
  if (1 != BR_FWRITE(&uiw->theme, sizeof(uiw->theme), 1, file))               BR_ERRORE("Failed to write theme.");
  if (1 != BR_FWRITE(&uiw->def,   sizeof(uiw->def),   1, file))               BR_ERRORE("Failed to write default ui values.");

error:
  return success;
}

bool brui_load(BR_FILE* file, brui_window_t* uiw) {
  int fl_read_error = 0;
  int success = true;

  if (false == br_anim_load(file, &uiw->anims))                            BR_ERRORE("Failed to load animations.");
  brfl_read(file, uiw->resizables, fl_read_error); if (fl_read_error != 0) BR_ERRORE("Failed to read resizables.");
  if (false == brsp_read(file, &uiw->sp))                                  BR_ERRORE("Failed to read string pool.");
  if (1 != BR_FREAD(&uiw->theme, sizeof(uiw->theme), 1, file))                BR_ERRORE("Failed to read theme.");
  if (1 != BR_FREAD(&uiw->def,   sizeof(uiw->def),   1, file))                BR_ERRORE("Failed to read default ui values.");

  brsp_compress(&uiw->sp, 1.3f, 16);
  goto done;

error:
  brfl_free(uiw->anims.all);
  brfl_free(uiw->anims.alive);
  brsp_free(&uiw->sp);
  brfl_free(uiw->resizables);

done:
  ACPARM.text.offset_ahandle = br_animf_new(&uiw->anims, 0, 0);
  return success;
}

static void brui_push_simple(brui_stack_el_t el) {
  BRUI_LOG("push simp");
  br_da_push(brui_state, el);
  BRUI_LOG("push simp post");
}

static brtr_stack_el_t* brui_push_text(brui_stack_el_t el) {
  brui_push_simple(el);
  return brtr_state_push();
}

static brtr_stack_el_t* brui_pop_text(void) {
  brui_pop_simple();
  return brtr_state_pop();
}

void brui_pop_simple(void) {
  --brui_state.len;
}

static void brui_cur_set(float cur_x, float cur_y, float cur_content_height) {
  TOP.cur_pos = BR_VEC2(cur_x, cur_y);
  TOP.cur_content_height = cur_content_height;
  brtr_state()->pos = TOP.cur_pos;
}

static void brui_cur_push(float height) {
  brui_cur_set(TOP.limit.min_x + TOP.psum.x, TOP.cur_pos.y + height, TOP.cur_content_height + height);
}

#undef TOP
#undef TOP2
#undef Z
#undef ZGL
#undef ACTION
#undef ACPARM
