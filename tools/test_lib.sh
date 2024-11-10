#!/usr/bin/env sh

set -ex

make BACKEND=X11 TYPE=LIB GUI=RAYLIB CONFIG=DEBUG
gcc -ggdb -I . -L . -o example ./tests/example_use_brplot_as_lib.c -lbrplot
LD_LIBRARY_PATH="$PWD" gf2 ./example
