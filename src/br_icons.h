#pragma once
#include ".generated/icons.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct br_shader_icon_t br_shader_icon_t;

void br_icons_init(br_shader_icon_t* shader);
void br_icons_draw(br_shader_icon_t* shader, br_extent_t screen, br_extent_t atlas, br_color_t bg, br_color_t fg, int z);
void br_icons_deinit(void);
br_extent_t br_icons_y_mirror(br_extent_t icon);
br_extent_t br_icons_top(br_extent_t icon, float percent);

br_extent_t br_icons_bot(br_extent_t icon, float percent);
br_extent_t br_icons_left(br_extent_t icon, float percent);
br_extent_t br_icons_right(br_extent_t icon, float percent);

br_extent_t br_icons_tc(br_extent_t icon, float top, float center);
br_extent_t br_icons_lc(br_extent_t icon, float left, float center);
br_extent_t br_icons_bc(br_extent_t icon, float top, float center);
br_extent_t br_icons_rc(br_extent_t icon, float left, float center);

#if defined(__cplusplus)
}
#endif
