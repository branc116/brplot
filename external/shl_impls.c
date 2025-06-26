#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wgnu-binary-literal"
#endif

#define BR_UNIT_TEST_IMPLEMENTATION
#include "external/tests.h"

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

#define BR_STR_IMPLMENTATION
#include "src/br_str.h"

#define BR_LICENSE_IMPLEMENTATION
#include "src/br_license.h"

#define BR_LICENSE_IMPLEMENTATION
#include "src/br_license.h"
