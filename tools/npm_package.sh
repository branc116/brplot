#!/usr/bin/env sh
set -ex

./nob -wl
cp bin/brplot.js packages/npm/brplot.js
cp bin/brplot.wasm packages/npm/
python -m http.server -d packages/npm
