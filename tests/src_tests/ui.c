#include "tests/src_tests/shl.h"

int main(void) {
  BR_FILE* file = BR_FOPEN("test", "rwb");
  int res_handle;
  {
    brui_window_t win = { .pl = { .kind = brpl_window_headless } };
    brui_window_init(&win);
    res_handle = brui_resizable_new(&win.resizables, BR_EXTENT(0, 0, 720, 480), 0);
    brui_save(file, &win);
    brui_window_deinit(&win);
  }

  {
    brui_window_t win = { .pl = { .kind = brpl_window_headless } };
    brui_window_init(&win);
    bool is_ok = brui_load(file, &win);
    TEST_EQUAL(is_ok, true);
    brui_resizable_t rs = br_da_get(win.resizables, res_handle);
    {
      br_extent_t ex = br_animex_get_target(&win.anims, rs.cur_extent_ah);
      TEST_EQUALF(ex.x, 0); TEST_EQUALF(ex.y, 0);
      TEST_EQUALF(ex.width, 720); TEST_EQUALF(ex.height, 480);
    }
    {
      br_extent_t ex = br_animex(&win.anims, rs.cur_extent_ah);
      TEST_EQUALF(ex.x, 0); TEST_EQUALF(ex.y, 0);
      TEST_EQUALF(ex.width, 720); TEST_EQUALF(ex.height, 480);
    }
    brui_window_deinit(&win);
  }
}

