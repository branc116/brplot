#!/usr/bin/env sh

clang -I. -fsanitize=fuzzer -I./external/raylib-5.0/src -DNUMBER_OF_STEPS=100 -DLINUX=1 -DPLATFORM_DESKTOP=1 -g \
  -DUNIT_TEST -fpie -pg -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=signed-integer-overflow -fsanitize=integer-divide-by-zero \
  -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
  -Wconversion -Wall -Wpedantic -Wextra -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof \
  -o bin/fuzz_read_input src/read_input.c ./src/desktop/linux/read_input.c src/str.c src/q.c src/graph_utils.c
./bin/fuzz_read_input
