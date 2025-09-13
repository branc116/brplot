#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if !defined(BR_MALLOC)
#  define BR_MALLOC(SIZE) br_malloc_trace(false, SIZE, __FILE__, __LINE__)
#endif

#if !defined(BR_CALLOC)
#  define BR_CALLOC(N, SIZE) br_malloc_trace(true, (N * SIZE), __FILE__, __LINE__)
#endif

#if !defined(BR_REALLOC)
#  define BR_REALLOC(PTR, NEW_SIZE) br_realloc_trace(PTR, NEW_SIZE, __FILE__, __LINE__)
#endif

#if !defined(BR_FREE)
#  define BR_FREE(PTR) br_free_trace(PTR, __FILE__, __LINE__)
#endif

#if !defined(BR_THREAD_LOCAL)
#  if (__TINYC__)
#    define BR_THREAD_LOCAL       thread_local
#  else
#    define BR_THREAD_LOCAL       _Thread_local
#  endif
#endif

#if !defined(BR_ASSERT)
#  include <assert.h>
#  define BR_ASSERT(expr) assert(expr)
#endif

#if !defined(BR_LOGW)
#  include <stdio.h>
#  define BR_LOGW(msg, ...) printf("[WARRNING] " msg "\n", ##__VA_ARGS__)
#endif

#if !defined(BR_LOGE)
#  include <stdio.h>
#  define BR_LOGE(msg, ...) printf("[ERROR] " msg "\n", ##__VA_ARGS__)
#endif

#if !defined(BR_LOGF)
#  include <stdio.h>
#  define BR_LOGF(msg, ...) do { \
     printf("[FATAL] " msg "\n", ##__VA_ARGS__); \
     abort(); \
   } while(0)
#endif

typedef enum br_malloc_tracker_event_kind_t {
  br_malloc_tracker_event_alloc,
  br_malloc_tracker_event_realloc,
  br_malloc_tracker_event_freed,
} br_malloc_tracker_event_kind_t;

typedef struct br_malloc_tracker_node_t {
  const char* at_file_name;
  size_t size;
  int at_line_num;
  int frame_num;

  int next_nid;
  int prev_nid;
  int realloc_count;
  br_malloc_tracker_event_kind_t kind;
} br_malloc_tracker_node_t;

typedef struct br_malloc_tracker_frame_t {
  int start_nid;
  int len;
  int frame_num;
} br_malloc_tracker_frame_t;

typedef struct br_malloc_tracker_frames_t {
  br_malloc_tracker_frame_t* arr;
  size_t len, cap;
} br_malloc_tracker_frames_t;

typedef struct br_malloc_tracker_t {
  br_malloc_tracker_node_t* arr;
  size_t len, cap;

  int current_frame;
  int cur_frame_nodes_len;

  size_t total_alloced;
  size_t cur_alloced;
  size_t max_alloced;
  size_t cur_frame_alloced;
  size_t cur_frame_freed;

  br_malloc_tracker_frames_t frames;
} br_malloc_tracker_t;

void* br_malloc_trace (bool zero,      size_t size, const char* file_name, int line_num);
void* br_calloc_trace (size_t  n,      size_t size, const char* file_name, int line_num);
void* br_realloc_trace(void* old,  size_t new_size, const char* file_name, int line_num);
void  br_free_trace   (void* ptr,                   const char* file_name, int line_num);

br_malloc_tracker_t br_malloc_tracker_get(void);
void br_malloc_frame(void);
void br_malloc_stack_print(int top_nid);

#if defined(BR_MEMORY_TRACER_IMPLEMENTATION)

// malloc tracker must not be tracked by itself because it would create recursive calls.
#define br_malloc_da_push_t(SIZE_T, ARR, VALUE) do {                                                                \
  if ((ARR).cap == 0) {                                                                                             \
    BR_ASSERT((ARR).arr == NULL && "Cap is set to null, but arr is not null");                                      \
    (ARR).arr = malloc(sizeof(*(ARR).arr));                                                                         \
    if ((ARR).arr != NULL) {                                                                                        \
      (ARR).cap = 1;                                                                                                \
      (ARR).arr[(ARR).len++] = (VALUE);                                                                             \
    }                                                                                                               \
  }                                                                                                                 \
  else if ((ARR).len < (ARR).cap) (ARR).arr[(ARR).len++] = (VALUE);                                                 \
  else {                                                                                                            \
    BR_ASSERT((ARR).arr != NULL);                                                                                   \
    SIZE_T cap_diff = (ARR).cap;                                                                                    \
    bool is_ok = false;                                                                                             \
    while (!is_ok && cap_diff > 0) {                                                                                \
      SIZE_T new_cap =  (ARR).cap + cap_diff;                                                                       \
      void* new_arr = realloc((ARR).arr, (size_t)new_cap * sizeof(*(ARR).arr));                                     \
      if (new_arr) {                                                                                                \
        (ARR).arr = new_arr;                                                                                        \
        (ARR).cap = new_cap;                                                                                        \
        (ARR).arr[(ARR).len++] = ((VALUE));                                                                         \
        is_ok = true;                                                                                               \
      } else {                                                                                                      \
        cap_diff >>= 1;                                                                                             \
      }                                                                                                             \
    }                                                                                                               \
    BR_ASSERT(is_ok);                                                                                               \
  }                                                                                                                 \
} while(0)                                                                                                          \

#define br_malloc_da_push(ARR, VALUE) br_malloc_da_push_t(size_t, ARR, VALUE)
BR_THREAD_LOCAL br_malloc_tracker_t br_malloc_tracker;

void* br_malloc_trace(bool zero, size_t size, const char* file_name, int line_num) {
  size_t* memory = malloc(size + sizeof(size_t));
  if (zero) memset(memory, 0, size);
  *memory = br_malloc_tracker.len;
  br_malloc_tracker_node_t node = {
    .kind = br_malloc_tracker_event_alloc,
    .at_file_name = file_name,
    .at_line_num = line_num,
    .frame_num = br_malloc_tracker.current_frame,

    .size = size,
    .next_nid = -1,
    .prev_nid = -1
  };
  br_malloc_da_push(br_malloc_tracker, node);
  void* ret_memory = memory + 1;

  br_malloc_tracker.total_alloced += size;
  br_malloc_tracker.cur_alloced += size;
  if (br_malloc_tracker.cur_alloced > br_malloc_tracker.max_alloced) br_malloc_tracker.max_alloced = br_malloc_tracker.cur_alloced;
  br_malloc_tracker.cur_frame_alloced += size;

  return ret_memory;
}

static void br_malloc_node_print(br_malloc_tracker_node_t node) {
  switch (node.kind) {
    case br_malloc_tracker_event_alloc:
    {
      printf("%s:%d Alloc %zu bytes at Frame %d\n", node.at_file_name, node.at_line_num, node.size, node.frame_num);
    } break;
    case br_malloc_tracker_event_realloc:
    {
      printf("%s:%d Realloc to %zu bytes at Frame %d\n", node.at_file_name, node.at_line_num, node.size, node.frame_num);
    } break;
    case br_malloc_tracker_event_freed:
    {
      printf("%s:%d Free %zu bytes at Frame %d\n", node.at_file_name, node.at_line_num, node.size, node.frame_num);
    } break;
    default: printf("Unknown event node kind %d\n", node.kind);
  }
}

void br_malloc_stack_print(int top_nid) {
  printf("=================================\n");
  br_malloc_tracker_node_t node = br_malloc_tracker.arr[top_nid];
  if (node.next_nid != -1) {
    printf("Newer nodes:\n");
    {
      int newest_nid = top_nid;
      br_malloc_tracker_node_t node = br_malloc_tracker.arr[newest_nid];
      while (node.next_nid != -1) {
        newest_nid = node.next_nid;
        node = br_malloc_tracker.arr[newest_nid];
      }
      while (newest_nid != top_nid) {
        br_malloc_node_print(node);
        newest_nid = node.prev_nid;
        node = br_malloc_tracker.arr[newest_nid];
      }
    }
    printf("--------------------------------\n");
  }
  printf("Current node:\n");
  br_malloc_node_print(node);
  if (node.prev_nid != -1) {
    printf("--------------------------------\n");
    printf("Older node:\n");
    {
      while (node.prev_nid != -1) {
        node = br_malloc_tracker.arr[node.prev_nid];
        br_malloc_node_print(node);
      }
    }
  }
  printf("=================================\n\n");
}

void* br_realloc_trace(void* old, size_t new_size, const char* file_name, int line) {
  if (old == NULL) BR_LOGF("Trying to realloc NULL at: %s:%d", file_name, line);
  size_t index = *(((size_t*)old) - 1);
  if (index >= br_malloc_tracker.len) BR_LOGF("Trying to realloc something that was not mallocked at: %s:%d", file_name, line);
  br_malloc_tracker_node_t* node = &br_malloc_tracker.arr[index];
  ssize_t diff = (ssize_t)new_size - (ssize_t)node->size;
  size_t new_nid = br_malloc_tracker.len;
  switch (node->kind) {
    case br_malloc_tracker_event_alloc:
    case br_malloc_tracker_event_realloc: {
      if (new_size <= node->size) {
        br_malloc_stack_print(index);
        BR_LOGW("SUS realloc: old size: %zu, new size: %zu at: %s:%d", node->size, new_size, file_name, line);
        br_malloc_tracker.cur_alloced -= -diff;
        br_malloc_tracker.cur_frame_freed += -diff;
      } else if (node->next_nid != -1) {
        br_malloc_stack_print(index);
        BR_LOGF("Double realloc at %s:%d", file_name, line);
      } else {
        br_malloc_tracker.total_alloced += diff;
        br_malloc_tracker.cur_alloced += diff;
        if (br_malloc_tracker.cur_alloced > br_malloc_tracker.max_alloced) br_malloc_tracker.max_alloced = br_malloc_tracker.cur_alloced;
        br_malloc_tracker.cur_frame_alloced += diff;
      }
      br_malloc_tracker_node_t new_node = {
        .at_file_name = file_name,
        .size = new_size,
        .at_line_num = line,
        .frame_num = br_malloc_tracker.current_frame,

        .next_nid = -1,
        .prev_nid = index,
        .realloc_count = node->realloc_count + 1,
        .kind = br_malloc_tracker_event_realloc,
      };
      node->next_nid = new_nid;
      br_malloc_da_push(br_malloc_tracker, new_node);
    } break;
    case br_malloc_tracker_event_freed: {
      br_malloc_stack_print(index);
      BR_LOGF("Trying to realloc freed memory at: %s:%d", file_name, line);
    } break;
    default: BR_LOGF("Unknown node kind: %d", node->kind);
  }

  size_t* new_memory = malloc(new_size + sizeof(size_t));
  memcpy(new_memory + 1, old, node->size);
  *new_memory = new_nid;

  // Resize old memory to just fit the index of the tracker node
  // But don't actually do it, because it could invalidate the memory or something..
  // realloc(((size_t*)old - 1), new_size);
  return new_memory + 1;
}

void br_free_trace(void* old, const char* file_name, int line) {
  if (NULL == old) BR_LOGF("Trying to free NULL at: %s:%d", file_name, line);
  size_t* index = (((size_t*)old) - 1);
  if (*index >= br_malloc_tracker.len) BR_LOGF("Trying to free something that was not mallocked at: %s:%d", file_name, line);
  br_malloc_tracker_node_t* node = &br_malloc_tracker.arr[*index];
  if (node->kind == br_malloc_tracker_event_freed) {
    br_malloc_stack_print(*index);
    BR_LOGF("%s:%d -> Double free!", file_name, line);
  }
  if (node->next_nid != -1) {
    br_malloc_stack_print(*index);
    BR_LOGF("%s:%d -> Freeing realloced memory!", file_name, line);
  }

  br_malloc_tracker_node_t new_node = {
    .at_file_name = file_name,
    .size = node->size,
    .at_line_num = line,
    .frame_num = br_malloc_tracker.current_frame,

    .next_nid = -1,
    .prev_nid = *index,
    .realloc_count = node->realloc_count,
    .kind = br_malloc_tracker_event_freed,
  };

  int new_nid = br_malloc_tracker.len;
  node->next_nid = new_nid;
  br_malloc_da_push(br_malloc_tracker, new_node);

  br_malloc_tracker.cur_alloced -= node->size;
  br_malloc_tracker.cur_frame_freed += node->size;

  // Resize old memory to just fit the index of the tracker node
  // But don't actually do it, because it could invalidate the memory or something..
  // realloc(((size_t*)old - 1), new_size);
  *index = new_nid;
}

br_malloc_tracker_t br_malloc_tracker_get(void) {
  return br_malloc_tracker;
}

void br_malloc_frame(void) {
  if (br_malloc_tracker.cur_frame_nodes_len != br_malloc_tracker.len) {
    br_malloc_tracker_frame_t new_frame = {
      .start_nid = br_malloc_tracker.cur_frame_nodes_len,
      .len = br_malloc_tracker.len - br_malloc_tracker.cur_frame_nodes_len,
      .frame_num = br_malloc_tracker.current_frame
    };
    br_malloc_da_push(br_malloc_tracker.frames, new_frame);
  }
  ++br_malloc_tracker.current_frame;
  br_malloc_tracker.cur_frame_nodes_len = br_malloc_tracker.len;
  br_malloc_tracker.cur_frame_alloced = 0;
  br_malloc_tracker.cur_frame_freed = 0;
}
#endif
