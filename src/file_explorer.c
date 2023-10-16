#include "plotter.h"
#include <string.h>
#include <stdlib.h>

static float padding = 4.f;
file_saver_t* file_saver_malloc(const char* cwd, const char* file_exension, const char* default_filename, float font_size, void (*callback)(void*, bool)) {
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
  return fe;
}

void file_saver_free(file_saver_t* fe) {
  free(fe);
}

void file_saver_draw(file_saver_t* fe) {
  context.mouse_screen_pos = GetMousePosition();
  ui_stack_buttons_init((Vector2) {.x = fe->rect.x, .y = fe->rect.y}, &fe->scroll_position, fe->font_size);
  ui_stack_set_size((Vector2){.x = fe->rect.width, .y = fe->rect.height });
  ui_stack_buttons_add(NULL, fe->cwd);
  if (2 == ui_stack_buttons_add(NULL, "..")) {
    // TODO: This will not work on windows...
    size_t s = strlen(fe->cwd);
    for (size_t i = s - 1; i > 0; --i) {
      fe->cwd[i] = (char)0;
      if (fe->cwd[i] == '/') {
        break;
      }
    }
    UnloadDirectoryFiles(fe->paths);
    fe->paths = LoadDirectoryFiles(fe->cwd);
  }
  if (fe->paths.paths == NULL) {
    fe->paths = LoadDirectoryFiles(fe->cwd);
  }
  for (size_t i = 0; i < fe->paths.count; ++i) {
    bool is_on = false;
    if (2 == ui_stack_buttons_add(&is_on, fe->paths.paths[i])) {
      size_t sz = strlen(fe->paths.paths[i]);
      memcpy(fe->cwd, fe->paths.paths[i], sz);
      fe->cwd[sz] = (char)0;
      UnloadDirectoryFiles(fe->paths);
      fe->paths = LoadDirectoryFiles(fe->cwd);
      break;
    }
  }
  ui_stack_buttons_end();
}

char const* file_saver_get_full_path(file_saver_t* fs) {
  (void)fs;
  return "TODO.png";
}
