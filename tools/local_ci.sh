#!/usr/bin/env sh

set -ex

test -f nob || cc nob.c -O2 -I. -lm -o nob
./nob amalgam

tcc -ggdb -o bin/ui -I. -Iinclude ./tests/test_ui.c -lm
tcc -ggdb -o bin/ui -I.generated ./tests/test_ui.c -lm
bin/ui

tcc -ggdb -o bin/hello_world -I.generated tests/hello_world.c -lm
bin/hello_world

tcc -ggdb -o bin/animations -I.generated tests/animations.c -lm
bin/animations

./nob
./nob unittests -a
./nob -l
./nob -wl
./nob -W

ls -alFh bin && echo "OK"
