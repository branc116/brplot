#include "src/br_anim.h"
#include "src/br_theme.h"
#include "src/br_da.h"
#include "src/br_free_list.h"

#include <errno.h>
#include <string.h>

static BR_THREAD_LOCAL struct br_anims_state {
  br_theme_t* theme;
} br_anims_state;

void br_anims_construct(br_theme_t* theme) {
  br_anims_state.theme = theme;
}

int br_anim_newf(br_anims_t* anims, float current, float target) {
  br_anim_t anim = { .kind = br_anim_float, .current = current, .target = target };
  int handle = brfl_push(anims->all, anim);
  if (current != target) {
    brfl_push(anims->alive, handle);
  }
  return handle;
}

void br_anim_delete(br_anims_t* anims, int anim_handle) {
  brfl_foreach(i, anims->alive) {
    if (br_da_get(anims->alive, i) == anim_handle) {
      brfl_remove(anims->alive, i);
      break;
    }
  }
  brfl_remove(anims->all, anim_handle);
}

void br_anim_tick(br_anims_t* anims, float dt) {
  int to_kill = -1;
  float lerp_factor = br_anims_state.theme->ui.animation_speed * dt;
  lerp_factor = br_float_clamp(lerp_factor, 0.f, 1.f);
  brfl_foreach(i, anims->alive) {
    int anim_handle = br_da_get(anims->alive, i);
    br_anim_t* anim = br_da_getp(anims->all, anim_handle);
    switch (anim->kind) {
      case br_anim_float: {
        anim->current = br_float_lerp(anim->current, anim->target, lerp_factor);
        if (fabsf(anim->current - anim->target) < 1e-7) to_kill = i;
      } break;
      default: BR_UNREACHABLE("Anim kind: %d", anim->kind);
    }
  }
  // NOTE: We will remove only one animation per tick
  //       I mean it's not that bad..
  if (to_kill >= 0) {
    anims->all.arr[anims->alive.arr[to_kill]].is_alive = false;
    brfl_remove(anims->alive, to_kill);
  }
}

void br_anim_setf(br_anims_t* anims, int anim_handle, float target_value) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  BR_ASSERTF(anim->kind == br_anim_float, "Anim kind should be float, but it's: %d", anim->kind);
  if (anim->target == target_value) return;
  anim->target = target_value;
  LOGI("Anim %d = %f, alive=%d", anim_handle, target_value, anim->is_alive);
  BR_STACKTRACE();
  if (anim->is_alive) return;
  anim->is_alive = true;
  brfl_push(anims->alive, anim_handle);
}

float br_anim_getf(br_anims_t* anims, int anim_handle) {
  br_anim_t anim = br_da_get(anims->all, anim_handle);
  return anim.current;
}

float br_anim_getft(br_anims_t* anims, int anim_handle) {
  br_anim_t anim = br_da_get(anims->all, anim_handle);
  return anim.target;
}

bool br_anim_save(FILE* file, const br_anims_t* anims) {
  int error = 0;
  brfl_write(file, anims->all, error);
  if (error) return false;
  brfl_write(file, anims->alive, error);
  if (error) return false;
  return true;
}

bool br_anim_load(FILE* file, br_anims_t* anims) {
  int error = 0;
  brfl_read(file, anims->all, error);
  if (error) return false;
  brfl_read(file, anims->alive, error);
  if (error) return false;
  return true;
}
