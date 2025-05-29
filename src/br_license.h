#pragma once
#include "src/br_str.h"

extern const br_strv_t br_license[];
extern int br_license_lines;
#if defined(BR_LICENSE_IMPLEMENTATION)
#  include ".generated/br_license.c"
#endif
