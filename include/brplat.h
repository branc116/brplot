/*
 *  Br Platform - Author: Branimir Ričko
 *    The MIT License (MIT)
 *    Copyright © Branimir Ričko [branc116]
 *
 *    Permission is hereby granted, free of charge, to any person  obtaining a copy of
 *    this software and associated  documentation files (the   “Software”), to deal in
 *    the Software without  restriction,  including without  limitation  the rights to
 *    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *    the Software,  and to permit persons to whom the Software is furnished to do so,
 *    subject to the following conditions:
 *
 *    The above  copyright notice and this  permission notice shall be included in all
 *    copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED   “AS IS”,   WITHOUT WARRANTY OF ANY KIND,   EXPRESS OR
 *    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   IN NO EVENT SHALL THE AUTHORS OR
 *    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER
 *    IN  AN  ACTION  OF CONTRACT,   TORT OR OTHERWISE,   ARISING FROM,   OUT OF OR IN
 *    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Br Platform - brpl is a cross platform platform library.
 * It should work on Linux, Windows, Mac and Web.
 * It initializes opengl and handles inputs.
 * Example:
 * ```main.c
#define BRPLAT_IMPLEMENTATION
#include <brplat.h>

int main(void) {
  brpl_window_t window = { 0 };
  while (false == window->should_close) {
    while (brpl_event_frame_next != brpl_event_next(&window).kind); // You can also handle some of the events 
    brpl_frame_start(&window);
    brpl_frame_end(&window);
  }
}
 * ```
 * Compile as: cc main.c
 * Functions that are part of simple api can be found by following a tag *BRUI_API*
 *
 * */
#if !defined(BR_INCLUDE_BR_PLAT_H)
#define BR_INCLUDE_BR_PLAT_H
#  include "include/brplat_header.h"
#endif // !defined(BR_INCLUDE_BR_PLAT_H)

#if defined(BRPLAT_IMPLEMENTATION)

#  if !defined(BR_INCLUDE_BR_STR_IMPL_H)
#    define BR_INCLUDE_BR_STR_IMPL_H
#    if !defined(BR_STR_IMPLEMENTATION)
#      define BR_STR_IMPLEMENTATION
#      include "src/br_str.h"
#    endif
#  endif

#  if !defined(BR_INCLUDE_GL_C)
#    define BR_INCLUDE_GL_C
#    include "src/gl.c"
#  endif

#  if !defined(BR_INCLUDE_PLATFORM2_C)
#    define BR_INCLUDE_PLATFORM2_C
#    include "src/platform2.c"
#  endif

#endif


