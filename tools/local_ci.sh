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
  for COMPILER in GCC CLANG ZIG MUSL
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


#exit 1
#
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=DEBUG                          && \
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=DEBUG   HEADLESS=YES           && \
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=RELEASE                        && \
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=RELEASE HEADLESS=YES           && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=DEBUG                          && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=DEBUG   HEADLESS=YES           && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=RELEASE                        && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=RELEASE HEADLESS=YES           && \
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=DEBUG                TYPE=LIB  && \
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=DEBUG   HEADLESS=YES TYPE=LIB  && \
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=RELEASE              TYPE=LIB  && \
#make $@ PLATFORM=LINUX COMPILER=GCC   CONFIG=RELEASE HEADLESS=YES TYPE=LIB  && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=DEBUG                TYPE=LIB  && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=DEBUG   HEADLESS=YES TYPE=LIB  && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=RELEASE              TYPE=LIB  && \
#make $@ PLATFORM=LINUX COMPILER=CLANG CONFIG=RELEASE HEADLESS=YES TYPE=LIB  && \
#make $@ PLATFORM=WINDOWS              CONFIG=DEBUG                          && \
#make $@ PLATFORM=WINDOWS              CONFIG=DEBUG   HEADLESS=YES           && \
#make $@ PLATFORM=WINDOWS              CONFIG=RELEASE                        && \
#make $@ PLATFORM=WINDOWS              CONFIG=RELEASE HEADLESS=YES           && \
#make $@ PLATFORM=WEB                  CONFIG=DEBUG                          && \
#make $@ PLATFORM=WEB                  CONFIG=DEBUG   HEADLESS=YES           && \
#make $@ PLATFORM=WEB                  CONFIG=RELEASE                        && \
#make $@ PLATFORM=WEB                  CONFIG=RELEASE HEADLESS=YES           && \
#make $@ PLATFORM=WEB                  CONFIG=DEBUG                 TYPE=LIB && \
#make $@ PLATFORM=WEB                  CONFIG=DEBUG   HEADLESS=YES  TYPE=LIB && \
#make $@ PLATFORM=WEB                  CONFIG=RELEASE               TYPE=LIB && \
#make $@ PLATFORM=WEB                  CONFIG=RELEASE HEADLESS=YES  TYPE=LIB && \
#echo "Running fuzztests" && time cat /dev/random | head --bytes=1M ./bin/brplot_headless_linux_debug_gcc > /dev/null && echo "Fuzz test OK" && \
#time ./bin/brplot_no_x11_linux_debug_gcc --unittest && echo "UNIT TESTS for raylib OK" && \
#time ./bin/brplot_headless_linux_debug_gcc --unittest && echo "UNIT TESTS for headless OK" && \
#echo "Running fuzztests" && time cat /dev/random | head --bytes=1M | ./bin/brplot_headless_linux_debug_clang > /dev/null && echo "Fuzz test OK" && \
#time ./bin/brplot_raylib_linux_debug_clang --unittest && echo "UNIT TESTS for raylib OK" && \
#time ./bin/brplot_headless_linux_debug_clang --unittest && echo "UNIT TESTS for headless OK" && \
#ls -alFh bin && \
#echo "OK"
