#if defined(IMGUI)
#  define IMGUI_DEFINE_MATH_OPERATORS
#  include "imgui/gui.cpp"
#elif defined(RAYLIB)
#elif defined(HEADLESS)
#else
#  error "Gui is not selected add define flag e.g. -DIMGUI for imgui gui, or -DRAYLIB for raylib gui, or -DHEADLESS for headless gui."
#endif

