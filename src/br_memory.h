/*
 *  br_memory - Author: Branimir Ričko
 *    The MIT License (MIT)
 *    Copyright © Branimir Ričko [branc116]
 *
 *    Permission is hereby granted, free of charge, to any person  obtaining a copy of
 *    this software and associated  documentation files (the   “Software”), to deal in
 *    the Software without  restriction,  including without  limitation  the rights to
 *    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *    the Software,  and to permit persons to whom the Software is furnished to do so,
 *    subject to the following conditions:
 *
 *    The above  copyright notice and this  permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED   “AS IS”,   WITHOUT WARRANTY OF ANY KIND,   EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   IN NO EVENT SHALL THE AUTHORS OR
 *    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER
 *    IN  AN  ACTION  OF CONTRACT,   TORT OR OTHERWISE,   ARISING FROM,   OUT OF OR IN
 *    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * br_memory - track memory allocations and such
 *
 * */

#if 0
/* Example of simple API: */
#define BR_MEMORY_TRACER_IMPLEMENTATION
#include <br_memory.h>

int main(void) {
  void* mem = BR_MALLOC(128);
  BR_FREE(mem);
   /* Print information about 1th allocation event */
  br_memory_stack_print(0);
  /* This should trigger double free fatal error message */
  BR_FREE(mem);
  return 0;
}
#endif

#if !defined(BR_INCLUDE_BR_MEMORY_H)
#define BR_INCLUDE_BR_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if !defined(BR_MALLOC)
#  define BR_MALLOC(SIZE) br_memory_malloc(false, SIZE, __FILE__, __LINE__)
#endif

#if !defined(BR_CALLOC)
#  define BR_CALLOC(N, SIZE) br_memory_malloc(true, (N * SIZE), __FILE__, __LINE__)
#endif

#if !defined(BR_REALLOC)
#  define BR_REALLOC(PTR, NEW_SIZE) br_memory_realloc(PTR, NEW_SIZE, __FILE__, __LINE__)
#endif

#if !defined(BR_FREE)
#  define BR_FREE(PTR) br_memory_free(PTR, __FILE__, __LINE__)
#endif

typedef enum br_memory_event_kind_t {
  br_memory_event_alloc,
  br_memory_event_realloc,
  br_memory_event_free
} br_memory_event_kind_t;

typedef struct br_memory_node_t {
  const char* at_file_name;
  size_t size;
  void *addr;
  int at_line_num;
  int frame_num;

  int next_nid;
  int prev_nid;
  int realloc_count;
  br_memory_event_kind_t kind;
} br_memory_node_t;

typedef struct br_memory_frame_t {
  int start_nid;
  int len;
  int frame_num;
} br_memory_frame_t;

typedef struct br_memory_frames_t {
  br_memory_frame_t* arr;
  size_t len, cap;
} br_memory_frames_t;

typedef struct br_memory_t {
  br_memory_node_t* arr;
  size_t len, cap;

  int current_frame;
  int cur_frame_nodes_len;

  size_t total_alloced;
  size_t cur_alloced;
  size_t max_alloced;
  size_t cur_frame_alloced;
  size_t cur_frame_freed;

  br_memory_frames_t frames;
} br_memory_t;

void* br_memory_malloc (bool zero,      size_t size, const char* file_name, int line_num);
void* br_memory_realloc(void* old,  size_t new_size, const char* file_name, int line_num);
void  br_memory_free   (void* ptr,                   const char* file_name, int line_num);

br_memory_t br_memory_get(void);
/* Call to indicate that the new frame has happend. Think games and Frames per seconds. */
void br_memory_frame(void);
void br_memory_stack_print(int top_nid);
/* Call this when you exit the program to deallocate the memory and to check for memory leaks */
void br_memory_finish(void);
#endif

/*=======================================*/
/*            IMPLEMENTATION             */
/*=======================================*/
#if defined(BR_MEMORY_TRACER_IMPLEMENTATION)

/* TODO: ISO C90 does not support ‘_Thread_local’ */
#if !defined(BR_THREAD_LOCAL)
#  if (__TINYC__)
#    define BR_THREAD_LOCAL
#  else
#    define BR_THREAD_LOCAL
#  endif
#endif

#if !defined(BR_ASSERT)
#  include <assert.h>
#  define BR_ASSERT(expr) assert(expr)
#endif

#if !defined(BR_LOGW)
#  include <stdio.h>
/* TODO: anonymous variadic macros were introduced in C99  */
#  define BR_LOGW(msg, ...) printf("[WARRNING] " msg "\n", ##__VA_ARGS__)
#endif

#if !defined(BR_LOGE)
#  include <stdio.h>
/* TODO: anonymous variadic macros were introduced in C99  */
#  define BR_LOGE(msg, ...) printf("[ERROR] " msg "\n", ##__VA_ARGS__)
#endif

#if !defined(BR_LOGF)
#  include <stdio.h>
/* TODO: anonymous variadic macros were introduced in C99  */
#  define BR_LOGF(msg, ...) do { \
     printf("[FATAL] " msg "\n", ##__VA_ARGS__); \
     abort(); \
   } while(0)
#endif

/* malloc tracker must not be tracked by itself because it would create recursive calls. */
#define br_memory_da_push_t(SIZE_T, ARR, VALUE) do {                                                                \
  if ((ARR).cap == 0) {                                                                                             \
    BR_ASSERT((ARR).arr == NULL && "Cap is set to null, but arr is not null");                                      \
    (ARR).arr = malloc(16 * sizeof(*(ARR).arr));                                                                    \
    if ((ARR).arr != NULL) {                                                                                        \
      (ARR).cap = 16;                                                                                               \
      (ARR).arr[(ARR).len++] = (VALUE);                                                                             \
    }                                                                                                               \
  }                                                                                                                 \
  else if ((ARR).len < (ARR).cap) (ARR).arr[(ARR).len++] = (VALUE);                                                 \
  else {                                                                                                            \
    BR_ASSERT((ARR).arr != NULL);                                                                                   \
    (ARR).arr = realloc((ARR).arr, sizeof((ARR).arr[0]) * (ARR).cap * 2);                                           \
    (ARR).cap *= 2;                                                                                                 \
    (ARR).arr[(ARR).len++] = (VALUE);                                                                               \
  }                                                                                                                 \
} while(0)                                                                                                          \

#define br_memory_da_push(ARR, VALUE) br_memory_da_push_t(size_t, ARR, VALUE)
BR_THREAD_LOCAL br_memory_t br_memory;

void* br_memory_malloc(bool zero, size_t size, const char* file_name, int line_num) {
  br_memory_node_t node;
  void* ret_memory;
  size_t* memory;

  memory = malloc(size + sizeof(size_t));
  if (zero) memset(memory, 0, size);
  *memory = br_memory.len;

  node.at_file_name = file_name;
  node.size = size;
  node.addr = memory;
  node.at_line_num = line_num;
  node.frame_num = br_memory.current_frame;
  node.next_nid = -1;
  node.prev_nid = -1;
  node.realloc_count = 0;
  node.kind = br_memory_event_alloc;

  br_memory_da_push(br_memory, node);
  ret_memory = memory + 1;

  br_memory.total_alloced += size;
  br_memory.cur_alloced += size;
  if (br_memory.cur_alloced > br_memory.max_alloced) br_memory.max_alloced = br_memory.cur_alloced;
  br_memory.cur_frame_alloced += size;

  return ret_memory;
}

void* br_memory_realloc(void* old, size_t new_size, const char* file_name, int line) {
  size_t index;
  long diff, new_nid;
  br_memory_node_t *node, new_node;
  size_t* new_memory;

  if (old == NULL) BR_LOGF("Trying to realloc NULL at: %s:%d", file_name, line);
  index = *(((size_t*)old) - 1);
  if (index >= br_memory.len) BR_LOGF("Trying to realloc something that was not mallocked at: %s:%d", file_name, line);
  node = &br_memory.arr[index];
  diff = (long)new_size - (long)node->size;
  new_nid = br_memory.len;
  switch (node->kind) {
    case br_memory_event_alloc:
    case br_memory_event_realloc: {
      if (new_size <= node->size) {
        br_memory_stack_print(index);
        BR_LOGW("SUS realloc: old size: %ld, new size: %ld at: %s:%d", (long)node->size, (long)new_size, file_name, line);
        br_memory.cur_alloced -= -diff;
        br_memory.cur_frame_freed += -diff;
      } else if (node->next_nid != -1) {
        br_memory_stack_print(index);
        BR_LOGF("Double realloc at %s:%d", file_name, line);
      } else {
        br_memory.total_alloced += diff;
        br_memory.cur_alloced += diff;
        if (br_memory.cur_alloced > br_memory.max_alloced) br_memory.max_alloced = br_memory.cur_alloced;
        br_memory.cur_frame_alloced += diff;
      }

      memset(&new_node, 0, sizeof(new_node));
      new_node.at_file_name = file_name;
      new_node.size = new_size;
      new_node.at_line_num = line;
      new_node.frame_num = br_memory.current_frame;
      new_node.next_nid = -1;
      new_node.prev_nid = index;
      new_node.realloc_count = node->realloc_count + 1;
      new_node.kind = br_memory_event_realloc;
    } break;
    case br_memory_event_free: {
      br_memory_stack_print(index);
      BR_LOGF("Trying to realloc freed memory at: %s:%d", file_name, line);
    } break;
    default: BR_LOGF("Unknown node kind: %d", node->kind);
  }

  new_memory = malloc(new_size + sizeof(size_t));
  memcpy(new_memory + 1, old, node->size);
  *new_memory = new_nid;
  new_node.addr = new_memory;
  node->next_nid = new_nid;
  br_memory_da_push(br_memory, new_node);

  /* Resize old memory to just fit the index of the tracker node */
  /* But don't actually do it, because it could invalidate the memory or something.. */
  /* realloc(((size_t*)old - 1), new_size); */
  return new_memory + 1;
}

void br_memory_free(void* old, const char* file_name, int line) {
  size_t *index;
  br_memory_node_t *node, new_node;
  int new_nid;

  if (NULL == old) BR_LOGF("Trying to free NULL at: %s:%d", file_name, line);
  index = (((size_t*)old) - 1);
  if (*index >= br_memory.len) BR_LOGF("Trying to free something that was not mallocked at: %s:%d", file_name, line);
  node = &br_memory.arr[*index];
  if (node->kind == br_memory_event_free) {
    br_memory_stack_print(*index);
    BR_LOGF("%s:%d -> Double free!", file_name, line);
  }
  if (node->next_nid != -1) {
    br_memory_stack_print(*index);
    BR_LOGF("%s:%d -> Freeing realloced memory!", file_name, line);
  }

  memset(&new_node, 0, sizeof(new_node));
  new_node.at_file_name = file_name;
  new_node.size = node->size;
  new_node.addr = NULL;
  new_node.at_line_num = line;
  new_node.frame_num = br_memory.current_frame;
  new_node.next_nid = -1;
  new_node.prev_nid = *index;
  new_node.realloc_count = node->realloc_count;
  new_node.kind = br_memory_event_free;

  new_nid = br_memory.len;
  node->next_nid = new_nid;
  br_memory_da_push(br_memory, new_node);

  br_memory.cur_alloced -= node->size;
  br_memory.cur_frame_freed += node->size;

  /* Resize old memory to just fit the index of the tracker node */
  /* But don't actually do it, because it could invalidate the memory or something.. */
  /* realloc(((size_t*)old - 1), new_size); */
  *index = new_nid;
}

br_memory_t br_memory_get(void) {
  return br_memory;
}

void br_memory_frame(void) {
  if (br_memory.cur_frame_nodes_len != (int)br_memory.len) {
    br_memory_frame_t new_frame;
    new_frame.start_nid = br_memory.cur_frame_nodes_len;
    new_frame.len = br_memory.len - br_memory.cur_frame_nodes_len;
    new_frame.frame_num = br_memory.current_frame;
    br_memory_da_push(br_memory.frames, new_frame);
  }
  ++br_memory.current_frame;
  br_memory.cur_frame_nodes_len = br_memory.len;
  br_memory.cur_frame_alloced = 0;
  br_memory.cur_frame_freed = 0;
}

static void br_memory_node_print(br_memory_node_t node);
void br_memory_stack_print(int top_nid) {
  br_memory_node_t node, node_inner;
  int newest_nid;

  printf("=================================\n");
  node = br_memory.arr[top_nid];
  if (node.next_nid != -1) {
    printf("Newer nodes:\n");
    newest_nid = top_nid;
    node_inner = br_memory.arr[newest_nid];
    while (node_inner.next_nid != -1) {
      newest_nid = node_inner.next_nid;
      node_inner = br_memory.arr[newest_nid];
    }
    while (newest_nid != top_nid) {
      br_memory_node_print(node_inner);
      newest_nid = node_inner.prev_nid;
      node_inner = br_memory.arr[newest_nid];
    }
    printf("--------------------------------\n");
  }
  printf("Current node:\n");
  br_memory_node_print(node);
  if (node.prev_nid != -1) {
    printf("--------------------------------\n");
    printf("Older node:\n");
    {
      while (node.prev_nid != -1) {
        node = br_memory.arr[node.prev_nid];
        br_memory_node_print(node);
      }
    }
  }
  printf("=================================\n\n");
}

void br_memory_finish(void) {
  bool *visited;
  size_t i;
  int cur;

  visited = calloc(br_memory.len, sizeof(bool));
  for (i = 0; i < br_memory.len; ++i) {
    cur = (int)i;
    if (visited[i]) continue;
    visited[i] = true;
    while (br_memory.arr[cur].next_nid != -1) {
      cur = br_memory.arr[cur].next_nid;
      visited[cur] = true;
    }
    if (br_memory.arr[cur].kind != br_memory_event_free) {
      printf("====Memory leak!=====\n");
      br_memory_stack_print(i);
      printf("====End of Memory leak!=====\n\n");
    }
  }

  for (i = 0; i < br_memory.len; ++i) {
    if (br_memory.arr[i].addr) free(br_memory.arr[i].addr);
  }

  if (br_memory.arr) free(br_memory.arr);
  if (br_memory.frames.arr) free(br_memory.frames.arr);
  memset(&br_memory, 0, sizeof(br_memory));
  free(visited);
}

static void br_memory_node_print(br_memory_node_t node) {
  switch (node.kind) {
    case br_memory_event_alloc:
    {
      printf("%s:%d Alloc %d bytes at Frame %d\n", node.at_file_name, node.at_line_num, (int)node.size, node.frame_num);
    } break;
    case br_memory_event_realloc:
    {
      printf("%s:%d Realloc to %d bytes at Frame %d\n", node.at_file_name, node.at_line_num, (int)node.size, node.frame_num);
    } break;
    case br_memory_event_free:
    {
      printf("%s:%d Free %d bytes at Frame %d\n", node.at_file_name, node.at_line_num, (int)node.size, node.frame_num);
    } break;
    default: printf("Unknown event node kind %d\n", node.kind);
  }
}

#endif
