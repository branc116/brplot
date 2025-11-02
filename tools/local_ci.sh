#!/usr/bin/env sh

set -ex

test -f nob || cc nob.c -O2 -I. -lm -o nob
./nob amalgam

cc -ggdb -c -DBRPLOT_IMPLEMENTATION .generated/brplot.c -o build/amal_brplot.o
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
