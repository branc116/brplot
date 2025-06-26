#include "src/br_pp.h"
#include "src/br_string_pool.h"
#include "src/br_str.h"
#include "src/br_da.h"
#include "src/br_free_list.h"

#include <string.h>
#include <errno.h>

brsp_id_t brsp_new(brsp_t* sp) {
  return brsp_new1(sp, 128);
}

brsp_id_t brsp_new1(brsp_t* sp, int size) {
  int prev = -1;
  if (sp->free_len == 0) {
    brsp_free(sp);
    *sp = (brsp_t) { 0 };
  } else {
    brfl_foreach_free(i, *sp) {
      brsp_node_t node = br_da_get(*sp, i);
      if (node.cap >= size) {
        if (prev == -1) sp->free_next      = sp->free_arr[i];
        else            sp->free_arr[prev] = sp->free_arr[i];
        sp->free_arr[i] = -1;
        ++sp->free_len;
        return i;
      } else if (node.cap == -1) {
        if (prev == -1) sp->free_next      = sp->free_arr[i];
        else            sp->free_arr[prev] = sp->free_arr[i];
        sp->free_arr[i] = -1;
        ++sp->free_len;
        node.start_index = (int)sp->pool.len;
        node.cap = size;
        node.len = 0;
        br_str_push_uninitialized(&sp->pool, (uint32_t)size);
        return i;
      }
      prev = i;
    }
  }
  brsp_node_t new_node = {
    .start_index = (int)sp->pool.len,
    .len = 0,
    .cap = size,
  };
  int index = sp->len;
  index = brfl_push_end(*sp, new_node);
  br_str_push_uninitialized(&sp->pool, (unsigned int)size);
  return index;
}

br_strv_t brsp_get(brsp_t sp, brsp_id_t t) {
  brsp_node_t ti = br_da_get(sp, t);
  return BR_STRV(sp.pool.str + ti.start_index, (uint32_t)ti.len);
}

br_strv_t brsp_try_get(brsp_t sp, brsp_id_t t) {
  if (t <= 0 || t >= sp.len || brfl_is_free(sp, t)) return BR_STRV(NULL, 0);
  return brsp_get(sp, t);
}

bool brsp_is_in(brsp_t sp, brsp_id_t t) {
  if (t < 0) return false;
  if (sp.len <= t) return false;
  if (brfl_is_free(sp, t)) return false;
  return true;
}

bool brsp_resize(brsp_t* sp, brsp_id_t t, int new_size) {
  brsp_node_t tn = br_da_get(*sp, t);
  if (tn.cap >= new_size) return false;
  brfl_foreach_free(i, *sp) {
    brsp_node_t* node = br_da_getp(*sp, i);
    int taken_cap = node->cap;
    if (taken_cap < new_size) continue;
    brsp_node_t tmp = *node;
    *node = tn;
    tmp.cap = taken_cap;
    tmp.len = new_size;
    br_da_set(*sp, t, tmp);
    return true;
  }
  brsp_node_t new_node = tn;
  new_node.len = 0;
  int new_id = brfl_push_end(*sp, new_node);
  sp->free_arr[new_id] = sp->free_next == -1 ? -2 : sp->free_next;
  sp->free_next = new_id;

  int ex_size = new_size * 2;
  tn = (brsp_node_t) {
    .start_index = (int)sp->pool.len,
    .len = tn.len,
    .cap = (int)ex_size,
  };
  br_da_set(*sp, t, tn);
  br_str_push_uninitialized(&sp->pool, (unsigned int)(ex_size));
  return true;
}

void brsp_set(brsp_t* sp, brsp_id_t t, br_strv_t str) {
  brsp_resize(sp, t, (int)str.len);
  brsp_node_t* tn = br_da_getp(*sp, t);
  BR_ASSERTF(tn->start_index >= 0, "Start index is %d", tn->start_index);
  memmove(&sp->pool.str[tn->start_index], str.str, str.len);
  tn->len = (int)str.len;
}

void brsp_insert_char(brsp_t* sp, brsp_id_t t, int at, char c) {
  brsp_node_t* node = br_da_getp(*sp, t);
  int old_loc = node->start_index;
  if (brsp_resize(sp, t, node->len + 2)) {
    node = br_da_getp(*sp, t);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  for (int i = node->len; i >= at; --i) sp->pool.str[node->start_index + i + 1] = sp->pool.str[node->start_index + i];
  sp->pool.str[node->start_index + at] = c;
  ++node->len;
}

void brsp_insert_char_at_end(brsp_t* sp, brsp_id_t id, char c) {
  brsp_node_t* node = br_da_getp(*sp, id);
  int old_loc = node->start_index;
  if (brsp_resize(sp, id, node->len + 2)) {
    node = br_da_getp(*sp, id);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  sp->pool.str[node->start_index + node->len++] = c;
}

void brsp_insert_strv_at_end(brsp_t* sp, brsp_id_t id, br_strv_t sv) {
  brsp_node_t* node = br_da_getp(*sp, id);
  int old_loc = node->start_index;
  if (brsp_resize(sp, id, node->len + (int)sv.len)) {
    node = br_da_getp(*sp, id);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  for (uint32_t i = 0; i < sv.len; ++i) sp->pool.str[node->start_index + node->len++] = sv.str[i];
}

void brsp_zero(brsp_t* sp, brsp_id_t id) {
  brsp_node_t* node = br_da_getp(*sp, id);
  int old_loc = node->start_index;
  if (brsp_resize(sp, id, node->len + 2)) {
    node = br_da_getp(*sp, id);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  sp->pool.str[node->start_index + node->len] = '\0';
}

void brsp_clear(brsp_t* sp, brsp_id_t id) {
  brsp_node_t* node = br_da_getp(*sp, id);
  node->len = 0;
}

char brsp_remove_char_end(brsp_t* sp, brsp_id_t id) {
  brsp_node_t* node = br_da_getp(*sp, id);
  if (node->len > 0) --node->len;
  return sp->pool.str[node->start_index + node->len];
}

int brsp_remove_utf8_after(brsp_t* sp, brsp_id_t id, int position) {
  brsp_node_t* node = br_da_getp(*sp, id);
  if (node->len == 0) return 0;
  if (position >= node->len) return 0;
  int to_remove = 0;
  int start = node->start_index + position;
  while (position + ++to_remove < node->len && ((sp->pool.str[to_remove + start] & 0b11000000) == 0b10000000));
  BR_ASSERTF(position + to_remove <= node->len, "pos: %d, to_remove: %d, node->len: %d", position, to_remove, node->len);
  int to_move = node->len - position - to_remove;
  node->len -= to_remove;
  if (to_move <= 0) return to_remove;
  memmove(&sp->pool.str[start], &sp->pool.str[start + to_remove], (size_t)to_move);
  return to_remove;
}

void brsp_remove(brsp_t* sp, brsp_id_t t) {
  if (t <= 0) return;
  brfl_remove(*sp, t);
}

brsp_id_t brsp_copy(brsp_t* sp, brsp_id_t id) {
  brsp_node_t node = br_da_get(*sp, id);
  brsp_id_t ret_id = brsp_new1(sp, node.len + /* Give a bit of slack, just in case... */ 16);
  brsp_node_t* ret_node = br_da_getp(*sp, ret_id);
  memcpy(&sp->pool.str[ret_node->start_index], &sp->pool.str[node.start_index], (size_t)node.len);
  ret_node->len = node.len;
  return ret_id;
}

bool brsp_compress(brsp_t* sp, float factor, int slack) {
  int full_len = 1;
  for (int i = 0; i < sp->len; ++i) {
    brsp_node_t* node = br_da_getp(*sp, i);
    bool is_free = brfl_is_free(*sp, i) || node->len < 0 || node->cap < 0;
    int new_cap = is_free ? 0 : (int)((float)node->len * factor) + slack;
    full_len += new_cap;
  }
  char* new_pool = BR_MALLOC((size_t)full_len);
  if (NULL == new_pool) {
    LOGE("Failed to compress string pool, failed to alloc new pool: %s", strerror(errno));
    return false;
  }
  int cur_index = 0;
  for (int i = 0; i < sp->len; ++i) {
    brsp_node_t* node = br_da_getp(*sp, i);
    bool is_free = brfl_is_free(*sp, i) || node->len < 0 || node->cap < 0;
    if (is_free) {
      node->cap = -1;
      node->start_index = -1;
    } else {
      node->cap = (int)((float)node->len * factor) + slack;
      memcpy(new_pool + cur_index, sp->pool.str + node->start_index, (size_t)node->len);
      node->start_index = cur_index;
      cur_index += node->cap;
    }
  }
  br_str_free(sp->pool);
  sp->pool.str = new_pool;
  sp->pool.len = (uint32_t)full_len;
  sp->pool.cap = (uint32_t)full_len;
  return true;
}

void brsp_free(brsp_t* sp) {
  br_str_free(sp->pool);
  sp->pool.len = sp->pool.cap = 0;
  sp->pool.str = NULL;
  brfl_free(*sp);
}

bool brsp_write(FILE* file, brsp_t* sp) {
  brsp_compress(sp, 1.f, 0);

  int error = false;
  brfl_write(file, *sp, error); if (error != 0) return false;
  if (sp->pool.len != fwrite(sp->pool.str, 1, sp->pool.len, file)) {
    LOGE("Failed to write pool from the string pool: %s", strerror(errno));
    return false;
  }
  return true;
}

bool brsp_read(FILE* file, brsp_t* sp) {
  int error = 0;
  bool success = true;
  memset(sp, 0, sizeof(*sp));
  brfl_read(file, (*sp), error); if (error != 0) BR_ERROR("Failed to read free list");
  sp->pool.str = NULL;
  if (sp->len < 0) BR_ERROR("string pool length %d < 0", sp->len);
  for (int i = 0; i < sp->len; ++i) {
    brsp_node_t n = br_da_get(*sp, i);
    if (n.start_index >= 0 && n.start_index + n.cap > (int)sp->pool.len) BR_ERROR("i=%d, start_index = %d < 0, start_index + cap = %d > pool.len = %d", i, n.start_index, n.start_index + n.cap, sp->pool.len);
  }
  sp->pool.str = BR_MALLOC(sp->pool.cap);
  if (NULL == sp->pool.str) BR_ERROR("Failed to allocated the pool");
  if (sp->pool.len != fread(sp->pool.str, 1, sp->pool.len, file)) BR_ERROR("Failed to read the pool");
  goto done;

error:
  brsp_free(sp);
  success = false;

done:
  return success;
}

#if defined(BR_UNIT_TEST)
#include "external/tests.h"

static void brsp_debug(brsp_t sp) {
  int len_sum = 0;
  int cap_sum = 0;
  LOGI("\n");
  LOGI("sp = { .len = %d, .cap = %d, .free_len = %d, .free_next = %d .pool = { .len = %u, .cap = %u } }", sp.len, sp.cap, sp.free_len, sp.free_next, sp.pool.len, sp.pool.cap);
  brfl_foreach(i, sp) {
    LOGI("sp[%d] = { .next_free = %d, .start_index = %d, .len = %d, .cap = %d }", i, sp.free_arr[i], sp.arr[i].start_index, sp.arr[i].len, sp.arr[i].cap);
    len_sum += sp.arr[i].len;
    cap_sum += sp.arr[i].cap;
  }
  LOGI("F");
  brfl_foreach_free(i, sp) {
    LOGI("sp[%d] = { .next_free = %d, .start_index = %d, .len = %d, .cap = %d }", i, sp.free_arr[i], sp.arr[i].start_index, sp.arr[i].len, sp.arr[i].cap);
  }
  LOGI("efficiency len/cap: %f", (float)len_sum / (float)sp.pool.cap);
  LOGI("efficiency len/len: %f", (float)len_sum / (float)sp.pool.len);
  LOGI("efficiency cap/len: %f", (float)cap_sum / (float)sp.pool.len);
  LOGI("efficiency cap/cap: %f", (float)cap_sum / (float)sp.pool.cap);
  LOGI("\n");
}


TEST_CASE(string_pool_init) {
  (void)brsp_debug;
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  br_strv_t str = brsp_get(sp, t);
  TEST_EQUAL(str.len, 0);
  brsp_set(&sp, t, BR_STRL("HEllo"));
  brsp_remove(&sp, t);
  brsp_free(&sp);
}

TEST_CASE(string_pool_resize) {
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  br_str_t s = { 0 };
  for (int i = 0; i < 129; ++i) {
    br_str_push_char(&s, 'c');
    brsp_set(&sp, t, br_str_as_view(s));
  }
  brsp_remove(&sp, t);
  brsp_free(&sp);
  br_str_free(s);
}

TEST_CASE(string_pool_resize2) {
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  brsp_id_t t2 = brsp_new(&sp);
  br_str_t s = { 0 };
  br_str_t s2 = { 0 };
  for (int i = 0; i < 256; ++i) {
    br_str_push_char(&s, 'c');
    brsp_set(&sp, t, br_str_as_view(s));

    br_str_push_char(&s2, 'd');
    br_str_push_char(&s2, 'd');
    br_str_push_char(&s2, 'd');
    brsp_set(&sp, t2, br_str_as_view(s2));
  }
  TEST_STRNEQUAL(s.str, brsp_get(sp, t).str, s.len);
  brsp_remove(&sp, t);
  brsp_remove(&sp, t2);
  brsp_free(&sp);
  br_str_free(s);
  br_str_free(s2);
}

#endif
