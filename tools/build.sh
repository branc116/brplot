CC=clang
CXX=clang++

$CXX -DIMGUI_DISABLE_DEBUG_TOOLS -DIMGUI_DISABLE_DEMO_WINDOWS -fdata-sections -ffunction-sections -DRELEASE -flto -Os -DLINUX=1 -DPLATFORM_DESKTOP=1 -DIMGUI -I. -Isrc -I ./external/imgui-docking -I./external/raylib-5.0/src/ -I./external/Tracy -c src/unity/brplot.cpp -o build/brplot++.o & \
$CC                                                           -fdata-sections -ffunction-sections -DRELEASE -flto -Os -DLINUX=1 -DPLATFORM_DESKTOP=1 -DIMGUI -I. -Isrc                             -I./external/raylib-5.0/src/ -I./external/Tracy -c src/unity/brplot.c   -o build/brplot.o && \
$CXX -Os -fdata-sections -ffunction-sections -Wl,--gc-sections -flto build/brplot.o build/brplot++.o -lglfw -lm -o bin/brp

