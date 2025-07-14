#!/usr/bin/env sh

set -ex
for TYPE in EXE
do
  make $@ PLATFORM=WEB HEADLESS=NO TYPE=$TYPE
done

for TYPE in EXE
do
  make $@ PLATFORM=WINDOWS TYPE=$TYPE
done

for TYPE in LIB EXE
do
  make $@ PLATFORM=LINUX TYPE=$TYPE
done

cc nob.c -O2 -I. -lm -o nob
./nob
./nob unittests -a

ls -alFh bin && echo "OK"
