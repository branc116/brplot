#if !defined(BR_ASSERT)
#  define BR_ASSERT(EXPR)
#endif

#if !defined(BR_ASSERTF)
#  define BR_ASSERTF(EXPR, FMT, ...)
#endif

#if !defined(BR_MALLOC)
#  define BR_MALLOC(SIZE) malloc(SIZE)
#endif

#if !defined(BR_FREE)
#  define BR_FREE(PTR) malloc(PTR)
#endif

#define brfl_push(FL, VALUE, HANDLE) \
  (HANDLE = brfl_push_internal_get_handle(&(FL).arr, &(FL).free_arr, &(FL).len, &(FL).cap, &(FL).free_len, &(FL).free_next, sizeof(VALUE), __FILE__, __LINE__), \
   (FL)->arr[HANDLE] = (VALUE), \
   HANDLE)

#define brfl_remove(FL, HANDLE) do { \
  int fn = (FL).free_next; \
  (FL).free_next = handle; \
  (FL).free_arr[handle] = fn; \
  --(FL).free_len; \
} while (0)

#define brfl_free(FL) do { \
  br_da_free(FL); \
  BR_FREE((FL).free_arr); \
  (FL).free_arr = NULL; \
  (FL).free_len = 0; \
  (FL).free_next = -1; \
} while (0)

#if !defined(_MSC_VER)

TEST_CASE(free_list_test) {
  typedef struct {
    int* arr;
    int* free_arr;
    int len, cap;
    int free_len;
    int free_next;
  } ints;

  ints is = { 0 };
  int one_handle = brfl_push(&is, 1);
  TEST_EQUAL(one_handle, 0);
  int two_handle = brfl_push(&is, 2);
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
#endif
