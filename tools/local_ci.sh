#!/usr/bin/env sh

set -ex

test -f nob || cc nob.c -O2 -I. -lm -o nob
./nob amalgam

tcc -ggdb -o bin/ui -I. -Iinclude ./tests/test_ui.c -lm
bin/ui

tcc -ggdb -o bin/glfw_attach -I. -Iinclude ./tests/glfw_attach.c -lm -lglfw -lGL
bin/glfw_attach

tcc -ggdb -o bin/hello_world -I. -Iinclude tests/hello_world.c -lm
bin/hello_world

tcc -ggdb -o bin/animations -I. -Iinclude tests/animations.c -lm
bin/animations

./nob
./nob unittests -a
./nob -l
./nob -wl
./nob -W

ls -alFh bin && echo "OK"
