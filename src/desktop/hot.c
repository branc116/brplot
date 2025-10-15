#include "src/br_pp.h"
#include "src/br_text_renderer.h"
#include "src/br_ui.h"
#include "src/br_plotter.h"
#include "external/stb_ds.h"

#include <stdio.h>

static bool context_menu_opened = false; 

void br_hot_init(void* br) {
  (void)br;
  printf("Init\n");
}

void brui_context_menu_delete(void);
void brui_context_menu(void) {
  /*
  bool should_delete = false;
  if (context_menu_opened) {
    brui_resizable_temp_push_t rt = brui_resizable_temp_push(BR_STRL("context_menu"));
    if (rt.just_created) {
      brui_resizable_t* t = brui_resizable_get(rt.resizable_handle);
      br_vec2_t pos = brui_resizable_to_global(rt.resizable_handle, brtl_mouse_pos());
      t->cur_extent.pos = pos;
      t->target.cur_extent.pos = pos;
      t->title_enabled  = false;
    }
    if (brui_button(BR_STRL("Close Context Menu"))) {
      should_delete = true;
    }
    brui_textf("Hello Context: %d", brui_stack()->active_resizable);
    brui_resizable_pop();
  }
  if (brtl_mouser_pressed()) {
    context_menu_opened = !context_menu_opened;
    if (false == context_menu_opened) brui_context_menu_delete();
  }
  if (brtl_mousel_pressed()) {
    context_menu_opened = false;
  }
  if (should_delete) brui_context_menu_delete();
  */
}

void brui_context_menu_delete(void) {
  /*
  context_menu_opened = false;
  brui_resizable_temp_delete(BR_STRL("context_menu"));
  */
}

void br_hot_loop(void* obr) { (void)obr; }
void br_hot_loop_ui(void* obr) {
  (void)obr;
  brui_context_menu();
}
