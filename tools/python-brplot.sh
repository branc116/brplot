#!/usr/bin/env bash

set -ex

./nob pip

test -f build/venv/bin/activate || python -m venv build/venv;
. build/venv/bin/activate
pip install packages/pip
python
