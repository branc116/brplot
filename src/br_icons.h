#pragma once
#include ".generated/icons.h"

#if defined(__cplusplus)
extenr "C" {
#endif

typedef struct br_shader_icon_t br_shader_icon_t;

void br_icons_init(br_shader_icon_t* shader);
void br_icons_draw(br_shader_icon_t* shader, br_extent_t screen, br_extent_t atlas, br_color_t bg, br_color_t fg, float z);
void br_icons_deinit(void);
br_extent_t br_icons_y_mirror(br_extent_t icon);

#if defined(__cplusplus)
}
#endif
