/*
 *  brui - Author: Branimir Ričko
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
 * Brui - Brui is a cross platform UI library.
 * It should work on Linux, Windows, Mac and Web.
 * Example:
 * ```main.c
#define BRUI_IMPLEMENTATION
#include <brui.h>

int main(void) {
  brui_window_t window = { 0 };
  while (false == window->pl.should_close) {
    while (brpl_event_frame_next != brui_event_next(&window).kind); // You can also handle some of the events 
    brui_frame_start(&window);
      if (brui_buttonf("Hello")) printf("Hello world\n");
    brui_frame_end(&window);
  }
}
 * ```
 * Compile as: cc main.c
 * Functions that are part of simple api can be found by following a tag *BRUI_API*
 * */
#if !defined(BR_INCLUDE_BRUI_H)
#define BR_INCLUDE_BRUI_H
#include "include/brplat.h"


typedef struct brui_window_t {
  brpl_window_t pl; // Platform window
  bool inited;
} brui_window_t;

// --------------------------------------------- BRUI_API ----------------------------------------------
BR_EXPORT bool brui_window_init(brui_window_t* ui_window);
BR_EXPORT brpl_event_t brui_event_next(brui_window_t* ui_window);
BR_EXPORT void brui_window_frame_start(brui_window_t* ui_window);
BR_EXPORT void brui_window_frame_end(brui_window_t* ui_window);

BR_EXPORT bool brui_buttonf(const char* fmt, ...);

#endif

#if defined(BRUI_IMPLEMENTATION)
#  if !defined(BR_INCLUDE_UNITY_BRUI_C)
#    define BR_INCLUDE_UNITY_BRUI_C
#    include "tools/unity/brui.c"
#  endif
#endif
