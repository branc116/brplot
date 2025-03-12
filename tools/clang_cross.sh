#!/usr/bin/env sh

set -ex
clang -I. ./tests/hello_world.c -lm
clang -I. -target x86_64-pc-win32-gnu ./tests/hello_world.c -lgdi32
clang -I. -target x86_64-pc-win32-gnu ./tests/hello_world.c -lgdi32
clang -I. -target x86_64-apple-darwin ./tests/hello_world.c -lgdi32
