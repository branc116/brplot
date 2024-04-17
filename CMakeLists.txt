cmake_minimum_required(VERSION 3.23.0)
include(CMakeDependentOption)
project(Brplot)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release Debug)
if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Release)
endif ()
message("Config=${CMAKE_BUILD_TYPE}")

set_property(CACHE GUI PROPERTY STRINGS raylib imgui headless)
if (GUI STREQUAL "")
  set(GUI imgui)
endif ()
message("GUI=${GUI}")

cmake_dependent_option(USE_LTO "Enable LTO" ON "CMAKE_BUILD_TYPE STREQUAL Release" OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${USE_LTO})
message("USE_LTO=${USE_LTO}")

option(USE_TRACY OFF "Use tracy for profiling")
cmake_dependent_option(USE_HOTRELOAD "Enable Hot reload" ON "CMAKE_BUILD_TYPE STREQUAL Debug; GUI STREQUAL imgui" OFF)

add_library(raylib STATIC
  ./external/raylib-5.0/src/rmodels.c
  ./external/raylib-5.0/src/rshapes.c
  ./external/raylib-5.0/src/rtext.c
  ./external/raylib-5.0/src/rtextures.c
  ./external/raylib-5.0/src/utils.c
  ./external/raylib-5.0/src/rcore.c
)
target_include_directories(raylib PUBLIC .)

add_library(imgui STATIC
  ./external/imgui-docking/imgui.cpp
  ./external/imgui-docking/imgui_draw.cpp
  ./external/imgui-docking/imgui_tables.cpp
  ./external/imgui-docking/imgui_widgets.cpp
  ./external/imgui-docking/backends/imgui_impl_glfw.cpp
  ./external/imgui-docking/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ./external/imgui-docking/)

add_executable(brplot
  src/main.c          src/help.c        src/data.c        src/smol_mesh.c src/q.c
  src/read_input.c    src/plotter.c     src/keybindings.c src/str.c       src/memory.cpp 
  src/resampling2.cpp src/graph_utils.c src/shaders.c     src/plot.c
)

if (MSVC)
else ()
target_compile_options(brplot PUBLIC -Wconversion -Wall -Wpedantic -Wextra)
endif ()

if (USE_TRACY)
  message("Using tracy")
  target_compile_definitions(brplot PUBLIC TRACY_ENABLE=1)
  target_link_libraries(brplot tracy)
endif ()

if (USE_HOTRELOAD)
  message("Using hot reload")
  target_sources(brplot PUBLIC ./src/imgui/hotreload.c)
  target_link_options(brplot PUBLIC -fpie -rdynamic)
  target_compile_options(brplot PUBLIC -fpie)
endif ()

add_executable(font_export tools/font_export.c)
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/misc/default_font.h
  COMMAND font_export fonts/PlayfairDisplayRegular-ywLOY.ttf src/misc/default_font.h
  DEPENDS font_export fonts/PlayfairDisplayRegular-ywLOY.ttf
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(font_export PUBLIC m)
target_sources(brplot PRIVATE src/misc/default_font.h)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  target_compile_definitions(brplot PUBLIC DEBUG)
  target_sources(imgui PUBLIC ./external/imgui-docking/imgui_demo.cpp)
elseif (CMAKE_BUILD_TYPE STREQUAL Release)
  target_compile_definitions(brplot PUBLIC RELEASE IMGUI_DISABLE_DEMO_WINDOWS IMGUI_DISABLE_DEBUG_TOOLS)
  if (MSVC)
  else ()
    target_compile_options(imgui PUBLIC -fdata-sections -ffunction-sections)
    target_compile_options(raylib PUBLIC -fdata-sections -ffunction-sections)
    target_compile_options(brplot PUBLIC -fdata-sections -ffunction-sections)
    target_link_options(raylib PUBLIC -fdata-sections -ffunction-sections -Wl,--gc-sections)
    target_link_options(imgui PUBLIC -fdata-sections -ffunction-sections -Wl,--gc-sections)
    target_link_options(brplot PUBLIC -fdata-sections -ffunction-sections -Wl,--gc-sections)
  endif ()
  add_executable(shaders_bake tools/shaders_bake.c src/str.c)
  target_include_directories(shaders_bake PUBLIC . ./external/raylib-5.0/src)
  file(GLOB shaders src/desktop/shaders/*)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/misc/shaders.h
    COMMAND shaders_bake ${CMAKE_SYSTEM_NAME} src/misc/shaders.h
    DEPENDS shaders_bake ${shaders}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_sources(brplot PUBLIC ./src/misc/shaders.h)
else ()
  message(FATAL_ERROR "CMAKE_BUILD_TYPE=`${CMAKE_BUILD_TYPE}` not supported. (Debug|Release)")
endif()

if (GUI STREQUAL raylib)
  target_compile_definitions(brplot PUBLIC RAYLIB)
  target_sources(brplot PUBLIC ./src/raylib/gui.c ./src/raylib/ui.c)
  target_link_libraries(brplot raylib)
elseif (GUI STREQUAL imgui)
  target_compile_definitions(brplot PUBLIC IMGUI)
  target_sources(brplot PUBLIC
    ./src/imgui/file_saver.cpp
    ./src/imgui/gui.cpp
    ./src/imgui/imgui_extensions.cpp
    ./src/imgui/ui_info.cpp
    ./src/imgui/ui_settings.cpp
  )
  target_link_libraries(brplot imgui raylib)
  target_include_directories(brplot PUBLIC external/imgui-docking)
elseif (GUI STREQUAL headless)
  target_compile_definitions(brplot PUBLIC HEADLESS)
  target_sources(brplot 
    ./src/headless/gui.c
    ./src/headless/raylib_headless.c
  )
else ()
  message(FATAL_ERROR "GUI=${GUI} not supported. (raylib|imgui|headless)")
endif ()


if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  target_compile_definitions(brplot PUBLIC LINUX)
  target_compile_definitions(raylib PUBLIC PLATFORM_DESKTOP=1)
  target_link_libraries(raylib glfw)
  target_sources(brplot PUBLIC 
    ./src/desktop/platform.c
    ./src/desktop/linux/read_input.c
  )
else ()
  message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
endif ()

set(PLAT_CONF ${CMAKE_SYSTEM_NAME}_${CMAKE_BUILD_TYPE})

if (PLAT_CONF STREQUAL Linux_Debug)
  target_sources(brplot PUBLIC
    ./src/desktop/linux/refresh_shaders.c)
elseif (PLAT_CONF STREQUAL Linux_Release)
else ()
  message(FATAL_ERROR "${PLAT_CONF} not supported")
endif ()

target_include_directories(brplot PUBLIC . external/Tracy external/raylib-5.0/src)
