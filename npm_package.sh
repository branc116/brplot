#!/usr/bin/env sh
set -ex

make PLATFORM=WEB CONFIG=RELEASE GUI=RAYLIB TYPE=LIB
cp ./www/brplot_raylib_release_lib.wasm ./packages/npm/dist
cp ./www/brplot_raylib_release_lib.wasm ./packages/npm/src
cp ./www/brplot_raylib_release_lib.js ./packages/npm/src/brplot.js
