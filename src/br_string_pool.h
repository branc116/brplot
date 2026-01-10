#if !defined(BR_INCLUDE_BR_STRING_POOL_H)
#  define BR_INCLUDE_BR_STRING_POOL_H
#  include "include/br_string_pool_header.h"
#endif // !defined(BR_INCLUDE_BR_STRING_POOL_H)


#if defined(BRSP_IMPLEMENTATION)
static bool brsp_resize(brsp_t* sp, brsp_id_t t, int new_size);

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
      brsp_node_t* node = br_da_getp(*sp, i);
      int next_free = sp->free_arr[i];
      if (node->cap >= size) {
        if (prev == -1) sp->free_next      = next_free < 0 ? -1 : next_free;
        else            sp->free_arr[prev] = next_free;
        sp->free_arr[i] = -1;
        ++sp->free_len;
        node->len = 0;
        return i + 1;
      } else if (node->cap == -1) {
        if (prev == -1) sp->free_next      = next_free < 0 ? -1 : next_free;
        else            sp->free_arr[prev] = next_free;
        sp->free_arr[i] = -1;
        ++sp->free_len;
        node->start_index = (int)sp->pool.len;
        node->cap = size;
        node->len = 0;
        br_str_push_uninitialized(&sp->pool, (uint32_t)size);
        return i + 1;
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
  return index + 1;
}

brsp_id_t brsp_push(brsp_t* sp, br_strv_t sv) {
  brsp_id_t new_id = brsp_new1(sp, (int)sv.len + 8);
  brsp_insert_strv_at_end(sp, new_id, sv);
  return new_id;
}

br_strv_t brsp_get(brsp_t sp, brsp_id_t t) {
  brsp_node_t ti = br_da_get(sp, t - 1);
  return BR_STRV(sp.pool.str + ti.start_index, (uint32_t)ti.len);
}

br_strv_t brsp_try_get(brsp_t sp, brsp_id_t t) {
  if (t <= 0 || t > sp.len || brfl_is_free(sp, t - 1)) return BR_STRV(NULL, 0);
  return brsp_get(sp, t);
}

bool brsp_is_in(brsp_t sp, brsp_id_t t) {
  if (t <= 0) return false;
  if (sp.len < t) return false;
  if (brfl_is_free(sp, t - 1)) return false;
  return true;
}

void brsp_set(brsp_t* sp, brsp_id_t t, br_strv_t str) {
  brsp_resize(sp, t, (int)str.len);
  brsp_node_t* tn = br_da_getp(*sp, t - 1);
  BR_ASSERTF(tn->start_index >= 0, "Start index is %d", tn->start_index);
  memmove(&sp->pool.str[tn->start_index], str.str, str.len);
  tn->len = (int)str.len;
}

void brsp_insert_char(brsp_t* sp, brsp_id_t t, int at, unsigned char c) {
  brsp_node_t* node = br_da_getp(*sp, t - 1);
  int old_loc = node->start_index;
  if (brsp_resize(sp, t, node->len + 2)) {
    node = br_da_getp(*sp, t - 1);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  for (int i = node->len; i >= at; --i) sp->pool.str[node->start_index + i + 1] = sp->pool.str[node->start_index + i];
  sp->pool.str[node->start_index + at] = (char)c;
  ++node->len;
}

int brsp_insert_unicode(brsp_t* sp, brsp_id_t t, int at, uint32_t u) {
  //  U+0000    U+007F    0yyyzzzz
  //  U+0080    U+07FF    110xxxyy  10yyzzzz => xxxyy yyzzzz
  //  U+0800    U+FFFF    1110wwww  10xxxxyy  10yyzzzz => wwww xxxxyy yyzzzz
  //  U+010000  U+10FFFF  11110uvv  10vvwwww  10xxxxyy  10yyzzzz
  //  U+uvwxyz
  if (u < 0x80) {
    brsp_insert_char(sp, t, at, (uint8_t)u);
    return 1;
  } else if (u < 0x0800) {
    brsp_insert_char(sp, t, at + 0, (uint8_t)(0b11000000 | ((u >>  6) & 0b00011111)));
    brsp_insert_char(sp, t, at + 1, (uint8_t)(0b10000000 | ((u >>  0) & 0b00111111)));
    return 2;
  } else if (u < 0x800) {
    brsp_insert_char(sp, t, at + 0, (uint8_t)(0b11100000 | ((u >> 12) & 0b00001111)));
    brsp_insert_char(sp, t, at + 1, (uint8_t)(0b10000000 | ((u >>  6) & 0b00111111)));
    brsp_insert_char(sp, t, at + 2, (uint8_t)(0b10000000 | ((u >>  0) & 0b00111111)));
    return 3;
  } else if (u < 0x10000) {
    brsp_insert_char(sp, t, at + 0, (uint8_t)(0b11110000 | ((u >> 18) & 0b00000111)));
    brsp_insert_char(sp, t, at + 1, (uint8_t)(0b10000000 | ((u >> 12) & 0b00111111)));
    brsp_insert_char(sp, t, at + 2, (uint8_t)(0b10000000 | ((u >>  6) & 0b00111111)));
    brsp_insert_char(sp, t, at + 3, (uint8_t)(0b10000000 | ((u >>  0) & 0b00111111)));
    return 4;
  }
  BR_UNREACHABLE("Don't know how to encode character %u", u);
  BR_RETURN_IF_TINY_C(0);
}

void brsp_insert_char_at_end(brsp_t* sp, brsp_id_t id, char c) {
  brsp_node_t* node = br_da_getp(*sp, id - 1);
  int old_loc = node->start_index;
  if (brsp_resize(sp, id, node->len + 2)) {
    node = br_da_getp(*sp, id - 1);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  sp->pool.str[node->start_index + node->len++] = c;
}

void brsp_insert_strv_at_end(brsp_t* sp, brsp_id_t id, br_strv_t sv) {
  brsp_node_t* node = br_da_getp(*sp, id - 1);
  //LOGI("node start_index: %d len: %d", node->start_index, node->len);
  int old_loc = node->start_index;
  if (brsp_resize(sp, id, node->len + (int)sv.len)) {
    node = br_da_getp(*sp, id - 1);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  for (uint32_t i = 0; i < sv.len; ++i) sp->pool.str[node->start_index + node->len++] = sv.str[i];
}

void brsp_zero(brsp_t* sp, brsp_id_t id) {
  brsp_node_t* node = br_da_getp(*sp, id - 1);
  int old_loc = node->start_index;
  if (brsp_resize(sp, id, node->len + 2)) {
    node = br_da_getp(*sp, id - 1);
    memmove(sp->pool.str + node->start_index, sp->pool.str + old_loc, (size_t)node->len);
  }
  sp->pool.str[node->start_index + node->len] = '\0';
}

void brsp_clear(brsp_t* sp, brsp_id_t id) {
  brsp_node_t* node = br_da_getp(*sp, id - 1);
  node->len = 0;
}

char brsp_remove_char_end(brsp_t* sp, brsp_id_t id) {
  brsp_node_t* node = br_da_getp(*sp, id - 1);
  if (node->len > 0) --node->len;
  return sp->pool.str[node->start_index + node->len];
}

int brsp_remove_utf8_after(brsp_t* sp, brsp_id_t id, int position) {
  brsp_node_t* node = br_da_getp(*sp, id - 1);
  if (node->len == 0) return 0;
  if (position >= node->len) return 0;
  int to_remove = 0;
  int start = node->start_index + position;
  while (position + ++to_remove < node->len && ((sp->pool.str[to_remove + start] & 0b11000000) == 0b10000000))
    ;
  BR_ASSERTF(position + to_remove <= node->len, "pos: %d, to_remove: %d, node->len: %d", position, to_remove, node->len);
  int to_move = node->len - position - to_remove;
  node->len -= to_remove;
  if (to_move <= 0) return to_remove;
  memmove(&sp->pool.str[start], &sp->pool.str[start + to_remove], (size_t)to_move);
  return to_remove;
}

void brsp_remove(brsp_t* sp, brsp_id_t t) {
  if (t <= 0) return;
  brfl_remove(*sp, t - 1);
}

brsp_id_t brsp_copy(brsp_t* sp, brsp_id_t id) {
  brsp_node_t node = br_da_get(*sp, id - 1);
  brsp_id_t ret_id = brsp_new1(sp, node.len + /* Give a bit of slack, just in case... */ 16);
  brsp_node_t* ret_node = br_da_getp(*sp, ret_id - 1);
  memcpy(&sp->pool.str[ret_node->start_index], &sp->pool.str[node.start_index], (size_t)node.len);
  ret_node->len = node.len;
  return ret_id;
}

bool brsp_compress(brsp_t* sp, float factor, int slack) {
  if (sp->free_len == 0) {
    sp->len = 0;
    sp->free_next = -1;
  }
  if (sp->len == 0) sp->pool.len = 0;
  if (sp->pool.len == 0) return true;

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
      node->len = -1;
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

bool brsp_write(BR_FILE* file, brsp_t* sp) {
  brsp_compress(sp, 1.f, 0);

  int error = false;
  brfl_write(file, *sp, error); if (error != 0) return false;
  if (sp->pool.len == 0) return true;
  if (sp->pool.len != BR_FWRITE(sp->pool.str, 1, sp->pool.len, file)) {
    LOGE("Failed to write pool from the string pool: %s", strerror(errno));
    return false;
  }
  return true;
}

bool brsp_read(BR_FILE* file, brsp_t* sp) {
  int error = 0;
  bool success = true;
  int i = 0;
  brsp_node_t n = { 0 };
  int sum = 0;
  bool is_free = false;

  memset(sp, 0, sizeof(*sp));
  brfl_read(file, (*sp), error); if (error != 0) return false;
  sp->pool.str = NULL;
  if (sp->pool.len <= 1) {
    brsp_free(sp);
    memset(sp, 0, sizeof(*sp));
    return true;
  }
  if (sp->len < 0)                                                       BR_ERROR("string pool length %d < 0", sp->len);
  if (sp->pool.len > sp->pool.cap)                                       BR_ERROR("string pool length %d is bigger than cap %d.", sp->pool.len, sp->pool.cap); 
  if (sp->pool.cap > 0xFFFFF)                                            BR_ERROR("string pool len must be less than 0xFFFFF, it's %d", sp->pool.cap);
  for (i = 0; i < sp->len; ++i) {
    n = br_da_get(*sp, i);
    if (n.start_index < -1)                                              BR_ERROR("Start index is: %d", n.start_index);
    if (n.len < -1)                                                      BR_ERROR("Len is: %d", n.len);
    if ((n.len == -1) ^ (n.cap == -1))                                   BR_ERROR("Len is: %d, cap is: %d", n.len, n.cap);
    if ((n.len == -1) ^ (n.start_index == -1))                           BR_ERROR("Len is: %d, start_index is: %d", n.len, n.start_index);
    if (n.cap < -1)                                                      BR_ERROR("Cap is: %d", n.cap);
    if (n.len > (int)sp->pool.len)                                       BR_ERROR("Len is: %d", n.len);
    if (n.start_index > (int)sp->pool.len)                               BR_ERROR("Start index is: %d, pool len: %u", n.start_index, sp->pool.len);
    if (n.cap > 0 && n.cap > (int)sp->pool.len)                          BR_ERROR("Start cap is: %d, pool len: %u", n.cap, sp->pool.len);
    if (n.cap > 0 && n.cap < (int)n.len)                                 BR_ERROR("Node cap: %d, Node len: %u", n.cap, n.len);
    if (n.start_index >= 0 && n.start_index + n.cap > (int)sp->pool.len) BR_ERROR("i=%d, start_index = %d < 0, start_index + cap = %d > pool.len = %d", i, n.start_index, n.start_index + n.cap, sp->pool.len);
    if (n.cap > 0) sum += n.cap;
    is_free = brfl_is_free(*sp, i);
    if (false == is_free) {
      if (n.cap < 0)                                                     BR_ERROR("Cap is: %d", n.cap);
      if (n.len < 0)                                                     BR_ERROR("Len is: %d", n.len);
      if (n.start_index < 0)                                             BR_ERROR("Start index is: %d", n.start_index);
    }
  }
  if (sum + 1 != (int)sp->pool.len)                                      BR_ERROR("Cap len don't match. sum=%d, len=%u, cap=%u", sum, sp->pool.len, sp->pool.cap);
  sp->pool.str = BR_MALLOC(sp->pool.cap);
  if (NULL == sp->pool.str)                                              BR_ERROR("Failed to allocated the pool");
  if (sp->pool.len != BR_FREAD(sp->pool.str, 1, sp->pool.len, file))     BR_ERROR("Failed to read the pool");
  goto done;

error:
  brsp_free(sp);
  memset(sp, 0, sizeof(*sp));
  success = false;

done:
  //LOGI("sp->poll.cap: %d, sp->pool.len: %d, sp->len: %d, sp->cap: %d, sp->free_len: %d", sp->pool.cap, sp->pool.len, sp->len, sp->cap, sp->free_len);
  return success;
}

// STATICS

static bool brsp_resize(brsp_t* sp, brsp_id_t t, int new_size) {
  brsp_node_t tn = br_da_get(*sp, t - 1);
  if (tn.cap >= new_size) return false;
  brfl_foreach_free(i, *sp) {
    brsp_node_t* node = br_da_getp(*sp, i);
    int taken_cap = node->cap;
    if (taken_cap < new_size) continue;
    brsp_node_t tmp = *node;
    *node = tn;
    tmp.cap = taken_cap;
    tmp.len = new_size;
    br_da_set(*sp, t - 1, tmp);
    return true;
  }
  brsp_node_t new_node = tn;
  new_node.len = 0;
  int new_id = brfl_push_end(*sp, new_node);
  sp->free_arr[new_id] = sp->free_next == -1 ? -2 : sp->free_next;
  sp->free_next = new_id;
  --sp->free_len;

  int ex_size = new_size * 2;
  tn = (brsp_node_t) {
    .start_index = (int)sp->pool.len,
    .len = tn.len,
    .cap = (int)ex_size,
  };
  br_da_set(*sp, t - 1, tn);
  br_str_push_uninitialized(&sp->pool, (unsigned int)(ex_size));
  return true;
}

#endif
