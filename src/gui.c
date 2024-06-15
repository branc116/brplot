#if defined(IMGUI)
#  ifndef RELEASE
#    if defined(__linux__)
#      include "imgui/hotreload.c"
#    endif
#  endif
#elif defined(RAYLIB)
#  include "raylib/gui.c"
#elif defined(HEADLESS)
#  include "headless/gui.c"
#else
#  error "Gui is not selected add define flag e.g. -DIMGUI for imgui gui, or -DRAYLIB for raylib gui, or -DHEADLESS for headless gui."
#endif

void ISO_C_requires_a_translation_unit_to_contain_at_least_one_declaration_Wempty_translation_unit(void) {}

