#if !defined(BR_INCLUDE_BR_FREE_LIST)
#  include "include/br_free_list_header.h"
#endif // !defined(BR_INCLUDE_BR_FREE_LIST)

#if defined(BRFL_IMPLEMENTATION)
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int brfl_push_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line) {
  (void)file; (void)line;
  int* free_arr = *free_arrp;
  int len = *lenp;
  int cap = *capp;
  int free_next = *free_nextp;
  if (cap == 0) {
    BR_ASSERTF(*arrp == NULL, "[%s:%d] Cap is set to null, but arr is not null", file, line);
    *arrp = BR_MALLOC(value_size);
    *free_arrp = BR_MALLOC(sizeof(free_arr[0]));
    *free_nextp = -1;
    if (*arrp != NULL && *free_arrp != NULL) {
      *capp = 1;
      **free_arrp = -1;
      *free_lenp = 1;
      *lenp = 1;
      return 0;
    } else {
      BR_LOGE("[%s:%d] Failed to allocate memory for free list arrays.\n"
           "        arr=%p, free_arr=%p, value_size=%zu", file, line, *arrp, (void*)*free_arrp, value_size);
      return -1;
    }
  } else if (free_next != -1) {
    int ret = free_next;
    BR_ASSERTF(ret < len, "[%s:%d] Free list is corupet, next free should be %d, but array only has %d elements.", file, line, ret, len);
    BR_ASSERTF(ret >= 0, "[%s:%d] Free list is corupet, ret is %d", file, line, ret);
    free_next = free_arr[ret];
    free_arr[ret] = -1;
    *free_nextp = free_next == -2 ? -1 : free_next;
    ++(*free_lenp);
    return ret;
  } else if (len < cap) {
    free_arr[len] = -1;
    ++(*lenp);
    ++(*free_lenp);
    return len;
  } else {
    BR_ASSERTF(*arrp != NULL, "[%s:%d] Array capacity is %d, but array pointr is NULL", file, line, cap);
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
      ++(*free_lenp);
      return *lenp - 1;
    }
    else {
      BR_ASSERTF(is_ok, "[%s:%d] Failed to reallocate array.\n"
                     "        Most likely out of memory.\n"
                     "        Cur capacity: %d, element size: %zu",
                     file, line, cap, value_size);
      return -1;
    }
  }
}
int brfl_push_end_internal_get_handle(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, const char* file, int line) {
  (void)file; (void)line;
  int* free_arr = *free_arrp;
  int len = *lenp;
  int cap = *capp;
  if (cap == 0) {
    BR_ASSERTF(*arrp == NULL, "[%s:%d] Cap is set to null, but arr is not null", file, line);
    *arrp = BR_MALLOC(value_size);
    *free_arrp = BR_MALLOC(sizeof(free_arr[0]));
    *free_nextp = -1;
    if (*arrp != NULL && *free_arrp != NULL) {
      *capp = 1;
      **free_arrp = -1;
      *free_lenp = 1;
      *lenp = 1;
      return 0;
    } else {
      BR_LOGE("[%s:%d] Failed to allocate memory for free list arrays.\n"
           "        arr=%p, free_arr=%p, value_size=%zu", file, line, *arrp, (void*)*free_arrp, value_size);
      return -1;
    }
  } else if (len < cap) {
    free_arr[len] = -1;
    ++(*lenp);
    ++(*free_lenp);
    return len;
  } else {
    BR_ASSERTF(*arrp != NULL, "[%s:%d] Array capacity is %d, but array pointr is NULL", file, line, cap);
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
      ++(*free_lenp);
      return *lenp - 1;
    }
    else {
      BR_ASSERTF(is_ok, "[%s:%d] Failed to reallocate array.\n"
                     "        Most likely out of memory.\n"
                     "        Cur capacity: %d, element size: %zu",
                     file, line, cap, value_size);
      return -1;
    }
  }
}

int brfl_push_internal(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, void* value, const char* file, int line) {
  int handle = brfl_push_internal_get_handle(arrp, free_arrp, lenp, capp, free_lenp, free_nextp, value_size, file, line);
  memcpy((char*)*arrp + (value_size * (size_t)handle), value, value_size);
  return handle;
}

int brfl_push_end_internal(void** const arrp, int** const free_arrp, int* const lenp, int* const capp, int* const free_lenp, int* const free_nextp, size_t value_size, void* value, const char* file, int line) {
  int handle = brfl_push_end_internal_get_handle(arrp, free_arrp, lenp, capp, free_lenp, free_nextp, value_size, file, line);
  memcpy((char*)*arrp + (value_size * (size_t)handle), value, value_size);
  return handle;
}

int brfl_next_taken(int const* free_arr, int len, int index) {
  for (++index; index < len; ++index) if (free_arr[index] == -1) return index;
  return index;
}

bool brfl_is_valid(int const* free_arr, int len, int free_next, int free_len) {
  struct { int* arr; size_t len, cap; } visited = { 0 };
  bool success = true;
  int i = 0;
  bool contains = false;

  if (free_next == -1)  {
    for (i = 0; i < len; ++i) if (free_arr[i] != -1) BR_ERROR("Invalid");
    if (len == free_len) goto done;
    else BR_ERROR("Invalid len: %d, free_len: %d", len, free_len);
  }
  if (free_next < -1)   BR_ERROR("invalid free_next: %d", free_next);
  if (free_next >= len) BR_ERROR("invalid free_next: %d", free_next);
  br_da_push(visited, free_next);
  for (i = 0; i < len; ++i) {
    int nf = free_arr[i];
    if (nf >= 0) {
      if (nf >= len) BR_ERROR("bad nf: %d", nf);
      br_da_contains(visited, nf, contains);
      if (contains) BR_ERROR("%d is freed twice", nf);
      br_da_push(visited, nf);
    } else if (nf < -2) {
      BR_ERROR("%d is not valid", nf);
    } else if (nf == -1) {
      --free_len;
    }
  }
  if (free_len != 0) BR_ERROR("free_len: %d", free_len);
  goto done;

error:
  success = false;

done:
//printf("len: %d, free_next: %d, free_len: %d, arr: ", len, free_next, free_len);
//for (int i = 0; i < len; ++i) printf("%d ", free_arr[i]);
//printf("\n");
  br_da_free(visited);
  return success;
}

#endif
