#include "src/br_anim.h"
#include "src/br_theme.h"
#include "src/br_da.h"
#include "include/br_free_list_header.h"

#include <errno.h>
#include <string.h>

static BR_THREAD_LOCAL struct br_anims_state {
  float* animation_speed;
} br_anims_state;

void br_anims_construct(float* animation_speed) {
  br_anims_state.animation_speed = animation_speed;
}

void br_anims_tick(br_anims_t* anims, float dt) {
  int to_kill = -1;
  float lerp_factor = *br_anims_state.animation_speed * dt;
  lerp_factor = br_float_clamp(lerp_factor, 0.f, 1.f);
  brfl_foreach(i, anims->alive) {
    int anim_handle = br_da_get(anims->alive, i);
    br_anim_t* anim = br_da_getp(anims->all, anim_handle);
    switch (anim->kind) {
      case br_anim_float: {
        anim->f.current = br_float_lerp(anim->f.current, anim->f.target, lerp_factor);
        if (fabsf(anim->f.current - anim->f.target) < 1e-7f) to_kill = i;
      } break;
      case br_anim_vec3: {
        bool still_alive = false;
        if (anim->is_slerp) {
          if (anim->vec3.slerp_origin >= 0) {
            br_vec3_t origin = br_anim3(anims, anim->vec3.slerp_origin);
            br_vec3_t c = br_vec3_sub(anim->vec3.current, origin);
            br_vec3_t t = br_vec3_sub(anim->vec3.target, origin);
            c = br_vec3_slerp(c, t, 0.5f*lerp_factor);
            anim->vec3.current = br_vec3_add(c, origin);
            if (br_vec3_dist(c, t) >= 1e-8f) still_alive = true;
          } else {
            anim->vec3.current = br_vec3_slerp(anim->vec3.current, anim->vec3.target, 0.5f*lerp_factor);
            if (br_vec3_dist(anim->vec3.current, anim->vec3.target) >= 1e-8) still_alive = true;
          }
        } else {
          for (int j = 0; j < 3; ++j) {
            anim->vec3.current.arr[j] = br_float_lerp(anim->vec3.current.arr[j], anim->vec3.target.arr[j], lerp_factor);
            if (fabsf(anim->vec3.current.arr[j] - anim->vec3.target.arr[j]) > 1e-7) still_alive = true;
          }
        }
        if (false == still_alive) to_kill = i;
      } break;
      case br_anim_extent: {
        bool still_alive = false;
        for (int j = 0; j < 4; ++j) {
          anim->ex.current.arr[j] = br_float_lerp(anim->ex.current.arr[j], anim->ex.target.arr[j], lerp_factor);
          if (fabsf(anim->ex.current.arr[j] - anim->ex.target.arr[j]) > 1e-7) still_alive = true;
        }
        if (false == still_alive) to_kill = i;
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


int br_animf_new(br_anims_t* anims, float current, float target) {
  br_anim_t anim = { .kind = br_anim_float, .f = {.current = current, .target = target } };
  anim.is_alive = current != target;
  int handle = brfl_push(anims->all, anim);
  if (anim.is_alive) {
    brfl_push(anims->alive, handle);
  }
  return handle;
}

int br_animex_new(br_anims_t* anims, br_extent_t current, br_extent_t target) {
  br_anim_t anim = { .kind = br_anim_extent, .ex = {.current = current, .target = target } };
  anim.is_alive = false == br_extent_eq(current, target);
  int handle = brfl_push(anims->all, anim);
  if (anim.is_alive) {
    brfl_push(anims->alive, handle);
  }
  return handle;
}

int br_anim3_new(br_anims_t* anims, br_vec3_t current, br_vec3_t target) {
  br_anim_t anim = { .kind = br_anim_vec3, .vec3 = {.current = current, .target = target, .slerp_origin = -1 } };
  anim.is_alive = false == br_vec3_dist2(current, target) <= 1e-8;
  int handle = brfl_push(anims->all, anim);
  if (anim.is_alive) {
    brfl_push(anims->alive, handle);
  }
  return handle;
}

void br_anims_delete(br_anims_t* anims) {
  brfl_free(anims->all);
  brfl_free(anims->alive);
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

bool br_anim_alive(br_anims_t* anims, int anim_handle) {
  return (br_da_get(anims->all, anim_handle)).is_alive;
}

void br_anim_instant(br_anims_t* anims, int anim_handle) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  anim->is_instant = true;
  if (anim->is_alive) {
    anim->is_alive = false;
  }
  switch (anim->kind) {
    case br_anim_float:  anim->f.current = anim->f.target; break;
    case br_anim_extent: anim->ex.current = anim->ex.target; break;
    default:             BR_UNREACHABLE("unknown kind: %d", anim->kind);
  }
}

void br_anim_slerp(br_anims_t* anims, int anim_handle, bool should_slerp) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  anim->is_slerp = should_slerp;
}

void br_anim_slerp_origin(br_anims_t* anims, int anim_handle, int origin) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  anim->vec3.slerp_origin = origin;
}

#if BR_DEBUG
void br_anim_trace(br_anims_t* anims, int anim_handle) {
  br_anim_t* a = br_da_getp(anims->all, anim_handle);
  a->trace = true;
}
#endif

void br_animf_set(br_anims_t* anims, int anim_handle, float target_value) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  BR_ASSERTF(anim->kind == br_anim_float, "Anim kind should be float, but it's: %d", anim->kind);
#if BR_DEBUG
  if (anim->trace) {
    LOGI("ANIM %d: %f -> %f (current: %f)", anim_handle, anim->f.target, target_value, anim->f.current);
    BR_STACKTRACE();
  }
#endif
  if (anim->is_instant) {
    anim->f.target = target_value;
    anim->f.current = target_value;
  } else {
    if (anim->f.target == target_value) return;
    anim->f.target = target_value;
    if (anim->is_alive) return;
    anim->is_alive = true;
    brfl_push(anims->alive, anim_handle);
  }
}

float br_animf(br_anims_t* anims, int anim_handle) {
  br_anim_t anim = br_da_get(anims->all, anim_handle);
  BR_ASSERTF(anim.kind == br_anim_float, "Anim kind should be float, but it's: %d", anim.kind);
  return anim.f.current;
}

br_vec3_t br_anim3(br_anims_t* anims, int anim_handle) {
  br_anim_t anim = br_da_get(anims->all, anim_handle);
  BR_ASSERTF(anim.kind == br_anim_vec3, "Anim kind should be vec3, but it's: %d", anim.kind);
  return anim.vec3.current;
}

br_vec3_t br_anim3_get_target(br_anims_t* anims, int anim_handle) {
  br_anim_t anim = br_da_get(anims->all, anim_handle);
  BR_ASSERTF(anim.kind == br_anim_vec3, "Anim kind should be vec3, but it's: %d", anim.kind);
  return anim.vec3.target;
}

void br_anim3_set(br_anims_t* anims, int anim_handle, br_vec3_t target_value) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  BR_ASSERTF(anim->kind == br_anim_vec3, "Anim kind should be vec3, but it's: %d", anim->kind);
  if (anim->is_instant) {
    anim->vec3.target = target_value;
    anim->vec3.current = target_value;
  } else {
    if (br_vec3_dist2(anim->vec3.target, target_value) < 1e-8f) return;
    anim->vec3.target = target_value;
    if (anim->is_alive) return;
    anim->is_alive = true;
    brfl_push(anims->alive, anim_handle);
  }
}

void br_animex_set(br_anims_t* anims, int anim_handle, br_extent_t target_value) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  BR_ASSERTF(anim->kind == br_anim_extent, "Anim kind should be extent, but it's: %d", anim->kind);
  if (anim->is_instant) {
    anim->ex.target = target_value;
    anim->ex.current = target_value;
  } else {
    if (br_extent_eq(anim->ex.target, target_value)) return;
    anim->ex.target = target_value;
    if (anim->is_alive) return;
    anim->is_alive = true;
    brfl_push(anims->alive, anim_handle);
  }
}

br_extent_t br_animex(br_anims_t* anims, int anim_handle) {
  br_anim_t anim = br_da_get(anims->all, anim_handle);
  BR_ASSERTF(anim.kind == br_anim_extent, "Anim kind should be extent, but it's: %d", anim.kind);
  return anim.ex.current;
}

br_extent_t br_animex_get_target(br_anims_t* anims, int anim_handle) {
  br_anim_t anim = br_da_get(anims->all, anim_handle);
  BR_ASSERTF(anim.kind == br_anim_extent, "Anim kind should be extent, but it's: %d", anim.kind);
  return anim.ex.target;
}

br_extent_t br_anim_rebase(br_anims_t* anims, int anim_handle, br_vec2_t rebase_for) {
  br_anim_t* anim = br_da_getp(anims->all, anim_handle);
  BR_ASSERTF(anim->kind == br_anim_extent, "Anim kind should be extent, but it's: %d", anim->kind);
  anim->ex.target.pos = br_vec2_sub(anim->ex.target.pos, rebase_for);
  anim->ex.current.pos = br_vec2_sub(anim->ex.current.pos, rebase_for);
  return anim->ex.current;
}

bool br_anim_save(BR_FILE* file, const br_anims_t* anims) {
  int error = 0;
  brfl_write(file, anims->all, error);
  if (error) return false;
  brfl_write(file, anims->alive, error);
  if (error) return false;
  return true;
}

bool br_anim_load(BR_FILE* file, br_anims_t* anims) {
  int error = 0;
  brfl_read(file, anims->all, error);
  if (error) return false;
  brfl_read(file, anims->alive, error);
  if (error) return false;
  return true;
}
