#include "plotter.h"
#include <raylib.h>
#include <string.h>
#include <stdlib.h>
#include "tests.h"

static float padding = 4.f;

static void file_saver_change_cwd_index(file_saver_t* fs, size_t i);
static void file_saver_change_cwd_up(file_saver_t* fs);
static void file_saver_handle_input(file_saver_t* fs);

file_saver_t* file_saver_malloc(const char* cwd, const char* default_filename, const char* file_exension, float font_size, void (*callback)(void*, bool), void* arg) {
  size_t full_size = sizeof(file_saver_t) + CWD_MAX_SIZE + FILE_EXTENSION_MAX_SIZE + CUR_NAME_MAX_SIZE;
  file_saver_t* fe = (file_saver_t*)malloc(full_size);
  memset(fe, 0, full_size);
  fe->cwd = (char*)fe + sizeof(file_saver_t);
  fe->file_extension = (char*)fe + sizeof(file_saver_t) + CWD_MAX_SIZE;
  fe->cur_name = (char*)fe + sizeof(file_saver_t) + CWD_MAX_SIZE + FILE_EXTENSION_MAX_SIZE;
  fe->rect = (Rectangle) { .x = padding, .y = padding, .height = (float)GetScreenHeight() - 2*padding, .width = (float)GetScreenWidth() - 2*padding };
  memcpy(fe->cwd, cwd, strlen(cwd));
  memcpy(fe->file_extension, file_exension, strlen(file_exension));
  memcpy(fe->cur_name, default_filename, strlen(default_filename));
  fe->scroll_position = 0.f;
  fe->font_size = font_size;
  fe->callback = callback;
  fe->paths = (FilePathList){0};
  fe->arg = arg;
  return fe;
}

void file_saver_free(file_saver_t* fe) {
  UnloadDirectoryFiles(fe->paths);
  free(fe);
}

void file_saver_draw(file_saver_t* fe) {
  context.mouse_screen_pos = GetMousePosition();
  ui_stack_buttons_init((Vector2) {.x = fe->rect.x, .y = fe->rect.y}, &fe->scroll_position, fe->font_size);
  ui_stack_set_size((Vector2){.x = fe->rect.width, .y = fe->rect.height });
  ui_stack_buttons_add(NULL, fe->cwd);
  bool is_selected = fe->selected_index == 0;
  if (2 == ui_stack_buttons_add(&is_selected, "..")) {
    file_saver_change_cwd_up(fe);
  }
  if (fe->paths.paths == NULL) {
    fe->paths = LoadDirectoryFiles(fe->cwd);
  }
  for (size_t i = 0, j = 0; i < fe->paths.count; ++i) {
    if (DirectoryExists(fe->paths.paths[i]) == false) continue;
    is_selected = ++j == fe->selected_index;
    if (2 == ui_stack_buttons_add(&is_selected, fe->paths.paths[i])) {
      file_saver_change_cwd_index(fe, i);
      break;
    }
  }
  ui_stack_buttons_end();
  if (CheckCollisionPointRec(context.mouse_screen_pos, fe->rect)) {
    file_saver_handle_input(fe);
  }
}

char const* file_saver_get_full_path(file_saver_t* fs) {
  static char path[CWD_MAX_SIZE];
  int index = 0;
  int len_cwd = strlen(fs->cwd), len_name = strlen(fs->cur_name), len_ext = strlen(fs->file_extension);
  memcpy(path, fs->cwd, len_cwd), index += len_cwd;
  if (path[index - 1] != '/') path[index++] = '/';
  memcpy(&path[index], fs->cur_name, len_name), index += len_name;
  for (int i = index - 1, j = len_ext - 1; i >= 0 && j >= 0; --i, --j) {
    if (fs->file_extension[j] != path[i]) {
      memcpy(&path[index], fs->file_extension, len_ext), index += len_ext;
      break;
    }
  }
  path[index] = (char)0;
  return path;
}

static void file_saver_filter_directories(FilePathList* list) {
  for (size_t i = 0; i < list->count; ) {
    if (DirectoryExists(list->paths[i]) == false) {
      RL_FREE(list->paths[i]);
      for (size_t j = i + 1; j < list->count; ++j) {
        list->paths[j - 1] = list->paths[j];
      }
      --list->count;
      --list->capacity;
    } else ++i;
  }
  qsort(list->paths, list->count, sizeof(list->paths), (int (*)(const void*, const void*))strcmp);
}

static void file_saver_change_cwd_index(file_saver_t* fs, size_t i) {
  size_t sz = strlen(fs->paths.paths[i]);
  memcpy(fs->cwd, fs->paths.paths[i], sz);
  fs->cwd[sz] = (char)0;
  UnloadDirectoryFiles(fs->paths);
  fs->paths = LoadDirectoryFiles(fs->cwd);
  fs->selected_index = 0;
  fs->scroll_position = 0.f;
  file_saver_filter_directories(&fs->paths);
}

static void file_saver_change_cwd_up(file_saver_t* fs) {
  // TODO: This will not work on windows...
  size_t s = strlen(fs->cwd);
  for (size_t i = s - 1; i > 0; --i) {
    if (fs->cwd[i] == '/') {
      fs->cwd[i] = (char)0;
      break;
    }
    fs->cwd[i] = (char)0;
  }
  UnloadDirectoryFiles(fs->paths);
  fs->paths = LoadDirectoryFiles(fs->cwd);
  fs->selected_index = 0;
  fs->scroll_position = 0.f;
  file_saver_filter_directories(&fs->paths);
}

static void file_saver_handle_input(file_saver_t* fs) {
  if (IsKeyPressed(KEY_ESCAPE)) fs->callback(fs->arg, false);
  if (IsKeyPressed(KEY_ENTER)) {
    if (fs->selected_index == 0) {
      file_saver_change_cwd_up(fs);
    }
    for (size_t i = 0, j = 1; i < fs->paths.count; ++i) {
      if (DirectoryExists(fs->paths.paths[i])) {
        if (j == fs->selected_index) {
          file_saver_change_cwd_index(fs, i);
          break;
        }
        ++j;
      }
    }
  }
  if (IsKeyPressed(KEY_UP)) {
    fs->selected_index = fs->selected_index == 0 ? 0 : fs->selected_index - 1;
  }
  if (IsKeyPressed(KEY_DOWN)) {
    fs->selected_index = minui64(fs->paths.count, fs->selected_index + 1);
  }
}

TEST_CASE(FullNameTest) {
  file_saver_t* fs = file_saver_malloc("/home", "test", ".png", 1.f, NULL, NULL);
  TEST_STREQUAL(file_saver_get_full_path(fs), "/home/test.png");
  file_saver_free(fs);
  fs = file_saver_malloc("/home/", "test.png", ".png", 1.f, NULL, NULL);
  TEST_STREQUAL(file_saver_get_full_path(fs), "/home/test.png");
  file_saver_free(fs);
  fs = file_saver_malloc("/home/", "test.pn", ".png", 1.f, NULL, NULL);
  TEST_STREQUAL(file_saver_get_full_path(fs), "/home/test.pn.png");
  file_saver_free(fs);
}
