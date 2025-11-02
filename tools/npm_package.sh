#!/usr/bin/env sh
set -ex

# make PLATFORM=WEB CONFIG=RELEASE GUI=IMGUI TYPE=LIB
# cp ./www/brplot_imgui_release_lib.wasm ./packages/npm
# cp ./www/brplot_imgui_release_lib.js ./packages/npm/brplot.js

./nob -wl
cp bin/brplot.js packages/npm/brplot.js
cp bin/brplot.wasm packages/npm/
python -m http.server -d packages/npm
