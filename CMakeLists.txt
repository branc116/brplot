# I don't know what I'm doing with cmake. This is all vodoo...

cmake_minimum_required(VERSION 3.22.1)
include(CMakeDependentOption)
project(Brplot)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release Debug)
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
if ((NOT DEFINED CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL ""))
  set(CMAKE_BUILD_TYPE Release)
endif ()
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

set(GUI imgui CACHE STRING "What gui to use")
set_property(CACHE GUI PROPERTY STRINGS raylib imgui headless)
message("GUI=${GUI}")

cmake_dependent_option(USE_LTO "Enable LTO" ON "CMAKE_BUILD_TYPE STREQUAL Release" OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${USE_LTO})
message("USE_LTO=${USE_LTO}")

option(USE_TRACY OFF "Use tracy for profiling")
cmake_dependent_option(USE_HOTRELOAD "Enable Hot reload" ON "CMAKE_BUILD_TYPE STREQUAL Debug; GUI STREQUAL imgui" OFF)

# I don't how msvc can hot reload...
if (MSVC)
  set(USE_HOTRELOAD OFF)
endif ()

add_library(raylib STATIC
  ./external/raylib-5.0/src/rmodels.c
  ./external/raylib-5.0/src/rshapes.c
  ./external/raylib-5.0/src/rtext.c
  ./external/raylib-5.0/src/rtextures.c
  ./external/raylib-5.0/src/utils.c
  ./external/raylib-5.0/src/rcore.c
)
target_include_directories(raylib PUBLIC . ./external/glfw/include)
set_target_properties(raylib PROPERTIES LINKER_LANGUAGE C)

add_executable(brplot
  src/main.c           src/help.c        src/data.c           src/smol_mesh.c src/q.c
  src/read_input.c     src/plotter.c     src/keybindings.c    src/str.c       src/memory.cpp
  src/resampling2.c    src/graph_utils.c src/shaders.c        src/plot.c      src/permastate.c
  src/gui.c            src/filesystem.c  src/data_generator.c src/platform.c  src/text_renderer.c
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
  target_link_options(brplot PUBLIC -fpie -rdynamic)
  target_compile_options(brplot PUBLIC -fpie)
endif ()

add_executable(font_bake tools/font_bake.c)
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/build/default_font.c
  COMMAND font_bake fonts/PlayfairDisplayRegular-ywLOY.ttf ${CMAKE_CURRENT_SOURCE_DIR}/build/default_font.c
  DEPENDS font_bake fonts/PlayfairDisplayRegular-ywLOY.ttf
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  target_compile_definitions(brplot PUBLIC DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL Release)
  target_compile_definitions(brplot PUBLIC RELEASE IMGUI_DISABLE_DEMO_WINDOWS IMGUI_DISABLE_DEBUG_TOOLS)
  if (MSVC)
  else ()
    if (GUI STREQUAL imgui)
    endif ()
    target_compile_options(raylib PUBLIC -fdata-sections -ffunction-sections)
    target_compile_options(brplot PUBLIC -fdata-sections -ffunction-sections)
    target_link_options(raylib PUBLIC -fdata-sections -ffunction-sections -Wl,--gc-sections)
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
  target_link_libraries(brplot raylib)
  set_target_properties(brplot PROPERTIES LINKER_LANGUAGE C)
elseif (GUI STREQUAL imgui)
  add_library(imgui STATIC
    ./external/imgui-docking/imgui.cpp
    ./external/imgui-docking/imgui_draw.cpp
    ./external/imgui-docking/imgui_tables.cpp
    ./external/imgui-docking/imgui_widgets.cpp
    ./external/imgui-docking/backends/imgui_impl_glfw.cpp
    ./external/imgui-docking/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui PUBLIC ./external/imgui-docking/ ./external/glfw/include)
  if (CMAKE_BUILD_TYPE STREQUAL Release)
    if (MSVC)
    else ()
      target_compile_options(imgui PUBLIC -fdata-sections -ffunction-sections)
      target_link_options(imgui PUBLIC -fdata-sections -ffunction-sections -Wl,--gc-sections)
    endif ()
  endif ()

  target_compile_definitions(brplot PUBLIC IMGUI)
  target_link_libraries(brplot imgui raylib)
  target_include_directories(brplot PUBLIC external/imgui-docking)
  target_sources(brplot PRIVATE src/gui++.cpp src/filesystem++.cpp)
  target_compile_features(brplot PUBLIC cxx_std_17)
elseif (GUI STREQUAL headless)
  target_compile_definitions(brplot PUBLIC HEADLESS)
else ()
  message(FATAL_ERROR "GUI=${GUI} not supported. (raylib|imgui|headless)")
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  target_compile_definitions(brplot PUBLIC PLATFORM_DESKTOP=1 LINUX=1)
  target_compile_definitions(raylib PUBLIC PLATFORM_DESKTOP=1)
  target_link_libraries(raylib glfw)
elseif (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  target_compile_definitions(brplot PUBLIC PLATFORM_DESKTOP=1 LINUX=1)
  target_compile_definitions(raylib PUBLIC PLATFORM_DESKTOP=1)
  target_link_libraries(raylib glfw pthread)
  target_include_directories(imgui PUBLIC /usr/local/include)
  target_link_directories(brplot PUBLIC /usr/local/lib /usr/lib)
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_compile_definitions(brplot PUBLIC PLATFORM_DESKTOP=1 WINDOWS=1 _WIN32=1 WIN32_LEAN_AND_MEAN=1)
  target_compile_definitions(raylib PUBLIC PLATFORM_DESKTOP=1 _WIN32=1 WIN32_LEAN_AND_MEAN=1)
  target_compile_definitions(imgui PUBLIC PLATFORM_DESKTOP=1 _WIN32=1 WIN32_LEAN_AND_MEAN=1)
  target_link_libraries(brplot opengl32 gdi32 winmm)
  target_sources(raylib PUBLIC ./external/raylib-5.0/src/rglfw.c)
else ()
  message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} not supported")
endif ()

set(PLAT_CONF ${CMAKE_SYSTEM_NAME}_${CMAKE_BUILD_TYPE})

if (PLAT_CONF STREQUAL Linux_Debug)
elseif (PLAT_CONF STREQUAL Linux_Release)
elseif (PLAT_CONF STREQUAL FreeBSD_Release)
elseif (PLAT_CONF STREQUAL Windows_Release)
elseif (PLAT_CONF STREQUAL Windows_Debug)
else ()
  message(FATAL_ERROR "${PLAT_CONF} not supported")
endif ()

target_include_directories(brplot PUBLIC . external/Tracy external/raylib-5.0/src)
