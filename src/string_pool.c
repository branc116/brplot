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
  brfl_foreach_free(i, *sp) {
    brsp_node_t node = br_da_get(*sp, i);
    if (node.cap >= size) {
      if (prev == -1) sp->free_next      = sp->free_arr[i];
      else            sp->free_arr[prev] = sp->free_arr[i];
      sp->free_arr[i] = -1;
      break;
    }
    prev = i;
  }
  brsp_node_t new_node = {
    .start_index = (int)sp->pool.cap,
    .len = 0,
    .cap = size,
    .prev_in_memory = sp->len - 1,
    .next_in_memory = -1
  };
  int index = sp->len;
  if (sp->len > 0) br_da_getp(*sp, sp->last_in_memory)->next_in_memory = index;
  sp->last_in_memory = index;
  brfl_push_end(*sp, new_node);
  br_str_push_uninitialized(&sp->pool, (unsigned int)size);
  return index;
}

br_strv_t brsp_get(brsp_t sp, brsp_id_t t) {
  brsp_node_t ti = br_da_get(sp, t);
  return BR_STRV(sp.pool.str + ti.start_index, (uint32_t)ti.len);
}

void brsp_set(brsp_t* sp, brsp_id_t t, br_strv_t str) {
  brsp_node_t tn = br_da_get(*sp, t);
  if (tn.cap < (int)str.len) {
    brfl_foreach_free(i, *sp) {
      int j = i;
      int count = 0;
      brsp_node_t* node = br_da_getp(*sp, i);
      brsp_node_t* n = node;
      int taken_cap = 0;
      for (;taken_cap < (int)str.len &&
            j < sp->len &&
            j != -1 &&
            brfl_is_free(*sp, j);) {
        taken_cap += br_da_get(*sp, j).cap;
        ++count;
        j = br_da_get(*sp,j).next_in_memory;
      }
      if (taken_cap < (int)str.len) continue;
      for (int k = 1; k < count; ++k) {
        n = br_da_getp(*sp, n->next_in_memory);
        n->len = n->cap = n->start_index = 0;
        n->next_in_memory = n->prev_in_memory = -1;
      }
      brsp_node_t tmp = *node;
      *node = tn;
      tmp.cap = taken_cap;
      tmp.len = (int)str.len;
      if (-1 != tmp.next_in_memory) sp->arr[tmp.next_in_memory].prev_in_memory = t;
      if (-1 != tmp.prev_in_memory) sp->arr[tmp.prev_in_memory].next_in_memory = t;
      if (-1 != node->next_in_memory) sp->arr[node->next_in_memory].prev_in_memory = i;
      if (-1 != node->prev_in_memory) sp->arr[node->prev_in_memory].next_in_memory = i;
      if (i == sp->first_in_memory) sp->first_in_memory = t;
      if (t == sp->first_in_memory) sp->first_in_memory = i;
      if (i == sp->last_in_memory) sp->last_in_memory = t;
      if (t == sp->last_in_memory) sp->last_in_memory = i;
      br_da_set(*sp, t, tmp);
      goto out;
    }
    brsp_node_t new_node = tn;
    new_node.next_in_memory = t;
    new_node.len = 0;
    int new_id = brfl_push_end(*sp, new_node);
    if (-1 != tn.next_in_memory) sp->arr[tn.next_in_memory].prev_in_memory = new_id;
    if (-1 != tn.prev_in_memory) sp->arr[tn.prev_in_memory].next_in_memory = new_id;
    sp->free_arr[new_id] = sp->free_next == -1 ? -2 : sp->free_next;
    if (sp->first_in_memory == t) {
      sp->first_in_memory = new_id;
    }
    sp->free_next = new_id;

    
    int ex_size = (int)str.len * 2; 
    tn = (brsp_node_t) {
      .start_index = (int)sp->pool.len,
      .len = (int)str.len,
      .cap = (int)ex_size,
      .next_in_memory = -1,
      .prev_in_memory = sp->last_in_memory == t ? new_id : sp->last_in_memory
    };
    br_da_set(*sp, t, tn);
    sp->last_in_memory = t;
    br_str_push_uninitialized(&sp->pool, (unsigned int)ex_size);
  } else {
    br_da_getp(*sp, t)->len = (int)str.len;
  }
out:
  memcpy(&sp->pool.str[tn.start_index], str.str, str.len);
}

void brsp_remove(brsp_t* sp, brsp_id_t t) {
  brfl_remove(*sp, t);
}

void brsp_free(brsp_t* sp) {
  br_str_free(sp->pool);
  brfl_free(*sp);
  sp->first_in_memory = sp->last_in_memory = 0;
}

bool brsp_write(FILE* file, brsp_t sp) {
  int error = false;
  brfl_write(file, sp, error); if (error != 0) return false;
  if (sp.pool.len != fwrite(sp.pool.str, 1, sp.pool.len, file)) {
    LOGE("Failed to write pool from the string pool: %s", strerror(errno));
    return false;
  }
  return true;
}

bool brsp_read(FILE* file, brsp_t* sp) {
  int error = false;
  bool success = true;
  brfl_read(file, (*sp), error); if (error != 0) goto error;
  sp->pool.str = BR_MALLOC(sp->pool.cap);
  if (NULL == sp->pool.str) goto error;
  if (sp->pool.len != fread(sp->pool.str, 1, sp->pool.len, file)) goto error;
  goto done;

error:
  LOGE("Failed to read string pool: %s", strerror(errno));
  brsp_free(sp);
  success = false;

done:
  return success;
}

#if defined(BR_UNIT_TEST)
#include "external/tests.h"

void brsp_debug(brsp_t sp) {
  int len_sum = 0; 
  int cap_sum = 0; 
  LOGI("\n");
  LOGI("sp = { .len = %d, .cap = %d, .free_len = %d, .free_next = %d .pool = { .len = %u, .cap = %u }, .first_in_memory = %d, .last_in_memory = %d }", sp.len, sp.cap, sp.free_len, sp.free_next, sp.pool.len, sp.pool.cap, sp.first_in_memory, sp.last_in_memory);
  brfl_foreach(i, sp) {
    LOGI("sp[%d] = { .next_free = %d, .start_index = %d, .len = %d, .cap = %d, .next = %d, .prev = %d }", i, sp.free_arr[i], sp.arr[i].start_index, sp.arr[i].len, sp.arr[i].cap, sp.arr[i].next_in_memory, sp.arr[i].prev_in_memory);
    len_sum += sp.arr[i].len;
    cap_sum += sp.arr[i].cap;
  }
  LOGI("F");
  brfl_foreach_free(i, sp) {
    LOGI("sp[%d] = { .next_free = %d, .start_index = %d, .len = %d, .cap = %d, .next = %d, .prev = %d }", i, sp.free_arr[i], sp.arr[i].start_index, sp.arr[i].len, sp.arr[i].cap, sp.arr[i].next_in_memory, sp.arr[i].prev_in_memory);
  }
  LOGI("efficiency len/cap: %f", (float)len_sum / (float)sp.pool.cap);
  LOGI("efficiency len/len: %f", (float)len_sum / (float)sp.pool.len);
  LOGI("efficiency cap/len: %f", (float)cap_sum / (float)sp.pool.len);
  LOGI("efficiency cap/cap: %f", (float)cap_sum / (float)sp.pool.cap);
  LOGI("\n");
}


void brsp_test_memory_ptrs(brsp_t sp) {
  if (sp.len == 0) return;
  brsp_node_t n = br_da_get(sp, sp.first_in_memory);
  int cur_index = 0;
  int prev_ptr = -1;
  int cur_ptr = sp.first_in_memory;
  while (true) {
    BR_ASSERTF(n.start_index == cur_index, "%d != %d", n.start_index, cur_index);
    BR_ASSERTF(n.prev_in_memory == prev_ptr,"%d != %d", n.prev_in_memory, prev_ptr);
    prev_ptr = cur_ptr;
    cur_index += n.cap;
    cur_ptr = n.next_in_memory;
    if (n.next_in_memory == -1) break;
    n = br_da_get(sp, n.next_in_memory);
  }
  BR_ASSERTF(cur_index == (int)sp.pool.cap, "Links corupted: sum_cap (%d) != cap (%u) ", cur_index, sp.pool.cap);
  BR_ASSERTF(n.next_in_memory == -1, "%d", n.next_in_memory);
  BR_ASSERTF(prev_ptr == sp.last_in_memory, "%d != %d", cur_index, sp.last_in_memory);
}


TEST_CASE(string_pool_init) {
  brsp_t sp = { 0 };
  brsp_id_t t = brsp_new(&sp);
  br_strv_t str = brsp_get(sp, t);
  TEST_EQUAL(str.len, 0);
  brsp_set(&sp, t, BR_STRL("HEllo"));
  brsp_test_memory_ptrs(sp);
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
    brsp_test_memory_ptrs(sp);
  }
  brsp_test_memory_ptrs(sp);
  brsp_debug(sp);
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
    LOGI("i = %d", i);
    brsp_debug(sp);
    brsp_test_memory_ptrs(sp);

    br_str_push_char(&s2, 'd');
    br_str_push_char(&s2, 'd');
    br_str_push_char(&s2, 'd');
    brsp_set(&sp, t2, br_str_as_view(s2));
    LOGI("i2 = %d", i);
    brsp_debug(sp);
    brsp_test_memory_ptrs(sp);
  }
  TEST_STRNEQUAL(s.str, brsp_get(sp, t).str, s.len);
  brsp_debug(sp);
  brsp_remove(&sp, t);
  brsp_remove(&sp, t2);
  brsp_free(&sp);
  br_str_free(s);
  br_str_free(s2);
}

#endif
