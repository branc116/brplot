#!/usr/bin/env sh

set -ex

make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=DEBUG   GUI=IMGUI              && \
make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=DEBUG   GUI=RAYLIB             && \
make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=DEBUG   GUI=HEADLESS           && \
make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=RELEASE GUI=IMGUI              && \
make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=RELEASE GUI=RAYLIB             && \
make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=RELEASE GUI=HEADLESS           && \
make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=DEBUG   GUI=IMGUI              && \
make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=DEBUG   GUI=RAYLIB             && \
make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=DEBUG   GUI=HEADLESS           && \
make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=RELEASE GUI=IMGUI              && \
make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=RELEASE GUI=RAYLIB             && \
make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=RELEASE GUI=HEADLESS           && \
make $@ PLATFORM=WINDOWS              CONFIG=DEBUG   GUI=IMGUI              && \
make $@ PLATFORM=WINDOWS              CONFIG=DEBUG   GUI=RAYLIB             && \
make $@ PLATFORM=WINDOWS              CONFIG=DEBUG   GUI=HEADLESS           && \
make $@ PLATFORM=WINDOWS              CONFIG=RELEASE GUI=IMGUI              && \
make $@ PLATFORM=WINDOWS              CONFIG=RELEASE GUI=RAYLIB             && \
make $@ PLATFORM=WINDOWS              CONFIG=RELEASE GUI=HEADLESS           && \
make $@ PLATFORM=WEB                  CONFIG=DEBUG   GUI=IMGUI              && \
make $@ PLATFORM=WEB                  CONFIG=DEBUG   GUI=RAYLIB             && \
make $@ PLATFORM=WEB                  CONFIG=DEBUG   GUI=HEADLESS           && \
make $@ PLATFORM=WEB                  CONFIG=RELEASE GUI=IMGUI              && \
make $@ PLATFORM=WEB                  CONFIG=RELEASE GUI=RAYLIB             && \
make $@ PLATFORM=WEB                  CONFIG=RELEASE GUI=HEADLESS           && \
make $@ PLATFORM=WEB                  CONFIG=DEBUG   GUI=IMGUI     TYPE=LIB && \
make $@ PLATFORM=WEB                  CONFIG=DEBUG   GUI=RAYLIB    TYPE=LIB && \
make $@ PLATFORM=WEB                  CONFIG=DEBUG   GUI=HEADLESS  TYPE=LIB && \
make $@ PLATFORM=WEB                  CONFIG=RELEASE GUI=IMGUI     TYPE=LIB && \
make $@ PLATFORM=WEB                  CONFIG=RELEASE GUI=RAYLIB    TYPE=LIB && \
make $@ PLATFORM=WEB                  CONFIG=RELEASE GUI=HEADLESS  TYPE=LIB && \
echo "Running fuzztests" && time cat /dev/random | head --bytes=1M ./bin/brplot_headless_linux_debug_gcc > /dev/null && echo "Fuzz test OK" && \
time ./bin/brplot_imgui_linux_debug_gcc --unittest && echo "UNIT TESTS for imgui OK" && \
time ./bin/brplot_raylib_linux_debug_gcc --unittest && echo "UNIT TESTS for raylib OK" && \
time ./bin/brplot_headless_linux_debug_gcc --unittest && echo "UNIT TESTS for headless OK" && \
echo "Running fuzztests" && time cat /dev/random | head --bytes=1M | ./bin/brplot_headless_linux_debug_clang > /dev/null && echo "Fuzz test OK" && \
time ./bin/brplot_imgui_linux_debug_clang --unittest && echo "UNIT TESTS for imgui OK" && \
time ./bin/brplot_raylib_linux_debug_clang --unittest && echo "UNIT TESTS for raylib OK" && \
time ./bin/brplot_headless_linux_debug_clang --unittest && echo "UNIT TESTS for headless OK" && \
ls -alFh bin && \
echo "OK"
