# I don't know what I'm doing with cmake. This is all vodoo...

cmake_minimum_required(VERSION 3.22.1)
include(CMakeDependentOption)
project(Brplot)

if ((NOT DEFINED CMAKE_BUILD_TYPE) OR (CMAKE_BUILD_TYPE STREQUAL ""))
  set(CMAKE_BUILD_TYPE Release)
endif ()
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

option(HEADLESS OFF "Enable headless build")

cmake_dependent_option(USE_LTO "Enable LTO" ON "CMAKE_BUILD_TYPE STREQUAL Release" OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${USE_LTO})
message("USE_LTO=${USE_LTO}")

option(USE_TRACY OFF "Use tracy for profiling")
cmake_dependent_option(USE_HOTRELOAD "Enable Hot reload" ON "CMAKE_BUILD_TYPE STREQUAL Debug;" OFF)
message("USE_TRACY=${USE_TRACY}")

# I don't how msvc can hot reload...
if (MSVC)
  set(USE_HOTRELOAD OFF)
endif ()

add_executable(brplot
  src/main.c           src/help.c        src/data.c           src/smol_mesh.c src/q.c
  src/read_input.c     src/plotter.c     src/keybindings.c    src/str.c       src/theme.c
  src/resampling2.c    src/graph_utils.c src/shaders.c        src/plot.c      src/permastate.c
  src/gui.c            src/filesystem.c  src/data_generator.c src/platform.c  src/text_renderer.c
  src/gl.c             src/icons.c
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
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/.generated/default_font.h
  COMMAND font_bake content/font.ttf ${CMAKE_CURRENT_SOURCE_DIR}/.generated/default_font.h
  DEPENDS font_bake content/font.ttf
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(pack_icons tools/pack_icons.c)
target_include_directories(pack_icons PUBLIC .)
target_link_libraries(pack_icons PUBLIC m)
file(GLOB icons content/*.png)
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/.generated/icons.c ${CMAKE_CURRENT_SOURCE_DIR}/.generated/icons.h
  COMMAND pack_icons
  DEPENDS font_bake ${icons}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(brplot PUBLIC .generated/icons.h)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  target_compile_definitions(brplot PUBLIC BR_DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL Release)
  if (MSVC)
  else ()
    target_compile_options(brplot PUBLIC -fdata-sections -ffunction-sections)
    target_link_options(brplot PUBLIC -fdata-sections -ffunction-sections -Wl,--gc-sections)
  endif ()
  add_executable(shaders_bake tools/shaders_bake.c src/str.c)
  target_include_directories(shaders_bake PUBLIC . ./external/raylib-5.0/src)
  file(GLOB shaders src/desktop/shaders/*)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/.generated/shaders.h
    COMMAND shaders_bake ${CMAKE_SYSTEM_NAME} .generated/shaders.h
    DEPENDS shaders_bake ${shaders}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_sources(brplot PUBLIC .generated/shaders.h)
else ()
  message(FATAL_ERROR "CMAKE_BUILD_TYPE=`${CMAKE_BUILD_TYPE}` not supported. (Debug|Release)")
endif()

if (HEADLESS)
  target_compile_definitions(brplot PUBLIC HEADLESS)
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  target_link_libraries(brplot dl m)
  target_compile_definitions(brplot PUBLIC _GNU_SOURCE)
elseif (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  target_link_libraries(brplot dl m)
  target_link_directories(brplot PUBLIC /usr/local/lib /usr/lib)
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_compile_definitions(brplot PUBLIC _WIN32=1 WIN32_LEAN_AND_MEAN=1)
  target_link_libraries(brplot opengl32 gdi32 winmm)
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

target_include_directories(brplot PUBLIC . external/Tracy external/glfw/include)
