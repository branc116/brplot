#!/usr/bin/env sh

make PROFILE=YES GUI=HEADLESS COMPILER=CLANG CONFIG=DEBUG -B -j2

export LLVM_PROFILE_FILE="fuzz_debug.profraw"
cat "/dev/random" | bin/brplot_headless_linux_debug_clang
export LLVM_PROFILE_FILE="tests_debug.profraw"
bin/brplot_headless_linux_debug_clang --unittest

llvm-profdata merge -sparse tests_debug.profraw fuzz_debug.profraw -o debug.profdata
llvm-cov report bin/brplot_headless_linux_debug_clang -instr-profile=debug.profdata > report
