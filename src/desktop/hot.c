#include "src/br_pp.h"
#include "src/br_text_renderer.h"
#include "src/br_tl.h"
#include "src/br_ui.h"
#include "src/br_plotter.h"
#include "external/stb_ds.h"


#include <stdio.h>



typedef struct brui_resizable_temp_t {
  size_t key;
  int value;
} brui_resizable_temp_t;

typedef struct brui_resizable_temp_push_t {
  int resizable_handle;
  bool just_created;
} brui_resizable_temp_push_t;

static bool context_menu_opened = false; 
static brui_resizable_temp_t* res = NULL;

br_vec2_t brui_resizable_to_global(int resizable_handle, br_vec2_t pos) {
  if (resizable_handle == 0) return pos;
  brui_resizable_t* r = brui_resizable_get(resizable_handle);
  pos = br_vec2_sub(pos, r->cur_extent.pos);
  float hidden_heigth = r->full_height - r->cur_extent.height;
  if (hidden_heigth > 0) pos.y += r->scroll_offset_percent * hidden_heigth;
  return brui_resizable_to_global(r->parent, pos);
}

brui_resizable_temp_push_t brui_resizable_temp_push(br_strv_t id) {
  // TODO: Handle hash collisions
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeefdeadbeef);
  int res_handle = -1;

  ptrdiff_t index = stbds_hmgeti(res, hash);
  bool just_created = false;
  if (index < 0) {
    res_handle = brui_resizable_new(BR_EXTENT(0, 0, 100, 100), brui_stack()->active_resizable);
    index = stbds_hmput(res, hash, res_handle);
    just_created = true;
  } else {
    res_handle = res[index].value;
  }

  brui_resizable_push(res_handle);
  return (brui_resizable_temp_push_t) { .resizable_handle = res_handle, .just_created = just_created };
}

void brui_resizable_temp_delete(br_strv_t id) {
  size_t hash = stbds_hash_bytes((void*)id.str, id.len, 0xdeadbeefdeadbeef);
  int handle = -1;
  ptrdiff_t index = stbds_hmgeti(res, hash);
  if (index >= 0) {
    handle = res[index].value;
    brui_resizable_delete(handle);
    stbds_hmdel(res, hash);
  }
}

void br_hot_init(void* br) {
  (void)br;
  res = NULL;
  printf("Eat a dick\n");
}

void brui_context_menu_delete(void);
void brui_context_menu(void) {
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
}

void brui_context_menu_delete(void) {
  context_menu_opened = false;
  brui_resizable_temp_delete(BR_STRL("context_menu"));
}

void br_hot_loop(void* obr) { (void)obr; }
void br_hot_loop_ui(void* obr) {
  (void)obr;
  brui_context_menu();
}
