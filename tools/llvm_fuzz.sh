#!/usr/bin/env sh
set -ex

test -d .generated/corpus || mkdir .generated/corpus
clang -I. -fsanitize=fuzzer,address,leak,undefined -DBR_DISABLE_LOG -DBR_LIB -DBR_DEBUG -DBR_NO_WAYLAND -DBR_NO_X11 -DHEADLESS -DFUZZ -g \
  -fpie -pg -fsanitize=leak -fsanitize=undefined -fsanitize=signed-integer-overflow -fsanitize=integer-divide-by-zero \
  -fsanitize=shift -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
  -Wconversion -Wall -Wpedantic -Wextra -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-newline-eof -Wno-gnu-zero-variadic-macro-arguments \
  -o bin/fuzz_read_input tools/unity/brplot.c
./bin/fuzz_read_input .generated/corpus -workers=16
