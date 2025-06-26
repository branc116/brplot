#!/usr/bin/env sh

set -ex
for CONFIG in DEBUG RELEASE
do
  for TYPE in LIB EXE
  do
    make $@ PLATFORM=WEB CONFIG=$CONFIG HEADLESS=NO TYPE=$TYPE
  done
done

for CONFIG in DEBUG RELEASE
do
  for HEADLESS in YES NO
  do
    for TYPE in LIB EXE
    do
      make $@ PLATFORM=WINDOWS CONFIG=$CONFIG HEADLESS=$HEADLESS TYPE=$TYPE
    done
  done
done

for CONFIG in DEBUG RELEASE
do
  for COMPILER in GCC CLANG ZIG
  do
    for HEADLESS in YES NO
    do
      for TYPE in LIB EXE
      do
        make $@ PLATFORM=LINUX COMPILER=$COMPILER CONFIG=$CONFIG HEADLESS=$HEADLESS TYPE=$TYPE
      done
    done
  done
done

cc nob.c -O2 -I. -lm -o nob
./nob
./nob unittests -a

ls -alFh bin && echo "OK"
