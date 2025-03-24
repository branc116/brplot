#include "src/br_pp.h"
#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"

typedef struct {
  int* arr;
  int* free_arr;
  int len, cap;
  int free_len;
  int free_next;
} ints;

typedef struct {
  void* arr;
  int* free_arr;
  int len, cap;
  int free_len;
  int free_next;
} brfl_proto_fl_t;

int brfl_push_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line) {
  void* arr = *arrp;
  void* free_arr = *free_arrp;
  int len = *lenp;
  int cap = *capp;
  int free_len = *free_lenp;
  int free_next = *free_nextp;
  if (cap == 0) {
    BR_ASSERTF(arr == NULL, "[%s:%d] Cap is set to null, but arr is not null", file, line);
    *arrp = BR_MALLOC(value_size);
    *free_arrp = BR_MALLOC(sizeof(*free_arr[0]));
    *free_nextp = -1;
    if (*arr != NULL && *free_arr != NULL) {
      *capp = 1;
      **free_arrp = -1;
      *free_lenp = 1;
      *lenp = 1;
      return 0;
    } else {
      LOGE("[%s:%d] Failed to allocate memory for free list arrays.\n"
           "        arr=%p, free_arr=%p, value_size=%zu", file, line, *arrp, *free_arrp, value_size);
      return -1;
    }
  } else if (fl->free_next != -1) {
    int ret = free_next;
    BR_ASSERTF(ret < len, "[%s:%d] Free list is corupet, next free should be %d, but array only has %d elements.", ret, len);
    free_next = free_arr[ret];
    free_arr[ret] = -1;
    *free_nextp = free_next;
    ++(*free_lenp);
    return ret;
  } else if (len < cap) {
    free_arr[len] = -1;
    ++(*lenp);
    ++(*free_lenp);
    return len;
  } else {
    BR_ASSERTF(arr != NULL, "[%s:%d] Array capacity is %d, but array pointr is NULL", file, line, fl->cap);
    int cap_diff = cap;
    bool is_ok = false;
    while (!is_ok && cap_diff > 0) {
      int new_cap = cap + cap_diff;
      void* new_arr = BR_REALLOC(*arrp, (size_t)new_cap * value_size);
      int* new_free_arr = BR_REALLOC(*free_arrp, (size_t)new_cap * sizeof(int));
      if (new_arr && new_free_arr) {
        *arrp = new_arr;
        *free_arrp = new_free_arr;
        *capp = new_cap;
        (*free_arrp)[len] = -1;
        ++(*lenp);
        is_ok = true;
      } else {
        if (new_arr) *arrp = new_arr;
        if (new_free_arr) *free_arrp = new_free_arr;
        cap_diff >>= 1;
      }
    }
    if (is_ok) {
      ++(*free_len);
      return *lenp - 1;
    }
    else {
      BR_LOGE(is_ok, "[%s:%d] Failed to reallocate array.\n"
                     "        Most likely out of memory.\n"
                     "        Cur capacity: %d, element size: %zu",
                     file, line, cap, value_size);
      return -1;
    }
  }
}

void brfl_remove(ints* is, int handle) {
  int fn = is->free_next;
  is->free_next = handle;
  is->free_arr[handle] = fn;
  --is->free_len;
}

void brfl_free(ints* is) {
  br_da_free(*is);
  BR_FREE(is->free_arr);
  is->free_arr = NULL;
  is->free_len = 0;
  is->free_next = -1;
}

TEST_CASE(free_list_test) {
  ints is = { 0 };
  int one_handle = fl_push(&is, 1);
  TEST_EQUAL(one_handle, 0);
  int two_handle = fl_push(&is, 2);
  TEST_EQUAL(one_handle, 1);
  int one = br_da_get(is, one_handle);
  TEST_EQUAL(one, 1);
  int two = br_da_get(is, two_handle);
  TEST_EQUAL(two, 2);
  fl_remove(&is, one_handle);
  TEST_EQUAL(is.free_len, 1);
  TEST_EQUAL(is.free_next, one_handle);
  two = br_da_get(is, two_handle);
  TEST_EQUAL(two, 2);
  fl_free(&is);
}
