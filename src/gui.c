#include "src/br_gui.h"
#include ".generated/br_version.h"
#include ".generated/icons.h"
#include "include/brplot.h"
#include "src/br_da.h"
#include "src/br_filesystem.h"
#include "src/br_free_list.h"
#include "src/br_gl.h"
#include "src/br_license.h"
#include "src/br_math.h"
#include "src/br_permastate.h"
#include "src/br_plot.h"
#include "src/br_plotter.h"
#include "src/br_pp.h"
#include "src/br_resampling.h"
#include "src/br_shaders.h"
#include "src/br_smol_mesh.h"
#include "src/br_str.h"
#include "src/br_string_pool.h"
#include "src/br_theme.h"
#include "src/br_tl.h"
#include "src/br_ui.h"
#include "src/br_memory.h"

static void draw_left_panel(br_plotter_t* br);
static bool brgui_draw_plot_menu(br_plot_t* plot, br_datas_t datas);
static void brgui_draw_legend(br_plot_t* plot, br_datas_t datas);
static void brgui_draw_debug_window(br_plotter_t* br);
static void brgui_draw_license(br_plotter_t* br);
static void brgui_draw_about(br_plotter_t* br);
static void brgui_draw_add_expression(br_plotter_t* br);
static void brgui_draw_show_data(brgui_show_data_t* d, br_datas_t datas);
static void brgui_draw_help(br_plotter_t* br);

#if defined(BR_HAS_MEMORY)
static void brgui_draw_memory(br_plotter_t* br);
#endif

void br_plotter_draw(br_plotter_t* br) {
  brsp_t* sp = brtl_brsp();
  BR_PROFILE("Plotter draw") {
    br_plotter_begin_drawing(br);
    BR_PROFILE("Draw Plots") {
      for (int i = 0; i < br->plots.len; ++i) {
#define PLOT br_da_getp(br->plots, i)
        brui_resizable_t* r = brui_resizable_get(PLOT->extent_handle);
        if (brui_resizable_is_hidden(PLOT->extent_handle)) continue;
        PLOT->cur_extent = BR_EXTENT_TOI(r->cur_extent);
        PLOT->mouse_inside_graph = PLOT->extent_handle == brui_resizable_active();
        br_plots_update_variables(br, PLOT, br->groups, brtl_mouse_pos());
        br_plot_update_context(PLOT, brtl_mouse_pos());
        br_plot_update_shader_values(PLOT, &br->shaders);

        brgl_enable_framebuffer(PLOT->texture_id, PLOT->cur_extent.width, PLOT->cur_extent.height);
        brgl_clear(BR_COLOR_COMPF(BR_THEME.colors.plot_bg));
        if (PLOT->kind == br_plot_kind_2d) {
          br_smol_mesh_grid_draw(PLOT, &br->shaders);
          br_shaders_draw_all(br->shaders); // TODO: This should be called whenever a other shader are being drawn.
          br_datas_draw(br->groups, PLOT);
          br_shaders_draw_all(br->shaders);
          draw_grid_numbers(br->text, PLOT);
        } else if (PLOT->kind == br_plot_kind_3d) {
          br_datas_draw(br->groups, PLOT);
          br_shaders_draw_all(br->shaders);
          br_smol_mesh_grid_draw(PLOT, &br->shaders);
          br_shaders_draw_all(br->shaders);
          draw_grid_numbers(br->text, PLOT);
        }
      }
    }

    brgl_enable_framebuffer(0, br->win.size.width, br->win.size.height);
    brgl_clear(BR_COLOR_COMPF(BR_THEME.colors.bg));

    BR_PROFILE("UI") {
      brui_begin();
        br_shaders_draw_all(br->shaders);
#if BR_HAS_HOTRELOAD
        br_hotreload_tick_ui(&br->hot_state);
#endif
        int to_remove = -1;
        for (int i = 0; i < br->plots.len; ++i) {
          if (brui_resizable_is_hidden(PLOT->extent_handle)) continue;
            brui_resizable_push(PLOT->extent_handle);
              brui_img(PLOT->texture_id);
              if (brgui_draw_plot_menu(PLOT, br->groups)) to_remove = i;
              brgui_draw_legend(PLOT, br->groups);
            brui_resizable_pop();
#undef PLOT
        }
        if (to_remove != -1) br_plotter_remove_plot(br, to_remove);
        draw_left_panel(br);
        brgui_draw_debug_window(br);
        brgui_draw_license(br);
        brgui_draw_about(br);
        brgui_fm_result_t fs_r = brgui_draw_file_manager(&br->ui.fm_state);
        if (fs_r.is_selected) {
          switch (br->ui.fm_state.action) {
            case brgui_file_manager_import_csv: {
              brsp_remove(sp, br->ui.csv_state.read_id);
              br->ui.csv_state.read_id = brsp_copy(sp, fs_r.selected_file);
              brsp_insert_char_at_end(sp, br->ui.csv_state.read_id, '\0');
              br->ui.csv_state.is_open = true;
              brui_resizable_show(fs_r.resizable_handle, false);
            } break;
            case brgui_file_manager_load_font: {
              br_strv_t strv = brsp_get(*sp, fs_r.selected_file);
              if (br_text_renderer_load_font(br->text, strv)) {
                brsp_remove(sp, br->ui.font_path_id);
                br->ui.font_path_id = brsp_copy(sp, fs_r.selected_file);
                brsp_insert_char_at_end(sp, br->ui.font_path_id, '\0');
              };
            } break;
            default: {
              LOGI("Unknown action: %d", br->ui.fm_state.action);
            } break;
          }
        }
        brgui_draw_csv_manager(&br->ui.csv_state, &br->csv_parser);
        brgui_draw_add_expression(br);
        brgui_draw_show_data(&br->ui.show_data, br->groups);
        brgui_draw_help(br);
#if defined(BR_HAS_MEMORY)
        brgui_draw_memory(br);
#endif
        brui_resizable_update();
      brui_end();
    }
    br_plotter_end_drawing(br);
  }
}

static void brgui_draw_legend(br_plot_t* plot, br_datas_t datas) {
  if (brui_resizable_is_hidden(plot->legend_extent_handle)) return;
  brui_resizable_push(plot->legend_extent_handle);
    bool is_active = brui_active();
    int active_group = -1;
    bool pressed = brtl_mousel_pressed();
    br_plot_data_t* di = NULL;
    br_vec2_t mp = brtl_mouse_pos();
    brui_padding_y_set(1.f);
    for (int i = 0; i < plot->data_info.len; ++i) {
      bool active = is_active;
      br_plot_data_t* cdi = &plot->data_info.arr[i];
      br_data_t* data = br_data_get1(datas, cdi->group_id);
      active &= brui_y() < mp.y;
      brui_vsplitvp(3, BRUI_SPLITA((float)brui_text_size()), BRUI_SPLITA(brui_padding_x()), BRUI_SPLITR(1));
        if (plot->selected_data == data->group_id) {
          brui_icon((float)brui_text_size(), BR_EXTENT_TOBB(br_icon_cb_0((float)brui_text_size())), brtl_theme()->colors.btn_hovered, data->color);
        } else {
          brui_icon((float)brui_text_size(), BR_BB(0,0,0,0), data->color, data->color);
        }
      brui_vsplit_pop();
      brui_vsplit_pop();
        brui_text_align_set(br_text_renderer_ancor_left_mid);
        brui_height_set((float)brui_text_size());
        brui_text_input(data->name);
      brui_vsplit_end();
      active &= brui_y() >= mp.y;
      if (active) {
        active = false;
        if (cdi->thickness_multiplyer_target != 0) {
          if (pressed) {
            cdi->thickness_multiplyer_target = 0.f;
            plot->selected_data_old = cdi->group_id;
            plot->selected_data = -1;
          } else {
            active_group = data->group_id;
            di = cdi;
          }
        } else {
          if (brtl_mousel_pressed()) {
            cdi->thickness_multiplyer_target = 1.f;
          }
        }
      }
    }
    if (active_group != plot->selected_data_old) {
      if (di != NULL) di->thickness_multiplyer_target = 2.f;
      if (false == pressed) {
        if (plot->selected_data_old >= 0) {
          int index = 0;
          br_da_find(plot->data_info, group_id, plot->selected_data_old, index);
          if (index < plot->data_info.len) {
            plot->data_info.arr[index].thickness_multiplyer_target = 1.f;
          }
        }
      }
      plot->selected_data_old = active_group;
      plot->selected_data = active_group;
    }
  brui_resizable_pop();
}

brgui_fm_result_t brgui_draw_file_manager(brgui_file_manager_t* state) {
  brgui_fm_result_t ret = { 0 };
  if (false == state->is_open && false == state->is_inited) return ret;

  br_strv_t cur_path;
  brsp_t* sp = brtl_brsp();
  brui_action_t* action = brui_action();

  state->is_inited = false;

  brui_resizable_temp_push_t t = brui_resizable_temp_push(BR_STRL("File Manager"));
    state->is_open = true;
    ret.resizable_handle = t.resizable_handle;
    if (t.just_created) {
      if (false == brsp_is_in(*sp, state->path_id)) {
        state->path_id = brsp_new(sp);
        cur_path = BR_STRL("/home/branimir");
        brsp_set(sp, state->path_id, cur_path);
      } else {
        cur_path = brsp_get(*sp, state->path_id);
      }
      state->cur_dir = (br_fs_files_t) { 0 };
      t.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      t.res->target.cur_extent.width -= 100;
      t.res->target.cur_extent.height -= 100;
      t.res->target.cur_extent.x += 50;
      t.res->target.cur_extent.y += 50;
      state->select_index = -1;
      if (false == br_fs_list_dir(cur_path, &state->cur_dir)) {
        char c = brsp_remove_char_end(sp, state->path_id);
        while (cur_path.len > 1 && c != '/' && c != '\\') {
          c = brsp_remove_char_end(sp, state->path_id);
          cur_path = brsp_get(*sp, state->path_id);
        }
        br_fs_list_dir(cur_path, &state->cur_dir);
      }
      action->kind = brui_action_typing;
      action->args.text.cursor_pos = (int)cur_path.len;
      action->args.text.id = state->path_id;
    }
    switch (state->action) {
      case brgui_file_manager_import_csv: brui_text(BR_STRL("Import CSV"));     break;
      case brgui_file_manager_load_font:  brui_text(BR_STRL("Load Font"));      break;
      default:                            brui_text(BR_STRL("Unknown action")); break;
    }

    cur_path = brsp_get(*sp, state->path_id);

    br_str_t cur_dir = state->cur_dir.last_good_dir;
    uint32_t cur_dir_name_len = cur_dir.len;
    bool tab = brtl_key_pressed(BR_KEY_TAB);
    bool dir_changed = false;
    brui_push();
      int ts = brui_text_size();
      brui_vsplitvp(3, BRUI_SPLITA((float)ts + 5.f), BRUI_SPLITA((float)ts + 5.f), BRUI_SPLITR(1));
        if (brui_button_icon(BR_SIZEI(ts, ts), state->show_hidden_files ? br_icon_hidden_1((float)ts) : br_icon_hidden_0((float)ts))) state->show_hidden_files = !state->show_hidden_files;
      brui_vsplit_pop();
        if (brui_button_icon(BR_SIZEI(ts, ts), br_icon_back((float)ts)) || (brui_active() && brtl_key_pressed(BR_KEY_BACKSPACE) && brtl_key_ctrl())) {
          if (cur_path.len > 1) {
            brsp_remove_char_end(sp, state->path_id);
            cur_path = brsp_get(*sp, state->path_id);
            while (cur_path.len > 1 && cur_path.str[cur_path.len - 1] != '/' && cur_path.str[cur_path.len - 1] != '\\') {
              brsp_remove_char_end(sp, state->path_id);
              cur_path = brsp_get(*sp, state->path_id);
            }
            if (cur_path.len > 0 && cur_path.str[cur_path.len - 1] != '/') brsp_insert_char_at_end(sp, state->path_id, '/');
            cur_path = brsp_get(*sp, state->path_id);
            dir_changed = true;
          }
        }
      brui_vsplit_pop();
        if (brui_text_input(state->path_id)) {
          dir_changed = true;
          cur_path = brsp_get(*sp, state->path_id);
        }
        if (action->kind == brui_action_typing && action->args.text.id == state->path_id) {
          bool shift = brtl_key_shift();
          if (tab) {
            if (shift) --state->select_index;
            else       ++state->select_index;
            if (state->select_index < 0 || state->select_index >= (int)state->cur_dir.real_len) state->select_index = 0;
          }
        }
      brui_vsplit_end();
    brui_pop();
    brui_new_lines(1);
    brui_push();
      int real_i = 0;
      bool split = brui_width() > 400;
      bool listing_dirs = true;
      br_strv_t search_str = br_str_sub1(cur_path, cur_dir_name_len);
      if (split) brui_vsplitvp(3, BRUI_SPLITR(1), BRUI_SPLITA(5), BRUI_SPLITR(1));
      for (size_t i = 0; i < state->cur_dir.real_len; ++i) {
        br_fs_file_t file = state->cur_dir.arr[i];
        if (false == state->show_hidden_files && (file.name.len > 0 && file.name.str[0] == '.')) continue;
        if (false == br_strv_match(br_str_as_view(file.name), search_str)) continue;
        bool is_selected = state->select_index == (int)real_i;
        ++real_i;
        bool entered = false;
        if (listing_dirs && file.kind != br_fs_file_kind_dir) {
          listing_dirs = false;
          if (split) {
            brui_vsplit_pop();
            brui_vsplit_pop();
          }
        }
        if (is_selected) {
          brui_select_next();
          if (tab) {
            float ly = brui_local_y();
            float y = brui_y();
            float slack = (float)(2*brui_text_size());
            if (ly < 0) brui_scroll_move(ly - slack);
            if (y + slack > brui_max_y()) brui_scroll_move(y + slack -brui_max_y());
          }
          entered = brtl_key_pressed(BR_KEY_ENTER);
        }
        brui_vsplitvp(2, BRUI_SPLITA((float)ts), BRUI_SPLITR(1));
          if (brui_button_icon(BR_SIZEI(ts, ts), listing_dirs ? br_icon_folder((float)ts) : br_icon_file((float)ts))) entered = true;
        brui_vsplit_pop();
          if (is_selected) brui_select_next();
          if (brui_button(br_str_as_view(file.name)) || entered) {
            char last = cur_dir.str[cur_dir.len - 1];
            if (file.kind == br_fs_file_kind_dir) {
              if (false == dir_changed) {
                brsp_set(sp, state->path_id, br_str_as_view(cur_dir));
                if (last != '/' && last != '\\')  brsp_insert_char_at_end(sp, state->path_id, '/');
                brsp_insert_strv_at_end(sp, state->path_id, br_str_as_view(file.name));
                cur_path = brsp_get(*sp, state->path_id);
                if (cur_path.len > 0 && cur_path.str[cur_path.len - 1] != '/') brsp_insert_char_at_end(sp, state->path_id, '/');
                dir_changed = true;
                i = 10000;
              }
            } else if (file.kind == br_fs_file_kind_file) {
              if (state->file_selected == 0 || false == brsp_is_in(*sp, state->file_selected)) state->file_selected = brsp_new(sp);
              brsp_id_t selected_file = state->file_selected;
              brsp_clear(sp, selected_file);
              brsp_set(sp, selected_file, br_str_as_view(cur_dir));
              if (last != '/' && last != '\\')  brsp_insert_char_at_end(sp, selected_file, '/');
              brsp_insert_strv_at_end(sp, selected_file, br_str_as_view(file.name));
              brsp_zero(sp, selected_file);
              ret.is_selected = true;
              ret.selected_file = selected_file;
              i = 10000;
            }
          }
        brui_vsplit_end();
      }
      if (split) {
        if (listing_dirs) {
          brui_vsplit_pop();
          brui_vsplit_pop();
        }
        brui_vsplit_end();
      }
      if (real_i == 0) state->select_index = -1;
      else state->select_index = state->select_index % (int)real_i;
    brui_pop();
  if (brui_resizable_temp_pop()) state->is_open = false;
  if (dir_changed) {
    cur_path = brsp_get(*sp, state->path_id);
    if (true == br_fs_list_dir(cur_path, &state->cur_dir)) {
      state->select_index = -1;
      action->kind = brui_action_typing;
      action->args.text.id = state->path_id;
      action->args.text.cursor_pos = (int)cur_path.len;
    };
  }

  return ret;
}


static bool br_csv_parse(br_csv_parser_t* parser) {
  bool read_header = true;
  br_strv_t str = br_str_as_view(parser->content);
  br_strv_t cur = BR_STRV(str.str, 0);
  const char delimiter = ',';
  br_csv_cells_t* cells = NULL;
  int line = 1;
  bool success = true;
  (void)success;

  parser->header.len = 0;
  parser->rows.real_len = 0;

  if (parser->rows.len <= parser->rows.real_len) br_da_push(parser->rows, ((br_csv_cells_t) { 0 }));
  cells = br_da_getp(parser->rows, parser->rows.real_len);
  cells->len = 0;

  for (uint32_t i = 0; i < str.len; ++i) {
    char c = str.str[i];
    if (delimiter == c) {
      if (read_header) {
        br_da_push(parser->header, cur);
        cur.str = str.str + i + 1;
        cur.len = 0;
      } else {
        br_da_push(*cells, cur);
        cur.str = str.str + i + 1;
        cur.len = 0;
      }
    } else if ('\n' == c || '\r' == c) {
      if (read_header) {
        br_da_push(parser->header, cur);
        cur.str = str.str + i + 1;
        cur.len = 0;
        read_header = false;
        ++line;
      } else {
        if (cells->len != 0 || cur.len != 0 ) {
          br_da_push(*cells, cur);
          if (cells->len != parser->header.len) BR_ERROR("Expected %zu rows, but got %zu on line %d", parser->header.len, cells->len, line);
          ++parser->rows.real_len;
          if (parser->rows.len <= parser->rows.real_len) br_da_push(parser->rows, ((br_csv_cells_t) { 0 }));
          cells = br_da_getp(parser->rows, parser->rows.real_len);
          cells->len = 0;
          cur.str = str.str + i + 1;
          cur.len = 0;
        } else {
          cur.str = str.str + i + 1;
          cur.len = 0;
        }
      }
    } else ++cur.len;
  }
  if (0 != cells->len) BR_ERROR("Bad last line: %.*s", cur.len, cur.str);

error:
  return success;
}

void brgui_draw_csv_manager(brgui_csv_reader_t* reader, br_csv_parser_t* parser) {
  if (false == reader->is_open) return;

  brsp_t* sp = brtl_brsp();

  br_strv_t r_name = BR_STRL("CSV");
  brui_resizable_temp_push_t tr = brui_resizable_temp_push(r_name);
    if (tr.just_created) {
      tr.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      tr.res->target.cur_extent.width -= 100;
      tr.res->target.cur_extent.height -= 100;
      tr.res->target.cur_extent.x += 50;
      tr.res->target.cur_extent.y += 50;
      reader->first_coord = -1;
    }
    switch (parser->state) {
      case br_csv_state_init: {
        parser->file_path.len = 0;
        br_str_push_strv(&parser->file_path, brsp_get(*sp, reader->read_id));
        br_str_push_zero(&parser->file_path);
        if (false == br_fs_read(parser->file_path.str, &parser->content)) {
          parser->state = br_csv_state_file_read_error;
          break;
        }
        if (false == br_csv_parse(parser)) {
          parser->state = br_csv_state_parse_error;
          break;
        }
        parser->state = br_csv_state_ok;
      } break;
      default: {
        br_strv_t new_file_path = brsp_get(*sp, reader->read_id);
        if (false == br_strv_eq(new_file_path, br_str_as_view(parser->file_path))) {
          parser->state = br_csv_state_init;
        }
      } break;
    }
    brui_vsplitvp(3, BRUI_SPLITA(80), BRUI_SPLITA(4), BRUI_SPLITR(1));
      if (brui_button(BR_STRL("Refresh"))) {
        parser->state = br_csv_state_init;
      }
    brui_vsplit_pop();
    brui_vsplit_pop();
      brui_text(brsp_get(*sp, reader->read_id));
    brui_vsplit_end();
    switch (parser->state) {
      case br_csv_state_init: {
        brui_textf("Init");
      } break;
      case br_csv_state_ok: {
        brui_textf("Rows: %zu, Cols: %zu", parser->rows.real_len, parser->header.len);
        if (-1 == reader->first_coord) brui_textf("Select first coordinate");
        else brui_textf("Select second coordinate, first is '%.*s'", parser->header.arr[reader->first_coord].len, parser->header.arr[reader->first_coord].str);
        brui_vsplit((int)parser->header.len);
          for (size_t j = 0; j < parser->header.len; ++j) {
            brui_push();
              brui_text(parser->header.arr[j]);
              for (size_t i = 0; i < parser->rows.real_len; ++i) {
                br_csv_cells_t cells = br_da_get(parser->rows, i);
                brui_text(br_da_get(cells, j));
              }
            if (brui_pop()) {
              if (reader->first_coord == -1) {
                reader->first_coord = (int)j;
              } else {
                for (size_t k = 0; k < parser->rows.real_len; ++k) {
                  double x = strtod(parser->rows.arr[k].arr[reader->first_coord].str, NULL);
                  double y = strtod(parser->rows.arr[k].arr[j].str, NULL);
                  br_data_push_xy(brtl_datas(), x, y, 0);
                }
                reader->first_coord = -1;
              }
            }
            if (j + 1 < parser->header.len) brui_vsplit_pop();
          }
        brui_vsplit_end();
      } break;
      case br_csv_state_file_read_error: brui_text(BR_STRL("File read error")); break;
      case br_csv_state_parse_error: brui_text(BR_STRL("Parse error")); break;
    }
  if (brui_resizable_temp_pop()) reader->is_open = false;
}

static void brgui_draw_show_data(brgui_show_data_t* d, br_datas_t datas) {
  if (false == d->show) return;

  brui_resizable_temp_push_t tmp = brui_resizable_temp_push(BR_STRL("Data"));
    if (tmp.just_created) {
      tmp.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      tmp.res->target.cur_extent.width -= 100;
      tmp.res->target.cur_extent.height -= 100;
      tmp.res->target.cur_extent.x += 50;
      tmp.res->target.cur_extent.y += 50;
    }

    br_data_t* data = br_data_get1(datas, d->data_id);
    if (NULL != data) {
      float local_y = brui_local_y();
      float ts = (float)brui_text_size() + brui_padding_y();
      int i = 0;
      if (local_y < -ts) {
        int n = (int)(-(local_y + 2.f*ts)/ts);
        if (n > 0) {
          brui_new_lines(n);
          i += n;
        }
      }
      float space_left = brui_max_y() - brui_y();
      int max_n = (int)(space_left / ts) + 1;
      int lines = (int)data->len;
      int j = 0;
      switch (data->kind) {
        case br_data_kind_2d: {
          br_vec2d_t v2 = br_data_el_xy1(*data, i);
          brui_vsplit(2);
            for (j = 0; j < max_n && j + i < lines; ++j) v2 = br_data_el_xy1(*data, i + j), brui_textf("%f", v2.x);
          brui_vsplit_pop();
            for (j = 0; j < max_n && j + i < lines; ++j) v2 = br_data_el_xy1(*data, i + j), brui_textf("%f", v2.y);
          brui_vsplit_end();
        } break;
        case br_data_kind_3d: {
          br_vec3d_t v2;
          brui_vsplit(3);
            for (j = 0; j < max_n && j + i < lines; ++j) v2 = br_data_el_xyz1(*data, i + j), brui_textf("%f", v2.x);
          brui_vsplit_pop();
            for (j = 0; j < max_n && j + i < lines; ++j) v2 = br_data_el_xyz1(*data, i + j), brui_textf("%f", v2.y);
          brui_vsplit_pop();
            for (j = 0; j < max_n && j + i < lines; ++j) v2 = br_data_el_xyz1(*data, i + j), brui_textf("%f", v2.z);
          brui_vsplit_end();
        } break;
        default: BR_UNREACHABLE("Kind %d not handled", data->kind);
      }
      brui_new_lines(lines - i - j);
    }
  if (brui_resizable_temp_pop()) d->show = false;
}

static void brgui_draw_help(br_plotter_t* br) {
  brui_action_t* action = brui_action();
  if (action->kind == brui_action_none && brtl_key_pressed('h')) br->ui.help.show = !br->ui.help.show;
  if (false == br->ui.help.show) return;

  brui_resizable_temp_push_t tmp = brui_resizable_temp_push(BR_STRL("Help"));
    if (tmp.just_created) {
      tmp.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      tmp.res->target.cur_extent.width -= 100;
      tmp.res->target.cur_extent.height -= 100;
      tmp.res->target.cur_extent.x += 50;
      tmp.res->target.cur_extent.y += 50;
    }
    if (brui_collapsable(BR_STRL("Key bindings"), &br->ui.help.key_bindings)) {
      brui_text(BR_STRL("Right mouse button + Mouse move - Change plot offset"));
      brui_text(BR_STRL("Mouse wheel- Change plot zoom"));
      brui_text(BR_STRL("X + Mouse whele - Change zoom only in X axis"));
      brui_text(BR_STRL("Y + Mouse whele - Change zoom only in Y axis"));
      brui_text(BR_STRL("X|Y + SHIFT|CTRL - Zoom In|Out in X|Y axis"));
      brui_text(BR_STRL("F - Follow the line"));
      brui_text(BR_STRL("CTRL F - Focus on last points"));
      brui_text(BR_STRL("T - Add test points"));
      brui_text(BR_STRL("SHIFT + C - Clear all datas"));
      brui_text(BR_STRL("C - Empty all datas"));
      brui_text(BR_STRL("R - Reset camera to default values"));
      brui_text(BR_STRL("SHIFT + R - Reset camera zoom to default values"));
      brui_text(BR_STRL("CTRL + R - Reset camera offset to default values"));
      brui_text(BR_STRL("D - Toggle debug view"));
      brui_text(BR_STRL(""));
      brui_text(BR_STRL("LCTRL + Mouse - Mouse subwindows around the window"));
      brui_text(BR_STRL("LCTRL + Mouse on edge of the window - Resize subwindows"));
      brui_collapsable_end();
    }
    if (brui_collapsable(BR_STRL("CLI examples"), &br->ui.help.cli_help)) {
      brui_text(BR_STRL("Plot numbers from 1 to 100:"));
      brui_text(BR_STRL("   seq 100 | brplot"));
      brui_text(BR_STRL("Plot squeres of numbers from 1 to 100"));
      brui_text(BR_STRL("   python -c \"[print(x*x) for x in range(100)]\" | brplot;"));
      brui_text(BR_STRL(""));
      brui_text(BR_STRL("10      - Insert point (new_x, 10) to line group 0"));
      brui_text(BR_STRL("10,12   - Insert point (10, 12) to line group 0"));
      brui_text(BR_STRL("10;1    - Insert point (new_x, 10) to line group 1"));
      brui_text(BR_STRL("10,12;2 - Insert point (10, 12) to line group 2"));
      brui_collapsable_end();
    }
    if (brui_collapsable(BR_STRL("C example"), &br->ui.help.c_help)) {
      brui_text(BR_STRL("#define BRPLOT_IMPLEMENTATION"));
      brui_text(BR_STRL("#include \"brplot.h\""));
      brui_text(BR_STRL(""));
      brui_text(BR_STRL("int main(void) {"));
      brui_text(BR_STRL(" for (int i = -10; i < 10; ++i) brp_1(i, /* group_id */ 0);"));
      brui_text(BR_STRL(" brp_wait(); // Wait until plot window is closed"));
      brui_text(BR_STRL("}"));
      brui_collapsable_end();
    }
    if (brui_collapsable(BR_STRL("Python example"), &br->ui.help.python_help)) {
      brui_text(BR_STRL("import brplot"));
      brui_text(BR_STRL("brplot.plot(range(100))"));
      brui_collapsable_end();
    }
  if (brui_resizable_temp_pop()) br->ui.help.show = false;
}

void br_size_to_human_readable(size_t size, float* out_num, const char** out_unit) {
  if (size >> 10 == 0) {
    *out_num = (float)size;
    *out_unit = "B";
  } else if (size >> 20 == 0) {
    *out_num = (float)(size >> 10);
    *out_num += (float)(size - ((size_t)*out_num << 10)) / (float)(1<<10);
    *out_unit = "kB";
  } else if (size >> 30 == 0) {
    *out_num = (float)(size >> 20);
    *out_num += (float)(size - ((size_t)*out_num << 20)) / (float)(1<<20);
    *out_unit = "MB";
#if defined(BR_IS_SIZE_T_32_BIT)
  } else {
#else
  } else if (size >> 40 == 0) {
#endif
    *out_num = (float)(size >> 30);
    *out_num += (float)(size - ((size_t)*out_num << 30)) / (float)(1<<30);
    *out_unit = "GB";
#if defined(BR_IS_SIZE_T_32_BIT)
  }
#else
  } else {
    *out_num = (float)(size >> 40LLU);
    *out_num += (float)(size - ((size_t)*out_num << 40LLU)) / (float)(1LLU<<40LLU);
    *out_unit = "TB";
  }
#endif
}

#if defined(BR_HAS_MEMORY)
static void brgui_draw_memory(br_plotter_t* br) {
  if (false == br->ui.memory.show) return;

  brui_resizable_temp_push_t tmp = brui_resizable_temp_push(BR_STRL("Malloc"));
    if (tmp.just_created) {
      tmp.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      tmp.res->target.cur_extent.width -= 100;
      tmp.res->target.cur_extent.height -= 100;
      tmp.res->target.cur_extent.x += 50;
      tmp.res->target.cur_extent.y += 50;
      br->ui.memory.selected_frame = -1;
      br->ui.memory.selected_nid = -1;
    }
    br_memory_t tr = br_memory_get();
    float human = 0;
    const char* unit = NULL;
    br_size_to_human_readable(tr.total_alloced, &human, &unit);
    brui_textf("Total alloced: %f%s", human, unit);
    br_size_to_human_readable(tr.cur_alloced, &human, &unit);
    brui_textf("Current alloced: %f%s", human, unit);
    br_size_to_human_readable(tr.max_alloced, &human, &unit);
    brui_textf("Max alloced: %f%s", human, unit);
    brui_text(BR_STRL(""));
    brui_textf("Current frame: %d", tr.current_frame);
    br_size_to_human_readable(tr.cur_frame_alloced, &human, &unit);
    brui_textf("Current frame alloced: %f%s", human, unit);
    br_size_to_human_readable(tr.cur_frame_freed, &human, &unit);
    brui_textf("Current frame freed: %f%s", human, unit);
    brui_text(BR_STRL(""));
    br_memory_frame_t last = br_da_top(tr.frames);
    brui_textf("Last frame with some action: %d", last.frame_num);
    brui_text(BR_STRL(""));
    brui_textf("Events Len: %zu", tr.len);

    int sel_frame = br->ui.memory.selected_frame;
    int sel_nid = br->ui.memory.selected_nid;
    brui_push();
      char* scrach = br_scrach_get(1024);
      if (sel_frame == -1) {
      } else {
        if (sel_nid == -1) {
          brui_vsplitvp(2, BRUI_SPLITR(1), BRUI_SPLITR(3));
        } else {
          brui_vsplitvp(3, BRUI_SPLITR(1), BRUI_SPLITR(3), BRUI_SPLITR(4));
        }
      }

      for (size_t i = 0; i < tr.frames.len; ++i) {
        brui_vsplitvp(2, BRUI_SPLITA(1.f * (float)brui_text_size()), BRUI_SPLITR(1.f));
          brui_textf("%zu", i);
        brui_vsplit_pop();
          int n = sprintf(scrach, "Frame %d len=%d", tr.frames.arr[i].frame_num, tr.frames.arr[i].len);
          if (brui_button(BR_STRV(scrach, (uint32_t)n))) br->ui.memory.selected_frame = (int)i;
        brui_vsplit_end();
      }

      if (sel_frame == -1) {
      } else {
        brui_vsplit_pop();
          br_memory_frame_t frame = tr.frames.arr[sel_frame];
          for (int i = 0; i < frame.len; ++i) {
            br_memory_node_t mem_node = tr.arr[frame.start_nid + i];
            int n;
            const char* word = "Unknown";
            switch (mem_node.kind) {
              case br_memory_event_free:    word = "Free"; break;
              case br_memory_event_realloc: word = "Realloced"; break;
              case br_memory_event_alloc:   word = "Alloeced"; break;
              default: break;
            }
            n = sprintf(scrach, "[%s:%d] %s %zu bytes", mem_node.at_file_name, mem_node.at_line_num, word, mem_node.size);
            if (brui_button(BR_STRV(scrach, (uint32_t)n))) {
              br->ui.memory.selected_nid = frame.start_nid + i;
            }
          }
          if (sel_nid != -1) {
            brui_vsplit_pop();
            int cur_nid = sel_nid;
            br_memory_node_t mem_node = br_da_get(tr, sel_nid);
            while (mem_node.next_nid != -1) {
              cur_nid = mem_node.next_nid;
              mem_node = br_da_get(tr, mem_node.next_nid);
            }
            while (cur_nid != -1) {
              int n;
              mem_node = br_da_get(tr, cur_nid);
              const char* word = "Unknown";
              switch (mem_node.kind) {
                case br_memory_event_free:    word = "Free"; break;
                case br_memory_event_realloc: word = "Realloced"; break;
                case br_memory_event_alloc:   word = "Alloeced"; break;
                default: break;
              }
              n = sprintf(scrach, "[%s:%d Frame %d] %s %zu bytes", mem_node.at_file_name, mem_node.at_line_num, mem_node.frame_num, word, mem_node.size);
              brui_text(BR_STRV(scrach, (uint32_t)n));
              cur_nid = mem_node.prev_nid;
            }
          }
        brui_vsplit_end();
      }

      br_scrach_free();
    brui_pop();
  if (brui_resizable_temp_pop()) br->ui.memory.show = false;
}
#endif

void brgui_draw_add_expression(br_plotter_t* br) {
  brgui_add_expression_t* e = &br->ui.add_expression;
  if (false == e->show) return;

  bool refresh = false;
  int group_id = 12;
  brsp_t* sp = brtl_brsp();
  if (e->input_id <= 0) e->input_id = brsp_new(sp);

  if (brui_resizable_temp_push(BR_STRL("Add expression")).just_created) {
    refresh = true;
  }
    brui_push();
      brui_text(BR_STRL("Expression:"));
      brui_push();
        if (brui_text_input(e->input_id)) refresh = true;
      brui_pop();
    brui_pop();
  if (brui_resizable_temp_pop()) e->show = false;
  if (refresh) {
    br_plotter_data_remove(br, group_id);
    br_dagens_add_expr_str(&br->dagens, &br->groups, brsp_get(*sp, e->input_id), group_id);
  }
}

static bool brgui_draw_plot_menu(br_plot_t* plot, br_datas_t datas) {
  int og_text_size = brui_text_size();
  int icon_size = og_text_size;
  bool ret = false;
  if (brui_resizable_is_hidden(plot->menu_extent_handle)) {
    if (brui_button_icon(BR_SIZEI(icon_size, icon_size), br_icon_menu((float)og_text_size))) brui_resizable_show(plot->menu_extent_handle, true);
  } else {
    brui_resizable_push(plot->menu_extent_handle);
      brui_vsplitvp(2, BRUI_SPLITA((float)icon_size), BRUI_SPLITR(1));
        char* c = br_scrach_get(4096);
        if (brui_button_icon(BR_SIZEI(icon_size, icon_size), br_icon_back((float)og_text_size))) brui_resizable_show(plot->menu_extent_handle, false);
      brui_vsplit_pop();
        brui_text_size_set(og_text_size);
        brui_text_align_set(br_text_renderer_ancor_mid_mid);
        brui_maxy_set(brui_min_y() + (float)icon_size);
        brui_textf("%s Plot", plot->kind == br_plot_kind_2d ? "2D" : "3D");
      brui_vsplit_end();
      brui_text(BR_STRL("_______________________________________________________"));
      if (brui_button(BR_STRL("Remove Plot"))) {
        ret = true;
      }
      if (plot->kind == br_plot_kind_2d) {
        brui_sliderf2(BR_STRL("Line Thickness"), &plot->dd.line_thickness);
        brui_vsplit(2);
          brui_sliderf2(BR_STRL("Grid Thick"), &plot->dd.grid_line_thickness);
        brui_vsplit_pop();
          brui_sliderf2(BR_STRL("Major Grid Thick"), &plot->dd.grid_major_line_thickness);
        brui_vsplit_end();
      }
      brui_text_size_set(og_text_size/5*4);
      brui_checkbox(BR_STRL("Follow"), &plot->follow);
      bool show_legend = false == brui_resizable_is_hidden(plot->legend_extent_handle);
      if (brui_checkbox(BR_STRL("Show Legend"), &show_legend)) {
        brui_resizable_show(plot->legend_extent_handle, show_legend);
      };
      for (size_t k = 0; k < datas.len; ++k) {
        bool is_shown = false;
        br_data_t* data = br_da_getp(datas, k);
        for (int j = 0; j < plot->data_info.len; ++j) {
          if (br_da_get(plot->data_info, j).group_id == data->group_id) {
            is_shown = true;
            break;
          }
        }
        br_strv_t name = brsp_get(*brtl_brsp(), data->name);
        sprintf(c, "%.*s ( #%d )", name.len, name.str, data->group_id);
        if (brui_checkbox(br_strv_from_c_str(c), &is_shown)) {
          if (false == is_shown) br_da_remove_feeld(plot->data_info, group_id, data->group_id);
          else {
            br_da_push_t(int, plot->data_info, BR_PLOT_DATA(data->group_id));
          }
        }
      }
      for (int j = 0; j < plot->data_info.len; ++j) {
        br_plot_data_t di = plot->data_info.arr[j];
        brui_textf("  thick: %.2f -> %.2f", di.thickness_multiplyer, di.thickness_multiplyer_target);
      }
      br_scrach_free();
      if (plot->kind == br_plot_kind_2d) {
        brui_textf("Offset: %f %f", BR_VEC2_(plot->dd.offset));
        brui_textf("Zoom: %f %f", BR_VEC2_(plot->dd.zoom));
      } else if (plot->kind == br_plot_kind_3d) {
        brui_sliderf2(BR_STRL("Fov"), &plot->ddd.fov_y);
      }
    brui_resizable_pop();
  }
  return ret;
}

static void draw_left_panel(br_plotter_t* br) {
  brui_resizable_push(br->resizables.menu_extent_handle);
    if (brui_collapsable(BR_STRL("File"), &br->ui.expand_file)) {
      if (brui_button(BR_STRL("Import CSV"))) {
        br->ui.fm_state.is_inited = true;
        br->ui.fm_state.action = brgui_file_manager_import_csv;
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Plots"), &br->ui.expand_plots)) {
      brui_vsplit(2);
        if (brui_button(BR_STRL("Add 2D"))) {
          br_plotter_add_plot_2d(br);
        }
      brui_vsplit_pop();
        if (brui_button(BR_STRL("Add 3D"))) {
          br_plotter_add_plot_3d(br);
        }
      brui_vsplit_end();
      if (brui_button(BR_STRL("Add Expression"))) {
        br->ui.add_expression.show = true;
      }

      for (int i = 0; i < br->plots.len; ++i) {
        char* scrach = br_scrach_get(4096);
          br_plot_t* plot = br_da_getp(br->plots, i);
          int n = sprintf(scrach, "%s Plot %d", plot->kind == br_plot_kind_2d ? "2D" : "3D", i);
          bool is_visible = !brui_resizable_is_hidden(plot->extent_handle);
          if (brui_checkbox(BR_STRV(scrach, (uint32_t)n), &is_visible)) brui_resizable_show(plot->extent_handle, is_visible);
        br_scrach_free();
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Optimizations"), &br->ui.expand_optimizations)) {
      brui_sliderf3(BR_STRL("min something"), brtl_min_sampling(), 4);
      brui_sliderf2(BR_STRL("cull min"), brtl_cull_min());
      brui_checkbox(BR_STRL("Debug"), brtl_debug());
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("UI Styles"), &br->ui.expand_ui_styles)) {
      if (brui_checkbox(BR_STRL("Dark Theme"), &br->ui.dark_theme)) {
        if (br->ui.dark_theme) br_theme_dark();
        else br_theme_light();
      }
      brui_vsplit(2);
        brui_sliderf(BR_STRL("padding.y"), &BR_THEME.ui.padding.y);
      brui_vsplit_pop();
        brui_sliderf(BR_STRL("padding.x"), &BR_THEME.ui.padding.x);
      brui_vsplit_end();
      brui_sliderf2(BR_STRL("thick"), &BR_THEME.ui.border_thick);
      brui_slideri(BR_STRL("Font Size"), &BR_THEME.ui.font_size);
      brui_sliderf2(BR_STRL("Animation Speed"), &BR_THEME.ui.animation_speed);
      if (brui_button(BR_STRL("Load font"))) {
        br->ui.fm_state.is_inited = true;
        br->ui.fm_state.action = brgui_file_manager_load_font;
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Export"), &br->ui.expand_export)) {
      if (brui_button(BR_STRL("Brp format"))) {
        br_plotter_export(br, "test.brp");
      }
      if (brui_button(BR_STRL("CSV"))) {
        br_plotter_export_csv(br, "test.csv");
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("Data"), &br->ui.expand_data)) {
      brui_text_size_set((int)((float)brui_text_size() * 0.8f));
      for (size_t i = 0; i < br->groups.len; ++i) {
        brui_push();
          br_data_t data = br_da_get(br->groups, i);
          br_strv_t name = brsp_get(*brtl_brsp(), data.name);
          brui_text_input(data.name);
          brui_textf("%.*s (%d) (%zu points)", name.len, name.str, data.group_id, data.len);
          brui_textf("%.2fms (%.3f %.3f)", br_resampling_get_draw_time(data.resampling)*1000.0f, br_resampling_get_something(data.resampling), br_resampling_get_something2(data.resampling));
          brui_textf("name id: %d", data.name);
          switch (data.kind) {
            case br_data_kind_2d: {
              brui_textf("2D rebase: %.3f, %.3f", data.dd.rebase_x, data.dd.rebase_y);
              br_bb_t bb = br_bb_add(data.dd.bounding_box, BR_VEC2((float)data.dd.rebase_x, (float)data.dd.rebase_y));
              brui_textf("   box: (%.3f, %.3f) (%.3f, %3.f)", BR_BB_(bb));
            } break;
            case br_data_kind_3d: {
              brui_textf("3D rebase: %.3f, %.3f, %.3f", data.ddd.rebase_x, data.ddd.rebase_y, data.ddd.rebase_z);
            } break;
          }
        if (brui_pop()) {
          br->ui.show_data.show = true;
          br->ui.show_data.data_id = data.group_id;
        }
      }
      brui_collapsable_end();
    }

    if (brui_collapsable(BR_STRL("About"), &br->ui.expand_about)) {
      if (brui_button(BR_STRL("About"))) {
        br->ui.show_about = true;
      }
      if (brui_button(BR_STRL("License"))) {
        br->ui.show_license = true;
      }
      if (brui_button(BR_STRL("Help"))) {
        br->ui.help.show = true;
      }
#if defined(BR_HAS_MEMORY)
      if (brui_button(BR_STRL("Memory"))) {
        br->ui.memory.show = true;
      }
#endif
      brui_collapsable_end();
    }

    if (brui_button(BR_STRL("Exit"))) {
      br->should_close = true;
    }
  brui_resizable_pop();
}

static void brgui_draw_debug_window_rec(br_plotter_t* br, int handle, int depth) {
   brui_resizable_t r = brtl_bruirs()->arr[handle];
   brui_textf("%*s, %d Res: z: %d, max_z: %d, max_sib_z: %d, parent: %d", depth*2, "", handle, r.z, r.max_z, brui_resizable_sibling_max_z(handle), r.parent);
   brfl_foreach(i, *brtl_bruirs()) if (i != 0 && brtl_bruirs()->arr[i].parent == handle) brgui_draw_debug_window_rec(br, i, 1+depth);
}

static void brgui_draw_debug_window(br_plotter_t* br) {
  if (false == *brtl_debug()) return;
  if (brui_resizable_temp_push(BR_STRL("Debug")).just_created) {
    brui_ancor_set(brui_ancor_left);
  }
    brgui_draw_debug_window_rec(br, 0, 0);
    uint32_t chars_per_row = 8;
    brui_push();
      if (brui_button(BR_STRL("Compress"))) {
        brsp_compress(brtl_brsp(), 1.0f, 0);
      }
      brui_textf("Pool size: %d, elements: %d, free_len: %d", brtl_brsp()->pool.len, brtl_brsp()->len, brtl_brsp()->free_len);
      for (int i = 0; i < brtl_brsp()->len; ++i) {
        brsp_node_t node = brtl_brsp()->arr[i];
        brui_textf("#%d |%c| start_index: %04x, len: %d, cap: %d", i, brfl_is_free(*brtl_brsp(), i) ? ' ' : 'X', node.start_index, node.len, node.cap);
      }
      char* buff = br_scrach_get(128);
        for (uint32_t i = 0; i < brtl_brsp()->pool.len; i += chars_per_row) {
          brui_vsplitvp(2, BRUI_SPLITA(70), BRUI_SPLITR(1));
            brui_text_color_set(brtl_theme()->colors.btn_txt_inactive);
            brui_textf("0x%x", i);
          brui_vsplit_pop();
            char* cur = buff;
            for (uint32_t j = 0; j < chars_per_row && i + j < brtl_brsp()->pool.len; ++j) {
              unsigned char c = (unsigned char)brtl_brsp()->pool.str[i + j];
              if (c < 16) {
                *cur++ = '0';
                *cur++ = (char)((int)c >= 10 ? 'A'+(char)(c - 10) : ('0' + c));
                *cur++ = ' ';
              } else {
                char upper = (char)((int)c >> 4);
                char downer = c & 0x0F;
                *cur++ = (int)upper >= 10 ? 'A'+(char)(upper - 10) : ('0' + upper);
                *cur++ = (int)downer >= 10 ? 'A'+(char)(downer - 10) : ('0' + downer);
                *cur++ = ' ';
              }
            }
            brui_text(BR_STRV(buff, (uint32_t)(cur - buff)));
          brui_vsplit_end();
        }
      br_scrach_free();
    brui_pop();
  if (brui_resizable_temp_pop()) *brtl_debug() = false;
}

static void brgui_draw_license(br_plotter_t* br) {
  if (false == br->ui.show_license) return;
  brui_resizable_temp_push_t tmp = brui_resizable_temp_push(BR_STRL("License"));
    if (tmp.just_created) {
      tmp.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      tmp.res->target.cur_extent.width -= 100;
      tmp.res->target.cur_extent.height -= 100;
      tmp.res->target.cur_extent.x += 50;
      tmp.res->target.cur_extent.y += 50;
      for (int i = 0; i < br_license_lines; ++i) printf("%.*s\n", br_license[i].len, br_license[i].str);
    }
    float local_y = brui_local_y();
    float ts = (float)brui_text_size() + brui_padding_y();
    int i = 0;
    if (local_y < -ts) {
      int n = (int)(-(local_y + 2.f*ts)/ts);
      if (n > 0) {
        brui_new_lines(n);
        i += n;
      }
    }
    float space_left = brui_max_y() - brui_y();
    int max_n = (int)(space_left / ts) + 1;

    for (int j = 0; j < max_n && i < br_license_lines; ++j, ++i) brui_text(br_license[i]);

    brui_new_lines(br_license_lines - i);
  if (brui_resizable_temp_pop()) br->ui.show_license = false;
}

static void brgui_draw_about(br_plotter_t* br) {
  if (false == br->ui.show_about) return;
  brui_resizable_temp_push_t tmp = brui_resizable_temp_push(BR_STRL("About"));
    if (tmp.just_created) {
      tmp.res->target.cur_extent = BR_EXTENTI_TOF(brtl_viewport());
      tmp.res->target.cur_extent.width -= 100;
      tmp.res->target.cur_extent.height -= 100;
      tmp.res->target.cur_extent.x += 50;
      tmp.res->target.cur_extent.y += 50;
    }
    brui_text(BR_STRL("Brplot © 2025 Branimir Ričko [branc116]"));
    brui_textf("%d.%d.%d-%s-%s", BR_MAJOR_VERSION, BR_MINOR_VERSION, BR_PATCH_VERSION, BR_GIT_BRANCH, BR_GIT_HASH);
  if (brui_resizable_temp_pop()) br->ui.show_about = false;
}

void br_plot_screenshot(br_text_renderer_t* tr, br_plot_t* plot, br_shaders_t* shaders, br_datas_t groups, char const* path) {
  (void)tr; (void)plot; (void)shaders; (void)groups; (void)path;
#// TODO: Make this more better...
//  int left_pad = 80;
//  int bottom_pad = 80;
//  plot->resolution = BR_SIZEI(1280, 720);
//  RenderTexture2D target = LoadRenderTexture(plot->resolution.width, plot->resolution.height); // TODO: make this values user defined.
//  plot->graph_screen_rect = BR_EXTENTI(left_pad, 0, plot->resolution.width - left_pad, plot->resolution.height - bottom_pad);
//  br_plot_update_context(plot, brtl_mouse_get_pos());
//  br_plot_update_shader_values(plot, shaders);
//  BeginTextureMode(target);
//    br_smol_mesh_grid_draw(plot, shaders);
//    br_datas_draw(groups, plot, shaders);
//    draw_grid_numbers(tr, plot);
//  EndTextureMode();
//  Image img = LoadImageFromTexture(target.texture);
//  ImageFlipVertical(&img);
//  ExportImage(img, path);
//  UnloadImage(img);
//  UnloadRenderTexture(target);
}

