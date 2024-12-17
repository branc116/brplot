#pragma once
#include ".generated/icons.h"

#if defined(__cplusplus)
extenr "C" {
#endif

typedef struct br_shader_icon_t br_shader_icon_t;

void br_icons_init(br_shader_icon_t* shader);
void br_icons_draw(br_shader_icon_t* shader, br_extent_t screen, br_extent_t atlas, br_vec4_t bg, br_vec4_t fg);
void br_icons_deinit(void);

#if defined(__cplusplus)
}
#endif
