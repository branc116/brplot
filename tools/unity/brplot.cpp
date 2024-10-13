#include "../../external/imgui-docking/imgui.cpp"
#include "../../external/imgui-docking/imgui_draw.cpp"
#include "../../external/imgui-docking/imgui_tables.cpp"
#include "../../external/imgui-docking/imgui_widgets.cpp"
#include "../filesystem++.cpp"
#include "../gui++.cpp"
#include "../memory.cpp"
#include "../resampling2.cpp"
#include "../../external/imgui-docking/backends/imgui_impl_glfw.cpp"
#include "../../external/imgui-docking/backends/imgui_impl_opengl3.cpp"

// c++ -DLINUX=1 -DPLATFORM_DESKTOP=1 -DIMGUI -I. -Isrc -I ./external/imgui-docking -I./external/raylib-5.0/src/ -I ./external/Tracy -c src/unity/brplot.cpp
