#include "src/br_pp.h"
#include "src/br_str.h"
#include "src/br_da.h"
#include "external/tests.h"

#include <string.h>

typedef struct {
  int id;
} text_t;

typedef enum {
  text_node_taken,
  text_node_free
} text_node_state_t;

typedef struct {
  text_node_state_t state;
  union {
    struct {
      int start_index;
      int len;
      int cap;
    };
    int node_free_next;
  };
} text_node_t;

typedef struct {
  struct {
    text_node_t* arr;
    int len, cap;
  } string_nodes;

  br_str_t pool;
  int node_free_root;
  int node_free_last;
  int free_index;
} string_pool_t;

string_pool_t string_pool(void) {
  return (string_pool_t) {
    .node_free_root = -1,
    .node_free_last = -1,
  };
}

text_t text_new1(string_pool_t* sp, int size) {
  text_node_t new_node = {
    .state = text_node_taken,
    .start_index = sp->free_index,
    .cap = size
  };
  br_da_push_t(int, sp->string_nodes, new_node);
  br_str_push_uninitialized(&sp->pool, (unsigned int)size);
  sp->free_index += size;

  return (text_t) { .id = sp->string_nodes.len - 1 };
}

text_t text_new(string_pool_t* sp) {
  return text_new1(sp, 128);
}

void string_pool_deinit(string_pool_t* sp) {
  br_str_free(sp->pool);
  br_da_free(sp->string_nodes);
}

br_strv_t text_get(string_pool_t* sp, text_t* t) {
  text_node_t ti = br_da_get(sp->string_nodes, t->id);
  return BR_STRV(sp->pool.str + ti.start_index, (uint32_t)ti.len);
}

void text_free(string_pool_t* sp, text_t t) {
  if (sp->node_free_root == -1) {
    br_da_set(sp->string_nodes, t.id, ((text_node_t) {
      .state = text_node_free,
      .node_free_next = -1,
    }));
    sp->node_free_root = t.id;
    sp->node_free_last = t.id;
  } else {
    br_da_set(sp->string_nodes, t.id, ((text_node_t) {
      .state = text_node_free,
      .node_free_next = sp->node_free_last,
    }));
    sp->node_free_last = t.id;
  }
}

void text_set(string_pool_t* sp, text_t* t, br_strv_t str) {
  text_node_t tn = br_da_get(sp->string_nodes, t->id);
  BR_ASSERT(tn.state == text_node_taken);
  if (tn.cap < (int)str.len) {
    text_free(sp, *t);
    *t = text_new1(sp, (int)str.len);
    tn = br_da_get(sp->string_nodes, t->id);
  }
  tn.len = (int)str.len;
  memcpy(&sp->pool.str[tn.start_index], str.str, str.len);
}

#ifndef _MSC_VER
#include "external/tests.h"
TEST_CASE(string_pool_init) {
  string_pool_t sp = string_pool();
  text_t t = text_new(&sp);
  br_strv_t str = text_get(&sp, &t);
  TEST_EQUAL(str.len, 0);
  text_set(&sp, &t, BR_STRL("HEllo"));
  text_free(&sp, t);
  string_pool_deinit(&sp);
}

TEST_CASE(string_pool_resize) {
  string_pool_t sp = string_pool();
  text_t t = text_new(&sp);
  br_str_t s = { 0 };
  for (int i = 0; i < 256; ++i) {
    br_str_push_char(&s, 'c');
    text_set(&sp, &t, br_str_as_view(s));
  }
  text_free(&sp, t);
  string_pool_deinit(&sp);
  br_str_free(s);
}
#endif
