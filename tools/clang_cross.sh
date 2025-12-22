#!/usr/bin/env sh

set -ex
clang -I. ./tests/hello_world.c -lm
clang -I. -target x86_64-pc-win32-gnu ./tests/hello_world.c -lgdi32
clang -I. -target x86_64-pc-win32-gnu ./tests/hello_world.c -lgdi32
clang -I. -target x86_64-apple-darwin ./tests/hello_world.c -lgdi32
emcc -sWASM_BIGINT -sALLOW_MEMORY_GROWTH -sUSE_GLFW=3 -sUSE_WEBGL2=1 -sGL_ENABLE_GET_PROC_ADDRESS -sCHECK_NULL_WRITES=0 -sDISABLE_EXCEPTION_THROWING=1 -sFILESYSTEM=0 -sDYNAMIC_EXECUTION=0 -DBRPLOT_IMPLEMENTATION -x c .generated/brplot.h
