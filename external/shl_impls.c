#include "src/br_pp.h"

#define BR_MEMORY_TRACER_IMPLEMENTATION
#include "src/br_memory.h"

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#endif

#define STBRP_ASSERT BR_ASSERT
#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb_rect_pack.h"

#define STBTT_assert BR_ASSERT
#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define STBDS_ASSERT BR_ASSERT
#define STB_DS_IMPLEMENTATION
#include "external/stb_ds.h"

#undef STB_RECT_PACK_IMPLEMENTATION
#undef STB_TRUETYPE_IMPLEMENTATION
#undef STB_DS_IMPLEMENTATION

#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

#if !defined(BR_INCLUDE_BR_STR_IMPL_H)
#  define BR_INCLUDE_BR_STR_IMPL_H
#  if !defined(BR_STR_IMPLEMENTATION)
#    define BR_STR_IMPLEMENTATION
#    include "src/br_str.h"
#  endif
#endif

#define BR_LICENSE_IMPLEMENTATION
#include "src/br_license.h"

#define BR_LICENSE_IMPLEMENTATION
#include "src/br_license.h"

#define BRFL_IMPLEMENTATION
#include "src/br_free_list.h"

#define BRSP_IMPLEMENTATION
#include "src/br_string_pool.h"
