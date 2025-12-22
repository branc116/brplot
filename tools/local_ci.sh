#!/usr/bin/env sh

set -ex

test -f nob || cc nob.c -O2 -I. -lm -o nob
./nob amalgam

cc -ggdb -c -DBRUI_IMPLEMENTATION -x c .generated/brui.h -o build/amal_brui.o
cc -ggdb -o bin/ui -I.generated ./tests/test_ui.c build/amal_brui.o -lm
bin/ui

cc -ggdb -c -DBRPLOT_IMPLEMENTATION  -x c .generated/brplot.h -o build/amal_brplot.o

cc -ggdb -o bin/hello_world -I.generated tests/hello_world.c build/amal_brplot.o -lm
bin/hello_world

cc -ggdb -o bin/animations -I.generated tests/animations.c build/amal_brplot.o -lm
bin/animations

./nob
./nob unittests -a
./nob -l

for TYPE in EXE
do
  make $@ PLATFORM=WINDOWS TYPE=$TYPE
done

for TYPE in LIB EXE
do
  make $@ PLATFORM=LINUX TYPE=$TYPE
done

for TYPE in EXE
do
  make $@ PLATFORM=WEB HEADLESS=NO TYPE=$TYPE
done

ls -alFh bin && echo "OK"
