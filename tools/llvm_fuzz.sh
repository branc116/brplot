#!/usr/bin/env sh
set -ex

clang -I. -fsanitize=fuzzer -I./external/Tracy -I./external/raylib-5.0/src -DNUMBER_OF_STEPS=100 -DLINUX=1 -DHEADLESS -DPLATFORM_DESKTOP=1 -DFUZZ -g \
  -DUNIT_TEST -fpie -pg -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=signed-integer-overflow -fsanitize=integer-divide-by-zero \
  -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
  -Wconversion -Wall -Wpedantic -Wextra -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof \
  -o bin/fuzz_read_input src/read_input.c src/str.c src/q.c src/graph_utils.c src/data.c src/plotter.c src/gui.c src/resampling2.c src/plot.c src/shaders.c src/permastate.c src/platform.c \
                         src/text_renderer.c src/filesystem.c src/smol_mesh.c src/keybindings.c src/data_generator.c src/help.c
./bin/fuzz_read_input
