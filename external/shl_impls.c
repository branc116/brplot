#if defined(__GNUC__) || defined(__CLANG__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#endif

#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

#define STB_DS_IMPLEMENTATION
#include "external/stb_ds.h"

#undef STB_RECT_PACK_IMPLEMENTATION
#undef STB_TRUETYPE_IMPLEMENTATION
#undef STB_DS_IMPLEMENTATION

#if defined(__GNUC__) || defined(__CLANG__)
#  pragma GCC diagnostic pop
#endif
