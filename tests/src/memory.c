#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"

int main(void) {
#define SZ (sizeof(int)*16)
  int* ints = BR_MALLOC(SZ);
  br_memory_frame();
  ints = BR_REALLOC(ints, SZ*2);
  br_memory_frame();
  br_memory_frame();
  BR_FREE(ints);
/*
  br_memory_t tracker = br_memory_get();
  BR_ASSERT(tracker.len == 3);

  br_memory_node_t malloc_node = tracker.arr[0];
  BR_ASSERT(malloc_node.frame_num == 0);
  BR_ASSERT(malloc_node.kind == br_memory_event_alloc);
  BR_ASSERT(malloc_node.size == SZ);
  BR_ASSERT(malloc_node.next_nid == 1);
  br_memory_stack_print(0);

  br_memory_node_t realloc_node = tracker.arr[1];
  BR_ASSERT(realloc_node.frame_num == 1);
  BR_ASSERT(realloc_node.kind == br_memory_event_realloc);
  BR_ASSERT(realloc_node.size == 2*SZ);
  BR_ASSERT(realloc_node.next_nid == 2);
  BR_ASSERT(realloc_node.prev_nid == 0);
  BR_ASSERT(realloc_node.realloc_count == 1);
  br_memory_stack_print(1);

  br_memory_node_t free_node = tracker.arr[2];
  BR_ASSERT(free_node.frame_num == 3);
  BR_ASSERT(free_node.kind == br_memory_event_freed);
  BR_ASSERT(free_node.prev_nid == 1);
  BR_ASSERT(free_node.realloc_count == 1);
  br_memory_stack_print(2);

  BR_ASSERT(tracker.max_alloced == sizeof(int)*32);
  BR_ASSERT(tracker.total_alloced == sizeof(int)*32);
  BR_ASSERT(tracker.cur_alloced == 0);
  BR_ASSERT(tracker.cur_frame_alloced == 0);
  BR_ASSERT(tracker.cur_frame_freed == sizeof(int)*32);

  br_memory_frame();
  tracker = br_memory_get();
  br_memory_frames_t frames = tracker.frames;
  BR_ASSERT(frames.len == 3);
  BR_ASSERT(frames.arr[0].start_nid == 0);
  BR_ASSERT(frames.arr[0].len == 1);
  BR_ASSERT(frames.arr[0].frame_num == 0);

  BR_ASSERT(frames.arr[1].start_nid == 1);
  BR_ASSERT(frames.arr[1].len == 1);
  BR_ASSERT(frames.arr[1].frame_num == 1);

  BR_ASSERT(frames.arr[2].start_nid == 2);
  BR_ASSERT(frames.arr[2].len == 1);
  BR_ASSERT(frames.arr[2].frame_num == 3);
  return 0;
  */
}

void br_on_fatal_error(void) {}
