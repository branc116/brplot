#if !defined(BR_INCLUDE_BR_TEXT_RENDERER_H)
#define BR_INCLUDE_BR_TEXT_RENDERER_H
// Imagine a stack on which some properties as written.
// You can change those properties using this functions
// When you pop the current state. Values reset to
// the values of the older stack element.
// E.g.
#if 0
void foo(void) {
  brtr_stack_el_t* el = brtr_state_push();
    el->pos = BR_VEC2(100, 100);
    el->forground = BR_COLOR(128, 0, 0, 255);
    brtr_pushf("Hello"); // Red `Hello` at (100,100)
    el = brtr_state_push();
      el->forground = BR_COLOR(0, 128, 0, 255);
      brtr_pushf(" World."); // Green ` World.` after `Hello`.
    el = brtr_state_pop();
    brtr_pushf("\nHello Friend."); // Red `Hello Friend.` bellow `Hello`.
  el = brtr_state_pop();

  // MAYBE...
  BRTR(el) {
    el->pos = BR_VEC2(100, 100);
    el->forground = BR_COLOR(128, 0, 0, 255);
    brtr_pushf("Hello"); // Red `Hello` at (100,100)
    BRTR(el2) {
      el2->forground = BR_COLOR(0, 128, 0, 255);
      brtr_pushf(" World."); // Green ` World.` after `Hello`.
    }
    brtr_pushf("\nHello Friend."); // Red `Hello Friend.` bellow `Hello`.
  }
}
#endif

#include "include/br_str_header.h"
#include "src/br_math.h"

typedef struct brtr_t brtr_t;
typedef struct br_shaders_t br_shaders_t;

typedef struct {
  // This should be set to opengl viewport value. Needed for transformation to the opengl NDC space..
  br_extent_t viewport;
  // Which part of the screen is available. You you try to write outside of this bounding box, text will be clipped.
  br_bb_t limits;

  // Ancor and padding insided of the limits bounding box.
  //
  //  |--------------------| Justify: left_up        # |--------------------| Justify: left_up     # |--------------------| Justify: left_up
  //  |HELLO               | Ancor:   left_up        HELLO                  | Ancor:   mid_up     HELLO                   | Ancor:   right_up
  //  |                    | Pos:     0,0            # |                    | Pos:     0,0         # |                    | Pos:     0,0
  //  |                    |   You can't really see  # |                    |                      # |                    |
  //  |                    |   the up/down ancor     # |                    |  Again can't really  # |                    |
  //  |                    |   in this example..     # |                    |  see up/down         # |                    |
  //  |--------------------|                         # |--------------------|                      # |--------------------|
  //
  //  |--------------------| Justify: mid_mid.       # |--------------------| Justify: mid_mid.    # |--------------------| justify: mid_mid.
  //  |                    | Ancor:   left_up        # |                    | Ancor:   mid_mid     # |                    | Ancor:   right.
  //  |                    | Pos:     0,0            # |                    | Pos:     0,0         # |                    | Pos:     0,0
  //  |          HELLO     |                         # |        HELLO       |                      # |      HELLO         |
  //  |                    |                         # |                    |                      # |                    |
  //  |                    |                         # |                    |                      # |                    |
  //  |--------------------|                         # |--------------------|                      # |--------------------|
  //  ...
  //  TODO: Thing about how pos should be changed if justify is not left-top..
  br_dir_t ancor, justify;
  br_vec2_t pos;
  int font_size;
  // z=0        - Draw under everything
  // z=BR_Z_MAX - Draw  over everything
  int z;
  br_color_t forground;
  br_color_t background;
} brtr_stack_el_t;

// Call this before doing anything else with this library..
// It alocates memory needed and does stuff with OpenGL.
void brtr_construct(int bitmap_width, int bitmap_height, br_shaders_t* shaders);

// Free memrory and other resources allocated by this library.
// After this you can call brtr_construct once more ( for whatever reason.. )
void brtr_free(void);

// Load a new font.
// NOTE: For now only one font is supported at the time.
bool brtr_font_load(br_strv_t path);

brtr_stack_el_t* brtr_state_push(void);
brtr_stack_el_t* brtr_state_pop(void);
brtr_stack_el_t* brtr_state(void); // Current state

// OpenGL texture id...
uint32_t brtr_texture_id(void);

// Get the first substring that inside of the current brtr_state()->limits
br_strv_t brtr_fit(br_strv_t text);

// Size of the string
br_size_t brtr_measure(br_strv_t str);

br_extent_t brtr_push(br_strv_t text);
br_extent_t brtr_pushf(const char* fmt, ...);

void brtr_glyph_draw(br_bb_t where_screen, br_extent_t glyph);
// Draw a rectange on a screen with a brtr_state()->background color
void brtr_rectangle_draw(br_bb_t where_screen);
// Draw a triangle on a screen with a brtr_state()->background color
void brtr_triangle_draw(br_vec2_t a, br_vec2_t b, br_vec2_t c);

// Load icon or any image into the texture with the fonts.
// NOTE: Only one chanel. ( no RGB. )
// NOTE: To load a simly face such as this:
//       |-----------|
//       |     |----||
//       |     | :) ||
//       |     |----||
//       |           |
//       |-----------|
// pass buff: u8[13*6], buff_size = BR_SIZEI(13, 6), icon_extent = BR_EXTENTI(1/*TOP*/, 6/*LEFT*/, 6/*WIDTH*/, 3/*HEIGHT*/)
int brtr_icon_load(br_u8 const* buff, br_sizei_t buff_size, br_extenti_t icon_extent);

// Get the extent of a loaded icon inside of the texture.
// To draw it you can use brtr_glyph_draw.
br_extent_t brtr_icon(int icon);

br_extent_t brtr_glyph_y_mirror(br_extent_t icon);
br_extent_t brtr_glyph_top(br_extent_t icon, float percent);

#include ".generated/icons.h"

#endif
