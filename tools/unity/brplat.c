#define BRPLAT_IMPLEMENTATION
#include "include/brplat.h"

#if !defined(BR_INCLUDE_BR_STR_IMPL_H)
#  define BR_INCLUDE_BR_STR_IMPL_H
#  define BR_STR_IMPLEMENTATION
#  include "src/br_str.h"
#endif

#if !defined(BR_INCLUDE_PLATFORM2_C)
#  define BR_INCLUDE_PLATFORM2_C
#  include "src/platform2.c"
#endif
/*
*/

// cc -I. -o brploto tools/unity/brplot.c -lm


