#include "src/br_plot.h"
#include "stdbool.h"
#include "emscripten/emscripten.h"
#include "emscripten/html5.h"

br_plot_t* public_context;

EM_BOOL (br_resize)(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) {
  printf("Resize\n");
  return false;
}


void br_gui_init_specifics_platform(br_plot_t* br) {
  //EMSCRIPTEN_WEBGL_CONTEXT_HANDLE a = emscripten_webgl_get_current_context();
  emscripten_set_resize_callback("canvas", NULL, false, br_resize);
}
